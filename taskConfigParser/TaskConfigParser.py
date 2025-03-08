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
                self.logger.error(f"'{task_name}' already exists within TaskConfigParser.task_configs. Please ensure there is no duplicate task names. Aborting parsing of '{task_config_file_path}'.")
                return None
            self.task_configs[task_name] = {}
            task_entry = self.task_configs[task_name]
            task_entry.setdefault("task_dir", task_config_file_path.parent.absolute())
            task_entry.setdefault("task_config_file_path", task_config_file_path.absolute())
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

            if output_task not in self.task_configs:
                raise ValueError(f"Referenced task '{output_task}' not found in task_configs.")

            output_dir = self.task_configs[output_task].get("output_dir", None)
            if output_dir is None:
                raise ValueError(f"task_configs['{output_task}'] doesn't contain a 'output_dir' attribute. Please ensure a output dir is registered with task '{output_task}' first.")
            elif not output_dir.exists():
                raise FileNotFoundError(f"Output_dir of task '{output_task}' does not exist. Please ensure it exists first.")

            self.logger.debug(f"Calling _resolve_files_spec() with target_dir='{output_dir}', files_spec='{files_spec}'")
            resolved_output_reference = self._resolve_files_spec(Path(output_dir), files_spec)
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

