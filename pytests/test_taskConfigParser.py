import os
import pytest
from pathlib import Path
from taskConfigParser.TaskConfigParser import TaskConfigParser
from unittest.mock import MagicMock, patch, mock_open

def test_load_task_config_file_non_existent_file_path(tmp_path: Path):
    """Test loading a task_config.yaml which does not exist"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    non_existent_path = MagicMock(spec=Path)
    non_existent_path.exists.return_value = False

    task_config_dict = task_config_parser._load_task_config_file(non_existent_path)
    assert task_config_dict is None
    task_config_parser.logger.error.assert_called_once_with(f"'{non_existent_path}' does not exist. Cannot load it.")
    non_existent_path.exists.assert_called_once()

def test_load_task_config_file_missing_task_name(tmp_path: Path):
    """Test loading a task_config.yaml but it does not have the 'task_name' field."""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_file_path = MagicMock(spec=Path)
    task_config_file_path.__str__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.__fspath__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.exists.return_value = True

    # Simulated YAML content (missing 'task_name')
    mock_yaml_content = {
        "description": "This is a test task",
        "rtl_sources": ["source1.v", "source2.v"]
    }

    with patch("builtins.open", mock_open(read_data="fake content")), \
         patch("yaml.safe_load", return_value=mock_yaml_content):
        task_config_dict = task_config_parser._load_task_config_file(task_config_file_path)

    assert task_config_dict is None
    mock_logger.error.assert_called_once_with(
        f"'{task_config_file_path}' doesn't contain a mandatory key 'task_name'. Please ensure it exists first. Aborting parsing this task_config.yaml."
    )


