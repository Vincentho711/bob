from pathlib import Path
from typing import Any, Dict
import re
import os
import sys
import yaml
import subprocess
import logging
import shutil
import hashlib
import json

class Bob:
    def __init__(self, logger: logging.Logger) -> None:
        self.name = "bob"
        self.logger = logger
        # self.logger.debug("Bob instance initialised.")
        self.proj_root: str = os.environ.get("PROJ_ROOT", "Not Set")
        self.ip_config: Dict[str, Any] = {}
        self.task_configs: Dict[str, Any] = {}
        self.dotbob_dir: Path = Path(self.proj_root) / ".bob"
        self.dotbob_checksum_file: Path = self.dotbob_dir / "checksum.json"

    def get_proj_root(self) -> Path:
        return Path(self.proj_root)

    def run_subprocess(self) -> None:
        """Runs a subprocess to print environment variables."""
        try:
            self.logger.debug("Executing subprocess: env")
            subprocess.run(["env"], env=os.environ, check=True)
            self.logger.info("Subprocess executed successfully.")
        except subprocess.CalledProcessError as e:
            self.logger.error(f"Subprocess execution failed: {e}", exc_info=True)
        except Exception as e:
            self.logger.critical(f"Unexpected error during subprocess execution: {e}", exc_info=True)

    def set_env_var_val(self, env_key: str, env_val: str) -> None:
        """Set an env var to a value"""
        try:
            if (not isinstance(env_key, str) or not isinstance(env_val, str)):
                raise TypeError(f"{env_key} is of type {type(env_key)} and {env_val} is of type {type(env_val)}. Both of them has to be a str to be set as an env var.")
            env_key = env_key.upper()
            if (os.environ.get(env_key) is not None):
                self.logger.warning(f"Env_key: {env_key} has already been set to {os.environ.get(env_key)}.")
            else:
                self.logger.info(f"Setting env var, {env_key}={env_val}")
                os.environ[env_key] = env_val
        except TypeError as te:
            self.logger.error(f"Error: {te}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during set_env_var_val(): {e}", exc_info=True)

    def set_env_var_path(self, env_key: str, path: Path) -> None:
        """Set an env var to a path"""
        try:
            if (not isinstance(env_key, str)):
                raise TypeError(f"{env_key} is of type {type(env_key)}, it has to be a str to be set as a env var.")
            if (not isinstance(path, Path)):
                raise TypeError(f"{path} is of type {type(path)}, it has to be a Path object to be set to the env var {env_key}.")
            if not path.exists():
                raise FileNotFoundError(f"The path {path} does not exist, hence {env_key} is not set to that path.")
            env_key = env_key.upper()
            if (os.environ.get(env_key) is not None):
                self.logger.warning(f"Env_key: {env_key} has already been set to {os.environ.get(env_key)}")
            else:
                self.logger.info(f"Setting env var to a path, {env_key}={path}")
                os.environ[env_key] = str(path)
        except TypeError as te:
            self.logger.error(f"Error: {te}")
        except FileNotFoundError as fnfe:
            self.logger.error(f"Error: {fnfe}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during set_env_var_path(): {e}", exc_info=True)

    def append_env_var_path(self, env_key: str, path: Path) -> None:
        """Append a path to an env var"""
        try:
            if (not isinstance(env_key, str)):
                raise TypeError(f"{env_key} is of type {type(env_key)}, it has to be a str to be set as a env var.")
            if (not isinstance(path, Path)):
                raise TypeError(f"{path} is of type {type(path)}, it has to be a Path object to be set to the env var {env_key}.")
            if not path.exists():
                self.logger.warning(f"The path {path} does not exist, hence it is not appended to {env_key}.")
            else:
                env_key = env_key.upper()
                if (os.environ.get(env_key) is None):
                    os.environ[env_key] = str(path)
                else:
                    os.environ[env_key] += os.pathsep + str(path)
        except TypeError as te:
            self.logger.error(f"Error: {te}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during append_env_var_path(): {e}", exc_info=True)

    def load_ip_cfg(self) -> None:
        """Load the ip_config.yaml into an internal dict"""
        try:
            ip_cfg_path: Path = Path(self.proj_root) / "ip_config.yaml"
            if not ip_cfg_path.exists():
                raise FileNotFoundError(f"No ip_config.yaml is found under {self.proj_root}. Since {ip_cfg_path} is not found, parsing skipped.")
            with open(ip_cfg_path, "r") as yaml_file:
                self.ip_config = yaml.safe_load(yaml_file)
            self.logger.info(f"Parsing {ip_cfg_path}")

        except (FileNotFoundError, ValueError) as e:
            self.logger.error(f"Error: {e}")
            sys.exit(1)
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_ip_cfg(): {e}", exc_info=True)

    def parse_ip_cfg(self) -> Dict[str, Any]:
        """Parse and resolve placeholders in the entire ip_config dict"""
        return self._resolve_value(self.ip_config)

    def _resolve_value(self, value: Any, resolved_cache=None) -> Any:
        """
        Resolve the placeholders in the values (e.g., ${directories.root_dir})
        while ensuring correct path joining where applicable.
        """
        try:
            if resolved_cache is None:
                resolved_cache = {}
            if isinstance(value, str):
                if value in resolved_cache:
                    return resolved_cache[value] # Prevent duplicate resolution
                resolved_value = self._resolve_placeholder(value, resolved_cache)
                # if it is at the bottom of the hierarchy and all the elements in a list have been resolved, just return that list
                if (isinstance(resolved_value, list)):
                    return resolved_value
                # Don't correct path format if it is a string and there is a leading '/' which indicates abs path
                if resolved_value.startswith('/'):
                    return resolved_value
                else:
                    resolved_cache[value] = self._ensure_correct_path_format(value, resolved_value)
                    return resolved_cache[value]
            elif isinstance(value, list):
                return [self._resolve_value(item, resolved_cache) for item in value]
            elif isinstance(value, dict):
                return {key: self._resolve_value(val, resolved_cache) for key, val in value.items()}
            return value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_value(): {e}", exc_info=True)
            sys.exit(1)

    def _resolve_placeholder(self, value: str, resolved_cache: Dict[str, str]) -> Any:
        """
        Resolve placeholders, supporting both environment variables and hierarchical variables.
        Environment variables take precedence over hierarchical values.
        Resolve a placeholder like ${directories.root_dir} , or ${PROJ_ROOT} to their actual values.
        Uses caching to prevent duplicate resolution and avoids circular references.
        """
        # Regular expression to match ${<hierarchy>}
        try:
            placeholder_pattern = r"\$\{([^}]+)\}"
            matches = re.findall(placeholder_pattern, value)

            for match in matches:
                # Check if it is a environment variable first
                if match in os.environ:
                    resolved_value = os.environ[match]
                else:
                    resolved_value = self._get_value_from_hierarchy(match, resolved_cache)
                if resolved_value is not None:
                    if isinstance(resolved_value, list):
                        # If it's a list, resolve each element separately
                        resolved_value = [self._resolve_placeholder(str(item), resolved_cache) for item in resolved_value]
                        return resolved_value # Return the fully resolved list
                    elif not isinstance(resolved_value, str):
                        resolved_value = str(resolved_value)
                    value = value.replace(f"${{{match}}}", resolved_value)
                else:
                    self.logger.error(f"Warning: No matching value found for placeholder {match}")

            return value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_placeholder(): {e}", exc_info=True)
            sys.exit(1)

    def _get_value_from_hierarchy(self, hierarchy: str, resolved_cache: Dict[str, str]) -> str | None:
        """Fetch the value from the hierarchy (e.g., directories.root_dir)"""
        try:
            if hierarchy in resolved_cache:
                return resolved_cache[hierarchy]
            keys = hierarchy.split('.')
            value = self.ip_config

            for key in keys:
                if key in value:
                    value = value[key]
                else:
                    self.logger.error(f"No matching value found for hierarchy {hierarchy}")
                    return None
            resolved_value = self._resolve_value(value, resolved_cache)
            resolved_cache[hierarchy] = resolved_value
            self.logger.debug(f"hierarchy: {hierarchy}, value: {resolved_value}")
            return resolved_value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _get_value_from_hierarchy(): {e}", exc_info=True)
            sys.exit(1)

    def _ensure_correct_path_format(self, original: str, resolved: str) -> str:
        """
        Ensure correct path joining when a placeholder is part of a file/directory path.
        This prevents incorrect constructs like `.//src`.
        """
        if "/" in original or "\\" in original:  # Check if the value is path-like
            parts = resolved.split("/")
            return str(Path(*parts))  # Use pathlib to reconstruct the path correctly
        return resolved

    def discover_tasks(self):
        """Discover tasks by finding task_config.yaml files and extracting their task names."""
        try:
            for task_config_path in Path(self.proj_root).rglob("task_config.yaml"):
                task_dir = task_config_path.parent
                self.logger.info(f"task_config.yaml found in {task_config_path}")
                self.logger.info(f"task_dir = {task_dir}")
                with open(task_config_path, "r") as f:
                    task_data = yaml.safe_load(f)
                task_name = task_data.get("task_name")
                if not task_name:
                    raise ValueError(f"No task_name defined in {task_config_path}")
                self.task_configs[task_name] = {}
                self.task_configs[task_name]["task_config_path"] = task_config_path
                self.task_configs[task_name]["task_dir"] = task_dir
        except ValueError as e:
            self.logger.error(f"ValueError: {e}")
            exit(1)
        except Exception as e:
            self.logger.critical(f"Unexpected error during discover_tasks(): {e}", exc_info=True)
            sys.exit(1)

    def setup_build_dirs(self) -> None:
        """Create a dedicated build directory for each task under proj_root/build/."""
        try:
            build_root = Path(self.proj_root) / "build"
            build_root.mkdir(exist_ok=True)
            if len(self.task_configs) == 0:
                raise LookupError(f"No tasks defined in self.task_configs. Please ensure task configs are present.")
            for task_name in self.task_configs:
                build_dir = build_root / task_name
                build_dir.mkdir(exist_ok=True)
                self.logger.info(f"Build directory: {build_dir} has been created or it exists already.")
        except LookupError as e:
            (f"LookupError: {e}")
            sys.exit(1)
        except Exception as e:
            self.logger.critical(f"Unexpected  error during setup_build_dirs(): {e}", exc_info=True)
            sys.exit(1)

    def remove_task_build_dirs(self, task_names: list[str]) -> int:
        """Remove the build dirs for multiple tasks, return the number of task build dirs deleted"""
        deleted_count = 0
        try:
            build_root = Path(self.proj_root) / "build"
            invalid_tasks = [task for task in task_names if task not in self.task_configs]
            if invalid_tasks:
                self.logger.warning(f"The following tasks do no exist in task_configs and will not be deleted: {invalid_tasks}")
            for task_name in task_names:
                if task_name in self.task_configs:
                    self.logger.info(f"Found {task_name}")
                    build_dir = build_root / task_name
                    if build_dir.exists() and build_dir.is_dir():
                        shutil.rmtree(build_dir)
                        deleted_count += 1
                        self.logger.info(f"Deleted build directory: {build_dir}")
                    else:
                        self.logger.warning(f"Build directory not found: {build_dir}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during remove_task_build_dirs(): {e}", exc_info=True)
        return deleted_count

    def create_all_task_env(self) -> None:
        """ Create a separate task environment for each task defined in self.task_config based on the global environment"""
        try:
            if not self.task_configs:
                raise ValueError(f"No tasks defined within self.task_configs. Please run discover_tasks() first.")
            for task_name, task_config in self.task_configs.items():
                task_config["task_env"] = os.environ.copy();
        except ValueError as e:
            self.logger.error(f"ValueError: {e}")
            sys.exit(1)
        except Exception as e:
            self.logger.critical(f"Unexpected error during create_all_task_env(): {e}", exc_info=True)
            sys.exit(1)

    def append_task_env_var_val(self, task_name: str, env_key: str, env_val: str | Path) -> None:
        """ Append/Add an envionment variable within a task environment"""
        try:
            env_key = env_key.upper()
            existing_val = self.task_configs[task_name]["task_env"].get(env_key)
            if isinstance(env_val, Path) and not env_val.exists():
                raise FileNotFoundError(f"The path {env_val} does not exist, hence {env_key} is not set to that path.")
            if existing_val:
                if isinstance(env_val, Path):
                    self.task_configs[task_name]["task_env"][env_key] += os.pathsep + str(env_val)
                    self.logger.debug(f"For {task_name} env, appending path {str(env_val)} to env var {env_key}. {env_key} = {self.task_configs[task_name]['task_env'].get(env_key)}.")
                # If it is not a Path, we replace existing env_val with the new env_val
                else:
                    self.task_configs[task_name]["task_env"][env_key] = env_val
                    self.logger.debug(f"For {task_name} env, updating env var {env_key} = {env_val}.")
            else:
                if isinstance(env_val, Path):
                    env_val = str(env_val)
                self.task_configs[task_name]["task_env"][env_key] = env_val
                self.logger.debug(f"For {task_name} env, setting env var {env_key} = {env_val}.")
        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError : {fnfe}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during append_task_env_var_val(): {e}", exc_info=True)

    def append_task_src_files(self, task_name: str, src_files: Path| list[Path]) -> None:
        """Append a src file or a list of src files to the 'src_files' attribute of a task"""
        try:
            if task_name not in self.task_configs:
                self.logger.error(f"Task '{task_name}' does not exist in task_configs. Please run discover_tasks() first.")
                return
            if not isinstance(src_files, (Path, list)):
                raise ValueError(f"For {task_name}, the argument 'src_files' must be a Path or a list of Path objects, but got {type(src_files).__name__}.")

            if isinstance(src_files, Path):
                src_files = [src_files]

            # Validate list contents
            invalid_elements = [item for item in src_files if not isinstance(item, Path)]
            if invalid_elements:
                raise ValueError(f"For {task_name}, all elements in src_files must be Path objects. Invalid entries: {invalid_elements}")

            valid_files = [path for path in src_files if path.exists()]
            missing_files = set(src_files) - set(valid_files)

            if missing_files:
                self.logger.error(f"For {task_name}, the following paths do not exist and will be ignored: {missing_files}")
            if valid_files:
                self.logger.debug(f"Found valid_files: {valid_files}")
                task_entry = self.task_configs[task_name]
                src_list = task_entry.setdefault("src_files", [])

                # Append only new paths to avoid duplicates
                new_files = [path for path in valid_files if path not in src_list]
                if new_files:
                    src_list.extend(new_files)
                    self.logger.debug(f"Added {len(new_files)} new file(s) to task '{task_name}'.")
                else:
                    self.logger.info(f"No new files were added to task '{task_name}' (duplicates avoided).")
        except ValueError as ve:
            self.logger.critical(f"ValueError: {ve}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during append_task_src_files(): {e}", exc_info=True)

    def ensure_dotbob_dir_at_proj_root(self) -> None:
        """Create a .bob directory and checksum.json with default settings at project root if they don't exist yet"""
        try:
            if not self.task_configs:
                raise ValueError(f"No tasks defined within task_configs, cannot create dotbob_checksum_file with default values.")
            self.dotbob_dir.mkdir(exist_ok=True)
            if not self.dotbob_checksum_file.exists():
                self.logger.debug(f"Creating default checksum.json at {self.dotbob_checksum_file}.")
                initial_dotbob_checksum_file_dict = {
                    task_name : {"hash_sha256": "", "dirty" : True}
                        for task_name in self.task_configs
                }
                self._save_dotbob_checksum_file(initial_dotbob_checksum_file_dict)
            else:
                self.logger.debug(f"checksum.json already exists. Checking for new tasks...")
                self._update_dotbob_checksum_file()

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during create_dotbob_dir_at_proj_root() : {e}", exc_info=True)

    def _compute_task_src_files_hash_sha256(self, task_name: str) -> str | None:
        """Compute a SHA256 checksum from a list of src files for a task"""
        try:
            if task_name not in self.task_configs:
                self.logger.error(f"Task '{task_name}' does not exist in task_configs. Please run discover_tasks() first.")
                return None

            src_files = self.task_configs[task_name].get("src_files", [])
            if not src_files:
                self.logger.error(f"Task '{task_name}' has no source files defined.")
                return None

            hash_sha256 = hashlib.sha256()
            for file_path in sorted(src_files, key=lambda p: p.as_posix()):
                if file_path.exists() and file_path.is_file():
                    with file_path.open("rb") as f:
                        while chunk := f.read(8192):
                            hash_sha256.update(chunk)
            return hash_sha256.hexdigest()

        except Exception as e:
            self.logger.critical(f"Unexpected error during _compute_task_src_files_hash_sha256(): {e}", exc_info=True)

    def _update_dotbob_checksum_file(self) -> None:
        """Update checksum.json to include new tasks without modifying existing entries."""
        try:
            if not self.task_configs:
                raise ValueError(f"No tasks defined within task_configs, aborting _update_dotbob_checksum_file().")

            with self.dotbob_checksum_file.open("r") as f:
                existing_dotbob_checksum_file_dict = json.load(f)

            new_tasks = set(self.task_configs.keys()) - set(existing_dotbob_checksum_file_dict.keys())

            if new_tasks:
                self.logger.debug(f"New tasks detected: {new_tasks}. Updating checksum.json...")
                for task_name in new_tasks:
                    existing_dotbob_checksum_file_dict[task_name] = {"hash_sha256": "", "dirty": True}
                self._save_dotbob_checksum_file(existing_dotbob_checksum_file_dict)
            else:
                self.logger.debug("No new tasks detected. checksum.json is up to date")

        except json.JSONDecodeError:
            self.logger.error("checksum.json is corrupted or empty. Reinitialising...")
            self._save_dotbob_checksum_file({
                task_name: {"hash_sha256": "", "dirty": True} for task_name in self.task_configs
            })

        except ValueError as ve:
            self.logger.critical(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during _update_dotbob_checksum_file(): {e}", exc_info=True)

    def _load_dotbob_checksum_file(self) -> dict | None:
        """Loads the checksum file within dotbob dir"""
        try:
            if not self.dotbob_checksum_file.exists():
                self.logger.error(f"No checksum.json within .bob dir found.")
                return None
            with self.dotbob_checksum_file.open("r") as f:
                self.logger.debug(f"Loading checksum.json into a Python dictionary.")
                return json.load(f)
        except Exception as e:
            self.logger.critical(f"Unexpected error during _load_dotbob_checksum_file() : {e}", exc_info=True)
            return None

    def _save_dotbob_checksum_file(self, checksums: dict[str, dict]) -> None:
        """Validate the checksum dict, and save it to checksum.json"""
        try:
            # Check checksums is not empty
            if not checksums:
                raise ValueError(f"Chechsums is an empty dict, cannot set checksum.json to empty.")
            if not self.dotbob_checksum_file.exists():
                self.logger.error(f"No checksum.json within .bob dir found.")
                return None
            # Check that task names are unique
            task_names = list(checksums.keys())
            if len(task_names) != len(set(task_names)): # It is not possible for checksums to have duplicate keys, hence redundant
                raise ValueError(f"During _save_dotbob_checksum_file(), there are duplicate tasks with the same task_name in 'checksums'. Abort saving to checksum.json.")
            # Validate each task has both "hash_sha256" and "dirty" fields
            for task_name, task_info in checksums.items():
                if "hash_sha256" not in task_info or "dirty" not in task_info:
                    self.logger.error(f"During _save_dotbob_checksum_file(), within the input dict checksums, missing required 'hash_sha256' or/and 'dirty' fields for {task_name}.")
                    return None
            with self.dotbob_checksum_file.open("w") as f:
                json.dump(checksums, f, indent=4)

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during _save_dotbob_checksum_file(): {e}", exc_info=True)

    def should_rebuild_task(self, task_name: str) -> bool | None:
        """Determine whether a task needs to be rebuilt based on dirty flag and whether its hash_sha256 has changed"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task {task_name} not found in task_configs. Please ensure discover_tasks() have been executed first.")

            src_files = self.task_configs[task_name].get("src_files", [])

            if not src_files:
                self.logger.error(f"No source files defined for task {task_name}. Skipping build for this task.")
                return False

            current_hash_sha256 = self._compute_task_src_files_hash_sha256(task_name)

            if current_hash_sha256 is None:
                raise RuntimeError(f"_compute_task_src_files_checksum() returned None, hence current checksum cannot be computed for task {task_name}.")

            dotbob_checksum_file_dict: dict = self._load_dotbob_checksum_file()
            previous_task_checksum_entry = dotbob_checksum_file_dict.get(task_name, {})
            previous_hash_sha256 = previous_task_checksum_entry.get("hash_sha256")
            hash_sha256_is_dirty = previous_task_checksum_entry.get("dirty", True)

            if previous_hash_sha256 == current_hash_sha256 and not hash_sha256_is_dirty:
                self.logger.info(f"For task {task_name}, hash_sha256 unchanged, skipping build.")
                return False

            # Mark as dirty since hash_sha256 has changed
            dotbob_checksum_file_dict[task_name] = {
                "hash_sha256": current_hash_sha256,
                "dirty": True
            }
            # Update the checksum.json with the updated hash_sha256 and dirty fields
            self._save_dotbob_checksum_file(dotbob_checksum_file_dict)
            self.logger.info(f"For task {task_name}, hash_sha256 has changed, triggering rebuild.")
            return True

        except RuntimeError as re:
            self.logger.error(f"RuntimeError: {re}")
            return None

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during should_rebuild_task(): {e}", exc_info=True)
            return None

    def mark_task_as_clean_in_dotbob_checksum_file(self, task_name: str) -> None:
        """Mark a task as clean after a succesful build"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task {task_name} not found in task_configs. Please ensure discover_tasks() have been executed first.")

            dotbob_checksum_file_dict: dict = self._load_dotbob_checksum_file()

            if task_name not in dotbob_checksum_file_dict:
                raise KeyError(f"Within mark_task_as_clean_in_dotbob_checksum_file(), the task {task_name} does not exists in checksum.json, aborting marking it as clean.")

            dotbob_checksum_file_dict[task_name]["dirty"] = False
            self._save_dotbob_checksum_file(dotbob_checksum_file_dict)
            self.logger.debug(f"Marked task {task_name} as clean.")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during mark_task_as_clean_in_dotbob_checksum_file(): {e}", exc_info=True)
            return None

    def set_bob_dir(self) -> None:
        """Sets BOB_DIR based on proj_root"""
        try:
            proj_root = os.environ.get("proj_root")
            if not proj_root:
                raise ValueError("proj_root environment variable is not set or empty.")

            proj_root_path = Path(proj_root)

            if not proj_root_path.exists():
                raise FileNotFoundError(f"proj_root path does not exist: {proj_root_path}")

            bob_dir: Path = proj_root_path / "bob"

            if bob_dir.exists():
                os.environ["BOB_DIR"] = str(bob_dir)
                self.logger.info(f"BOB_DIR set to: {bob_dir}")
            else:
                self.logger.warning(f"BOB_DIR does not exist: {bob_dir}")

        except ValueError as ve:
            self.logger.error(f"Error: {ve}")
        except FileNotFoundError as fnfe:
            self.logger.error(f"Error: {fnfe}")
        except Exception as e:
            self.logger.critical(f"Unexpected error setting BOB_DIR: {e}", exc_info=True)

