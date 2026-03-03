#ifndef SIMULATION_ARGS_HELP_FORMATTER_H
#define SIMULATION_ARGS_HELP_FORMATTER_H
#include <iostream>
#include "simulation_args_argument_registry.h"
namespace simulation::args{
// ─────────────────────────────────────────────────────────────────────────────
// HelpFormatter — Formats --help and --dry-run output from OptionRegistry.
//
// Groups options by group_prefix, printing them in registration order.
// ─────────────────────────────────────────────────────────────────────────────
class HelpFormatter {
public:
    HelpFormatter() = delete;

    static void print_help(std::string_view program_name, std::string_view program_description, const ArgumentRegistry& registry, std::ostream& out = std::cout);
};
}
#endif
