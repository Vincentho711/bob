import yaml
from pathlib import Path
from logging import Logger
import shutil

class ToolConfigParser:
    def __init__(self, logger: Logger, proj_root: str):
        self.proj_root = Path(proj_root).resolve()
        self.logger = logger
        self.tool_config_path = self.proj_root / "tool_config.yaml"
        self.tool_paths: dict[str, str] = {}
        self.validated_tools: dict[str, str] = {}
        self._load_and_validate_tool_config()

    def _load_and_validate_tool_config(self):
        """Load the 'tool_config.yaml' from project root and validate it"""
        try:
            if not self.tool_config_path.exists():
                raise FileNotFoundError(f"Expected tool_config.yaml at: {self.tool_config_path}")

            with open(self.tool_config_path, "r") as file:
                self.tool_paths = yaml.safe_load(file) or {}

            if not isinstance(self.tool_paths, dict):
                raise ValueError(f"Malformed YAML: tool_config.yaml should be a dict of tools.")

            for tool, configured_path in self.tool_paths.items():
                resolved = shutil.which(str(configured_path))
                if resolved:
                    self.validated_tools[tool] = resolved
                else:
                    raise FileNotFoundError(f"Tool '{tool}' specified as '{configured_path}' not found in PATH or as an absolute path.")

            self.logger.debug(f"Tool config '{str(self.tool_config_path)}' validated successfully.")

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during _load_and_validate_tool_config(): {e}", exc_info=True)

    def get_tool_path(self, tool_name: str) -> str:
        """Returns the absolute path to the tool. First checks validated tools from the config. If not found, falls back to system PATH."""
        try:
            if tool_name in self.validated_tools:
                return self.validated_tools[tool_name]

            fallback = shutil.which(tool_name)
            if fallback:
                return fallback

            raise FileNotFoundError(f"Tool '{tool_name}' not specified in tool_config.yaml and not found in system PATH.")

        except FileNotFoundError as fnfe:
            self.logger.error(f"FileNotFoundError: {fnfe}")

        except Exception as e:
            self.logger.critical(f"Unexpected error during get_tool_path(): {e}", exc_info=True)

    def has_tool(self, tool_name: str) -> bool:
        """Returns whether a tool exists and is available to be used."""
        try:
            return tool_name in self.validated_tools or shutil.which(tool_name) is not None

        except Exception as e:
            self.logger.critical(f"Unexpected error during has_tool(): {e}", exc_info=True)
