import os
import pytest
import logging
from pathlib import Path
from ipConfigParser.IpConfigParser import IpConfigParser
from unittest.mock import MagicMock, patch, mock_open
from networkx import DiGraph

@pytest.fixture
def ip_config_parser(monkeypatch, tmp_path: Path):
    """Fixture to create a IpConfigParser instance with a test logger, mock PROJ_ROOT and ip_config.yaml"""
    proj_root = tmp_path
    monkeypatch.setenv("PROJ_ROOT", str(proj_root))
    logger = logging.getLogger("test_logger")
    logger.setLevel(logging.DEBUG)

    ip_config_parser = IpConfigParser(logger, str(proj_root))

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
    assert ip_config_parser.proj_root == str(proj_root)
    return ip_config_parser

def test_load_cfg_nonexistent(ip_config_parser: IpConfigParser, monkeypatch: pytest.MonkeyPatch, caplog: pytest.LogCaptureFixture):
    """Test load cfg with a non-existent ip_config.yaml"""
    # monkeypatch.setattr(Path, "exists", lambda self: False)
    monkeypatch.setattr(Path, "exists", lambda self: False)

    with pytest.raises(SystemExit) as exc_info, caplog.at_level(logging.ERROR):
        ip_config_parser.load_ip_cfg()
    assert "No ip_config.yaml is found" in caplog.text
    assert exc_info.value.code == 1

def test_load_cfg_existent(ip_config_parser: IpConfigParser, caplog: pytest.LogCaptureFixture):
    """Test that ip_config.yaml exists under PROJ_ROOT"""
    with caplog.at_level(logging.INFO):
        ip_config_parser.load_ip_cfg()
    assert f"Parsing {Path(ip_config_parser.proj_root) / "ip_config.yaml"}" in caplog.text
    assert isinstance(ip_config_parser.ip_config, dict)
    assert "project" in ip_config_parser.ip_config

def test_parse_ip_cfg(ip_config_parser):
    """Test placeholder resolution for entire config."""
    ip_config_parser.load_ip_cfg()
    expected_output = {
        "directories": {
            "root_dir": f"{ip_config_parser.proj_root}",
            "src_dir": f"{ip_config_parser.proj_root}/src",
            "build_dir": f"{ip_config_parser.proj_root}/build"
        },
        "project": {
            "name": "pong",
            "arch": "xc7",
            "board_name": "arty",
            "fpga_part": "xc7a35ticsg324-1L"
        },
        "sources": {
            "top_level": f"{ip_config_parser.proj_root}/src/top_pong.sv"
        },
        "tasks": {
                "tb_hello_world": { "depends_on" : [ f"{ip_config_parser.proj_root}/hello_world", "pong/bye_world", f"{ip_config_parser.proj_root}/task_0"]}
        }
    }
    assert ip_config_parser.parse_ip_cfg() == expected_output

def test_resolve_value_string(ip_config_parser):
    """Test resolving a single string placeholder."""
    ip_config_parser.load_ip_cfg()
    assert ip_config_parser._resolve_value("${directories.root_dir}") == ip_config_parser.proj_root
    assert ip_config_parser._resolve_value("${directories.src_dir}") == f"{ip_config_parser.proj_root}/src"
    assert ip_config_parser._resolve_value("${directories.root_dir}/test") == f"{ip_config_parser.proj_root}/test"

def test_resolve_value_list(ip_config_parser):
    """Test resolving placeholders in a list."""
    ip_config_parser.load_ip_cfg()
    assert ip_config_parser._resolve_value("${tasks.tb_hello_world.depends_on}") == [f"{ip_config_parser.proj_root}/hello_world", "pong/bye_world", f"{ip_config_parser.proj_root}/task_0"]

def test_resolve_value_dict(ip_config_parser):
    """Test resolving placeholders in a dictionary."""
    ip_config_parser.load_ip_cfg()
    input_dict = {
        "build_dir": "${directories.root_dir}/build",
        "source_file": "${directories.src_dir}/main.sv"
    }
    expected_output = {
        "build_dir": f"{ip_config_parser.proj_root}/build",
        "source_file": f"{ip_config_parser.proj_root}/src/main.sv"
    }
    assert ip_config_parser._resolve_value(input_dict) == expected_output

def test_get_value_from_hierarchy_valid(ip_config_parser):
    """Test fetching values from hierarchy."""
    ip_config_parser.load_ip_cfg()
    assert ip_config_parser._get_value_from_hierarchy("directories.root_dir", {}) == ip_config_parser.proj_root
    assert ip_config_parser._get_value_from_hierarchy("directories.src_dir", {"root_dir":f"{ip_config_parser.proj_root}"}) == f"{ip_config_parser.proj_root}/src"
    assert ip_config_parser._get_value_from_hierarchy("project.name", {}) == "pong"

def test_get_value_from_hierarchy_invalid(ip_config_parser):
    """Test fetching a non-existing value (should return None)."""
    ip_config_parser.load_ip_cfg()
    assert ip_config_parser._get_value_from_hierarchy("invalid.key", {}) is None
    assert ip_config_parser._get_value_from_hierarchy("directories.nonexistent", {}) is None

def test_resolve_placeholder_valid(ip_config_parser):
    """Test resolving valid placeholders."""
    ip_config_parser.load_ip_cfg()
    assert ip_config_parser._resolve_placeholder("${directories.root_dir}", {"root_dir": ip_config_parser.proj_root}) == ip_config_parser.proj_root
    assert ip_config_parser._resolve_placeholder("${project.name}", {"name": "pong"}) == "pong"
    assert ip_config_parser._resolve_placeholder("${sources.top_level}",{"src_dir":f"{ip_config_parser.proj_root}/src", "name": "pong"} ) == f"{ip_config_parser.proj_root}/src/top_pong.sv"

def test_resolve_placeholder_invalid(ip_config_parser, caplog):
    """Test resolving an invalid placeholder (should log an error)."""
    ip_config_parser.load_ip_cfg()
    with caplog.at_level(logging.ERROR):
        assert ip_config_parser._resolve_placeholder("${invalid.key}", {}) == "${invalid.key}"
    assert "Warning: No matching value found for placeholder invalid.key" in caplog.text

def test_ensure_correct_path_format(ip_config_parser):
    """Test proper path normalisation."""
    ip_config_parser.load_ip_cfg()
    expected_src_output = f"{ip_config_parser.proj_root}/src"
    expected_src_output = expected_src_output[1:]
    expected_build_output = f"{ip_config_parser.proj_root}/build"
    expected_build_output = expected_build_output[1:]
    assert ip_config_parser._ensure_correct_path_format("${directories.root_dir}/src", f"{ip_config_parser.proj_root}/src") == expected_src_output
    assert ip_config_parser._ensure_correct_path_format("${directories.root_dir}/build", f"{ip_config_parser.proj_root}/build") == expected_build_output

def test_resolve_env_var_absolute_path(ip_config_parser):
    """Test resolving PROJ_ROOT environment variable and preserving absolute paths."""
    assert ip_config_parser.proj_root.startswith('/')
    ip_config_parser.load_ip_cfg()
    resolved_config = ip_config_parser.parse_ip_cfg()
    assert resolved_config["directories"]["root_dir"].startswith('/')
    assert resolved_config["directories"]["src_dir"].startswith('/')

def test_build_task_dependency_graph_valid_1(tmp_path: Path):
    """Test building a valid ip_config.yaml with config 1"""
    mock_logger = MagicMock()
    ip_config_parser = IpConfigParser(mock_logger, str(tmp_path))
    ip_config_parser.ip_config = {
        "tasks" : {
            "sum_c_compile" : {
                "depends_on" : []
            },
            "hello_world_c_compile" : {
                "depends_on" : ["sum_c_compile"]
            },
            "hello_world" : {
                "depends_on" : []
            },
            "hello_world_2" : {
                "depends_on" : [
                    "hello_world_3",
                    "hello_world_6"
                ]
            },
            "hello_world_3" : {
                "depends_on" : [
                    "hello_world_4",
                    "hello_world_5"
                ]
            },
            "hello_world_4" : {
                "depends_on" : []
            },
            "hello_world_5" : {
                "depends_on" : []
            },
            "hello_world_6" : {
                "depends_on" : []
            }
        }
    }
    graph = ip_config_parser.build_task_dependency_graph()
    assert isinstance(graph, DiGraph)
    execution_chains = ip_config_parser.build_execution_chains()
    assert execution_chains == [['sum_c_compile', 'hello_world_c_compile'], ['hello_world'], ['hello_world_6', 'hello_world_4', 'hello_world_5', 'hello_world_3', 'hello_world_2']]
