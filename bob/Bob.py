from pathlib import Path
from typing import Any, Dict
from networkx import DiGraph
import os
import sys
import yaml
import subprocess
import multiprocessing
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
        self.dependency_graph = None
        self.dependency_count = {}
        self.ready_queue = None
        self.lock = multiprocessing.Lock() # Synchronisation lock

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

    def discover_tasks(self):
        """Discover tasks by finding task_config.yaml files and extracting their task names."""
        try:
            for task_config_file_path in Path(self.proj_root).rglob("task_config.yaml"):
                task_dir = task_config_file_path.parent
                self.logger.info(f"task_config.yaml found in {task_config_file_path}")
                self.logger.info(f"task_dir = {task_dir}")
                with open(task_config_file_path, "r") as f:
                    task_data = yaml.safe_load(f)
                task_name = task_data.get("task_name")
                if not task_name:
                    raise ValueError(f"No task_name defined in {task_config_file_path}")
                self.task_configs[task_name] = {}
                self.task_configs[task_name]["task_config_file_path"] = task_config_file_path
                self.task_configs[task_name]["task_dir"] = task_dir
                build_root = Path(self.proj_root) / "build"
                task_outdir = build_root / task_name
                self.task_configs[task_name]["output_dir"] = task_outdir
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

    def resolve_task_configs_output_src_files(self, task_name: str) -> list[str]:
        """Resolves output source files for a given task by replacing directories with their contained files."""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs.")
            output_src_files = self.task_configs[task_name].get("output_src_files", [])
            resolved_output_src_files = []

            for path in output_src_files:
                if os.path.isdir(path):
                    # List all files in the directory recursively
                    for root, _, files in os.walk(path):
                        for file in files:
                            resolved_output_src_files.append(os.path.join(root, file))
                elif os.path.isfile(path):
                    resolved_output_src_files.append(path)
                else:
                    raise ValueError(f"For task '{task_name}', the output_src_files path '{path}' does not exists or it is not a valid file/directory.")
            # Update the task_configs[task_name]["output_src_files"]
            self.task_configs[task_name]["output_src_files"] = resolved_output_src_files

            return resolved_output_src_files

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_task_configs_output_src_files(): {e}", exc_info=True)

    def update_task_env(self, task_name: str, env_key: str, env_val: str | list[str], override_env_val: bool = False) -> None:
        """Updates the environment variables for a given task in self.task_configs."""
        try:
            if task_name not in self.task_configs:
                raise KeyError(f"Task {task_name} not found in task configurations.")

            if "task_env" not in self.task_configs[task_name]:
                self.logger.info(f"Task '{task_name}' does not have an env var dict associated to the 'task_env' key. Creating it from current global env.")

            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_env' attribute within task_configs")

            existing_val = task_env.get(env_key, None)

            if override_env_val or existing_val is None:
                task_env[env_key] = os.pathsep.join(env_val) if isinstance(env_val, list) else env_val
                self.logger.debug(f"Task '{task_name}', overriding existing or creating env_key='{env_key}' with env_val='{task_env[env_key]}'")
                return;

            # An existing_val already exists
            # Attempt to separate str with os.pathsep to see if existing_val is already a list of filepaths
            existing_val = existing_val.split(os.pathsep)

            # Extend if env_val is a list, else just append the str
            if isinstance(env_val, list):
                self.logger.debug(f"Task '{task_name}', extending a list of env_val = '{env_val}' to env_key = '{env_key}'")
                existing_val.extend(env_val)
            else:
                self.logger.debug(f"Task '{task_name}', appending a single str/filepath env_val = '{env_val}' to env_key = '{env_key}'")
                existing_val.append(env_val)

            # Add os.pathsep as separators
            task_env[env_key] = os.pathsep.join(existing_val) if existing_val else ""
            self.logger.debug(f"Task '{task_name}', updated env_key = '{env_key}' with env_val = '{task_env[env_key]}'")

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return

        except Exception as e:
            self.logger.critical(f"Unexpected error during update_task_env() for task_name = '{task_name}' : {e}", exc_info=True)
            return

    def ensure_src_files_existence(self, task_name: str) -> bool:
        """Validate that all files within 'internal_src_files', 'external_src_files' and 'output_src_files' exists"""
        try:
            if task_name not in self.task_configs:
                self.logger.error(f"Task '{task_name}' not found in configuration.")
                return False

            missing_files = []
            task_config = self.task_configs[task_name]

            for key in ["internal_src_files", "external_src_files", "output_src_files"]:
                for file_path in task_config.get(key, []):
                    if not os.path.isfile(file_path):
                        missing_files.append(file_path)

            if missing_files:
                self.logger.error(f"Task '{task_name}' is missing the following files:")
                for file in missing_files:
                    self.logger.error(f" - {file}")
                return False

            return True

        except Exception as e:
            self.logger.critical(f"Unexpected error during update_task_env() for task_name = '{task_name}' : {e}", exc_info=True)
            return False

    def execute_c_compile(self, task_name: str) -> subprocess.Popen | None:
        """Execute a C compile with gcc, using attributes within env var"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs.")
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            if task_config_dict is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_config_dict' attribute.")
            task_type = task_config_dict.get("task_type", None)
            if task_type is None:
                raise ValueError(f"Task '{task_name}' does not have a 'task_type' attribute.")
            if task_type != "c_compile":
                raise ValueError(f"Task '{task_name}' has task type '{task_type}' which is not 'c_compile'. Please exeute the correct function to execute build.")
            # Resolve output_src_files list
            self.resolve_task_configs_output_src_files(task_name)
            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_env' attribute.")
            # Promote task_configs attributes to env var such that they can be used in subprocess
            # Ensuer that all src_files exist
            # Promote internal_src_files, external_src_files and output_src_files to task env var "C_COMPILE_SRC_FILES"
            if not self.ensure_src_files_existence(task_name):
                self.logger.error(f"Aborting execute_c_compile as one or more source files are missing.")
                return None
            src_files = sum((self.task_configs[task_name].get(key, []) for key in ["internal_src_files", "external_src_files", "output_src_files"]), [])
            self.logger.debug(f"Task '{task_name}' has src_files={src_files}")
            self.update_task_env(task_name, "C_COMPILE_SRC_FILES", src_files)
            executable_name = task_config_dict.get("executable_name", None)
            if executable_name:
                self.update_task_env(task_name, "C_COMPILE_EXECUTABLE_NAME", executable_name)

            output_dir = self.task_configs[task_name].get("output_dir", None)
            if output_dir is None:
                self.logger.error(f"Task '{task_name}' does not have the mandatory attribute 'output_dir'. Aborting execute_c_compile().")
                return None
            if executable_name:
                executable_path = output_dir / executable_name
                self.update_task_env(task_name, "C_COMPILE_EXECUTABLE_PATH", executable_path)

            # Check if we have external objects
            external_objects = self.task_configs[task_name].get("external_objects", [])
            if external_objects:
                self.logger.debug(f"Task '{task_name}' has external_objects={external_objects}")
                self.update_task_env(task_name, "C_COMPILE_EXTERNAL_OBJECT_PATHS", external_objects)

            # During compilation, header dirs containing the header files will have to be included
            include_header_dirs = self.task_configs[task_name].get("include_header_dirs", [])
            if include_header_dirs:
                self.logger.debug(f"Task '{task_name}' has include_header_dirs='{include_header_dirs}'")
                self.update_task_env(task_name, "C_COMPILE_INCLUDE_HEADER_DIRS", str(include_header_dirs))

            # Execute gcc compile
            # Compile each .c file to .o file
            object_files = []
            for src in src_files:
                obj_file = os.path.join(output_dir, os.path.basename(src).replace(".c", ".o"))
                cmd_compile = ["gcc", "-c", src, "-o", obj_file]
                if include_header_dirs:
                    for inc_dir in include_header_dirs:
                        cmd_compile.extend(["-I", inc_dir])
                self.logger.info(f"Executing c_compile command: {cmd_compile}")
                process = subprocess.Popen(cmd_compile, env=task_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                # if result.returncode != 0:
                #     self.logger.error(f"GCC compilation failed for file '{src}': {result.stderr}")
                #     return False
                object_files.append(obj_file)
                process.wait()

            self.logger.info(f"GCC compilation succeeded for task '{task_name}'. Output: {object_files}")

            # If 'executable_name' exists within task_config.yaml, then link object files into an executable
            # Link all .o files, including external ones) to create the final executable
            if executable_name:
                cmd_link = ["gcc"] + object_files + external_objects + ["-o", executable_path]
                self.logger.info(f"Executing c_link command: {cmd_link}")
                process = subprocess.Popen(cmd_link, env=task_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                # if result.returncode != 0:
                #     self.logger.error(f"GCC compilation failed for task '{task_name}': {result.stderr}")
                #     return False

                self.logger.info(f"GCC link succeeded for task '{task_name}'. Output: {executable_path}")
            return process

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_task_configs_output_src_files(): {e}", exc_info=True)
            return None

    def schedule_tasks(self):
        """Schedules tasks dynamically while respecting dependencies"""
        try:
            dependency_graph = self.dependency_graph
            if dependency_graph is None:
                raise ValueError(f"self.dependency_graph = None. Please ensure build_task_dependency is run, and Bob's attribute has been updated.")

            # Track number of dependencies (indegree) for each task
            dependency_count = {task: dependency_graph.in_degree(task) for task in dependency_graph.nodes}
            ready_queue = multiprocessing.Queue() # Process-safe queue

            # Populate queue withi initial tasks (indegree = 0)
            for task, count in dependency_count.items():
                if count == 0:
                    ready_queue.put(task)

            self.ready_queue = ready_queue
            self.dependency_count = dependency_count

            return dependency_count, ready_queue
        except Exception as e:
            self.logger.critical(f"Unexpected error during schedule_tasks(): {e}", exc_info=True)

    def execute_tasks(self):
        """Executes tasks with dynamic scheduling and parallel execution"""
        try:
            dependency_count, ready_queue = self.schedule_tasks()
            self.logger.debug(f"dependency_count={dependency_count}")
            process_pool = [] # Store active process handles
            self.logger.debug(f"ready_queue = {ready_queue}")

            while not self.ready_queue.empty() or process_pool:
                # Launch all available tasks in parallel
                while not self.ready_queue.empty():
                    task = self.ready_queue.get()
                    process = multiprocessing.Process(target=self.execute_task, args=(task, self.dependency_graph, self.dependency_count, self.ready_queue, self.lock))
                    process.start()
                    process_pool.append(process)

                # Remove completed tasks from process pool
                for process in process_pool:
                    if not process.is_alive():
                        process.join()
                        process_pool.remove(process)

        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_task(): {e}", exc_info=True)

    def execute_task(self, task_name:str, dependency_graph: DiGraph, dependency_count: dict[str, int], ready_queue: multiprocessing.Queue, lock: multiprocessing.Lock):
        """Executes a single task in a separate process"""
        try:
            task_config = self.task_configs.get(task_name, {})
            if not task_config:
                self.logger.error(f"Task '{task_name}' not found in configuration.")
                return
            task_config_dict = task_config.get("task_config_dict", {})
            if not task_config_dict:
                raise KeyError(f"task_configs[{task_name}] does not have 'task_config_dict' attribute.")
            task_type = task_config_dict.get("task_type", "")
            if task_type == "c_compile":
                self.logger.debug(f"Executing execute_c_compile()")
                process = self.execute_c_compile(task_name)
            else:
                self.logger.error(f"Undefined 'task_type' in task_configs[{task_name}]['task_config_dict'].")
                process = None

            if process:
                process.wait() # Ensure task completion before marking it as done

            with lock:
                for dependent in dependency_graph.successors(task_name):
                    dependency_count[dependent] -= 1
                    if dependency_count[dependent] == 0:
                        ready_queue.put(dependent)
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_task(): {e}", exc_info=True)


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

