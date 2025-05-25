from io import TextIOWrapper
from pathlib import Path
from typing import Any, Dict
from networkx import DiGraph, topological_sort, is_directed_acyclic_graph
from toolConfigParser.ToolConfigParser import ToolConfigParser
from ipConfigParser.IpConfigParser import IpConfigParser
from taskConfigParser.TaskConfigParser import TaskConfigParser
import os
import sys
import yaml
import subprocess
import multiprocessing
import logging
import shutil
import hashlib
import json
import datetime

class Bob:
    def __init__(self, logger: logging.Logger) -> None:
        self.name = "bob"
        self.logger = logger
        self.proj_root: str = os.environ.get("PROJ_ROOT", "Not Set")
        self.tool_config_parser = None
        self.ip_config_parser = None
        self.task_config_parser = None
        self.ip_config: Dict[str, Any] = {}
        self.task_configs: Dict[str, Any] = {}
        self.bob_root: Path = Path(__file__).resolve().parent.parent # Specifies the root which contains Bob implementation and its helper classes
        self.build_scripts_dir: Path = self.bob_root / "build_scripts"
        self.dotbob_dir: Path = Path(self.proj_root) / ".bob"
        self.dotbob_checksum_file: Path = self.dotbob_dir / "checksum.json"
        self.dependency_graph = None

    def get_proj_root(self) -> Path:
        return Path(self.proj_root)

    def associate_tool_config_parser(self, tool_config_parser: ToolConfigParser) -> None:
        """Associate a ToolConfigParser object to its 'tool_config_parser' attribute"""
        try:
            if not isinstance(tool_config_parser, ToolConfigParser):
                raise TypeError(f"tool_config_parser must be a ToolConfigParser object. type(tool_config_parser) = {type(tool_config_parser)}.")

            self.tool_config_parser = tool_config_parser

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during associate_tool_config_parser(): {e}", exc_info=True)

    def instantiate_and_associate_tool_config_parser(self) -> None:
        """Instantiate a ToolConfigParser and associate it to its 'tool_config_parser' attribute"""
        try:
            tool_config_parser = ToolConfigParser(self.logger, self.proj_root)
            self.associate_tool_config_parser(tool_config_parser)

        except Exception as e:
            self.logger.critical(f"Unexpected error during instantiate_and_associate_tool_config_parser(): {e}", exc_info=True)

    def associate_ip_config_parser(self, ip_config_parser: IpConfigParser) -> None:
        """Associate a IpConfigParser object to its 'ip_config_parser' attribute"""
        try:
            if not isinstance(ip_config_parser, IpConfigParser):
                raise TypeError(f"ip_config_parser must be a IpConfigParser object. type(ip_config_parser) = {type(ip_config_parser)}.")

            self.ip_config_parser = ip_config_parser

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during associate_ip_config_parser(): {e}", exc_info=True)

    def instantiate_and_associate_ip_config_parser(self) -> None:
        """Instantiate a IpConfigParser and associate it to its 'ip_config_parser' attribute"""
        try:
            ip_config_parser = IpConfigParser(self.logger, self.proj_root)
            self.associate_ip_config_parser(ip_config_parser)

        except Exception as e:
            self.logger.critical(f"Unexpected error during instantiate_and_associate_ip_config_parser(): {e}", exc_info=True)

    def associate_task_config_parser(self, task_config_parser: TaskConfigParser) -> None:
        """Associate a TaskConfigParser object to its 'task_config_parser' attribute"""
        try:
            if not isinstance(task_config_parser, TaskConfigParser):
                raise TypeError(f"task_config_parser must be a TaskConfigParser object. type(task_config_parser) = {type(task_config_parser)}.")

            self.task_config_parser = task_config_parser

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during associate_task_config_parser(): {e}", exc_info=True)

    def instantiate_and_associate_task_config_parser(self) -> None:
        """Instantiate a TaskConfigParser and associate it to its 'task_config_parser' attribute"""
        try:
            task_config_parser = TaskConfigParser(self.logger, self.proj_root)
            self.associate_task_config_parser(task_config_parser)

        except Exception as e:
            self.logger.critical(f"Unexpected error during instantiate_and_associate_task_config_parser(): {e}", exc_info=True)

    def setup_with_ip_config_parser(self) -> None:
        """Execute functions within ip_config_parser and asscoaite unfiltered dependency graph from ip_config_parser with self.ip_config_parser"""
        try:
            if self.ip_config_parser is None:
                raise AttributeError(f"self.ip_config_parser is None. Please ensure that associate_ip_config_parser() has been run.")

            # Load ip_config.yaml from proj_root
            self.ip_config_parser.load_ip_cfg()

            # Parse the loaded ip_config.yaml
            self.ip_config_parser.parse_ip_cfg()

            # Build unfiltered dependency graph and associate it to self.dependency_graph
            self.dependency_graph = self.ip_config_parser.build_task_dependency_graph()

        except AttributeError as ae:
            self.logger.error(f"AttributeError: {ae}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during setup_with_ip_config_parser(): {e}", exc_info=True)

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

    def _compute_task_input_src_files_hash_sha256(self, task_name: str) -> str | None:
        """Compute a SHA256 checksum from a list of input src files for a task"""
        try:
            if task_name not in self.task_configs:
                self.logger.error(f"Task '{task_name}' does not exist in task_configs. Please run discover_tasks() first.")
                return None

            input_src_files = self.task_configs[task_name].get("input_src_files")
            if not input_src_files:
                self.logger.error(f"Task '{task_name}' has no input source files defined.")
                return None

            task_config_file_path = self.task_configs[task_name].get("task_config_file_path")
            if not task_config_file_path:
                self.logger.error(f"Task '{task_name}' does not contain a 'task_config_file_path' attribute within task_configs[{task_name}].")
                return None
            task_config_file_path = str(task_config_file_path)

            # Prepare and sort the list of files for checksum calculation
            all_src_files = sorted(input_src_files + [task_config_file_path])
            self.logger.debug(f"Task '{task_name}' source files sorted: {all_src_files}")
            print(f"Task '{task_name}' source files sorted: {all_src_files}")

            hash_sha256 = hashlib.sha256()
            for file_path in map(Path, all_src_files):
                print(type(file_path))
                if file_path.exists() and file_path.is_file():
                    with file_path.open("rb") as f:
                        while chunk := f.read(8192):
                            hash_sha256.update(chunk)
            computed_hash = hash_sha256.hexdigest()
            self.logger.debug(f"Computed hash_sha256 for task '{task_name}': {computed_hash}")
            print(f"Computed hash_sha256 for task '{task_name}': {computed_hash}")
            return computed_hash

        except Exception as e:
            self.logger.critical(f"Unexpected error during _compute_task_input_src_files_hash_sha256(): {e}", exc_info=True)

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

            task_config_file_path = self.task_configs[task_name].get("task_config_file_path")

            if not task_config_file_path:
                self.logger.error(f"No 'task_config_file_path' attribute defined within task_configs[{task_name}]. Skipping build for this task.")
                return False

            internal_src_files = self.task_configs[task_name].get("internal_src_files", [])

            if not internal_src_files:
                self.logger.error(f"No internal source files defined for task {task_name}. Skipping build for this task.")
                return False

            current_hash_sha256 = self._compute_task_input_src_files_hash_sha256(task_name)

            if current_hash_sha256 is None:
                raise RuntimeError(f"_compute_task_src_files_checksum() returned None, hence current checksum cannot be computed for task {task_name}.")

            dotbob_checksum_file_dict: dict = self._load_dotbob_checksum_file()
            previous_task_checksum_entry = dotbob_checksum_file_dict.get(task_name, {})
            previous_hash_sha256 = previous_task_checksum_entry.get("hash_sha256")
            hash_sha256_is_dirty = previous_task_checksum_entry.get("dirty", True)

            self.logger.debug(f"Task '{task_name}' is_dirty = {hash_sha256_is_dirty}")
            self.logger.debug(f"    previous_hash_sha256 = {previous_hash_sha256}")
            self.logger.debug(f"    current_hash_sha256  = {current_hash_sha256}")

            if previous_hash_sha256 == current_hash_sha256 and not hash_sha256_is_dirty:
                self.logger.info(f"For task {task_name}, hash_sha256 unchanged. Will skip this build if all its dependencies also can be skipped.")
                return False

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

    def mark_task_as_dirty_in_dotbob_checksum_file(self, task_name: str) -> None:
        """Mark a task as dirty before the build is started"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs. Please ensure discover_tasks() have been executed first.")

            dotbob_checksum_file_dict: dict = self._load_dotbob_checksum_file()

            if task_name not in dotbob_checksum_file_dict:
                raise KeyError(f"Within mark_task_as_dirty_in_dotbob_checksum_file(), the task '{task_name}' does not exists in checksum.json, aborting marking it as dirty.")

            dotbob_checksum_file_dict[task_name]["dirty"] = True

            self._save_dotbob_checksum_file(dotbob_checksum_file_dict)
            self.logger.debug(f"Marked task '{task_name}' as dirty.")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during mark_task_as_dirty_in_dotbob_checksum_file(): {e}", exc_info=True)
            return None

    def mark_task_as_clean_in_dotbob_checksum_file(self, task_name: str) -> None:
        """Mark a task as clean and compute the updated hash_sha256 after a successful build"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs. Please ensure discover_tasks() have been executed first.")

            dotbob_checksum_file_dict: dict = self._load_dotbob_checksum_file()

            if task_name not in dotbob_checksum_file_dict:
                raise KeyError(f"Within mark_task_as_clean_in_dotbob_checksum_file(), the task '{task_name}' does not exists in checksum.json, aborting marking it as clean.")

            dotbob_checksum_file_dict[task_name]["dirty"] = False

            updated_hash_sha256 = self._compute_task_input_src_files_hash_sha256(task_name)
            self.logger.debug(f"Marking task '{task_name}' as clean after successful build.")
            self.logger.debug(f"Previous hash_sha256={dotbob_checksum_file_dict[task_name]["hash_sha256"]}")
            self.logger.debug(f"Updated hash_sha256={updated_hash_sha256}")
            print(f"Marking task '{task_name}' as clean after successful build.")
            print(f"Previous hash_sha256={dotbob_checksum_file_dict[task_name]["hash_sha256"]}")
            print(f"Updated hash_sha256={updated_hash_sha256}")
            dotbob_checksum_file_dict[task_name]["hash_sha256"] = updated_hash_sha256

            self._save_dotbob_checksum_file(dotbob_checksum_file_dict)
            self.logger.debug(f"Marked task '{task_name}' as clean and updated hash_sha256.")
            print(f"Marked task '{task_name}' as clean and updated hash_sha256.")

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

    def run_subprocess(self, task_name, cmd, env, log_file):
        """Executes a command as a subprocess and logs output line-by-line."""
        try:
            with subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as process:
                for line in process.stdout:
                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    formatted_line = f"[{timestamp}] {line.strip()}"
                    print(formatted_line)  # Prints to log file (redirected)
                    log_file.write(formatted_line + "\n")

                process.wait()
                return process.returncode == 0

        except Exception as e:
            self.logger.critical(f"Unexpected error during run_subprocess() for task_name = '{task_name}' : {e}", exc_info=True)

    def execute_c_compile(self, task_name: str) -> bool:
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

            output_dir = self.task_configs[task_name].get("output_dir", None)
            if output_dir is None:
                self.logger.error(f"Task '{task_name}' does not have the mandatory attribute 'output_dir'. Aborting execute_c_compile().")
                return False

            # Prepare log file
            log_file_path = output_dir / f"{task_name}.log"
            log_file = self.setup_task_logger(log_file_path)

            # Promote task_configs attributes to env var such that they can be used in subprocess
            # Ensuer that all src_files exist
            # Promote internal_src_files, external_src_files and output_src_files to task env var "C_COMPILE_SRC_FILES"
            if not self.ensure_src_files_existence(task_name):
                self.logger.error(f"Aborting execute_c_compile as one or more source files are missing.")
                return False
            src_files = sum((self.task_configs[task_name].get(key, []) for key in ["internal_src_files", "external_src_files", "output_src_files"]), [])
            self.logger.debug(f"Task '{task_name}' has src_files={src_files}")
            self.update_task_env(task_name, "C_COMPILE_SRC_FILES", src_files)
            executable_name = task_config_dict.get("executable_name", None)
            if executable_name:
                self.update_task_env(task_name, "C_COMPILE_EXECUTABLE_NAME", executable_name)

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

            # Extract the path of gcc
            gcc_path = self.tool_config_parser.get_tool_path("gcc")
            self.logger.debug(f"gcc_path={gcc_path}")

            # Execute gcc compile
            # Compile each .c file to .o file
            object_files = []
            for src in src_files:
                # Only operate on .c files
                if not src.endswith(".c"):
                    self.logger.debug(f"Skipping compilation into .o for non .c source file: {src}")
                    continue

                obj_file = os.path.join(output_dir, os.path.basename(src).replace(".c", ".o"))
                cmd_compile = [gcc_path, "-c", src, "-o", obj_file]
                if include_header_dirs:
                    for inc_dir in include_header_dirs:
                        cmd_compile.extend(["-I", inc_dir])
                self.logger.info(f"Executing c_compile command: {cmd_compile}")
                print(f"Executing c_compile command: {cmd_compile}")
                success = self.run_subprocess(task_name, cmd_compile, task_env, log_file)

                if not success:
                    self.logger.error(f"GCC compilation failed for file '{src}'. Check log: {log_file_path}")
                    print(f"GCC compilation failed for file '{src}'. Check log: {log_file_path}")
                    return False

                object_files.append(obj_file)

            self.logger.info(f"GCC compilation succeeded for task '{task_name}'. Output: {object_files}")
            print(f"GCC compilation succeeded for task '{task_name}'. Output: {object_files}")

            # If 'executable_name' exists within task_config.yaml, then link object files into an executable
            # Link all .o files, including external ones) to create the final executable
            if executable_name:
                cmd_link = [gcc_path] + object_files + external_objects + ["-o", executable_path]
                self.logger.info(f"Executing c_link command: {cmd_link}")
                print(f"Executing c_link command: {cmd_link}")
                success = self.run_subprocess(task_name, cmd_link, task_env, log_file)

                if not success:
                    self.logger.error(f"GCC compilation failed for task '{task_name}'. Check log: {log_file_path}")
                    print(f"GCC compilation failed for task '{task_name}'. Check log: {log_file_path}")
                    return False

                self.logger.info(f"GCC link succeeded for task '{task_name}'. Output: {executable_path}")
                print(f"GCC link succeeded for task '{task_name}'. Output: {executable_path}")
            return True

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return False
        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_task_configs_output_src_files(): {e}", exc_info=True)
            return False

    def execute_cpp_compile(self, task_name: str) -> bool:
        """Execute a C++ compile with g++, using attributes within env var"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs.")
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            if task_config_dict is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_config_dict' attribute.")
            task_type = task_config_dict.get("task_type", None)
            if task_type is None:
                raise ValueError(f"Task '{task_name}' does not have a 'task_type' attribute.")
            if task_type != "cpp_compile":
                raise ValueError(f"Task '{task_name}' has task type '{task_type}' which is not 'cpp_compile'. Please exeute the correct function to execute build.")
            # Resolve output_src_files list
            self.resolve_task_configs_output_src_files(task_name)
            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_env' attribute.")

            output_dir = self.task_configs[task_name].get("output_dir", None)
            if output_dir is None:
                self.logger.error(f"Task '{task_name}' does not have the mandatory attribute 'output_dir'. Aborting execute_c_compile().")
                return False

            # Prepare log file
            log_file_path = output_dir / f"{task_name}.log"
            log_file = self.setup_task_logger(log_file_path)

            # Promote task_configs attributes to env var such that they can be used in subprocess
            # Ensuer that all src_files exist
            # Promote internal_src_files, external_src_files and output_src_files to task env var "C_COMPILE_SRC_FILES"
            if not self.ensure_src_files_existence(task_name):
                self.logger.error(f"Aborting execute_c_compile as one or more source files are missing.")
                return False
            src_files = sum((self.task_configs[task_name].get(key, []) for key in ["internal_src_files", "external_src_files", "output_src_files"]), [])
            self.logger.debug(f"Task '{task_name}' has src_files={src_files}")
            self.update_task_env(task_name, "CPP_COMPILE_SRC_FILES", src_files)
            executable_name = task_config_dict.get("executable_name", None)
            if executable_name:
                self.update_task_env(task_name, "CPP_COMPILE_EXECUTABLE_NAME", executable_name)
                executable_path = output_dir / executable_name
                self.update_task_env(task_name, "CPP_COMPILE_EXECUTABLE_PATH", executable_path)

            # Check if we have external objects
            external_objects = self.task_configs[task_name].get("external_objects", [])
            if external_objects:
                self.logger.debug(f"Task '{task_name}' has external_objects={external_objects}")
                self.update_task_env(task_name, "CPP_COMPILE_EXTERNAL_OBJECT_PATHS", external_objects)

            # During compilation, header dirs containing the header files will have to be included
            include_header_dirs = self.task_configs[task_name].get("include_header_dirs", [])
            if include_header_dirs:
                self.logger.debug(f"Task '{task_name}' has include_header_dirs='{include_header_dirs}'")
                self.update_task_env(task_name, "CPP_COMPILE_INCLUDE_HEADER_DIRS", str(include_header_dirs))

            # Extract the path of gcc
            gpp_path = self.tool_config_parser.get_tool_path("g++")
            self.logger.debug(f"gpp_path={gpp_path}")

            # Execute g++ compile
            # Compile each .cpp file to .o file
            object_files = []
            for src in src_files:
                # Only operate on .cpp files
                if not src.endswith(".cpp"):
                    self.logger.debug(f"Skipping compilation into .o for non .cpp source file: {src}")
                    continue

                obj_file = os.path.join(output_dir, os.path.basename(src).replace(".cpp", ".o"))
                cmd_compile = [gpp_path, "-c", src, "-o", obj_file]
                if include_header_dirs:
                    for inc_dir in include_header_dirs:
                        cmd_compile.extend(["-I", inc_dir])
                self.logger.info(f"Executing cpp_compile command: {cmd_compile}")
                print(f"Executing cpp_compile command: {cmd_compile}")
                success = self.run_subprocess(task_name, cmd_compile, task_env, log_file)

                if not success:
                    self.logger.error(f"G++ compilation failed for file '{src}'. Check log: {log_file_path}")
                    print(f"G++ compilation failed for file '{src}'. Check log: {log_file_path}")
                    return False

                object_files.append(obj_file)

            self.logger.info(f"G++ compilation succeeded for task '{task_name}'. Output: {object_files}")
            print(f"G++ compilation succeeded for task '{task_name}'. Output: {object_files}")

            # If 'executable_name' exists within task_config.yaml, then link object files into an executable
            # Link all .o files, including external ones) to create the final executable
            if executable_name:
                cmd_link = [gpp_path] + object_files + external_objects + ["-o", executable_path]
                self.logger.info(f"Executing cpp_link command: {cmd_link}")
                print(f"Executing cpp_link command: {cmd_link}")
                success = self.run_subprocess(task_name, cmd_link, task_env, log_file)

                if not success:
                    self.logger.error(f"G++ compilation failed for task '{task_name}'. Check log: {log_file_path}")
                    print(f"G++ compilation failed for task '{task_name}'. Check log: {log_file_path}")
                    return False

                self.logger.info(f"G++ link succeeded for task '{task_name}'. Output: {executable_path}")
                print(f"G++ link succeeded for task '{task_name}'. Output: {executable_path}")
            return True

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return False

        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_cpp_compile(): {e}", exc_info=True)
            return False

    def execute_verilator_verilate(self, task_name:str) -> bool:
        """Execute verilation into C++ model with verilator"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs.")
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            if task_config_dict is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_config_dict' attribute.")
            task_type = task_config_dict.get("task_type", None)
            if task_type is None:
                raise ValueError(f"Task '{task_name}' does not have a 'task_type' attribute.")
            if task_type != "verilator_verilate":
                raise ValueError(f"Task '{task_name}' has task type '{task_type}' which is not 'verilator_verilate'. Please execute the correct function to execute build.")
            # Resolve output_src_files list
            self.resolve_task_configs_output_src_files(task_name)
            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_env' attribute.")

            output_dir = self.task_configs[task_name].get("output_dir", None)
            if output_dir is None:
                self.logger.error(f"Task '{task_name}' does not have the mandatory attribute 'output_dir'. Aborting execute_verilator_verilate().")
                return False

            # Prepare log file
            log_file_path = output_dir / f"{task_name}.log"
            log_file = self.setup_task_logger(log_file_path)

            # Ensuer that all src_files exist
            if not self.ensure_src_files_existence(task_name):
                self.logger.error(f"Aborting execute_verilator_verilate as one or more source files are missing.")
                return False

            # Promote task_configs attributes to env var such that they can be used in subprocess
            src_files = sum((self.task_configs[task_name].get(key, []) for key in ["internal_src_files", "external_src_files", "output_src_files"]), [])
            self.logger.debug(f"Task '{task_name}' has src_files={src_files}")
            self.update_task_env(task_name, "VERILATOR_VERILATE_SRC_FILES", src_files)
            top_module = task_config_dict.get("top_module", None)
            if top_module:
                self.logger.debug(f"Using top_module = '{top_module}' for task '{task_name}'.")
                self.update_task_env(task_name, "VERILATOR_VERILATE_TOP_MODULE", top_module)

            # Extract the path of verilator
            verilator_path = self.tool_config_parser.get_tool_path("verilator")
            self.logger.debug(f"gpp_path={verilator_path}")

            # Execute 'verilator -cc {VERILATOR_VERILATE_SRC_FILES} --Mdir {output_dir} [--top-module {VERILATOR_VERILATE_TOP_MODULE}]'
            cmd_verilate = [verilator_path, "--cc", *src_files, "--Mdir", output_dir]
            if top_module:
                cmd_verilate.extend(["--top-module", top_module])

            self.logger.info(f"Executing verilator_verilate command: {cmd_verilate}")
            print(f"Executing verilator_verilate command: {cmd_verilate}")
            success = self.run_subprocess(task_name, cmd_verilate, task_env, log_file)

            if not success:
                self.logger.error(f"Verilator verilation failed for task '{task_name}'. Check log: {log_file_path}")
                print(f"Verilator verilation failed for task '{task_name}'. Check log: {log_file_path}")
                return False

            self.logger.info(f"Verilator verilation succeeded for task '{task_name}'. Output_dir: {output_dir}")
            print(f"Verilator verilation succeeded for task '{task_name}'. Output_dir: {output_dir}")
            return True

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return False

        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_verilator_verilate() : {e}", exc_info=True)
            return False

    def execute_verilator_tb_compile(self, task_name: str) -> bool:
        """Execute a tb compilation with verilator, using attributes within env var"""
        try:
            if task_name not in self.task_configs:
                raise ValueError(f"Task '{task_name}' not found in task_configs.")
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            if task_config_dict is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_config_dict' attribute.")
            task_type = task_config_dict.get("task_type", None)
            if task_type is None:
                raise ValueError(f"Task '{task_name}' does not have a 'task_type' attribute.")
            if task_type != "verilator_tb_compile":
                raise ValueError(f"Task '{task_name}' has task type '{task_type}' which is not 'verilator_tb_compile'. Please exeute the correct function to execute build.")
            # Resolve output_src_files list
            self.resolve_task_configs_output_src_files(task_name)
            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_env' attribute.")

            output_dir = self.task_configs[task_name].get("output_dir", None)
            if output_dir is None:
                raise KeyError(f"Task '{task_name}' does not have the mandatory attribute 'output_dir'. Aborting execute_verilator_tb_compile().")

            # Prepare log file
            log_file_path = output_dir / f"{task_name}.log"
            log_file = self.setup_task_logger(log_file_path)

            # Ensuer that all src_files exist
            if not self.ensure_src_files_existence(task_name):
                raise FileNotFoundError(f"Aborting execute_verilator_verilate as one or more source files are missing.")

            # Extract the path of verilator
            verilator_path = self.tool_config_parser.get_tool_path("verilator")
            self.logger.debug(f"verilator_path={verilator_path}")

            # Ensure that build_scripts/verilator.mk exists
            verilator_mk_path = self.build_scripts_dir / "verilator.mk"
            if not verilator_mk_path.is_file():
                raise FileNotFoundError(f"Task '{task_name}' is a 'verilator_tb_compile' task type, hence it requires '{verilator_mk_path}' to exist.")

            cmd_verilate_tb_compile_make = [
                "make",
                "-C", str(output_dir),
                "-f", str(verilator_mk_path)
            ]

            self.logger.info(f"Executing verilator_tb_compile command: {cmd_verilate_tb_compile_make}")
            print(f"Executing verilator_tb_compile command: {cmd_verilate_tb_compile_make}")

            success = self.run_subprocess(task_name, cmd_verilate_tb_compile_make, task_env, log_file)

            if not success:
                self.logger.error(f"Verilator tb compilation failed for task '{task_name}'. Check log: {log_file_path}")
                print(f"Verilator tb compilation failed for task '{task_name}'. Check log: {log_file_path}")
                return False

            self.logger.info(f"Verilator tb compilation succeeded for task '{task_name}'. Output_dir: {output_dir}")
            print(f"Verilator tb compilation succeeded for task '{task_name}'. Output_dir: {output_dir}")
            return True

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return False

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")
            return False

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return False

        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_verilator_tb_compile() : {e}", exc_info=True)
            return False

    def filter_tasks_to_rebuild(self) -> DiGraph:
        """Returns a filtered dependency graph with only tasks that need to be rebuilt."""
        # A task must be rebuilt if:
        # - It explicitly needs rebuilding (should_rebuild_task is True)
        # - Any of its prerequisite tasks require rebuilding
        try:
            rebuild_graph = DiGraph()

            tasks_to_rebuild = set()
            checked_tasks = set()

            def should_rebuild_recursive(task):
                """Recursively determines if a task needs rebuilding."""
                if task in checked_tasks:
                    return task in tasks_to_rebuild  # If already checked, return previous decision

                checked_tasks.add(task)

                if self.should_rebuild_task(task):  # Task itself requires rebuild
                    tasks_to_rebuild.add(task)
                    return True

                # If any dependency needs a rebuild, this task must also rebuild
                for dependency in self.dependency_graph.predecessors(task):
                    if should_rebuild_recursive(dependency):
                        tasks_to_rebuild.add(task)
                        return True

                return False

            # Iterate over all tasks and determine rebuild requirements
            for task in self.dependency_graph.nodes:
                should_rebuild_recursive(task)

            # Construct the rebuild graph with only required tasks
            for task in tasks_to_rebuild:
                rebuild_graph.add_node(task)
                for successor in self.dependency_graph.successors(task):
                    if successor in tasks_to_rebuild:
                        rebuild_graph.add_edge(task, successor)

            self.logger.debug(f"Filtered rebuild graph: {rebuild_graph.nodes}")
            return rebuild_graph

        except Exception as e:
            self.logger.critical(f"Unexpected error in filter_tasks_to_rebuild(): {e}", exc_info=True)
            return DiGraph()  # Return empty graph on failure

    def schedule_tasks(self, dependency_count, ready_queue):
        """Filter dependency_graph based on whether tasks need to be rebuilt. Schedules tasks dynamically while respecting dependencies"""
        try:
            if self.dependency_graph is None:
                raise ValueError(f"self.dependency_graph = None. Please ensure build_task_dependency is run, and Bob's attribute has been updated.")

            filtered_dependency_graph = self.filter_tasks_to_rebuild()
            self.logger.debug(f"Filtered tasks to rebuild. filtered_dependency_graph = {filtered_dependency_graph}")
            self.dependency_graph = filtered_dependency_graph

            # Show visualisation of the filtered dependency graph if there are tasks to be built
            if self.dependency_graph.number_of_nodes():
                self.visualise_dependency_graph()

            # Track number of dependencies (indegree) for each task
            for task in self.dependency_graph.nodes:
                dependency_count[task] = self.dependency_graph.in_degree(task)

            # Populate queue within initial tasks (indegree = 0)
            for task, count in dependency_count.items():
                if count == 0:
                    ready_queue.put(task)
            self.logger.debug(f"Initial ready queue: {ready_queue.qsize()}")
            return dependency_count, ready_queue

        except Exception as e:
            self.logger.critical(f"Unexpected error during schedule_tasks(): {e}", exc_info=True)

    def visualise_dependency_graph(self) -> None:
        """Visualise a directed acyclic graph (DAG) in the terminal using ASCII characters."""
        try:
            dependency_graph = self.dependency_graph
            if not isinstance(dependency_graph, DiGraph):
                raise TypeError(f"Dependency graph must be a directed graph (nx.DiGraph).")

            if not is_directed_acyclic_graph(dependency_graph):
                raise ValueError(f"Dependency graph must be a Directed Acyclic Graph (DAG).")

            # Perform a topological sort to determine execution order
            execution_order = list(topological_sort(dependency_graph))

            # Build adjacency list for easier traversal
            adjacency_list = {node: list(dependency_graph.successors(node)) for node in execution_order}

            # Keep track of visited nodes
            visited_nodes = set()

            # Helper function for DFS traversal and visualisation
            def dfs(node, prefix="", branches=None):
                if branches is None:
                    branches = []

                if node in visited_nodes:
                    return
                visited_nodes.add(node)

                # Construct visual prefix
                branch_str = "".join("│   " if b else "    " for b in branches[:-1])
                branch_str += "├── " if branches else "* "

                print(f"{branch_str}[{node}]")

                children = adjacency_list.get(node, [])
                num_children = len(children)

                for i, child in enumerate(children):
                    new_branches = branches + [i < num_children - 1]
                    dfs(child, prefix, new_branches)

            print("\nDependency Graph Visualisation:\n")

            # Find root nodes (nodes with no incoming edges)
            root_nodes = [n for n in execution_order if dependency_graph.in_degree(n) == 0]

            # Start DFS from root nodes
            for root in root_nodes:
                dfs(root)

            print()

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during visualise_dependency_graph(): {e}")
            return None

    def setup_task_logger(self, log_file_path: Path) -> TextIOWrapper| None:
        """Redirect stdout and stderr to a file for a subprocess (per-task logging)."""
        log_file_path.parent.mkdir(parents=True, exist_ok=True)
        log_file = open(log_file_path, "w", buffering=1)
        sys.stdout = log_file
        sys.stderr = log_file
        return log_file

    def execute_tasks(self):
        """Executes tasks with dynamic scheduling and parallel execution"""
        try:
            if self.tool_config_parser is None:
                raise AttributeError(f"A ToolConfigParser object has not been associated to self.tool_config_parser.")

            with multiprocessing.Manager() as manager:
                dependency_count = manager.dict()
                ready_queue = manager.Queue()
                lock = manager.Lock()
                failure_event = manager.Event()
                failure_info = manager.dict()

                dependency_count, ready_queue = self.schedule_tasks(dependency_count, ready_queue)
                self.logger.debug(f"dependency_count={dependency_count}")

                process_pool = [] # Store active process handles
                # Prevent spawning too many process all at once and spending too much time in context switching
                # Tasks are dispached in controlled batches
                # I.e. If num_workers = 8, only 8 tasks are dispached in a batch
                num_workers = min(multiprocessing.cpu_count(), len(dependency_count))
                self.logger.debug(f"num_workers = {num_workers}")
                self.logger.debug(f"ready_queue = {ready_queue}")

                while not ready_queue.empty() or process_pool:
                    # Launch all available tasks in parallel
                    while not ready_queue.empty() and len(process_pool) < num_workers:
                        try:
                            task = ready_queue.get(timeout=1) # Prevent blocking indefinitely
                        except Empty:
                            break

                        process = multiprocessing.Process(target=self.execute_task, args=(task, self.dependency_graph, dependency_count, ready_queue, lock, failure_event, failure_info))
                        process.start()
                        process_pool.append((task, process))

                    # Check for failure and terminate all tasks if there is a failure
                    if failure_event.is_set():
                        for _, proc in process_pool:
                            if proc.is_alive():
                                proc.terminate()
                        break

                    # Clean up completed processes from process_pool
                    process_pool = [(t, p) for t, p in process_pool if p.is_alive()]

                if failure_event.is_set():
                    failed_task = failure_info.get("task_name", "Unknown Task")
                    log_path = failure_info.get("log_file_path", "Unknown Log Path")
                    self.logger.error(f"Build failed at task '{failed_task}'. Check log: {log_path}")
                else:
                    self.logger.info(f"Successfully built {self.dependency_graph.number_of_nodes()} tasks.")

                self.logger.debug(f"At the end of execute_tasks(): dependency_count={dependency_count}")
                self.logger.debug(f"At the end of execute_tasks(): ready_queue.qsize()={ready_queue.qsize()}")

        except AttributeError as ae:
            self.logger.error(f"AttributeError: {ae}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_task(): {e}", exc_info=True)

    def execute_task(self, task_name:str, dependency_graph: DiGraph, dependency_count: dict[str, int], ready_queue: multiprocessing.Queue, lock: multiprocessing.Lock, failure_event: multiprocessing.Event, failure_info):
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

            # Obtain lock before reading checksum.json, and marking task as dirty
            with lock:
                self.mark_task_as_dirty_in_dotbob_checksum_file(task_name)

            if task_type == "c_compile":
                self.logger.debug(f"Executing execute_c_compile()")
                success = self.execute_c_compile(task_name)
            elif task_type == "cpp_compile":
                self.logger.debug(f"Executing execute_cpp_compile()")
                success = self.execute_cpp_compile(task_name)
            elif task_type == "verilator_verilate":
                self.logger.debug(f"Executing execute_verilator_verilate()")
                success = self.execute_verilator_verilate(task_name)
            elif task_type == "verilator_tb_compile":
                self.logger.debug(f"Executing execute_verilator_tb_compile()")
                success = self.execute_verilator_tb_compile(task_name)
            else:
                self.logger.error(f"Undefined 'task_type' in task_configs[{task_name}]['task_config_dict'].")
                success = False

            self.logger.debug(f"execute_task() for task '{task_name}' completed with success={success}.")

            if success:
                with lock:
                    # Mark task as clean and update hash_sha256 if it runs successfully
                    self.mark_task_as_clean_in_dotbob_checksum_file(task_name)
                    for dependent in dependency_graph.successors(task_name):
                        dependency_count[dependent] -= 1
                        if dependency_count[dependent] == 0:
                            ready_queue.put(dependent)
            else:
                failure_info["task_name"] = task_name
                failure_info["log_file_path"] = str(task_config.get("output_dir") / f"{task_name}.log")
                failure_event.set()

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            failure_info["task_name"] = task_name
            failure_info["log_file_path"] = str(task_config.get("output_dir") / f"{task_name}.log")
            failure_event.set()
        except Exception as e:
            self.logger.critical(f"Unexpected error during execute_task(): {e}", exc_info=True)
            failure_info["task_name"] = task_name
            failure_info["log_file_path"] = str(task_config.get("output_dir") / f"{task_name}.log")
            failure_event.set()


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

