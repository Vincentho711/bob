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

def test_resolve_files_spec_invalid_target_dir_type(tmp_path: Path):
    """Test resolve_files_spec() invalid target_dir type"""
    target_dir_1 = ["task1", "task2"]
    target_dir_2 = "task3"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    files_spec_reference_1 = task_config_parser._resolve_files_spec(target_dir_1, "{@output:task_1:src1.v}")
    assert files_spec_reference_1 is None
    task_config_parser.logger.error.assert_called_once_with(f"TypeError: target_dir = '{target_dir_1}' must be of type Path.")

    files_spec_reference_2 = task_config_parser._resolve_files_spec(target_dir_2, "{@output:task_1:src1.v}")
    assert files_spec_reference_2 is None
    task_config_parser.logger.error.assert_any_call(f"TypeError: target_dir = '{target_dir_2}' must be of type Path.")

def test_resolve_files_spec_invalid_files_spec_type(tmp_path: Path):
    """Test resolve_files_spec() with invalid files_spec type"""
    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/target_dir/"
    target_dir.exists.return_value = True

    files_spec_1 = {"key1": "val1", "key2": "val2"}
    files_spec_2 = tmp_path

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference_1 = task_config_parser._resolve_files_spec(target_dir, files_spec_1)
    task_config_parser.logger.error.assert_called_once_with(f"TypeError: files_spec = '{files_spec_1}' must be of type str for it to be resolved, it is of type '{type(files_spec_1)}'.")
    assert files_spec_reference_1 is None

    files_spec_reference_2 = task_config_parser._resolve_files_spec(target_dir, files_spec_2)
    task_config_parser.logger.error.assert_any_call(f"TypeError: files_spec = '{files_spec_2}' must be of type str for it to be resolved, it is of type '{type(files_spec_2)}'.")
    assert files_spec_reference_2 is None

def test_resolve_files_spec_entire_target_dir_reference(tmp_path: Path):
    """Test resolve_files_spec() with files_spec='*"""
    output_reference_1 = "{@output:task1:*}"
    files_spec_1 = '*'
    target_dir_1 = MagicMock(spec=Path)
    target_dir_1.__str__.return_value = "/path/to/task1/outdir/"
    target_dir_1.exists.return_value = True

    input_src_reference_2 = "{@input_src:task2:*}"
    files_spec_2 = '*'
    target_dir_2 = MagicMock(spec=Path)
    target_dir_2.__str__.return_value = "/path/to/task2/"
    target_dir_2.exists.return_value = True

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task1"] = {"input_src": ["src1.v", "src2.v"]}
    task_config_parser.task_configs["task2"] = {"input_src": ["src3.v", "src4.v", "src5.v"]}

    files_spec_reference_1 = task_config_parser._resolve_files_spec(target_dir_1, files_spec_1)
    assert isinstance(files_spec_reference_1, str)
    assert files_spec_reference_1 == str(target_dir_1)
    task_config_parser.logger.debug.assert_called_once_with(f"For files_spec = '{files_spec_1}', resolved reference (entire target dir): {str(target_dir_1)}")


    files_spec_reference_2 = task_config_parser._resolve_files_spec(target_dir_2, files_spec_2)
    assert isinstance(files_spec_reference_2, str)
    assert files_spec_reference_2 == str(target_dir_2)
    task_config_parser.logger.debug.assert_any_call(f"For files_spec = '{files_spec_2}', resolved reference (entire target dir): {str(target_dir_2)}")

def test_resolve_files_spec_list_of_string_valid_with_single_quote(tmp_path: Path):
    """Test resolving a list of string with '' surrounding the str"""
    files_spec = "['src1.cpp', 'src2.cpp', 'src3.cpp']"
    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/task1/outdir/"
    target_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    target_dir.__truediv__.side_effect = lambda p: Path(target_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference = task_config_parser._resolve_files_spec(target_dir, files_spec)

    expected_output = [
        str(Path("/path/to/task1/outdir/") / "src1.cpp"),
        str(Path("/path/to/task1/outdir/") / "src2.cpp"),
        str(Path("/path/to/task1/outdir/") / "src3.cpp"),
    ]

    assert files_spec_reference == expected_output
    task_config_parser.logger.debug.assert_called_once_with(f"For files_spec = '{files_spec}', resolved reference (list of files within target dir): {expected_output}")

def test_resolve_files_spec_list_of_string_valid_with_double_quote(tmp_path: Path):
    """Test resolving a list of string with "" surrounding the str"""
    files_spec = '["src1.cpp", "src2.cpp", "src3.cpp"]'
    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/task1/outdir/"
    target_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    target_dir.__truediv__.side_effect = lambda p: Path(target_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference = task_config_parser._resolve_files_spec(target_dir, files_spec)

    expected_output = [
        str(Path("/path/to/task1/outdir/") / "src1.cpp"),
        str(Path("/path/to/task1/outdir/") / "src2.cpp"),
        str(Path("/path/to/task1/outdir/") / "src3.cpp"),
    ]

    assert files_spec_reference == expected_output
    task_config_parser.logger.debug.assert_called_once_with(f"For files_spec = '{files_spec}', resolved reference (list of files within target dir): {expected_output}")

def test_resolve_files_spec_list_of_string_valid_no_quote(tmp_path: Path):
    """Test resolving a list of string with nothing surrounding the str"""
    files_spec = '[src1.cpp, src2.cpp, src3.cpp]'
    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/task1/outdir/"
    target_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    target_dir.__truediv__.side_effect = lambda p: Path(target_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference = task_config_parser._resolve_files_spec(target_dir, files_spec)

    expected_output = [
        str(Path("/path/to/task1/outdir/") / "src1.cpp"),
        str(Path("/path/to/task1/outdir/") / "src2.cpp"),
        str(Path("/path/to/task1/outdir/") / "src3.cpp"),
    ]

    assert files_spec_reference == expected_output
    task_config_parser.logger.debug.assert_called_once_with(f"For files_spec = '{files_spec}', resolved reference (list of files within target dir): {expected_output}")

def test_resolve_files_spec_list_of_string_invalid_with_non_list(tmp_path: Path):
    """Test resolving a list with invalid content that causes an error"""
    files_spec = "[None, 123, {'invalid': 'dict'}]"

    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/task1/outdir/"
    target_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    target_dir.__truediv__.side_effect = lambda p: Path(target_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference = task_config_parser._resolve_files_spec(target_dir, files_spec)

    assert files_spec_reference is None
    task_config_parser.logger.error.assert_called()

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
