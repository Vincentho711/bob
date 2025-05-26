import yaml
from pathlib import Path
from logging import Logger
import shutil

class ToolConfigParser:
    def __init__(self, logger: Logger, proj_root: str):
        self.proj_root: Path = Path(proj_root).resolve()
        self.bob_root: Path = Path(__file__).resolve().parent.parent # Specifies the root which contains Bob implementation and its helper classes
        self.logger = logger
        self.tool_config_path = self.proj_root / "tool_config.yaml"
        self.tool_paths: dict[str, str] = {}
        self.tool_flags: dict[str, list[str]] = {}
        self.validated_tools: dict[str, str] = {}
        self._load_and_validate_tool_config()

    def _load_and_validate_tool_config(self):
        """Load the 'tool_config.yaml' from project root and and validate tool paths and default flags."""
        try:
            if not self.tool_config_path.exists():
                raise FileNotFoundError(f"Expected tool_config.yaml at: {self.tool_config_path}")

            with open(self.tool_config_path, "r") as file:
                tool_config = yaml.safe_load(file) or {}

            if not isinstance(tool_config, dict):
                raise ValueError(f"Malformed YAML: tool_config.yaml should be a dict of tools.")

            for tool, tool_entry in tool_config.items():
                if isinstance(tool_entry, str): # If a tool is only specified with legacy format which is  {tool} : {tool_path}
                    resolved = shutil.which(tool_entry)
                    if not resolved:
                        raise FileNotFoundError(f"Tool '{tool}' specified as '{tool_entry}' not found in PATH.")
                    self.tool_paths[tool] = resolved
                    self.tool_flags[tool] = []
                elif isinstance(tool_entry, dict):
                    path = tool_entry.get("path", "")
                    flags = tool_entry.get("default_flags", [])
                    resolved = shutil.which(path)
                    if not resolved:
                        raise FileNotFoundError(f"Tool '{tool}' path '{path}' not found in PATH.")
                    if not isinstance(flags, list):
                        raise ValueError(f"Tool '{tool}': 'default_flags' must be a list of strings.")
                    self.tool_paths[tool] = resolved
                    self.tool_flags[tool] = flags
                else:
                    raise ValueError(f"Tool '{tool}' entry must be either a string or a dict.")

            self.logger.debug(f"Tool config '{str(self.tool_config_path)}' validated successfully.")

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during _load_and_validate_tool_config(): {e}", exc_info=True)

    def get_tool_path(self, tool_name: str) -> str:
        """Return the absolute path of the tool."""
        try:
            return self.tool_paths.get(tool_name) or shutil.which(tool_name) or self._raise_tool_not_found(tool_name)

        except Exception as e:
            self.logger.critical(f"Unexpected error in get_tool_path(): {e}", exc_info=True)

    def get_tool_flags(self, tool_name: str) -> list[str]:
        """Return the default flags for the tool."""
        try:
            return self.tool_flags.get(tool_name, [])

        except Exception as e:
            self.logger.critical(f"Unexpected error in get_tool_flags(): {e}", exc_info=True)

    def get_command(self, tool_name: str, extra_flags: list[str] = None) -> list[str]:
        """
        Returns the full command to run the tool, including path and default flags.
        Optionally appends `extra_flags` at the end.
        """
        try:
            path = self.get_tool_path(tool_name)
            flags = self.get_tool_flags(tool_name)
            return [path] + flags + (extra_flags if extra_flags else [])
        except Exception as e:
            self.logger.critical(f"Unexpected error in get_command(): {e}", exc_info=True)

    def has_tool(self, tool_name: str) -> bool:
        """Returns True if the tool is valid and available."""
        try:
            return tool_name in self.tool_paths or shutil.which(tool_name) is not None

        except Exception as e:
            self.logger.critical(f"Unexpected error during has_tool(): {e}", exc_info=True)

    def _raise_tool_not_found(self, tool_name: str):
        try:
            raise FileNotFoundError(f"Tool '{tool_name}' not specified in tool_config.yaml and not found in system PATH.")

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")

        except Exception as e:
            self.logger.critical(f"Unexpected error in _raise_tool_not_found(): {e}", exc_info=True)
