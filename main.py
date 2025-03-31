from pathlib import Path
from bob.Bob import Bob
import os
import sys
import logging

from ipConfigParser.IpConfigParser import IpConfigParser
from pytests.test_ipConfigParser import ip_config_parser
from taskConfigParser.TaskConfigParser import TaskConfigParser

# Define ANSI color codes for terminal output
class ColorFormatter(logging.Formatter):
    COLORS = {
        'DEBUG': '\033[34m',  # Blue
        'INFO': '\033[32m',  # Green
        'WARNING': '\033[33m',  # Yellow
        'ERROR': '\033[35m',  # Magenta
        'CRITICAL': '\033[31m',  # Red
        'RESET': '\033[0m'  # Reset color
    }

    def format(self, record):
        levelname = f"{record.levelname:<8}"  # Fixed width, left-aligned (8 spaces)
        colored_levelname = f"{self.COLORS.get(record.levelname, self.COLORS['RESET'])}{levelname}{self.COLORS['RESET']}"
        record.levelname = colored_levelname  # Replace levelname with the colored one
        return super().format(record)

# Configure Logging
LOG_FILE = "main.log"
# Set the log format with fixed width for levelname and filename
LOG_FORMAT = "[%(asctime)s] [%(levelname)-8s] [%(filename)-12s:%(lineno)-4d] %(message)s"  # Ensure alignment for levelname and filename
DATE_FORMAT = "%Y-%m-%d %H:%M:%S"
logging.basicConfig(
    level=logging.DEBUG,  # Capture all levels (DEBUG and above)
    format=LOG_FORMAT,
    datefmt=DATE_FORMAT,
    handlers=[
    ],
)
logger = logging.getLogger(__name__)
# Add a file handler without color formatting, file_handler has be configured prior to console_handler to remove color code printed to log file
file_handler = logging.FileHandler(LOG_FILE, mode="a")
file_handler.setFormatter(logging.Formatter(LOG_FORMAT, datefmt=DATE_FORMAT))  # Use regular formatter without color
logger.addHandler(file_handler)
# Add the custom color formatter to the console handler
console_handler = logging.StreamHandler()
console_handler.setFormatter(ColorFormatter(LOG_FORMAT, datefmt=DATE_FORMAT))
logger.addHandler(console_handler)


def test_log_levels():
    # Log messages for each level to test color formatting
    logger.debug("This is a DEBUG level message.")
    logger.info("This is an INFO level message.")
    logger.warning("This is a WARNING level message.")
    logger.error("This is an ERROR level message.")
    logger.critical("This is a CRITICAL level message.")

def main() -> None:
    try:
        # PROJ_ROOT = Path(__file__).resolve().parent
        cwd = os.getcwd()
        # Check if there is an ip_config.yaml in proj_root
        proj_root_ip_cfg_path = Path(cwd) / "ip_config.yaml"
        if not proj_root_ip_cfg_path.exists():
            raise FileNotFoundError(f"An ip_config.yaml must be defined within the project root under {cwd}.")
        os.environ["PROJ_ROOT"] = str(cwd)
        logger.info(f"PROJ_ROOT set to: {cwd}")
    except FileNotFoundError as fnfe:
        logger.critical(f"Error locating ip_config.yaml at project root. {fnfe}")
        exit(1)
    except Exception as e:
        logger.critical(f"Error setting FUJI_ROOT: {e}", exc_info=True)
        exit(1)

    bob = Bob(logger)
    logger.info(f"Class name: {Bob.__name__}")
    bob_proj_root = bob.get_proj_root()
    print(f"bob_fuji_root = {str(bob_proj_root)}")
    boba_dir = bob_proj_root / "boba"
    bob.set_env_var_path("boba_dir", boba_dir)
    boba_boba_dir = boba_dir / "boba"
    bob.set_env_var_path("boba_dir", boba_boba_dir / "boba")
    bob.set_env_var_path("bob_dir", bob_proj_root / "bob")
    bob.set_env_var_path("bob_dir", bob_proj_root / "bobaa")

    bob.run_subprocess()

    test_log_levels()

    ip_config_parser = IpConfigParser(bob.logger, bob.proj_root)
    ip_config_parser.load_ip_cfg()
    ip_config_parser.parse_ip_cfg()
    dependency_graph = ip_config_parser.build_task_dependency_graph()

    # Assign dependency_graph to bob, such that it can use it in schedule_tasks()
    bob.dependency_graph = dependency_graph

    bob.discover_tasks()
    print(bob.task_configs)
    bob.setup_build_dirs()
    print(type(os.environ))
    bob.create_all_task_env()
    for task_name, task_config in bob.task_configs.items():
        assert(task_config["task_env"] == os.environ)

    bob.append_task_env_var_val("tb_hello_world", "new_key", "new_val")
    bob.append_task_env_var_val("tb_hello_world", "new_path", Path(os.getcwd()))
    print(bob.task_configs["tb_hello_world"]["task_env"])
    bob.append_task_env_var_val("hello_world", "hello_world_key", "hello_world_val")
    print(bob.task_configs["hello_world"]["task_env"])

    hello_world_file_path = Path(bob.proj_root) / "hello_world" / "hello_world_top.sv"
    assert(hello_world_file_path.exists())
    bob.append_task_src_files("hello_world", hello_world_file_path)
    print(bob.task_configs["hello_world"]["src_files"])

    tb_hello_world_file_path = Path(bob.proj_root) / "tb_hello_world" / "tb_hello_world.sv"
    assert(tb_hello_world_file_path.exists())
    bob.append_task_src_files("tb_hello_world", tb_hello_world_file_path)

    bob.ensure_dotbob_dir_at_proj_root()

    bob.task_configs["new_task"] = {}
    bob.ensure_dotbob_dir_at_proj_root()

    print(bob.task_configs)

    task_config_parser = TaskConfigParser(bob.logger, bob.proj_root)
    task_config_parser.inherit_task_configs(bob.task_configs)
    print("")
    print(f"task_configs_parser.task_configs={task_config_parser.task_configs}")
    arith_c_compile_config_file = task_config_parser.task_configs["arith_c_compile"].get("task_config_file_path", None)
    if arith_c_compile_config_file:
        task_config_parser._load_task_config_file(arith_c_compile_config_file)
        task_config_parser.parse_task_config_dict("arith_c_compile")
        # Pass the updated task_config back to bob
        bob.task_configs["arith_c_compile"] = task_config_parser.task_configs["arith_c_compile"]
        # Execute c compile
        # assert(bob.execute_c_compile("sum_c_compile"))
    hello_world_c_compile_config_file = task_config_parser.task_configs["hello_world_c_compile"].get("task_config_file_path", None)
    if hello_world_c_compile_config_file:
        task_config_parser._load_task_config_file(hello_world_c_compile_config_file)
        task_config_parser.parse_task_config_dict("hello_world_c_compile")
        print(f"task_configs['hello_world_c_compile']={task_config_parser.task_configs["hello_world_c_compile"]}")
        # Pass the updated task_config back to bob
        bob.task_configs["hello_world_c_compile"] = task_config_parser.task_configs["hello_world_c_compile"]
        # Execute c compile + c link
        # assert(bob.execute_c_compile("hello_world_c_compile"))
        # print(f"task_config['hello_world_c_compile'] = {bob.task_configs['hello_world_c_compile']}")

    bob.execute_tasks()

if __name__ == "__main__":
    main()
