import os
import pytest
from pathlib import Path
from toolConfigParser.ToolConfigParser import ToolConfigParser
from unittest.mock import MagicMock, patch, mock_open

@pytest.fixture
def mock_yaml_loader():
    """Fixture for open and safe_load"""
    with patch("builtins.open", mock_open()) as m, patch("yaml.safe_load") as mock_yaml:
        yield m, mock_yaml

def test_load_and_validate_tool_config_valid(mock_yaml_loader):
    """Test loading and validating a valid tool_config.yaml"""
    m, mock_yaml = mock_yaml_loader
    tool_path = "/mock/path/to/gcc"
    mock_yaml.return_value = {"gcc": tool_path}
    mock_logger = MagicMock()

    with patch("pathlib.Path.exists", return_value=True), \
        patch("pathlib.Path.is_file", return_value=True), \
        patch("shutil.which", return_value=tool_path):
        tool_config_parser = ToolConfigParser(mock_logger, proj_root="/mock/project")
        print(tool_config_parser.validated_tools)
        assert tool_config_parser.validated_tools["gcc"] == tool_path

def test_load_and_validate_tool_config_missing_tool_config_file():
    """Test loading and validating a tool_config.yaml but it does not exist"""
    with patch("pathlib.Path.exists", return_value=False):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")

    tool_config_parser.logger.error.assert_called_once_with(f"FileNotFoundError: Expected tool_config.yaml at: /mock/project/tool_config.yaml")

