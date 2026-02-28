#ifndef SIMULATION_ARGS_ARGUMENT_DESCRIPTOR_H
#define SIMULATION_ARGS_ARGUMENT_DESCRIPTOR_H
#include <string_view>
#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <cstdint>

namespace simulation::args {
enum class ArgumentSource : uint8_t {
    Default,     // No input supplied, value is the regsitered default
    EnvVar,      // Value come from SIM_<PREFIX>_<NAME> environment variable
    CommandLine, //Value come from --flag=value or --flag value on argv
};
[[nodiscard]] constexpr std::string_view source_name(ArgumentSource s) noexcept {
    switch (s) {
        case ArgumentSource::Default: return "default";
        case ArgumentSource::EnvVar:  return "env var";
        case ArgumentSource::CommandLine:  return "command line";
    }
    return "unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// ArgumentDescriptor — Central data structure for a single registered argument.
//
// Created by GroupApp::add_argument / add_flag / add_enum_argument and stored in
// ArgumentRegistry.
//
// The apply and serialise lambdas are the key mechanism:
//   apply:     string_view  → writes typed value to bound member variable
//   serialize: void         → reads bound member variable → returns string
//
// ─────────────────────────────────────────────────────────────────────────────
struct ArgumentDescriptor {
    std::string full_name; // e.g. "uart0.baud-rate"
    std::string cli_flag;  // e.g. "--uart0.baud-rate"
    std::string env_var;   // e.g. "SIM_UART0_BAUD_RATE"
    std::string description;
    std::string group_prefix; // e.g. "uart0" (empty for core group)
    std::string default_string; // string form of default, for --help output
    std::string type_hint; // e.g. "<uint32_t>", "<string>", for --help output

    bool is_flag = false; // true: --flag / --no-flag, no value token consumed
    bool required = false; //true: error if source remains Default after parse

    std::optional<std::vector<std::string>> valid_values; // Used for storing valid string enums
    ArgumentSource source = ArgumentSource::Default;

    // Type-erased setter - converts string_view to T and writes to binding
    std::function<void(std::string_view)> apply;

    // Type-erased getter - reads binding and returns its string representation
    std::function<std::string()> serialise;
};
}
#endif
