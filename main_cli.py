from pathlib import Path
from bob.Bob import Bob
import os
import sys
import logging
import argparse

from ipConfigParser.IpConfigParser import IpConfigParser
from taskConfigParser.TaskConfigParser import TaskConfigParser
from toolConfigParser.ToolConfigParser import ToolConfigParser

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
LOG_FILE = "main_cli.log"
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

def create_parser() -> "argparse.ArgumentParser":
    # Parent parser for shared (common) subparser arguments
    common_parser = argparse.ArgumentParser(add_help=False)
    common_parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging"
    )

    # Main parser
    parser = argparse.ArgumentParser(
        description='Bob: A build manager for a range of HW/SW tasks.',
        epilog='''
Examples:
    %(prog)s build all
    %(prog)s build -t task1 task2
    %(prog)s clean all
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    subparsers = parser.add_subparsers(dest="mode", required=True, help="Operating mode")

    # Build subparser
    build_subparser = subparsers.add_parser(
        "build",
        parents=[common_parser],
        help="Build defined tasks",
    )

    # In build subparser, 'all' and '-t' are mutually exclusive
    build_subparser_exclusive_group = build_subparser.add_mutually_exclusive_group(required=True)
    build_subparser_exclusive_group.add_argument(
        "all",
        nargs="?",
        const=True,
        default=False,
        help="Build all tasks"
    )
    build_subparser_exclusive_group.add_argument(
        "-t", "--tasks",
        nargs="+",
        help="Specific task names to build, regex pattern enabled"
    )

    # Clean subparser
    clean_subparser = subparsers.add_parser(
        "clean",
        parents=[common_parser],
        help="Clean output directories of defined tasks",
    )
    clean_subparser_exclusive_group = clean_subparser.add_mutually_exclusive_group(required=True)
    clean_subparser_exclusive_group.add_argument(
        "all",
        nargs="?",
        const=True,
        default=False,
        help="Clean all tasks"
    )
    clean_subparser_exclusive_group.add_argument(
        "-u", "--until",
        help="Perform a clean of a task and its dependencies"
    )

    return parser


def main() -> int:
    logger = logging.getLogger(__name__)
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(ColorFormatter(LOG_FORMAT, datefmt=DATE_FORMAT))
    logger.addHandler(console_handler)

    parser = create_parser()
    args = parser.parse_args()
    print(args)
    # try:
    #     # Set up PROJ_ROOT first, which bob will use as proj_root
    #     cwd = os.getcwd()
    #     os.environ["PROJ_ROOT"] = str(cwd)
    #
    #     # Instantiate Bob object
    #     bob = Bob(logger)
    #     print(f"proj_root = {bob.get_proj_root()}")
    #
    #     # Load tool_config.yaml and set up tool paths
    #     bob.instantiate_and_associate_tool_config_parser()
    #
    #     # Load ip_config.yaml and build unfiltered dependency_graph
    #     bob.instantiate_and_associate_ip_config_parser()
    #     bob.setup_with_ip_config_parser()
    #
    #     # Discover tasks and populate bob.task_configs
    #     bob.discover_tasks()
    #     print(bob.task_configs)
    #
    #     # Set up build dirs for each tasks
    #     bob.setup_build_dirs()
    #
    #     # Create task envs from global env
    #     bob.create_all_task_env()
    #
    #     # Ensure that the dotbob dir exists, and checksum.yaml exists
    #     bob.ensure_dotbob_dir_at_proj_root()
    #
    #     # Instantiate TaskConfigParser to parse all the tasks
    #     bob.instantiate_and_associate_task_config_parser()
    #     # Parse existing task_configs from Bob to TaskConfigParser
    #     bob.task_config_parser.inherit_task_configs(bob.task_configs)
    #     # Parse all tasks with task_config_parser's parse_all_tasks_in_task_configs()
    #     bob.task_config_parser.parse_all_tasks_in_task_configs()
    #
    #     # Execute all tasks
    #     bob.execute_tasks(True, [])
    return 0

    # except Exception as e:
    #     print(f"Error: {e}", file=sys.stderr)
    #     return 1

if __name__ == "__main__":
    sys.exit(main())
