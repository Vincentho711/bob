#ifndef SIMULATION_ARGS_TOKENISER_H
#define SIMULATION_ARGS_TOKENISER_H
#include <stdexcept>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include "simulation_args_argument_descriptor.h"
#include "simulation_args_argument_registry.h"

namespace simulation::args {
struct ArgumentToken {
    std::string key; // argument full name, e.g. "uart0.baud-rate"
    std::optional<std::string> value; // optional value for an argument
    std::string raw; // original argv string for error message
    std::size_t argv_index = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ArgTokeniser — Converts raw argv into a structured Token list.
//
// Supported forms:
//   --key=value       key + "value"
//   --key value       key + "value" (next argv element consumed as value)
//   --flag            boolean flag → key + "true"
//   --no-flag         inverted boolean flag → key + "false"
//   -h / --help       sets result.help_requested = true
//   anything else     collected in result.unknown (SimArgParser will error)
class ArgumentTokeniser {
public:
    explicit ArgumentTokeniser(const ArgumentRegistry& registry) noexcept
        : registry_(registry) {}

    struct Result {
        std::vector<ArgumentToken> tokens;
        std::vector<std::string> unknown; // unregonised argv strings
        bool help_requested = false;
    };

    [[nodiscard]] Result tokenise(std::span<char* const> argv) const;
private:
    const ArgumentRegistry& registry_;
};
}
#endif
