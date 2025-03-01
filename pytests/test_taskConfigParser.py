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

def test_load_task_config_file_valid_yaml(tmp_path: Path):
    """Test loading a valid task_config.yaml"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_file_path = MagicMock(spec=Path)
    task_config_file_path.__str__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.absolute.return_value = "/path/to/task_config.yaml"
    task_config_file_path.parent.absolute.return_value = "/path/to/"
    task_config_file_path.__fspath__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.exists.return_value = True

    # Simulated YAML content (missing 'task_name')
    mock_yaml_content = {
        "task_name" : "test_task",
        "description": "This is a test task",
        "rtl_sources": ["source1.v", "source2.v"]
    }

    with patch("builtins.open", mock_open(read_data="fake content")), \
         patch("yaml.safe_load", return_value=mock_yaml_content):
        task_config_dict = task_config_parser._load_task_config_file(task_config_file_path)

    assert task_config_dict == mock_yaml_content
    assert "test_task" in task_config_parser.task_configs
    assert task_config_parser.task_configs["test_task"]["task_dir"] == "/path/to/"
    assert task_config_parser.task_configs["test_task"]["task_config_file_path"] == task_config_file_path.absolute()

def test_resolve_output_reference_no_output_keyword(tmp_path: Path):
    """Test passing an invalid 'value' which does not have the '@output' keyword"""
    value = "{@no_output_keyword}"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    output_reference = task_config_parser._resolve_output_reference(value)
    assert output_reference is None
    task_config_parser.logger.error.assert_called_once_with(f"ValueError: Invalid output reference: '{value}'")

def test_resolve_output_reference_non_existent_task_name(tmp_path: Path):
    """Test referencing a non-existent output task"""
    value = "{@output:non_existent_task:src1.v}"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["existing_task"] = {"input_src": ["src1.v", "src2.v"]}

    output_reference = task_config_parser._resolve_output_reference(value)

    assert output_reference is None
    task_config_parser.logger.error(f"ValueError: Referenced task 'non_existent_task' not found in task_configs.")

def test_resolve_output_reference_non_existent_output_dir(tmp_path: Path):
    """Test the function but the output dir for the task doesn't exist"""
    value = "{@output:test_task:src1.v}"
    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = False

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["test_task"] = {"output_dir" : task_outdir}

    output_reference = task_config_parser._resolve_output_reference(value)
    assert output_reference is None
    task_config_parser.logger.error(f"FileNotFoundError: Output_dir of task 'test_task' does not exists. Please ensure it exists first.")

def test_resolve_output_reference_missing_output_dir_field(tmp_path: Path):
    """Test the function but the 'output_dir' field does not exist in task_configs['task_name']"""
    output_task = 'test_task'
    value = f"{{@output:{output_task}:src1.v}}"
    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = False

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["test_task"] = {"input_src" : ["src1.v", "src2.v"]}

    output_reference = task_config_parser._resolve_output_reference(value)
    assert output_reference is None
    task_config_parser.logger.error(f"ValueError: task_configs['{output_task}'] doesn't contain a 'output_dir' attribute. Please ensure a output dir is registered with task '{output_task}' first.")
