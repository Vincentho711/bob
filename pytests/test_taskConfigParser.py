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
    assert task_config_parser.task_configs["test_task"]["task_config_dict"] == mock_yaml_content

def test_load_task_config_file_existing_task_name(tmp_path: Path):
    """Test loading a task which already exists in task_configs, expected output is updating the existing task_configs[{task_name}]"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_file_path = MagicMock(spec=Path)
    task_config_file_path.__str__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.absolute.return_value = "/path/to/task_config.yaml"
    task_config_file_path.parent.absolute.return_value = "/path/to/"
    task_config_file_path.__fspath__.return_value = "/path/to/task_config.yaml"
    task_config_file_path.exists.return_value = True

    task_config_parser.task_configs["existing_task"] = {"task_type" : "c_compile", "task_dir" : "original_task_dir"}

    # Simulated YAML content
    mock_yaml_content = {
        "task_name" : "existing_task",
        "description": "This is an existing task",
        "rtl_sources": ["source1.c", "source2.c"]
    }

    with patch("builtins.open", mock_open(read_data="fake content")), \
         patch("yaml.safe_load", return_value=mock_yaml_content):
        task_config_dict = task_config_parser._load_task_config_file(task_config_file_path)

    assert task_config_dict == mock_yaml_content
    assert "task_config_dict" in task_config_parser.task_configs["existing_task"]
    # Check that logger warning was called twice with the expected messages
    expected_warnings = [
        "'existing_task' already exists within TaskConfigParser.task_configs. Updating existing task_configs[existing_task].",
        "Existing attribute task_configs['task_dir']=original_task_dir will be replaced with '/path/to/'."
    ]
    logged_warnings = [call.args[0] for call in mock_logger.warning.call_args_list]

    assert all(msg in logged_warnings for msg in expected_warnings)
    assert len(logged_warnings) == 2
    assert task_config_parser.task_configs["existing_task"].get("task_dir") == str(task_config_file_path.parent.absolute()) # Check that existing fields are overriden

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

def test_resolve_files_spec_single_valid_file(tmp_path: Path):
    """Test resolving a single file, or a long string which would be interpretated as a single file"""
    files_spec_1 = "single_file.cpp"
    files_spec_2 = "this is a single file"

    target_dir = MagicMock(spec=Path)
    target_dir.__str__.return_value = "/path/to/task1/outdir/"
    target_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    target_dir.__truediv__.side_effect = lambda p: Path(target_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    files_spec_reference_1 = task_config_parser._resolve_files_spec(target_dir, files_spec_1)
    assert files_spec_reference_1 == str(Path("/path/to/task1/outdir/") / "single_file.cpp")
    task_config_parser.logger.debug.assert_called_once_with(f"For files_spec = '{files_spec_1}', resolved reference (single file within target dir): {str(target_dir / files_spec_1)}")
    files_spec_reference_2 = task_config_parser._resolve_files_spec(target_dir, files_spec_2)
    assert files_spec_reference_2 == str(Path("/path/to/task1/outdir/") / "this is a single file")
    task_config_parser.logger.debug.assert_any_call(f"For files_spec = '{files_spec_2}', resolved reference (single file within target dir): {str(target_dir / files_spec_2)}")

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

def test_resolve_output_reference_valid_files_spec_list(tmp_path: Path):
    """Test the resolve output with a list of string as the files_spec"""
    files_spec = "['a.cpp', 'b.h', 'c.hpp']"
    value = f"{{@output:task1:{files_spec}}}"
    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.as_posix.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_outdir.__truediv__.side_effect = lambda p: Path(task_outdir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task1"] = {"input_src" : ["src1.v", "src2.v"], "output_dir" : task_outdir}
    resolved_output_reference = task_config_parser._resolve_output_reference(value)

    expected_output = [
        str(Path("/path/to/outdir/") / "a.cpp"),
        str(Path("/path/to/outdir/") / "b.h"),
        str(Path("/path/to/outdir/") / "c.hpp"),
    ]
    task_config_parser.logger.debug.assert_any_call(f"Calling _resolve_files_spec() with target_dir='{task_outdir}', files_spec='{files_spec}'")
    assert resolved_output_reference == expected_output
    task_config_parser.logger.debug.assert_any_call(f"For files_spec = '{files_spec}', resolved reference (list of files within target dir): {expected_output}")

def test_resolve_output_reference_valid_single_file(tmp_path: Path):
    """Test resolving output reference of a single file in the files_spec"""
    files_spec = "singlefile"
    value = f"{{@output:task1:{files_spec}}}"
    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.as_posix.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_outdir.__truediv__.side_effect = lambda p: Path(task_outdir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task1"] = {"input_src" : ["src1.v", "src2.v"], "output_dir" : task_outdir}
    resolved_output_reference = task_config_parser._resolve_output_reference(value)

    expected_output = str(Path("/path/to/outdir") / "singlefile")
    task_config_parser.logger.debug.assert_any_call(f"Calling _resolve_files_spec() with target_dir='{task_outdir}', files_spec='{files_spec}'")
    assert resolved_output_reference == expected_output
    task_config_parser.logger.debug.assert_any_call(f"For files_spec = '{files_spec}', resolved reference (single file within target dir): {str(task_outdir / files_spec)}")

def test_resolve_input_reference_valid_single_file(tmp_path: Path):
    """Test resolving a single file of another task"""
    files_spec = "single_file.cpp"
    value = f"{{@input:task1:{files_spec}}}"
    task_dir = MagicMock(spec=Path)
    task_dir_2 = MagicMock(spec=Path)
    task_dir.__str__.return_value = "/path/to/task1/"
    task_dir.as_posix.return_value = "/path/to/task1/"
    task_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_dir.__truediv__.side_effect = lambda p: Path(task_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task1"] = {
        "task_dir" : task_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs["task2"] = {
        "task_dir" : task_dir_2,
        "internal_input_src" : ["a.cpp", "b.cpp"], "output_dir" : tmp_path
    }
    resolved_input_reference = task_config_parser._resolve_input_reference(value)
    expected_output = str(Path("/path/to/task1") / "single_file.cpp")
    task_config_parser.logger.debug.assert_any_call(f"Calling _resolve_files_spec() with target_dir='{task_dir}', files_spec='{files_spec}'")
    assert resolved_input_reference == expected_output
    task_config_parser.logger.debug.assert_any_call(f"For files_spec = '{files_spec}', resolved reference (single file within target dir): {str(task_dir / files_spec)}")

def test_resolve_input_reference_valid_entire_input_dir(tmp_path: Path):
    """Test resolving an entire input dir of another task"""
    files_spec = "*"
    value = f"{{@input:task_1:{files_spec}}}"
    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_2_dir = MagicMock(spec=Path)
    task_2_dir.__str__.return_value = "/path/to/task_2"
    task_2_dir.as_posix.return_value = "/path/to/task_2"
    task_2_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_1_dir.__truediv__.side_effect = lambda p: Path(task_1_dir.__str__.return_value) / p
    task_2_dir.__truediv__.side_effect = lambda p: Path(task_2_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task_1"] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs["task_2"] = {
        "task_dir" : task_2_dir,
        "internal_input_src" : ["a.cpp", "b.cpp"], "output_dir" : tmp_path
    }

    resolved_input_reference = task_config_parser._resolve_input_reference(value)
    expected_output = str(task_1_dir)

    task_config_parser.logger.debug.assert_any_call(f"Calling _resolve_files_spec() with target_dir='{task_1_dir}', files_spec='{files_spec}'")
    assert resolved_input_reference == expected_output
    task_config_parser.logger.debug(f"For files_spec = '{files_spec}', resolved reference (entire target dir): {str(task_1_dir)}")

def test_resolve_reference_output_reference_with_list_of_files(tmp_path: Path):
    """Test whether an output reference like "{@output:output_task:['a.o','b.o']}" can be succesfully resolved"""
    task_name = "valid_task"
    referenced_task_name = "output_task"
    files_spec = "['a.o','b.o']"
    value = f"{{@output:{referenced_task_name}:{files_spec}}}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.as_posix.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_outdir.__truediv__.side_effect = lambda p: Path(task_outdir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "output_dir" : task_outdir
    }

    resolved_paths, resolved_type = task_config_parser.resolve_reference(task_name, value)
    assert resolved_type == "output"
    expected_output = [str(task_outdir / 'a.o'), str(task_outdir / 'b.o')]
    assert resolved_paths == expected_output

def test_resolve_reference_output_reference_with_every_files(tmp_path: Path):
    """Test whether an output reference like "{@output:output_task:*}" can be succesfully resolved"""
    task_name = "valid_task"
    referenced_task_name = "output_task"
    files_spec = "*"
    value = f"{{@output:{referenced_task_name}:{files_spec}}}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_outdir = MagicMock(spec=Path)
    task_outdir.__str__.return_value = "/path/to/outdir/"
    task_outdir.as_posix.return_value = "/path/to/outdir/"
    task_outdir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_outdir.__truediv__.side_effect = lambda p: Path(task_outdir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "output_dir" : task_outdir
    }

    resolved_paths, resolved_type = task_config_parser.resolve_reference(task_name, value)
    assert resolved_type == "output"
    expected_output = str(Path(task_outdir).resolve())
    assert resolved_paths == expected_output

def test_resolve_reference_input_reference_with_list_of_files(tmp_path: Path):
    """Test whether an input reference like "{@input:task_2:['a.cpp', 'b.cpp']}" can be succesfully resolved"""
    task_name = "task_1"
    referenced_task_name = "task_2"
    files_spec = "['a.cpp','b.cpp']"
    value = f"{{@input:{referenced_task_name}:{files_spec}}}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_2_dir = MagicMock(spec=Path)
    task_2_dir.__str__.return_value = "/path/to/task_2/"
    task_2_dir.as_posix.return_value = "/path/to/task_2/"
    task_2_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_1_dir.__truediv__.side_effect = lambda p: Path(task_1_dir.__str__.return_value) / p
    task_2_dir.__truediv__.side_effect = lambda p: Path(task_2_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "task_dir" : task_2_dir,
    }

    resolved_paths, resolved_type = task_config_parser.resolve_reference(task_name, value)
    assert resolved_type == "input"
    expected_output = [str(Path(task_2_dir / 'a.cpp').resolve()), str(Path(task_2_dir / 'b.cpp').resolve())]
    assert resolved_paths == expected_output

@patch.object(Path, "is_dir", return_value=True) # Mock resolved_path.is_dir()
@patch("pathlib.Path.iterdir", return_value=[
        Path("/mock/path/task_2/file1.v"),
        Path("/mock/path/task_2/file2.sv"),
        Path("/mock/path/task_2/file3.txt")
])
def test_resolve_reference_input_reference_every_files(mock_iterdir, mock_is_dir, tmp_path: Path):
    """Test whether an input reference like "{@input:task_2:*}" can be succesfully resolved to a list of file paths in str"""
    task_name = "task_1"
    referenced_task_name = "task_2"
    files_spec = "*"
    value = f"{{@input:{referenced_task_name}:{files_spec}}}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_2_dir = MagicMock(spec=Path)
    task_2_dir.__str__.return_value = "/path/to/task_2/"
    task_2_dir.as_posix.return_value = "/path/to/task_2/"
    task_2_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_1_dir.__truediv__.side_effect = lambda p: Path(task_1_dir.__str__.return_value) / p
    task_2_dir.__truediv__.side_effect = lambda p: Path(task_2_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "task_dir" : task_2_dir,
    }
    resolved_value, resolved_type = task_config_parser.resolve_reference(task_name, value)
    # Check that the returned value matches the expected list of file paths
    expected_files = [str(f.resolve()) for f in mock_iterdir.return_value]
    assert resolved_type == "input"
    assert resolved_value == expected_files

@patch("pathlib.Path.resolve", return_value=Path("/mock/path"))
@patch("pathlib.Path.is_dir", return_value=True)
@patch("pathlib.Path.iterdir")
@patch.object(TaskConfigParser, "_resolve_input_reference")  # Mock the method
def test_resolve_reference_input_reference_every_files_exclusion_list(mock_resolve_input_reference, mock_iterdir, mock_is_dir, mock_resolve, tmp_path: Path):
    """Test whether an input reference like "{@input:task_2:*}" can be succesfully resolved, but the task_config.yaml within the task dir should be excluded."""
    task_name = "task_1"
    referenced_task_name = "task_2"
    files_spec = "*"
    value = f"{{@input:{referenced_task_name}:{files_spec}}}"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.task_configs[task_name] = {
        "task_dir" : Path("/mock/path"),
        "internal_input_src" : ["single_file.cpp", "my_src.cpp", ""], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "task_dir" : Path("/mock/referenced_task")
    }

    # Mock the return of _resolve_input_reference to return task_dir as str
    mock_resolve_input_reference.return_value = str(task_config_parser.task_configs[referenced_task_name]["task_dir"])

    # Mock directory contents, including "task_config.yaml"
    mock_file1 = MagicMock(spec=Path)
    mock_file1.name = "file1.txt"
    mock_file1.__str__.return_value = "/mock/referenced_task/file1.txt"

    mock_task_config = MagicMock(spec=Path)
    mock_task_config.name = "task_config.yaml"  # Explicitly set correct name
    mock_task_config.__str__.return_value = "/mock/referenced_task/task_config.yaml"

    mock_file2 = MagicMock(spec=Path)
    mock_file2.name = "file2.log"
    mock_file2.__str__.return_value = "/mock/referenced_task/file2.log"

    # Set return value of iterdir() to include "task_config.yaml"
    mock_iterdir.return_value = [mock_file1, mock_task_config, mock_file2]


    resolved_value, resolved_type = task_config_parser.resolve_reference(task_name, value)
    # Expected filtered result (excluding "task_config.yaml")
    expected_files = ["/mock/referenced_task/file1.txt", "/mock/referenced_task/file2.log"]

    assert resolved_value == expected_files, f"Expected {expected_files}, but got {resolved_value}"
    assert resolved_type == "input"

@patch.object(Path, "is_dir", return_value=False) # Mock resolved_path.is_dir()
def test_resolve_reference_input_reference_single_file(mock_is_dir, tmp_path: Path):
    """Test resolving an input reference of a single file like "{@input:task_2:single_file.cpp}" """
    task_name = "task_1"
    referenced_task_name = "task_2"
    files_spec = "single_file.cpp"
    value = f"{{@input:{referenced_task_name}:{files_spec}}}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    task_2_dir = MagicMock(spec=Path)
    task_2_dir.__str__.return_value = "/path/to/task_2/"
    task_2_dir.as_posix.return_value = "/path/to/task_2/"
    task_2_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_1_dir.__truediv__.side_effect = lambda p: Path(task_1_dir.__str__.return_value) / p
    task_2_dir.__truediv__.side_effect = lambda p: Path(task_2_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    task_config_parser.task_configs[referenced_task_name] = {
        "task_dir" : task_2_dir,
    }
    resolved_value, resolved_type = task_config_parser.resolve_reference(task_name, value)
    assert resolved_type == "input"
    expected_files = str(Path(task_2_dir) / files_spec)
    assert resolved_value == expected_files

def test_resolve_reference_direct_reference_valid(tmp_path):
    """Test resolving a direct reference under the same task"""
    task_name = "task_1"
    referenced_task_name = task_name
    files_spec = "single_file.cpp"
    value = f"{files_spec}"


    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    # Mock __truediv__ to return a new Path object that correctly joins paths
    task_1_dir.__truediv__.side_effect = lambda p: Path(task_1_dir.__str__.return_value) / p

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.task_configs[task_name] = {
        "task_dir" : task_1_dir,
        "internal_input_src" : ["single_file.cpp", "my_src.cpp"], "output_dir" : tmp_path
    }
    resolved_value, resolved_type = task_config_parser.resolve_reference(task_name, value)
    assert resolved_type == "direct"
    expected_files = str(Path(task_1_dir) / files_spec)
    assert resolved_value == expected_files

def test_update_task_env_invalid_task_name(tmp_path: Path):
    """Test updating the task env of an invalid task"""
    task_name = 'invalid_task'

    task_1_dir = MagicMock(spec=Path)
    task_1_dir.__str__.return_value = "/path/to/task_1"
    task_1_dir.as_posix.return_value = "/path/to/task_1"
    task_1_dir.exists.return_value = True

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["task_1"] = {
            "task_dir": task_1_dir,
            "internal_input_src": ["a.cpp", "b.cpp", "c.cpp"]
    }

    task_config_parser.update_task_env(task_name, "env_key", "env_val", False)
    task_config_parser.logger.error.assert_called_once_with(
        f"KeyError: 'Task {task_name} not found in task configurations.'"
    )

def test_update_task_env_no_task_env_in_task_configs(tmp_path: Path):
    """Test updating the task but task configs does not have a task_env field"""
    task_name = "valid_task"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "abc" : {'USER' : 'bob'},
    }

    task_config_parser.update_task_env(task_name, "env_key", "env_val", False)
    task_config_parser.logger.info.assert_called_once_with(f"Task '{task_name}' does not have an env var dict associated to the 'task_env' key. Creating it from current global env.")
    task_config_parser.logger.debug.assert_called_once_with(f"Task '{task_name}', overriding existing or creating env_key='env_key' with env_val='env_val'")
    expected_env_dict = os.environ.copy()
    expected_env_dict["env_key"] = "env_val"
    assert task_config_parser.task_configs[task_name]["task_env"] == expected_env_dict

def test_update_task_env_existing_val_is_list_and_env_val_is_list(tmp_path: Path):
    """Test with an existing value of a list and env_val is also a list"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = ["/path/to/a", "/path/to/b"]

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/usr/local/bin:/usr/sbin"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, False)
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', extending a list of env_val = '{env_val}' to env_key = '{env_key}'")
    expected_env_dict = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/usr/local/bin:/usr/sbin:/path/to/a:/path/to/b"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', updated env_key = '{env_key}' with env_val = '{expected_env_dict["task_env"]["env_key"]}'")

def test_update_task_env_existing_val_is_list_and_env_val_is_str(tmp_path: Path):
    """Test with an existing value of a list but env_val is only a str"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = "/path/to/a"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, False)
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', appending a single str/filepath env_val = '{env_val}' to env_key = '{env_key}'")
    expected_env_dict = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b:/path/to/a"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', updated env_key = '{env_key}' with env_val = '{expected_env_dict["task_env"]["env_key"]}'")

def test_update_task_env_existing_val_is_str_and_env_val_is_list(tmp_path: Path):
    """Test with an existing value which is a str but env_val is a list"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = ["/path/to/a", "/path/to/b"]

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, False)
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', extending a list of env_val = '{env_val}' to env_key = '{env_key}'")
    expected_env_dict = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', updated env_key = '{env_key}' with env_val = '{expected_env_dict["task_env"]["env_key"]}'")

def test_update_task_env_existing_val_is_str_and_env_val_is_str(tmp_path: Path):
    """Test with an existing value which is a str and env_val is a str"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = "/path/to/a"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, False)
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', appending a single str/filepath env_val = '{env_val}' to env_key = '{env_key}'")
    expected_env_dict = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_any_call(f"Task '{task_name}', updated env_key = '{env_key}' with env_val = '{expected_env_dict["task_env"]["env_key"]}'")

def test_update_task_env_existing_val_is_str_override(tmp_path: Path):
    """Test overriding an existing value which is a str"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = "/path/to/a"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, True)
    expected_env_dict = {
        "task_env" : {'env_key' : "/path/to/a"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_called_once_with(f"Task '{task_name}', overriding existing or creating env_key='{env_key}' with env_val='{expected_env_dict["task_env"]["env_key"]}'")

def test_update_task_env_existing_val_is_str_override(tmp_path: Path):
    """Test overriding an existing value which is a list"""
    task_name = "valid_task"
    env_key = "env_key"
    env_val = "/path/to/c"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"}
    }
    task_config_parser.update_task_env(task_name, env_key, env_val, True)
    expected_env_dict = {
        "task_env" : {'env_key' : "/path/to/c"}
    }
    assert task_config_parser.task_configs["valid_task"] == expected_env_dict
    task_config_parser.logger.debug.assert_called_once_with(f"Task '{task_name}', overriding existing or creating env_key='{env_key}' with env_val='{expected_env_dict["task_env"]["env_key"]}'")

def test_validate_initial_task_config_dict_no_task_name(tmp_path: Path):
    """Test validating a task config which does not contain a valid task_name"""
    task_name = "invalid_task"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"}
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error(f"Task {task_name} not found in task configurations.")

def test_validate_initial_task_config_dict_no_task_env(tmp_path: Path):
    """Test validating a task config which does not contain a valid task_env within task_configs[task_name]"""
    task_name = "valid_task"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "my_var" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"}
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error(f"For task '{task_name}', 'task_env' is not found within task_configs. validate_initial_task_config_dict() failed.")

def test_validate_initial_task_config_dict_no_task_config_file_path(tmp_path: Path):
    """Test validating a task config which does not contain a valid task_config_file_path within task_configs[task_name]"""
    task_name = "valid_task"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        }
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"KeyError: \"Task '{task_name}' does not have a mandatory 'task_config_file_path' attribute within task_configs.\"")

def test_validate_initial_task_config_dict_no_task_config_dict(tmp_path: Path):
    """Test validating a task config which does not contain 'task_config_dict' within task_configs[task_name]"""
    task_name = "valid_task"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        },
        "task_config_file_path" : "/home/path/to/task_config"
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"KeyError: \"Task '{task_name}' does not have a 'task_config_dict' attribute within task_configs. Please ensure that load_task_config() has been executed for task '{task_name}' first.\"")

def test_validate_initial_task_config_dict_no_task_dir(tmp_path: Path):
    """Test validating a task config which does not contain 'task_dir' within task_configs[task_name]"""
    task_name = "valid_task"
    task_config_file_path = "/home/path/to/task_config"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        },
        "task_config_file_path" : task_config_file_path,
        "task_config_dict" : {
            "taks_name" : task_name,
            "task_type" : "c_compile"
        }
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"KeyError: \"task_configs[{task_name}] does not contain the mandatory field 'task_dir'.\"")

def test_validate_initial_task_config_dict_no_output_dir(tmp_path: Path):
    """Test validating a task config which does not contain 'output_dir' within task_configs[task_name]"""
    task_name = "valid_task"
    task_config_file_path = "/home/path/to/task_config"
    task_dir = tmp_path
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        },
        "task_config_file_path" : task_config_file_path,
        "task_config_dict" : {
            "taks_name" : task_name,
            "task_type" : "c_compile"
        },
        "task_dir" : task_dir
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"KeyError: \"task_configs[{task_name}] does not contain the mandatory field 'output_dir'.\"")

def test_validate_initial_task_config_dict_no_task_type(tmp_path: Path):
    """Test validating a task config which does not contain 'task_type' within task_configs[task_name]"""
    task_name = "valid_task"
    task_config_file_path = "/home/path/to/task_config"
    task_dir = tmp_path
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        },
        "task_config_file_path" : task_config_file_path,
        "task_config_dict" : {
            "taks_name" : task_name,
        },
        "task_dir" : task_dir,
        "output_dir" : "/path/to/output_dir"
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"KeyError: \"{task_config_file_path} does not contain the mandatory field 'task_type'.\"")

def test_validate_initial_task_config_dict_invalid_task_type(tmp_path: Path):
    """Test validating a task config with 'task_type' which is a list within task_configs[task_name]"""
    task_name = "valid_task"
    task_config_file_path = "/home/path/to/task_config"
    task_dir = tmp_path
    task_type = ["a", "b", "c"]
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task"] = {
        "task_env" : {
            'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"
        },
        "task_config_file_path" : task_config_file_path,
        "task_config_dict" : {
            "taks_name" : task_name,
            "task_type" : task_type
        },
        "task_dir" : task_dir,
        "output_dir" : "/path/to/output_dir"
    }
    task_config_parser.validate_initial_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"TypeError: {task_config_file_path} has the mandatory field 'task_type' defined as a non string. task_type = {task_type}")

def test_resolve_src_files_all_reference_types(tmp_path: Path):
    """Test parsing a list of unresolved src files which contains all 3 resolved types"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.resolve_reference = MagicMock(side_effect=[
        ("/abs/path/to/file1.v", "direct"),
        ("/abs/path/to/external/file2.v", "input"),
        ("/abs/path/to/output/file3.v", "output"),
    ])
    unresolved_src_files = ["file1.v", "{@input:taskB:file2.v}", "{@output:taskC:file3.v}"]
    resolved_src_files = task_config_parser.resolve_src_files("task_0", unresolved_src_files)

    assert resolved_src_files == {
        "internal_src_files": ["/abs/path/to/file1.v"],
        "external_src_files": ["/abs/path/to/external/file2.v"],
        "output_src_files":   ["/abs/path/to/output/file3.v"]
    }

def test_resolve_src_files_list_returned_by_reference(tmp_path: Path):
    """Test parsing a list of unresolved src files and see if it can return a list of references"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.resolve_reference = MagicMock(side_effect=[
        (["/abs/internal1.v", "/abs/internal2.v"], "direct"),
        (["/abs/input1.v", "/abs/input2.v"], "input"),
    ])
    unresolved_src_files = ["file_list_direct", "{@input:taskX:[input1.v,input2.v]}"]
    resolved_src_files = task_config_parser.resolve_src_files("task_0", unresolved_src_files)

    assert resolved_src_files == {
        "internal_src_files": ["/abs/internal1.v", "/abs/internal2.v"],
        "external_src_files": ["/abs/input1.v", "/abs/input2.v"],
        "output_src_files":   []
    }

def test_resolve_src_files_invalid_type_ignored(tmp_path: Path):
    """Test parsing a list of unresolved_src_files but contains an invalid return from self.resolve_reference()"""
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    task_config_parser.resolve_reference = MagicMock(side_effect=[
        (123, "direct"),
    ])
    unresolved_src_files = ["invalid_src_file"]
    resolved_src_files = task_config_parser.resolve_src_files("task_0", unresolved_src_files)

    assert resolved_src_files == {
        "internal_src_files": [],
        "external_src_files": [],
        "output_src_files": []
    }

    task_config_parser.logger.warning.assert_called_once_with("Resolved reference='123' is neither of type str or list. type(123)=<class 'int'>. Skip appending to resolved_src_files dict.")


def test_parse_task_config_dict_valid_task_type(tmp_path: Path):
    """Test parsing a valid task_type"""
    task_type_1 = "c_compile"
    task_type_2 = "cpp_compile"
    task_type_3 = "verilator_verilate"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task_1"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"},
        "task_config_dict" : {"task_type" : task_type_1}
    }
    task_config_parser.task_configs["valid_task_2"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/c:/path/to/d"},
        "task_config_dict" : {"task_type" : task_type_2}
    }
    task_config_parser.task_configs["valid_task_3"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/c:/path/to/e"},
        "task_config_dict" : {"task_type" : task_type_3}
    }
    task_config_parser.parse_c_compile = MagicMock()
    task_config_parser.parse_cpp_compile = MagicMock()
    task_config_parser.parse_verilator_verilate = MagicMock()

    with patch.object(task_config_parser, "validate_initial_task_config_dict", return_value=True): # Mock the return of internal function call
        task_config_parser.parse_task_config_dict("valid_task_1")
    task_config_parser.parse_c_compile.assert_called_once_with("valid_task_1")
    task_config_parser.parse_cpp_compile.assert_not_called()

    with patch.object(task_config_parser, "validate_initial_task_config_dict", return_value=True): # Mock the return of internal function call
        task_config_parser.parse_task_config_dict("valid_task_2")
    task_config_parser.parse_cpp_compile.assert_called_once_with("valid_task_2")

@pytest.mark.parametrize("task_type", ["c_compile", "cpp_compile", "verilator_verilate"])
def test_parse_task_config_dict_dispatch_valid_task_types(tmp_path: Path, task_type):
    """Test that the correct parse_{task_type} method is called based on the task_type."""
    task_name = f"{task_type}_task"
    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))

    # Assign a dummy config for the task
    task_config_parser.task_configs[task_name] = {
        "task_env": {'env_key': "/opt/homebrew/sbin:/path/to/a:/path/to/b"},
        "task_config_dict": {"task_type": task_type}
    }

    # Dynamically patch the correct parse_* method
    parse_method_name = f"parse_{task_type}"
    setattr(task_config_parser, parse_method_name, MagicMock())

    with patch.object(task_config_parser, "validate_initial_task_config_dict", return_value=True):
        task_config_parser.parse_task_config_dict(task_name)

    getattr(task_config_parser, parse_method_name).assert_called_once_with(task_name)

def test_parse_task_config_dict_invalid_task_type(tmp_path: Path):
    """Test parsing a invalid task_type that isn't supported"""
    task_name = "valid_task_1"
    task_type = "unsupported_task_type"

    mock_logger = MagicMock()
    task_config_parser = TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs["valid_task_1"] = {
        "task_env" : {'env_key' : "/opt/homebrew/sbin:/path/to/a:/path/to/b"},
        "task_config_dict" : {"task_type" : task_type}
    }
    with patch.object(task_config_parser, "validate_initial_task_config_dict", return_value=True): # Mock the return of internal function call
        task_config_parser.parse_task_config_dict(task_name)
    task_config_parser.logger.error.assert_called_once_with(f"ValueError: For task_name = '{task_name}', the task_type = '{task_type}' is not a support task type.")

def test_parse_all_tasks_in_task_configs_empty_task_configs(tmp_path: Path):
    """Test iterating task configs which is empty"""
    mock_logger = MagicMock()
    task_config_parser =  TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs = {}

    task_config_parser.parse_all_tasks_in_task_configs()
    task_config_parser.logger.error.assert_called_once_with("ValueError: self.task_configs is empty. Please ensure that inherit_task_configs() has been run.")

def test_parse_all_tasks_in_task_configs_missing_task_config_file_path(tmp_path: Path):
    """Test iterating task configs, with one task having a missing 'task_config_file_path' attribute"""
    mock_logger = MagicMock()
    task_config_parser =  TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs = {
        "task_a": {}
    }

    task_config_parser.parse_all_tasks_in_task_configs()
    task_config_parser.logger.error.call_args[0][0].startswith("KeyError: Missing 'task_config_file_path'")

def test_parse_all_tasks_in_task_configs_valid(tmp_path: Path):
    """Testing parsing a valid task_config"""
    mock_logger = MagicMock()
    task_config_parser =  TaskConfigParser(mock_logger, str(tmp_path))
    task_config_parser.task_configs = {
        "task_b": {"task_config_file_path": "/path/to/task_b/task_config.yaml"}
    }
    task_config_parser._load_task_config_file = MagicMock()
    task_config_parser.parse_task_config_dict = MagicMock()

    task_config_parser.parse_all_tasks_in_task_configs()

    task_config_parser._load_task_config_file.assert_called_once_with("/path/to/task_b/task_config.yaml")
    task_config_parser.parse_task_config_dict.assert_called_once_with("task_b")
