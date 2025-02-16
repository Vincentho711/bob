from pathlib import Path
from typing import Any, Dict
import re
import os
import sys
import yaml
import subprocess
import logging
import shutil

class Bob:
    def __init__(self, logger: logging.Logger) -> None:
        self.name = "bob"
        self.logger = logger
        # self.logger.debug("Bob instance initialised.")
        self.proj_root: str = os.environ.get("PROJ_ROOT", "Not Set")
        self.ip_config: Dict[str, Any] = {}
        self.task_configs: Dict[str, Any] = {}

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
            # task_dirs = [Path(self.proj_root).rglob("task_config.yaml")]
            for task_dir in Path(self.proj_root).rglob("task_config.yaml"):
                task_config_path = task_dir
                print(f"Discovered: {task_config_path}")
                self.logger.info(f"task_config.yaml found in {task_config_path}")
                with open(task_config_path, "r") as f:
                    task_data = yaml.safe_load(f)
                task_name = task_data.get("task_name")
                if not task_name:
                    raise ValueError(f"No task_name defined in {task_config_path}")
                self.task_configs[task_name] = {}
                self.task_configs[task_name]["task_config_path"] = task_config_path
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
                    self.logger.debug(f"For {task_name} env, appending path {str(env_val)} to env var {env_key}. {env_key} = {self.task_configs[task_name]["task_env"].get(env_key)}.")
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
                print(f"Have invalid_elements.")
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

