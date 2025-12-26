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
        patch("shutil.which", return_value=tool_path):
        tool_config_parser = ToolConfigParser(mock_logger, proj_root="/mock/project")
        assert tool_config_parser.tool_paths["gcc"] == tool_path
        assert tool_config_parser.tool_flags["gcc"] == []

def test_load_and_validate_tool_config_missing_tool_config_file():
    """Test loading and validating a tool_config.yaml but it does not exist"""
    with patch("pathlib.Path.exists", return_value=False):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")

    tool_config_parser.logger.error.assert_called_once_with(f"FileNotFoundError: Expected tool_config.yaml at: /mock/project/tool_config.yaml")

def test_load_and_validate_tool_config_invalid_tool_config_file(mock_yaml_loader):
    """Test loading a tool which doesn't exist"""
    m, mock_yaml = mock_yaml_loader
    tool = "gcc"
    configured_path = "/nonexistent/path/to/gcc"
    mock_yaml.return_value = {tool: configured_path}

    with patch("pathlib.Path.exists", return_value=True), \
         patch("shutil.which", return_value=None):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")

    tool_config_parser.logger.error.assert_called_once_with(f"FileNotFoundError: Tool '{tool}' specified as '{configured_path}' not found in PATH.")

def test_get_tool_path_valid_config(mock_yaml_loader):
    """Test getting a valid tool from ToolConfigParser"""
    m, mock_yaml = mock_yaml_loader
    tool_path = "/usr/bin/gcc"
    mock_yaml.return_value = {"gcc": tool_path}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value=tool_path):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger,"/mock/prject")
        assert tool_config_parser.get_tool_path("gcc") == tool_path

def test_get_tool_path_from_system(mock_yaml_loader):
    """Test getting a valid tool path from system rather than specified path"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value="/usr/bin/gcc"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        assert tool_config_parser.get_tool_path("gcc") == "/usr/bin/gcc"

def test_get_tool_path_not_found(mock_yaml_loader):
    """Test getting the tool path of a tool that doesn't exist"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {}
    tool = "gcc"
    configured_path = "/mock/nonexistent_path/gcc"

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value=None):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        tool_config_parser.get_tool_path(tool)
        tool_config_parser.logger.error.assert_called_once_with(f"FileNotFoundError: Tool '{tool}' not specified in tool_config.yaml and not found in system PATH.")

def test_get_tool_flags(mock_yaml_loader):
    """Testing getting all the default flags for a tool"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {"verilator": {"path": "/usr/bin/verilator", "default_flags": {"common" : ["-Wall", "-Wno-fatal"]}}}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value="/usr/bin/verilator"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        assert tool_config_parser.get_tool_flags("verilator") == ["-Wall", "-Wno-fatal"]

def test_get_tool_flags_default_flags_dict_no_common_field(mock_yaml_loader):
    """Test get_tool_flags, but mandatory flag 'common' missing in 'default_flags'"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {"verilator": {"path": "/usr/bin/verilator", "default_flags": {}}}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value="/usr/bin/verilator"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        tool_flags = tool_config_parser.get_tool_flags("verilator")
        tool_config_parser.logger.error.assert_called_once_with(f"ValueError: Tool 'verilator' does not have a mandatory 'common' field within its 'default_flags' attribute.")

def test_get_tool_flags_default_flags_common_and_linux_field(mock_yaml_loader):
    """Test get_tool_flags, with both 'common' and 'linux' defined"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {"gcc": {"path": "/usr/bin/gcc", "default_flags": {"common": ["-std=c++20"], "linux": ["-pg"]}}}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value="/usr/bin/gcc"), \
        patch("sys.platform", "linux"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        tool_flags = tool_config_parser.get_tool_flags("gcc")
        assert tool_flags == ["-std=c++20", "-pg"]

def test_get_command_with_extra_flags(mock_yaml_loader):
    """Test get command, verify that default flags and extra flags are there"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {"gcc": {"path": "/usr/bin/gcc", "default_flags": {"common": ["-O2"]}}}

    with patch("pathlib.Path.exists", return_value=True), \
         patch("shutil.which", return_value="/usr/bin/gcc"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        command = tool_config_parser.get_command("gcc", extra_flags=["main.cpp"])
        assert command == ["/usr/bin/gcc", "-O2", "main.cpp"]

def test_has_tool_configured(mock_yaml_loader):
    """Test has_tool() for a tool that has been configured through tool_config.yaml"""
    m, mock_yaml = mock_yaml_loader
    tool_path = "/tools/g++"
    mock_yaml.return_value = {"g++": tool_path}

    with patch("pathlib.Path.exists", return_value=True), \
        patch("shutil.which", return_value=tool_path):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        assert tool_config_parser.has_tool("g++") is True

def test_has_tool_from_path(mock_yaml_loader):
    """Test has_tool() for a tool which exists in PATH"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {}

    with patch("pathlib.Path.exists", return_value=True), \
         patch("shutil.which", return_value="/usr/bin/linker"):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        assert tool_config_parser.has_tool("linker") is True

def test_has_tool_absent(mock_yaml_loader):
    """Test has_tool() but tool is absent"""
    m, mock_yaml = mock_yaml_loader
    mock_yaml.return_value = {}

    with patch("pathlib.Path.exists", return_value=True), \
         patch("shutil.which", return_value=None):
        mock_logger = MagicMock()
        tool_config_parser = ToolConfigParser(mock_logger, "/mock/project")
        assert tool_config_parser.has_tool("ghosttool") is False

