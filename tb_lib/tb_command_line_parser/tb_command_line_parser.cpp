#include "tb_command_line_parser.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

TbCommandLineParser::TbCommandLineParser() {
    add_argument("--help", "Show this help message", false, false);
}

void TbCommandLineParser::add_argument(const std::string& name, const std::string& help,
                                       bool required, bool takes_value) {
    arg_defs[name] = {name, help, required, takes_value, std::nullopt};
}

void TbCommandLineParser::set_default_value(const std::string& name, const std::string& default_val) {
    auto it = arg_defs.find(name);
    if (it == arg_defs.end()) {
        throw std::runtime_error("Attempt to set default for undefined argument: " + name);
    }
    it->second.default_value = default_val;
}

void TbCommandLineParser::parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];

        if (token == "--help") {
            print_help(argv[0]);
            exit(0);
        }

        if (arg_defs.count(token)) {
            const Argument& arg = arg_defs[token];
            if (arg.takes_value) {
                if (i + 1 >= argc) {
                    throw std::runtime_error("Missing value for argument: " + token);
                }
                arg_values[token] = argv[++i];
            } else {
                flags.push_back(token);
            }
        } else {
            throw std::invalid_argument("Unknown argument: " + token);
        }
    }

    validate_args();
}

void TbCommandLineParser::validate_args() const {
    for (const auto& [name, arg] : arg_defs) {
        bool has_val = arg_values.count(name) > 0 || flags.end() != std::find(flags.begin(), flags.end(), name);
        if (arg.required && !has_val && !arg.default_value.has_value()) {
            throw std::runtime_error("Missing required argument: " + name);
        }
    }
}

std::optional<std::string> TbCommandLineParser::get(const std::string& name) const noexcept {
    if (auto it = arg_values.find(name); it != arg_values.end()) {
        return it->second;
    }
    if (auto it = arg_defs.find(name); it != arg_defs.end() && it->second.default_value.has_value()) {
        return it->second.default_value;
    }
    return std::nullopt;
}

bool TbCommandLineParser::has(const std::string& name) const noexcept {
    return std::find(flags.begin(), flags.end(), name) != flags.end();
}

void TbCommandLineParser::print_help(const std::string& program_name) const {
    std::cout << "Usage: " << program_name << " [options]\n\nOptions:\n";
    for (const auto& [name, arg] : arg_defs) {
        std::cout << "  " << name
                  << (arg.takes_value ? " <value>" : "")
                  << "\n      " << arg.help;

        if (arg.required) {
            std::cout << " (required)";
        } else if (arg.default_value.has_value()) {
            std::cout << " (default: " << arg.default_value.value() << ")";
        }

        std::cout << "\n";
    }
}

