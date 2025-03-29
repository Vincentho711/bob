from pathlib import Path
import logging
import yaml
import re
import os
import sys
import networkx as nx

class IpConfigParser:
    def __init__(self, logger: logging.Logger, proj_root: str) -> None:
        self.ip_configs = {}
        self.logger = logger
        self.proj_root = proj_root
        self.dependency_graph = None

    def load_ip_cfg(self) -> None:
        """Load the ip_config.yaml into an internal dict"""
        try:
            ip_cfg_path: Path = Path(self.proj_root) / "ip_config.yaml"
            if not ip_cfg_path.exists():
                raise FileNotFoundError(f"No ip_config.yaml is found under {self.proj_root}. Since {ip_cfg_path} is not found, parsing skipped.")
            with open(ip_cfg_path, "r") as yaml_file:
                self.ip_config = yaml.safe_load(yaml_file)
            self.logger.info(f"Parsing {ip_cfg_path}")

        except (FileNotFoundError, ValueError) as e:
            self.logger.error(f"Error: {e}")
            sys.exit(1)
        except Exception as e:
            self.logger.critical(f"Unexpected error during load_ip_cfg(): {e}", exc_info=True)

    def parse_ip_cfg(self) -> dict[str, str] | None:
        """Parse and resolve placeholders in the entire ip_config dict"""
        try:
            self.ip_config = self._resolve_value(self.ip_config)
            self.logger.debug(f"self.ip_config = {self.ip_config}")
            return self.ip_config
        except Exception as e:
            self.logger.critical(f"Unexpected error during parse_ip_cfg(): {e}", exc_info=True)
            return None

    def _resolve_value(self, value: str|list|dict, resolved_cache=None) -> str| list| dict[str, str]:
        """
        Resolve the placeholders in the values (e.g., ${directories.root_dir})
        while ensuring correct path joining where applicable.
        """
        try:
            if resolved_cache is None:
                resolved_cache = {}
            if isinstance(value, str):
                if value in resolved_cache:
                    return resolved_cache[value] # Prevent duplicate resolution
                resolved_value = self._resolve_placeholder(value, resolved_cache)
                # if it is at the bottom of the hierarchy and all the elements in a list have been resolved, just return that list
                if (isinstance(resolved_value, list)):
                    return resolved_value
                # Don't correct path format if it is a string and there is a leading '/' which indicates abs path
                if resolved_value.startswith('/'):
                    return resolved_value
                else:
                    resolved_cache[value] = self._ensure_correct_path_format(value, resolved_value)
                    return resolved_cache[value]
            elif isinstance(value, list):
                return [self._resolve_value(item, resolved_cache) for item in value]
            elif isinstance(value, dict):
                return {key: self._resolve_value(val, resolved_cache) for key, val in value.items()}
            return value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_value(): {e}", exc_info=True)
            sys.exit(1)

    def _resolve_placeholder(self, value: str, resolved_cache: dict[str, str]) -> str|list|dict:
        """
        Resolve placeholders, supporting both environment variables and hierarchical variables.
        Environment variables take precedence over hierarchical values.
        Resolve a placeholder like ${directories.root_dir} , or ${PROJ_ROOT} to their actual values.
        Uses caching to prevent duplicate resolution and avoids circular references.
        """
        # Regular expression to match ${<hierarchy>}
        try:
            placeholder_pattern = r"\$\{([^}]+)\}"
            matches = re.findall(placeholder_pattern, value)

            for match in matches:
                # Check if it is a environment variable first
                if match in os.environ:
                    resolved_value = os.environ[match]
                else:
                    resolved_value = self._get_value_from_hierarchy(match, resolved_cache)
                if resolved_value is not None:
                    if isinstance(resolved_value, list):
                        # If it's a list, resolve each element separately
                        resolved_value = [self._resolve_placeholder(str(item), resolved_cache) for item in resolved_value]
                        return resolved_value # Return the fully resolved list
                    elif not isinstance(resolved_value, str):
                        resolved_value = str(resolved_value)
                    value = value.replace(f"${{{match}}}", resolved_value)
                else:
                    self.logger.error(f"Warning: No matching value found for placeholder {match}")

            return value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _resolve_placeholder(): {e}", exc_info=True)
            sys.exit(1)

    def _get_value_from_hierarchy(self, hierarchy: str, resolved_cache: dict[str, str]) -> str | None:
        """Fetch the value from the hierarchy (e.g., directories.root_dir)"""
        try:
            if hierarchy in resolved_cache:
                return resolved_cache[hierarchy]
            keys = hierarchy.split('.')
            value = self.ip_config

            for key in keys:
                if key in value:
                    value = value[key]
                else:
                    self.logger.error(f"No matching value found for hierarchy {hierarchy}")
                    return None
            resolved_value = self._resolve_value(value, resolved_cache)
            resolved_cache[hierarchy] = resolved_value
            self.logger.debug(f"hierarchy: {hierarchy}, value: {resolved_value}")
            return resolved_value
        except Exception as e:
            self.logger.critical(f"Unexpected error during _get_value_from_hierarchy(): {e}", exc_info=True)
            sys.exit(1)

    def _ensure_correct_path_format(self, original: str, resolved: str) -> str:
        """
        Ensure correct path joining when a placeholder is part of a file/directory path.
        This prevents incorrect constructs like `.//src`.
        """
        try:
            if "/" in original or "\\" in original:  # Check if the value is path-like
                parts = resolved.split("/")
                return str(Path(*parts))  # Use pathlib to reconstruct the path correctly
            return resolved
        except Exception as e:
            self.logger.critical(f"Unexpected error during _ensure_correct_path_format(): {e}", exc_info=True)

    def build_task_dependency_graph(self) -> nx.DiGraph | None:
        """Build task dependency graph based on ip_config.yaml"""
        try:
            graph = nx.DiGraph()

            # Get the 'tasks' key within ip_config.yaml
            tasks_dict = self.ip_config.get("tasks", None)
            if tasks_dict is None:
                self.logger.error(f"Mandatory key 'tasks' does not exists within ip_config.yaml. Please ensure it exists.")
                return
            print(f"tasks_dict={tasks_dict}")

            # Build task dependency graph
            for task, config in tasks_dict.items():
                print(f"task={task}, config={config}")
                graph.add_node(task)
                for dep in config.get("depends_on", []):
                    print(f"{dep} -> {task}")
                    self.logger.debug(f"{dep} -> {task}")
                    graph.add_edge(dep, task)

            # Detect cyclic dependencies
            if not nx.is_directed_acyclic_graph(graph):
                self.logger.error("Cyclic dependency detected in task configuration.")
                raise ValueError("Cyclic dependency detected.")

            self.dependency_graph = graph
            return graph

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None

        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_execution_chains(): {e}", exc_info=True)
            return None

    def build_execution_chains(self):
        """Build execution chains with dependency_graph, ensuring tasks are unique across chains."""
        try:
            dependency_graph = self.dependency_graph
            if dependency_graph is None:
                raise ValueError(f"Attribute 'dependency_graph' = None. Please execute build_task_dependency_graph() first.")

            execution_chains = []
            assigned_tasks = set()

            # Identify independent execution chains
            for component in nx.weakly_connected_components(dependency_graph):
                print(f"component={component}")
                dependency_subgraph = dependency_graph.subgraph(component)
                print(f"dependency_subgraph={dependency_subgraph}")
                execution_chain = list(nx.topological_sort(dependency_subgraph))
                print(f"execution_chain={execution_chain}")

                # Validate that tasks do not appear in multiple chains
                for task in execution_chain:
                    print(f"task={task}")
                    if task in assigned_tasks:
                        print(f"Task '{task}' in assigned_tasks.")
                        raise ValueError(f"Task '{task}' appears in multiple execution chains. This is not allowed.")
                    print(f"assigned_tasks.add({task})")
                    assigned_tasks.add(task)

                execution_chains.append(execution_chain)
                print(f"execution_chains={execution_chains}")

            self.execution_chains = execution_chains 
            print(f"self.execution_chains= {execution_chains}")
            return execution_chains

        except ValueError as ve:
            self.logger.error(f"ValueError: {ve}")
            return None
        except Exception as e:
            self.logger.critical(f"Unexpected error during resolve_execution_chains(): {e}", exc_info=True)
            return None


