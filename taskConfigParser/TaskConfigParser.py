from pathlib import Path
import logging
import yaml
import re
import os

class TaskConfigParser:
    def __init__(self, logger: logging.Logger, proj_root: str) -> None:
        self.task_configs = {}
        self.logger = logger
        self.proj_root = proj_root
        self.build_scripts_dir = Path(self.proj_root) / "build_scripts"

    def inherit_task_configs(self, task_configs: dict):
        """Inherit task_configs from bob, and store it as a local attribute"""
        try:
            if isinstance(task_configs, dict):
                self.task_configs = task_configs
            else:
                raise TypeError(f"task_configs has to be of type 'dict', it is currently a {type(task_configs)}.")

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during inherit_task_configs() : {e}", exc_info=True)

    def _load_task_config_file(self, task_config_file_path: Path) -> dict | None:
        """Loads a task_config.yaml given file path"""
        try:
            if not task_config_file_path.exists():
                self.logger.error(f"'{task_config_file_path}' does not exist. Cannot load it.")
                return None
            with task_config_file_path.open("r") as f:
                self.logger.debug(f"Loading '{task_config_file_path}' into a Python dictionary.")
                task_config_dict = yaml.safe_load(f)

            task_name = task_config_dict.get("task_name", None)
            if task_name is None:
                self.logger.error(f"'{task_config_file_path}' doesn't contain a mandatory key 'task_name'. Please ensure it exists first. Aborting parsing this task_config.yaml.")
                return None
            if task_name in self.task_configs:
                self.logger.warning(f"'{task_name}' already exists within TaskConfigParser.task_configs. Updating existing task_configs[{task_name}].")
            else:
                self.task_configs[task_name] = {}
            task_entry = self.task_configs[task_name]
            if "task_dir" in task_entry:
                self.logger.warning(f"Existing attribute task_configs['task_dir']={task_entry['task_dir']} will be replaced with '{task_config_file_path.parent.absolute()}'.")
            task_entry["task_dir"] = task_config_file_path.parent.absolute()

            if "task_config_file_path" in task_entry:
                self.logger.warning(f"Existing attribute task_configs['task_config_file_path']={task_entry['task_config_file_path']} will be replaced with '{task_config_file_path.absolute()}'.")
            task_entry["task_config_file_path"] = task_config_file_path.absolute()

            if "task_config_dict" in task_entry:
                self.logger.warning(f"Existing attribute task_configs['task_config_dict'] already exists. It will be replaced with '{task_config_dict}'")
            task_entry["task_config_dict"] = task_config_dict

            return task_config_dict

        except yaml.YAMLError as ye:
            self.logger.error(f"Error while parsing '{task_config_file_path}': {ye}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during _load_task_config_file() : {e}", exc_info=True)

    def _resolve_files_spec(self, target_dir:Path, files_spec:str) -> str | list[str] | None:
        """Resolve files specification, e.g. content post @output or @input_src"""
        try:
            print(f"files_spec: {files_spec}")
            print(f"target_dir: {target_dir}")
            print(f"isinstance(target_dir, Path): {isinstance(target_dir, Path)}")
            print(f"str(target_dir): {str(target_dir)}")
            if not isinstance(files_spec, str):
                raise TypeError(f"files_spec = '{files_spec}' must be of type str for it to be resolved, it is of type '{type(files_spec)}'.")

            if not isinstance(target_dir, Path):
                raise TypeError(f"target_dir = '{target_dir}' must be of type Path.")

            # Not checking whether the target_dir exists
            if files_spec == "*": # if "*" is used, return the target_dir which contains every files for the task
                self.logger.debug(f"For files_spec = '{files_spec}', resolved reference (entire target dir): {str(target_dir)}")
                return str(target_dir)
            elif files_spec.startswith("[") and files_spec.endswith("]"): # parse it as a list
                requested_file_path_str_list = []
                requested_files = yaml.safe_load(files_spec) # Convert YAML list string to list
                for f in requested_files:
                    requested_file_path = target_dir / Path(f)
                    requested_file_path_str = str(requested_file_path)
                    requested_file_path_str_list.append(requested_file_path_str)
                self.logger.debug(f"For files_spec = '{files_spec}', resolved reference (list of files within target dir): {requested_file_path_str_list}")
                return requested_file_path_str_list
            else: # Assume it is a single file and return the single file file path
                self.logger.debug(f"For files_spec = '{files_spec}', resolved reference (single file within target dir): {str(target_dir / files_spec)}")
                return str(target_dir / files_spec)

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_files_spec() for files_spec = '{files_spec}' : {e}", exc_info=True)
            return None

    def _resolve_output_reference(self, value:str) -> str | list[str] | None:
        """Resolve output directory or specific output file references"""
        try:
            match = re.search(r"\{@output:([^:\[\]]+):(\*|\[.*\]|[^:\[\}]+)\}", value)
            if not match:
                raise ValueError(f"Invalid output reference: '{value}'")

            output_task = match.group(1)
            files_spec = match.group(2)
            print(f"output_task={output_task}, files_spec={files_spec}")

            if output_task not in self.task_configs:
                raise ValueError(f"Referenced task '{output_task}' not found in task_configs.")

            output_dir = self.task_configs[output_task].get("output_dir", None)
            print(f"output_dir={output_dir}")
            if output_dir is None:
                print(f"output_dir is None")
                raise ValueError(f"task_configs['{output_task}'] doesn't contain a 'output_dir' attribute. Please ensure a output dir is registered with task '{output_task}' first.")
            elif not output_dir.exists():
                print(f"output_dir does not exist.")
                raise FileNotFoundError(f"Output_dir of task '{output_task}' does not exist. Please ensure it exists first.")

            print(f"Calling _resolve_files_spec()")
            self.logger.debug(f"Calling _resolve_files_spec() with target_dir='{output_dir}', files_spec='{files_spec}'")
            resolved_output_reference = self._resolve_files_spec(Path(output_dir), files_spec)
            print(f"resolved_output_reference={resolved_output_reference}")
            return resolved_output_reference

        except yaml.YAMLError as ye:
            self.logger.error(f"Error while parsing list of files within output dir for value '{value}': {ye}")
            return None

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")
            return None

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_output_reference() for value'{value}' : {e}", exc_info=True)
            return None

    def _resolve_input_reference(self, value:str) -> str | list[str] | None:
        """Resolve input directory or specific input source files references"""
        try:
            match = re.search(r"\{@input:([^:\[\]]+):(\*|\[.*\]|[^:\[\}]+)\}", value)
            if not match:
                raise ValueError(f"Invalid input reference: '{value}'")

            input_task = match.group(1)
            files_spec = match.group(2)
            print(f"input_task = {input_task}")
            print(f"files_spec = {files_spec}")
            if input_task not in self.task_configs:
                raise ValueError(f"Referenced task '{input_task}' not found in task_configs.")

            task_dir = self.task_configs[input_task].get("task_dir", None)
            if task_dir is None:
                raise ValueError(f"task_configs['{input_task}'] doesn't contain a 'task_dir' attribute. Please ensure a output dir is registered with task '{input_task}' first.")
            elif not task_dir.exists():
                raise FileNotFoundError(f"Task_dir of task '{input_task}' does not exist. Please ensure it exists first.")

            self.logger.debug(f"Calling _resolve_files_spec() with target_dir='{task_dir}', files_spec='{files_spec}'")
            resolved_input_reference = self._resolve_files_spec(Path(task_dir), files_spec)
            return resolved_input_reference

        except yaml.YAMLError as ye:
            self.logger.error(f"Error while parsing list of files within task dir for value '{value}': {ye}")
            return None

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")
            return None

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_input_reference() for value '{value}' : {e}", exc_info=True)
            return None

    def resolve_reference(self, task_name: str, value: str) -> tuple[str | list[str], str] | None:
        """
        Resolves a reference value, whether it's an input, output, or a normal string.
        Returns absolute file paths as strings.
        """
        try:
            resolved_type = "unknown"
            if task_name not in self.task_configs:
                raise KeyError(f"Task {task_name} not found in task configurations.")

            # Check if value matches output reference pattern
            if re.match(r"\{@output:([^:\[\]]+):(\*|\[.*\]|[^:\[\}]+)\}", value):
                print(f"matched output")
                resolved_type = "output"
                resolved_value = self._resolve_output_reference(value)
            # Check if value matches input reference pattern
            elif re.match(r"\{@input:([^:\[\]]+):(\*|\[.*\]|[^:\[\}]+)\}", value):
                resolved_type = "input"
                resolved_value = self._resolve_input_reference(value)
            else:
                # If it's a normal string, assume it's a direct file path within current task_dir
                resolved_type = "direct"
                task_dir_path = Path(self.task_configs[task_name].get("task_dir", None))
                if task_dir_path is None:
                    raise KeyError(f"Task '{task_name}' does not have a 'task_dir' attribute with its task_config.")
                resolved_value = task_dir_path / value  # Join task_dir and value

                # Return the absolute path
                resolved_value = resolved_value.resolve()

                # Convert to string before returning
                resolved_value = str(resolved_value)

            if resolved_value is None:
                raise ValueError(f"Errors during resolving reference with value='{value}' and task_name='{task_name}'.")

            self.logger.debug(f"resolved_value={resolved_value}")

            if isinstance(resolved_value, str):
                resolved_path = Path(resolved_value).resolve()
                self.logger.debug(f"type(resolved_path)={type(resolved_path)}")
                self.logger.debug(f"resolved_path.is_dir()={resolved_path.is_dir()}")
                exclusion_list = {"task_config.yaml"}
                if resolved_path.is_dir() and resolved_type == "input":
                    resolved_filepaths = []
                    for p in resolved_path.iterdir():
                        self.logger.debug(f"p={p}, p.name={p.name}")
                        if p.name not in exclusion_list:
                            resolved_filepaths.append(str(p))
                    return resolved_filepaths, resolved_type
                else:
                    return str(resolved_path), resolved_type

            if isinstance(resolved_value, list):
                return [str(Path(p).resolve()) for p in resolved_value], resolved_type

            raise TypeError(f"Unexpected resolved_value: {resolved_value}, resolved_type: {resolved_type}")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")

        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_reference() for task_name='{task_name}', value='{value}' : {e}", exc_info=True)

    def update_task_env(self, task_name: str, env_key: str, env_val: str | list[str], override_env_val: bool = False) -> None:
        """Updates the environment variables for a given task in self.task_configs."""
        try:
            if task_name not in self.task_configs:
                raise KeyError(f"Task {task_name} not found in task configurations.")

            if "task_env" not in self.task_configs[task_name]:
                self.logger.info(f"Task '{task_name}' does not have an env var dict associated to the 'task_env' key. Creating it from current global env.")

            task_env = self.task_configs[task_name].setdefault("task_env", os.environ.copy())

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

    def parse_task_config_dict(self, task_name: str) -> None:
        """Parse a task config dict based on task type"""
        try:
            validate_sucess = self.validate_initial_task_config_dict(task_name)
            if not validate_sucess:
                return None

            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            task_type = task_config_dict.get("task_type", None)
            match task_type:
                case "c_compile":
                    self.parse_c_compile(task_name)
                case "cpp_compile":
                    self.parse_cpp_compile(task_name)
                case "verilator_verilate":
                    self.parse_verilator_verilate(task_name)
                case "verilator_tb_compile":
                    self.parse_verilator_tb_compile(task_name)
                case _:
                    raise ValueError(f"For task_name = '{task_name}', the task_type = '{task_type}' is not a support task type.")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_task_config_dict() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def validate_initial_task_config_dict(self, task_name: str) -> bool:
        """Validate the task_config_dict has basic mandatory fields initially"""
        try:
            if task_name not in self.task_configs:
                raise KeyError(f"Task {task_name} not found in task configurations.")

            task_env = self.task_configs[task_name].get("task_env", None)
            if task_env is None:
                raise KeyError(f"For task '{task_name}', 'task_env' is not found within task_configs. validate_initial_task_config_dict() failed.")

            task_config_file_path = self.task_configs[task_name].get("task_config_file_path", None)

            if task_config_file_path is None:
                raise KeyError(f"Task '{task_name}' does not have a mandatory 'task_config_file_path' attribute within task_configs.")

            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)

            if task_config_dict is None:
                raise KeyError(f"Task '{task_name}' does not have a 'task_config_dict' attribute within task_configs. Please ensure that load_task_config() has been executed for task '{task_name}' first.")

            task_dir = self.task_configs[task_name].get("task_dir", None)

            if task_dir is None:
                raise KeyError(f"task_configs[{task_name}] does not contain the mandatory field 'task_dir'.")

            output_dir = self.task_configs[task_name].get("output_dir", None)

            if output_dir is None:
                raise KeyError(f"task_configs[{task_name}] does not contain the mandatory field 'output_dir'.")

            task_type = task_config_dict.get("task_type", None)

            if task_type is None:
                raise KeyError(f"{task_config_file_path} does not contain the mandatory field 'task_type'.")

            elif not isinstance(task_type, str):
                raise TypeError(f"{task_config_file_path} has the mandatory field 'task_type' defined as a non string. task_type = {task_type}")

            return True

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return False
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return False

        except Exception as e:
            self.logger.critical(f"Unexpected error during validate_initial_task_config_dict() for task_name = '{task_name}' : {e}", exc_info=True)
            return False

    def resolve_src_files(self, task_name: str, unresolved_src_files: list) -> dict[str, list] | None:
        """Given a list of unresolved src files, resolve them into 'internal_src_files', 'external_src_files' or 'output_src_files'"""
        try:
            resolved_src_files ={
                "internal_src_files": [],
                "external_src_files": [],
                "output_src_files":   []
            }
            # Resolve every src file reference
            for unresolved_src_file in unresolved_src_files:
                resolved_reference, resolved_type = self.resolve_reference(task_name, unresolved_src_file)

                self.logger.debug(f"task_name='{task_name}', unresolved_src_file='{unresolved_src_file}', resolved_reference='{resolved_reference}', resolved_type='{resolved_type}'.")

                if isinstance(resolved_reference, str):
                    if resolved_type == "direct":
                        resolved_src_files["internal_src_files"].append(resolved_reference)
                    elif resolved_type == "input":
                        resolved_src_files["external_src_files"].append(resolved_reference)
                    elif resolved_type == "output":
                        resolved_src_files["output_src_files"].append(resolved_reference)
                elif isinstance(resolved_reference, list):
                    if resolved_type == "direct":
                        resolved_src_files["internal_src_files"].extend(resolved_reference)
                    elif resolved_type == "input":
                        resolved_src_files["external_src_files"].extend(resolved_reference)
                    elif resolved_reference == "output":
                        resolved_src_files["output_src_files"].extend(resolved_reference)
                else:
                    self.logger.warning(f"Resolved reference='{resolved_reference}' is neither of type str or list. type({resolved_reference})={type(resolved_reference)}. Skip appending to resolved_src_files dict.")

            return resolved_src_files

        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_src_files() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def parse_c_compile(self, task_name: str):
        """Set up the task_env for a C compilation task"""
        try:
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            task_config_file_path = self.task_configs[task_name].get("task_config_file_path", None)

            self.logger.debug(f"For task '{task_name}', parsing c_compile task type.")

            # Fetch 'external_objects' if it exists. If it does, add that while linking
            unresolved_external_objects = task_config_dict.get("external_objects", [])
            if not unresolved_external_objects:
                self.logger.debug(f"{task_config_file_path} which is a 'c_compile' build does not contain a 'external_objects' field. Will not link external objects.")
            if isinstance(unresolved_external_objects, str):
                unresolved_external_objects = [unresolved_external_objects]

            self.task_configs[task_name].setdefault("external_objects", [])
            external_objects = self.task_configs[task_name]["external_objects"]
            for unresolved_external_object in unresolved_external_objects:
                self.logger.debug(f"unresolved_external_object={unresolved_external_object}")
                if ".o" not in unresolved_external_object:
                    raise ValueError(f"'.o' must exists in the unresolved_external_object='{unresolved_external_object}'")
                resolved_reference, resolved_type = self.resolve_reference(task_name, unresolved_external_object)
                self.logger.debug(f"task_name='{task_name}', unresolved_external_object='{unresolved_external_object}', resolved_reference='{resolved_reference}', resolved_type='{resolved_type}'.")
                if resolved_type != "output":
                    raise ValueError(f"external object must have resolved_type=output.")
                external_objects.append(resolved_reference)

            # Fetch 'include_headers_from_tasks' if it exists. If it does, add them to the -I option during GCC compilation
            include_headers_from_tasks = task_config_dict.get("include_headers_from_tasks", [])
            if not include_headers_from_tasks:
                self.logger.debug(f"{task_config_file_path} which is a 'c_compile' build does not contain a 'include_headers_from_tasks' field. Will not include header dirs during linking.")
            if isinstance(include_headers_from_tasks, str):
                include_headers_from_tasks = [include_headers_from_tasks]
            self.task_configs[task_name].setdefault("include_header_dirs", [])
            include_header_dirs = self.task_configs[task_name]["include_header_dirs"]
            for task in include_headers_from_tasks:
                # Grab the task dir of the corresponding task
                task_dir = self.task_configs[task].get("task_dir", None)
                if task_dir is None:
                    raise KeyError(f"Task '{task}' does not have a 'task_dir' attribute within task_configs.")
                include_header_dirs.append(task_dir)

            # Fetch 'executable_name' if it exists. If it does, that means .exe should be generated
            executable_name = task_config_dict.get("executable_name", None)
            if executable_name is None:
                self.logger.debug(f"{task_config_file_path} which is a 'c_compile' build does not contain a 'executable_name' field. Only .o will be generated.")
            elif not isinstance(executable_name, str):
                raise TypeError(f"{task_config_file_path} field 'executable_name' is not of type str. Currnt type = {type(executable_name)}. Please ensure it is a str.")
            elif ".exe" not in executable_name:
                raise ValueError(f"{task_config_file_path} field 'executable_name' does not contain '.exe'. Current executable_name='{executable_name}'. Please ensure that '.exe' exists in the str.")
            else:
                self.update_task_env(task_name, "bob_executable_name", executable_name, True)

            # Fetch mandatory key 'src_files'
            unresolved_src_files = task_config_dict.get("src_files", None)
            if unresolved_src_files is None:
                raise KeyError(f"{task_config_file_path} which is a 'c_compile' build does not contain a mandatory field 'src_files'. ")
            elif not isinstance(unresolved_src_files, (str, list)):
                raise TypeError(f"{task_config_file_path} mandatory field 'src_files' should be either a str or a list. Current type = {type(unresolved_src_files)}.")

            # resolved_src_files : list[str] = []

            # Modify it into a list if it is a str for ease of manipulation
            unresolved_src_files = [unresolved_src_files] if isinstance(unresolved_src_files, str) else unresolved_src_files

            # Set up task_configs to contain internal_src_files, external_src_files and output_src_files attributes
            self.task_configs[task_name].setdefault("internal_src_files", [])
            self.task_configs[task_name].setdefault("external_src_files", [])
            self.task_configs[task_name].setdefault("output_src_files", [])

            # Retrieve references
            internal_src_files = self.task_configs[task_name]["internal_src_files"]
            external_src_files = self.task_configs[task_name]["external_src_files"]
            output_src_files = self.task_configs[task_name]["output_src_files"]

            # Resolve every src file reference
            resolved_src_files = self.resolve_src_files(task_name, unresolved_src_files)
            internal_src_files.extend(resolved_src_files["internal_src_files"])
            external_src_files.extend(resolved_src_files["external_src_files"])
            output_src_files.extend(resolved_src_files["external_src_files"])

            # input_src_files consists of internal_src_files and external_src_files
            self.task_configs[task_name].setdefault("input_src_files", internal_src_files + external_src_files)

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None
        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_c_compile() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def parse_cpp_compile(self, task_name: str):
        """Set up the task_env for a C++ compilation task"""
        try:
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            task_config_file_path = self.task_configs[task_name].get("task_config_file_path", None)

            self.logger.debug(f"For task '{task_name}', parsing cpp_compile task type.")

            # Fetch 'external_objects' if it exists. If it does, add that while linking
            unresolved_external_objects = task_config_dict.get("external_objects", [])
            if not unresolved_external_objects:
                self.logger.debug(f"{task_config_file_path} which is a 'cpp_compile' build does not contain a 'external_objects' field. Will not link external objects.")
            if isinstance(unresolved_external_objects, str):
                unresolved_external_objects = [unresolved_external_objects]

            self.task_configs[task_name].setdefault("external_objects", [])
            external_objects = self.task_configs[task_name]["external_objects"]
            for unresolved_external_object in unresolved_external_objects:
                self.logger.debug(f"unresolved_external_object={unresolved_external_object}")
                if ".o" not in unresolved_external_object:
                    raise ValueError(f"'.o' must exists in the unresolved_external_object='{unresolved_external_object}'")
                resolved_reference, resolved_type = self.resolve_reference(task_name, unresolved_external_object)
                self.logger.debug(f"task_name='{task_name}', unresolved_external_object='{unresolved_external_object}', resolved_reference='{resolved_reference}', resolved_type='{resolved_type}'.")
                if resolved_type != "output":
                    raise ValueError(f"external object must have resolved_type=output.")
                external_objects.append(resolved_reference)

            # Fetch 'include_headers_from_tasks' if it exists. If it does, add them to the -I option during G++ compilation
            include_headers_from_tasks = task_config_dict.get("include_headers_from_tasks", [])
            if not include_headers_from_tasks:
                self.logger.debug(f"{task_config_file_path} which is a 'cpp_compile' build does not contain a 'include_headers_from_tasks' field. Will not include header dirs during linking.")
            if isinstance(include_headers_from_tasks, str):
                include_headers_from_tasks = [include_headers_from_tasks]
            self.task_configs[task_name].setdefault("include_header_dirs", [])
            include_header_dirs = self.task_configs[task_name]["include_header_dirs"]
            for task in include_headers_from_tasks:
                # Grab the task dir of the corresponding task
                task_dir = self.task_configs[task].get("task_dir", None)
                if task_dir is None:
                    raise KeyError(f"Task '{task}' does not have a 'task_dir' attribute within task_configs.")
                include_header_dirs.append(task_dir)

            # Fetch 'executable_name' if it exists. If it does, that means .exe should be generated
            executable_name = task_config_dict.get("executable_name", None)
            if executable_name is None:
                self.logger.debug(f"{task_config_file_path} which is a 'cpp_compile' build does not contain a 'executable_name' field. Only .o will be generated.")
            elif not isinstance(executable_name, str):
                raise TypeError(f"{task_config_file_path} field 'executable_name' is not of type str. Currnt type = {type(executable_name)}. Please ensure it is a str.")
            elif ".exe" not in executable_name:
                raise ValueError(f"{task_config_file_path} field 'executable_name' does not contain '.exe'. Current executable_name='{executable_name}'. Please ensure that '.exe' exists in the str.")
            else:
                self.update_task_env(task_name, "bob_executable_name", executable_name, True)

            # Fetch mandatory key 'src_files'
            unresolved_src_files = task_config_dict.get("src_files", None)
            if unresolved_src_files is None:
                raise KeyError(f"{task_config_file_path} which is a 'cpp_compile' build does not contain a mandatory field 'src_files'. ")
            elif not isinstance(unresolved_src_files, (str, list)):
                raise TypeError(f"{task_config_file_path} mandatory field 'src_files' should be either a str or a list. Current type = {type(unresolved_src_files)}.")

            # resolved_src_files : list[str] = []

            # Modify it into a list if it is a str for ease of manipulation
            unresolved_src_files = [unresolved_src_files] if isinstance(unresolved_src_files, str) else unresolved_src_files

            # Set up task_configs to contain internal_src_files, external_src_files and output_src_files attributes
            self.task_configs[task_name].setdefault("internal_src_files", [])
            self.task_configs[task_name].setdefault("external_src_files", [])
            self.task_configs[task_name].setdefault("output_src_files", [])

            # Retrieve references
            internal_src_files = self.task_configs[task_name]["internal_src_files"]
            external_src_files = self.task_configs[task_name]["external_src_files"]
            output_src_files = self.task_configs[task_name]["output_src_files"]

            # Resolve every src file reference
            resolved_src_files = self.resolve_src_files(task_name, unresolved_src_files)
            internal_src_files.extend(resolved_src_files["internal_src_files"])
            external_src_files.extend(resolved_src_files["external_src_files"])
            output_src_files.extend(resolved_src_files["external_src_files"])

            # input_src_files consists of internal_src_files and external_src_files
            self.task_configs[task_name].setdefault("input_src_files", internal_src_files + external_src_files)

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None
        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_cpp_compile() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def parse_verilator_verilate(self, task_name: str):
        """Set up the task env for verilating SystemVerilog source files with verilator"""
        try:
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            task_config_file_path = self.task_configs[task_name].get("task_config_file_path", None)

            self.logger.debug(f"For task '{task_name}', parsing verilator_verilate task type.")

            # Fetch mandatory key 'src_files'
            unresolved_src_files = task_config_dict.get("src_files", None)
            if unresolved_src_files is None:
                raise KeyError(f"{task_config_file_path} which is a 'verilator_verilate' build does not contain a mandatory field 'src_files'. ")
            elif not isinstance(unresolved_src_files, (str, list)):
                raise TypeError(f"{task_config_file_path} mandatory field 'src_files' should be either a str or a list. Current type = {type(unresolved_src_files)}.")

            # resolved_src_files : list[str] = []

            # Modify it into a list if it is a str for ease of manipulation
            unresolved_src_files = [unresolved_src_files] if isinstance(unresolved_src_files, str) else unresolved_src_files

            # Set up task_configs to contain internal_src_files, external_src_files and output_src_files attributes
            self.task_configs[task_name].setdefault("internal_src_files", [])
            self.task_configs[task_name].setdefault("external_src_files", [])
            self.task_configs[task_name].setdefault("output_src_files", [])

            # Retrieve references
            internal_src_files = self.task_configs[task_name]["internal_src_files"]
            external_src_files = self.task_configs[task_name]["external_src_files"]
            output_src_files = self.task_configs[task_name]["output_src_files"]

            # Resolve every src file reference
            resolved_src_files = self.resolve_src_files(task_name, unresolved_src_files)
            internal_src_files.extend(resolved_src_files["internal_src_files"])
            external_src_files.extend(resolved_src_files["external_src_files"])
            output_src_files.extend(resolved_src_files["output_src_files"])

            # input_src_files consists of internal_src_files and external_src_files
            self.task_configs[task_name].setdefault("input_src_files", internal_src_files + external_src_files)

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None
        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_verilator_verilate() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def parse_verilator_tb_compile(self, task_name: str):
        """Set up the task env for executing a tb compilation with verilator"""
        try:
            task_config_dict = self.task_configs[task_name].get("task_config_dict", None)
            task_config_file_path = self.task_configs[task_name].get("task_config_file_path", None)

            self.logger.debug(f"For task '{task_name}', parsing verilator_tb_compile task type.")

            # Check if mandatory "verilator.mk" exists within proj_root/build_scripts dir
            verilator_mk_path = self.build_scripts_dir / "verilator.mk"
            if not verilator_mk_path.is_file():
                raise FileNotFoundError(f"Task '{task_name}' expects '{verilator_mk_path}' to exist.")

            # Set up task_configs to contain internal_src_files, external_src_files and output_src_files attributes
            self.task_configs[task_name].setdefault("internal_src_files", [])
            self.task_configs[task_name].setdefault("external_src_files", [])
            self.task_configs[task_name].setdefault("output_src_files", [])

            # Retrieve references
            internal_src_files = self.task_configs[task_name]["internal_src_files"]
            external_src_files = self.task_configs[task_name]["external_src_files"]
            output_src_files = self.task_configs[task_name]["output_src_files"]


            # A list to store all the unresolved src files that need to be resolved
            unresolved_src_files = []

            # Fetch mandatory key 'rtl_src_files'
            unresolved_rtl_src_files = task_config_dict.get("rtl_src_files", None)

            if unresolved_rtl_src_files is None:
                raise KeyError(f"{task_config_file_path} which is a 'verilator_tb_compile' build does not contain a mandatory field 'rtl_src_files'. ")
            elif not isinstance(unresolved_rtl_src_files, (str, list)):
                raise TypeError(f"{task_config_file_path} mandatory field 'rtl_src_files' should be either a str or a list. Current type = {type(unresolved_rtl_src_files)}.")

            # Modify it into a list if it is a str for ease of manipulation
            unresolved_rtl_src_files = [unresolved_rtl_src_files] if isinstance(unresolved_rtl_src_files, str) else unresolved_rtl_src_files

            # Resolve every rtl src file reference
            resolved_rtl_src_files = self.resolve_src_files(task_name, unresolved_rtl_src_files)
            internal_src_files.extend(resolved_rtl_src_files["internal_src_files"])
            external_src_files.extend(resolved_rtl_src_files["external_src_files"])
            output_src_files.extend(resolved_rtl_src_files["output_src_files"])

            # Update the task env var such that all the rtl src files can be referred to in Make script
            resolved_rtl_src_files_list = [file for files in resolved_rtl_src_files.values() for file in files]
            self.update_task_env(task_name, "RTL_SRC_FILES", resolved_rtl_src_files_list)

            # Fetch Mandatory key 'cpp_src_files'
            unresolved_cpp_src_files = task_config_dict.get("cpp_src_files", None)

            if unresolved_cpp_src_files is None:
                raise KeyError(f"{task_config_file_path} which is a 'verilator_tb_compile' build does not contain a mandatory field 'cpp_src_files'. ")
            elif not isinstance(unresolved_rtl_src_files, (str, list)):
                raise TypeError(f"{task_config_file_path} mandatory field 'cpp_src_files' should be either a str or a list. Current type = {type(unresolved_rtl_src_files)}.")

            unresolved_cpp_src_files = [unresolved_cpp_src_files] if isinstance(unresolved_cpp_src_files, str) else unresolved_cpp_src_files

            # Resolve every cpp src file reference
            resolved_cpp_src_files = self.resolve_src_files(task_name, unresolved_cpp_src_files)
            internal_src_files.extend(resolved_cpp_src_files["internal_src_files"])
            external_src_files.extend(resolved_cpp_src_files["external_src_files"])
            output_src_files.extend(resolved_cpp_src_files["output_src_files"])

            # Update the task env var such that all the cpp src files can be referred to in Make script
            resolved_cpp_src_files_list = [file for files in resolved_cpp_src_files.values() for file in files]
            self.update_task_env(task_name, "CPP_SRC_FILES", resolved_cpp_src_files_list)


            # input_src_files consists of internal_src_files and external_src_files
            self.task_configs[task_name].setdefault("input_src_files", internal_src_files + external_src_files)
            # Add verilator.mk as an input src files such that when it changes, this task will be rebuilt
            self.task_configs[task_name]["input_src_files"].append(verilator_mk_path.as_posix())

        except TypeError as te:
            self.logger.error(f"TypeError: {te}")
            return None
        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")
            return None
        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_verilator_tb_compile() for task_name = '{task_name}' : {e}", exc_info=True)
            return None

    def parse_all_tasks_in_task_configs(self):
        """Iterate through all the tasks defined within self.task_configs, and parse each of them according to their 'task_type'"""
        try:
            if not self.task_configs:
                raise ValueError(f"self.task_configs is empty. Please ensure that inherit_task_configs() has been run.")
            for task in self.task_configs:
                self.logger.debug(f"Within parse_all_task_configs_tasks(): parsing {task} ...")
                task_config = self.task_configs.get(task)
                task_config_file_path = task_config.get("task_config_file_path", None)
                if task_config_file_path is None:
                    raise KeyError(f"Missing 'task_config_file_path' for task '{task}'.")
                # Load the task_config.yaml file into self.task_configs[task]['task_config_dict']
                self._load_task_config_file(task_config_file_path)
                # Parse a task from its self.task_configs[task]['task_config_dict'] and populate self.task_configs[task]
                self.parse_task_config_dict(task)


        except KeyError as ke:
            self.logger.error(f"KeyError: {ke}")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_all_tasks_in_task_configs(): {e}", exc_info=True)
