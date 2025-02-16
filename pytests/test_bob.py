import os
import shutil
from sys import exc_info
import pytest
import logging
from typing import Generator
from pathlib import Path
from bob.Bob import Bob
from unittest.mock import MagicMock, patch, mock_open

@pytest.fixture
def bob_instance(monkeypatch, tmp_path: Path):
    """Fixture to create a Bob instance with a test logger and mock PROJ_ROOT"""
    # monkeypatch.setenv("proj_root", str(Path(__file__).resolve().parent.parent))
    monkeypatch.setenv("PROJ_ROOT", "/home/user/fuji")
    logger = logging.getLogger("test_logger")
    logger.setLevel(logging.DEBUG)

    bob_instance = Bob(logger)
    # Create a mock ip_config.yaml inside the proj_root for testing purposes
    ip_cfg_content = """
    project:
      name: "pong"
      arch: "xc7"
      board_name: "arty"
      fpga_part: "xc7a35ticsg324-1L"

    directories:
      root_dir: "${PROJ_ROOT}"
      src_dir: "${directories.root_dir}/src"
      build_dir: "${directories.root_dir}/build"

    sources:
      top_level: "${directories.src_dir}/top_${project.name}.sv"

    tasks:
      tb_hello_world:
        depends_on:
          - "${directories.root_dir}/hello_world"
          - "${project.name}/bye_world"
          - "${PROJ_ROOT}/task_0"
    """
    ip_cfg_path = tmp_path / "ip_config.yaml"
    ip_cfg_path.write_text(ip_cfg_content)

    # Ensure Bob is aware of the created ip_config.yaml
    bob_instance.proj_root = str(tmp_path)
    return bob_instance

@pytest.fixture
def create_valid_task_config(tmp_path: Path) -> Path:
    """Fixture to create a valid task_config.yaml file for testing"""
    task_config = """
    task_name: "test_task"
    description: "A sample task"
    """
    task_config_path = tmp_path / "task_config.yaml"
    task_config_path.write_text(task_config)
    return task_config_path

@pytest.fixture
def create_multiple_valid_task_config(monkeypatch) -> tuple[Bob, list[Path]]:
    """Fixture to mock multiple task_config.yaml files in different directory depths."""
    # Create a mock Bob instance
    logger = MagicMock()
    bob_instance = Bob(logger)

    # Mock proj_root environment variable
    monkeypatch.setenv("PROJ_ROOT", "/mock/proj_root")
    bob_instance.proj_root = "/mock/proj_root"  # Set mocked root directory
    
    # Mock the files to return for rglob
    task_config_1 = """
    task_name: task_1
    description: "Task 1 description"
    """
    task_config_2 = """
    task_name: task_2
    description: "Task 2 description"
    """
    task_config_3 = """
    task_name: task_3
    description: "Task 3 description"
    """
    task_config_4 = """
    task_name: task_4
    description: "Task 4 description"
    """

    # Create a list of mocked paths to simulate different directory depths
    mock_paths = [
        Path("/mock/proj_root/task_1_config.yaml"),  # Shallow-level
        Path("/mock/proj_root/subdir1/task_2_config.yaml"),  # Deeper level
        Path("/mock/proj_root/subdir2/subdir3/task_3_config.yaml"),  # Deepest level
        Path("/mock/proj_root/subdir2/task_4_config.yaml")  # Shallow-level in subdir2
    ]

    # Create a dictionary of path-to-content mappings
    mock_file_contents = {
        mock_paths[0]: task_config_1,  # task_1_config.yaml
        mock_paths[1]: task_config_2,  # task_2_config.yaml
        mock_paths[2]: task_config_3,  # task_3_config.yaml
        mock_paths[3]: task_config_4   # task_4_config.yaml
    }

    # Mock the rglob method to return the above paths
    with patch.object(Path, "rglob", return_value=mock_paths):
        # Ensure the paths exist in the test mock
        bob_instance.task_configs = {}  # Initialize an empty dictionary
        
        # Mock the open function with side_effect to return specific content based on file path
        def mock_open_side_effect(filepath, *args, **kwargs):
            # Use the file path to return the correct content
            return mock_open(read_data=mock_file_contents[Path(filepath)]).return_value

        # Simulate the file content loading by mocking the open function
        with patch("builtins.open", side_effect=mock_open_side_effect):
            bob_instance.discover_tasks()  # Run discover_tasks, which will use rglob()

        return bob_instance, mock_paths

def test_set_env_var_val_new_variable(bob_instance: Bob):
    """Test setting a new environment variable."""
    bob_instance.set_env_var_val("TEST_VAR", "test_value")
    assert os.environ.get("TEST_VAR") == "test_value"

def test_set_env_var_val_already_set(bob_instance: Bob, caplog: pytest.LogCaptureFixture):
    """Test setting an already existing environment variable."""
    os.environ["TEST_VAR"] = "initial_value"
    with caplog.at_level(logging.WARNING):
        bob_instance.set_env_var_val("TEST_VAR", "new_value")
        assert "has already been set" in caplog.text  # Ensure warning is logged
    assert os.environ.get("TEST_VAR") == "initial_value"  # Value should not change

def test_set_env_var_val_invalid_types(bob_instance: Bob, caplog: pytest.LogCaptureFixture):
    """Test passing invalid types for key and value."""
    with caplog.at_level(logging.ERROR):
        bob_instance.set_env_var_val(123, "valid_value")
        assert "Both of them has to be a str" in caplog.text

        bob_instance.set_env_var_val("VALID_KEY", 456)
        assert "Both of them has to be a str" in caplog.text

def test_set_env_var_val_upper_key(bob_instance: Bob):
    """Test uppercasing the env key."""
    bob_instance.set_env_var_val("lower", "my_string")
    assert os.environ.get("LOWER") == "my_string"
    assert os.environ.get("lower") == None

def test_set_env_var_path_valid(bob_instance: Bob):
    """Test setting an environment variable to a valid path without actually creating it."""
    mock_path = MagicMock(Path)
    mock_path.exists.return_value = True
    mock_path.__str__.return_value = "/mock/path"
    bob_instance.set_env_var_path("TEST_PATH", mock_path)
    assert os.environ.get("TEST_PATH") == "/mock/path"

def test_set_env_var_path_nonexistent(bob_instance: Bob, tmp_path: Path, caplog: pytest.LogCaptureFixture):
    """Test setting an environment variable to a nonexistent path."""
    nonexistent_path = tmp_path / "does_not_exist"
    with caplog.at_level(logging.ERROR):
        bob_instance.set_env_var_path("TEST_PATH_2", nonexistent_path)
        assert f"The path {nonexistent_path} does not exist" in caplog.text
    assert os.environ.get("TEST_PATH_2") is None  # Env var should not be set

def test_set_env_var_path_invalid_types(bob_instance: Bob, caplog: pytest.LogCaptureFixture):
    """Test passing invalid types for key and path."""
    with caplog.at_level(logging.ERROR):
        bob_instance.set_env_var_path(123, Path("/valid/path"))
        assert "it has to be a str" in caplog.text

        bob_instance.set_env_var_path("VALID_KEY", "not_a_path")
        assert "it has to be a Path object" in caplog.text

def test_append_env_var_path_invalid_types(bob_instance: Bob, caplog: pytest.LogCaptureFixture):
    """Test passing invalid types for key and value"""
    with caplog.at_level(logging.ERROR):
        bob_instance.append_env_var_path(123, "valid_value")
        assert "has to be a str" in caplog.text

        bob_instance.append_env_var_path("valid_key", 456)
        assert "has to be a str" in caplog.text

def test_append_env_var_path_first_path(bob_instance: Bob):
    """Test adding the first path"""
    mock_path = MagicMock(Path)
    mock_path.exists.return_value = True
    mock_path.__str__.return_value = "/mock/append_env_var_path"
    bob_instance.append_env_var_path("append_env_var", mock_path)
    assert os.environ.get("APPEND_ENV_VAR") == "/mock/append_env_var_path"

def test_append_env_var_path_subsequent_path(bob_instance: Bob):
    """Testing adding more paths to existing env var"""
    mock_path = MagicMock(Path)
    mock_path.exists.return_value = True
    mock_path.__str__.return_value = "/mock/append_env_var_path_2"
    assert os.environ.get("APPEND_ENV_VAR") == "/mock/append_env_var_path"
    bob_instance.append_env_var_path("append_env_var", mock_path)
    assert os.environ.get("APPEND_ENV_VAR") == "/mock/append_env_var_path:/mock/append_env_var_path_2"

def test_load_cfg_nonexistent(bob_instance: Bob, monkeypatch: pytest.MonkeyPatch, caplog: pytest.LogCaptureFixture):
    """Test load cfg with a non-existent ip_config.yaml"""
    # monkeypatch.setattr(Path, "exists", lambda self: False)
    monkeypatch.setattr(Path, "exists", lambda self: False)

    with pytest.raises(SystemExit) as exc_info, caplog.at_level(logging.ERROR):
        bob_instance.load_ip_cfg()
    assert "No ip_config.yaml is found" in caplog.text
    assert exc_info.value.code == 1

def test_load_cfg_existent(bob_instance: Bob, caplog: pytest.LogCaptureFixture):
    """Test that ip_config.yaml exists under PROJ_ROOT"""
    with caplog.at_level(logging.INFO):
        bob_instance.load_ip_cfg()
    assert f"Parsing {Path(bob_instance.proj_root) / "ip_config.yaml"}" in caplog.text
    assert isinstance(bob_instance.ip_config, dict)
    assert "project" in bob_instance.ip_config

def test_parse_ip_cfg(bob_instance):
    """Test placeholder resolution for entire config."""
    bob_instance.load_ip_cfg()
    expected_output = {
        "directories": {
            "root_dir": "/home/user/fuji",
            "src_dir": "/home/user/fuji/src",
            "build_dir": "/home/user/fuji/build"
        },
        "project": {
            "name": "pong",
            "arch": "xc7",
            "board_name": "arty",
            "fpga_part": "xc7a35ticsg324-1L"
        },
        "sources": {
            "top_level": "/home/user/fuji/src/top_pong.sv"
        },
        "tasks": {
                "tb_hello_world": { "depends_on" : [ "/home/user/fuji/hello_world", "pong/bye_world", "/home/user/fuji/task_0"]}
        }
    }
    assert bob_instance.parse_ip_cfg() == expected_output
# def test_parse_ip_cfg(bob_instance):
#     """Test full parsing and placeholder resolution of the ip_config.yaml."""
#     bob_instance.load_ip_cfg()
#     resolved_config = bob_instance.parse_ip_cfg()
#
#     assert resolved_config["directories"]["src_dir"] == "./src"
#     assert resolved_config["directories"]["build_dir"] == "./build"
#     assert resolved_config["sources"]["top_level"] == "./src/top_pong.sv"
#     assert resolved_config["tasks"]["tb_hello_world"]["depends_on"] == ["./hello_world", "pong/bye_world"]

def test_resolve_value_string(bob_instance):
    """Test resolving a single string placeholder."""
    bob_instance.load_ip_cfg()
    assert bob_instance._resolve_value("${directories.root_dir}") == "/home/user/fuji"
    assert bob_instance._resolve_value("${directories.src_dir}") == "/home/user/fuji/src"
    assert bob_instance._resolve_value("${directories.root_dir}/test") == "/home/user/fuji/test"

def test_resolve_value_list(bob_instance):
    """Test resolving placeholders in a list."""
    bob_instance.load_ip_cfg()
    assert bob_instance._resolve_value("${tasks.tb_hello_world.depends_on}") == ["/home/user/fuji/hello_world", "pong/bye_world", "/home/user/fuji/task_0"]

def test_resolve_value_dict(bob_instance):
    """Test resolving placeholders in a dictionary."""
    bob_instance.load_ip_cfg()
    input_dict = {
        "build_dir": "${directories.root_dir}/build",
        "source_file": "${directories.src_dir}/main.sv"
    }
    expected_output = {
        "build_dir": "/home/user/fuji/build",
        "source_file": "/home/user/fuji/src/main.sv"
    }
    assert bob_instance._resolve_value(input_dict) == expected_output

def test_get_value_from_hierarchy_valid(bob_instance):
    """Test fetching values from hierarchy."""
    bob_instance.load_ip_cfg()
    assert bob_instance._get_value_from_hierarchy("directories.root_dir", {}) == "/home/user/fuji"
    assert bob_instance._get_value_from_hierarchy("directories.src_dir", {"root_dir":"/home/user/fuji"}) == "/home/user/fuji/src"
    assert bob_instance._get_value_from_hierarchy("project.name", {}) == "pong"

def test_get_value_from_hierarchy_invalid(bob_instance):
    """Test fetching a non-existing value (should return None)."""
    bob_instance.load_ip_cfg()
    assert bob_instance._get_value_from_hierarchy("invalid.key", {}) is None
    assert bob_instance._get_value_from_hierarchy("directories.nonexistent", {}) is None

def test_resolve_placeholder_valid(bob_instance):
    """Test resolving valid placeholders."""
    bob_instance.load_ip_cfg()
    assert bob_instance._resolve_placeholder("${directories.root_dir}", {"root_dir": "/home/user/fuji"}) == "/home/user/fuji"
    assert bob_instance._resolve_placeholder("${project.name}", {"name": "pong"}) == "pong"
    assert bob_instance._resolve_placeholder("${sources.top_level}",{"src_dir":"/home/user/fuji/src", "name": "pong"} ) == "/home/user/fuji/src/top_pong.sv"

def test_resolve_placeholder_invalid(bob_instance, caplog):
    """Test resolving an invalid placeholder (should log an error)."""
    bob_instance.load_ip_cfg()
    with caplog.at_level(logging.ERROR):
        assert bob_instance._resolve_placeholder("${invalid.key}", {}) == "${invalid.key}"
    assert "Warning: No matching value found for placeholder invalid.key" in caplog.text

def test_ensure_correct_path_format(bob_instance):
    """Test proper path normalisation."""
    bob_instance.load_ip_cfg()
    assert bob_instance._ensure_correct_path_format("${directories.root_dir}/src", "home/user/fuji/src") == "home/user/fuji/src"
    assert bob_instance._ensure_correct_path_format("${directories.root_dir}/build", "home/user/fuji/build") == "home/user/fuji/build"

def test_resolve_env_var_absolute_path(bob_instance):
    """Test resolving PROJ_ROOT environment variable and preserving absolute paths."""
    assert bob_instance.proj_root.startswith('/')
    bob_instance.load_ip_cfg()
    resolved_config = bob_instance.parse_ip_cfg()
    assert resolved_config["directories"]["root_dir"].startswith('/')
    assert resolved_config["directories"]["src_dir"].startswith('/')

def test_discover_task_valid(create_valid_task_config: Path, bob_instance: Bob):
    """Test when a valid task_config.yaml is found"""
    # Set the proj_root to the directory containing the task_config.yaml
    bob_instance.proj_root = str(create_valid_task_config.parent)

    # Call discover_tasks
    bob_instance.discover_tasks()

    # Check if the task was correctly added to task_configs
    assert "test_task" in bob_instance.task_configs
    assert bob_instance.task_configs["test_task"]["task_config_path"] == create_valid_task_config

def test_discover_task_missing_task_name(bob_instance: Bob, tmp_path: Path, caplog):
    """Test when task_config.yaml is missing a task_name field"""
    # Create an invalid task_config.yaml (no task_name)
    task_config = """
    description: "A sample task"
    """
    task_config_path = tmp_path / "task_config.yaml"
    task_config_path.write_text(task_config)

    # Set proj_root to the directory containing the invalid task_config.yaml
    bob_instance.proj_root = str(tmp_path)

    # Mock sys.exit to prevent the test from exiting
    with pytest.raises(SystemExit) as mock_exit:
        bob_instance.discover_tasks()

def test_discover_task_no_files(bob_instance, tmp_path):
    """Test when no task_config.yaml files are found"""
    bob_instance.proj_root = str(tmp_path)  # Ensure there's no task_config.yaml in the temp directory

    # Call discover_tasks (no exception should be raised)
    bob_instance.discover_tasks()

    # Check that no tasks were added
    assert len(bob_instance.task_configs) == 0

def test_discover_task_unexpected_error(bob_instance, tmp_path):
    """Test when discover_tasks raises an unexpected error"""
    task_config_path = tmp_path / "task_config.yaml"
    task_config_path.write_text("task_name: test_task")  # Valid content

    # Set the proj_root to the directory containing the task_config.yaml
    bob_instance.proj_root = str(tmp_path)

    # Mock sys.exit to prevent test from exiting
    with patch("sys.exit") as mock_exit:
        # Monkeypatch open to raise an IOError
        with patch("builtins.open", side_effect=IOError("File read error")):
            # with pytest.raises(SystemExit):
            bob_instance.discover_tasks()

        # Assert that sys.exit was called with the expected exit code
        mock_exit.assert_called_once_with(1)

def test_discover_task_logging(bob_instance, create_valid_task_config):
    """Test that logging happens correctly during task discovery"""
    # Mock the logger
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger

    # Set proj_root to the directory containing the task_config.yaml
    bob_instance.proj_root = str(create_valid_task_config.parent)

    # Call discover_tasks
    bob_instance.discover_tasks()

    # Check that the logger's info method was called with the correct message
    mock_logger.info.assert_any_call(f"task_config.yaml found in {create_valid_task_config}")

def test_discover_tasks_multiple_configs_different_hierarchy(create_multiple_valid_task_config):
    """Test when discover_tasks finds multiple task_config.yaml files in different directory hierarchies"""
    bob_instance, expected_paths = create_multiple_valid_task_config

    # Mock the logger to capture log output
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger  # Replace with mock logger

    # Mock sys.exit to prevent the actual exit
    with patch("sys.exit") as mock_exit:
        # Run discover_tasks
        bob_instance.discover_tasks()

        # Check that task names are correctly extracted and stored in task_configs
        expected_task_names = ["task_1", "task_2", "task_3", "task_4"]
        assert len(bob_instance.task_configs) == 4
        for task_name, task_config_path in zip(expected_task_names, expected_paths):
            assert task_name in bob_instance.task_configs
            assert bob_instance.task_configs[task_name]["task_config_path"] == task_config_path

        # Ensure that sys.exit was never called (it should not be triggered in this test)
        mock_exit.assert_not_called()

@patch("pathlib.Path.mkdir")
def test_setup_build_dirs_success(mock_mkdir, bob_instance, create_valid_task_config):
    """Test the creation of build dir, and the task build dir"""
    # Mock the logger
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger

    bob_instance.proj_root = str(create_valid_task_config.parent)
    bob_instance.discover_tasks()

    bob_instance.setup_build_dirs()

    build_root = Path(bob_instance.proj_root) / "build"
    mock_mkdir.assert_any_call(exist_ok=True)  # Check if root build dir was created

    for task in bob_instance.task_configs:
        build_dir = build_root / task
        mock_mkdir.assert_any_call(exist_ok=True)

    # The 1 comes from the creation of the root build dir
    assert mock_mkdir.call_count == len(bob_instance.task_configs) + 1

@patch("pathlib.Path.mkdir")
def test_setup_build_dirs_no_tasks(mock_mkdir, bob_instance):
    """Test the setup dir function when there are no task"""
    # Mock the logger
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger

    bob_instance.discover_tasks()  # This should result in no tasks being found

    with pytest.raises(SystemExit):  # Should exit due to LookupError
        bob_instance.setup_build_dirs()

    bob_instance.logger.info.assert_not_called()
    bob_instance.logger.critical.assert_not_called()
    mock_mkdir.assert_called_once_with(exist_ok=True)  # Only root dir should be created

@patch("pathlib.Path.mkdir", side_effect=OSError("Permission Denied"))
def test_setup_build_dirs_oserror(mock_mkdir, bob_instance, create_valid_task_config):
    """Test the setup dir but mock that there is a Permission Denied error from creating dir"""
    # Mock the logger
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger

    bob_instance.proj_root = str(create_valid_task_config.parent)
    bob_instance.discover_tasks()

    with pytest.raises(SystemExit):  # Should exit due to exception
        bob_instance.setup_build_dirs()

    bob_instance.logger.critical.assert_called_once()

@patch("shutil.rmtree")
def test_remove_task_build_dirs(mock_rmtree):
    """Test that all the task build dirs have been removed"""
    # Create a mock Bob instance and logger
    logger = MagicMock()
    bob_instance = Bob(logger)
    bob_instance.proj_root = "/mock/proj_root"
    bob_instance.task_configs = {"task1":"", "task2":""}

    # Patch the methods that check for file existence and directory type
    with patch.object(Path, "exists") as mock_exists, patch.object(Path, "is_dir") as mock_is_dir:
        mock_exists.return_value = True
        mock_is_dir.return_value = True
        # Case 1: All tasks exist and directories are valid
        deleted_count = bob_instance.remove_task_build_dirs(["task1", "task2", "task3"])
        assert deleted_count == 2  # task1 and task2 should be deleted
        assert mock_rmtree.call_count == 2
        bob_instance.logger.info.assert_any_call("Deleted build directory: /mock/proj_root/build/task1")
        bob_instance.logger.info.assert_any_call("Deleted build directory: /mock/proj_root/build/task2")

def test_create_all_task_env_valid_tasks(create_multiple_valid_task_config):
    """Test the creation of new env based on the global env for all tasks defined"""
    bob_instance, expected_paths = create_multiple_valid_task_config

    # Mock the logger to capture log output
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger  # Replace with mock logger

    # Run discover_tasks
    bob_instance.discover_tasks()

    expected_task_names = ["task_1", "task_2", "task_3", "task_4"]

    # Set up global env
    cwd = os.getcwd()
    os.environ["PROJ_ROOT"] = str(cwd)

    # Run create all task environment
    bob_instance.create_all_task_env()

    assert len(bob_instance.task_configs) == 4
    for task_name, task_config_path in zip(expected_task_names, expected_paths):
        assert task_name in bob_instance.task_configs
        assert type(bob_instance.task_configs[task_name]["task_env"]) == dict
        assert bob_instance.task_configs[task_name]["task_env"].get("PROJ_ROOT") == cwd
        assert bob_instance.task_configs[task_name]["task_env"] == os.environ

def test_create_all_task_env_no_tasks():
    """Test the creation of task environments when there are no tasks defined"""
    # Create mock Bob instance
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    # Ensure task_configs is empty
    bob_instance.task_configs = {}

    with pytest.raises(SystemExit):  # Should exit due to exception
        bob_instance.create_all_task_env()
    bob_instance.logger.error.assert_called_once()

def test_create_all_task_env_exception(create_multiple_valid_task_config):
    """Test create_all_task_env when os.environ.copy() raises an exception."""
    bob_instance, _ = create_multiple_valid_task_config

    # Mock logger
    mock_logger = MagicMock()
    bob_instance.logger = mock_logger

    # Mock os.environ.copy to raise an exception
    with patch("os.environ.copy", side_effect=RuntimeError("Mocked environment error")):
        with pytest.raises(SystemExit):  # Ensure sys.exit(1) is triggered
            bob_instance.create_all_task_env()

    # Ensure critical log message was recorded
    mock_logger.critical.assert_called_once()

    assert "Unexpected error during create_all_task_env()" in mock_logger.critical.call_args[0][0]

def test_append_task_env_var_val_is_path_and_key_doesnt_exist():
    """Test setting a path to a env key which doesn't exist"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "/some/old/path"}},
    }
    task_name = "task1"
    env_key = "NEW_VAR"
    env_val = Path("/new/path/to/set")
    # Mock the existence of Path
    with patch.object(Path, "exists") as mock_exists:
        mock_exists.return_value = True
        bob_instance.append_task_env_var_val(task_name, env_key, env_val)

        updated_value = bob_instance.task_configs[task_name]["task_env"][env_key.upper()]
        assert updated_value == str(env_val)
        bob_instance.logger.debug.assert_called_once_with(f"For {task_name} env, setting env var {env_key} = {env_val}.")

def test_append_task_env_var_val_is_path_and_key_exists():
    """Test appending a path to a env key which already exists"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "/some/old/path"}},
    }
    task_name = "task1"
    env_key = "EXISTING_VAR"
    env_val = Path("/new/path/to/append")

    # Mock the existence of Path
    with patch.object(Path, "exists") as mock_exists:
        mock_exists.return_value = True
        bob_instance.append_task_env_var_val(task_name, env_key, env_val)

        updated_value = bob_instance.task_configs[task_name]["task_env"][env_key.upper()]
        assert updated_value == "/some/old/path" + os.pathsep + "/new/path/to/append"

        bob_instance.logger.debug.assert_called_once_with(f"For {task_name} env, appending path /new/path/to/append to env var {env_key.upper()}. {env_key.upper()} = /some/old/path{os.pathsep}/new/path/to/append.")

def test_append_task_env_var_val_is_path_and_key_doesnt_exist_and_path_doesnt_exist():
    """Test adding a non-existent path to an existing env_key, it should throw an exception"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "/some/old/path"}},
    }
    task_name = "task1"
    env_key = "EXISTING_VAR"
    env_val = Path("/non/existing/path")

    # Not mocking the existence of Path, so it should throw FileNotFoundError
    bob_instance.append_task_env_var_val(task_name, env_key, env_val)

    assert bob_instance.task_configs[task_name]["task_env"].get(env_key.upper()) == "/some/old/path"
    bob_instance.logger.error.assert_called_once_with(f"FileNotFoundError : The path {env_val} does not exist, hence {env_key.upper()} is not set to that path.")

def test_append_task_env_var_val_is_str_and_key_doesnt_exist():
    """Test adding a new env var"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "/some/old/path"}},
    }
    task_name = "task1"
    env_key = "NEW_VAR"
    env_val = "some_string_value"

    bob_instance.append_task_env_var_val(task_name, env_key, env_val)
    updated_value = bob_instance.task_configs[task_name]["task_env"][env_key]
    assert updated_value == env_val

    bob_instance.logger.debug.assert_called_once_with(f"For {task_name} env, setting env var {env_key.upper()} = {env_val}.")

def test_append_task_env_var_val_is_str_and_key_exist():
    """Test replacing old env val with new env val"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "old_value"}},
    }
    task_name = "task1"
    env_key = "EXISTING_VAR"
    env_val = "new_value"

    bob_instance.append_task_env_var_val(task_name, env_key, env_val)
    updated_value = bob_instance.task_configs[task_name]["task_env"][env_key]
    assert updated_value == env_val

    bob_instance.logger.debug.assert_called_once_with(f"For {task_name} env, updating env var {env_key.upper()} = {env_val}.")

def test_append_task_env_var_val_invalid_task_name():
    """Test setting env var for a non-existent task env"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "task1": {"task_env": {"EXISTING_VAR": "old_value"}},
    }
    task_name = "non_existent_task"
    env_key = "TASK_2_KEY"
    env_val = "some_string_value"

    bob_instance.append_task_env_var_val(task_name, env_key, env_val)
    bob_instance.logger.critical.assert_called_once_with(f"Unexpected error during append_task_env_var_val(): 'non_existent_task'", exc_info=True)

@patch.object(Path, "exists", return_value=True)
def test_append_task_src_files_single_valid_file(mock_exists):
    """Test adding a single valid file to 'src_files' within task_configs for a task"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "existing_task" : {},
    }

    test_file = Path("/valid/file1.v")
    bob_instance.append_task_src_files("existing_task", test_file)

    assert "src_files" in bob_instance.task_configs["existing_task"]
    assert bob_instance.task_configs["existing_task"]["src_files"] == [test_file]

@patch.object(Path, "exists", side_effect=[True, True, False])  # Simulate the first 2 files exists, and the last one missing
def test_append_task_src_files_multiple_files_with_missing(mock_exists):
    """Test adding multiple files to 'src_files' list, with one missing."""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "existing_task" : {},
    }
    files = [Path("/valid/file1.v"), Path("/valid/file2.v"), Path("/missing/file3.sv")]

    bob_instance.append_task_src_files("existing_task", files)

    assert "src_files" in bob_instance.task_configs["existing_task"]
    assert bob_instance.task_configs["existing_task"]["src_files"] == files[:2]  # Check that only valid ones are added
    bob_instance.logger.error.assert_called_once_with(f"For existing_task, the following paths do not exist and will be ignored: {{PosixPath('/missing/file3.sv')}}")

@patch.object(Path, "exists", return_value=True)
def test_append_task_src_files_duplicate_files(mock_exists):
    """Test that duplicate files are not added multiple times."""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "existing_task" : {},
    }
    file = Path("/valid/file4.v")

    # Add the file twice
    bob_instance.append_task_src_files("existing_task", file)
    bob_instance.append_task_src_files("existing_task", file)

    assert bob_instance.task_configs["existing_task"]["src_files"] == [file]
    bob_instance.logger.info.assert_called_once_with("No new files were added to task 'existing_task' (duplicates avoided).")

@patch.object(Path, "exists", return_value=True)
def test_append_task_src_files_non_existent_task(mock_exists):
    """Test adding src files to a non-existent task"""
    mock_logger = MagicMock()
    bob_instance = Bob(mock_logger)

    bob_instance.task_configs = {
            "existing_task" : {},
    }
    task_name = "non_existent_task"
    file = Path("/valid/file4.v")
    bob_instance.append_task_src_files(task_name, file)

    bob_instance.logger.error.assert_called_once_with(f"Task '{task_name}' does not exist in task_configs. Please run discover_tasks() first.")
