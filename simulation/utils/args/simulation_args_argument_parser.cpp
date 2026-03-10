#include "simulation_args_argument_parser.h"

#include <cstdlib>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>

namespace simulation::args {

SimulationArgumentParser::SimulationArgumentParser(std::string program_name,
                                                   std::string program_description)
    : program_name_(std::move(program_name))
    , program_description_(std::move(program_description)) {}

void SimulationArgumentParser::parse(int argc, char** argv) {
    const ArgumentTokeniser tokeniser{registry_};
    const auto result = tokeniser.tokenise(
        std::span<char* const>(argv, static_cast<std::size_t>(argc)));

    if (result.help_requested) {
        HelpFormatter::print_help(program_name_, program_description_, registry_);
        std::exit(0);
    }

    if (!result.unknown.empty()) {
        std::ostringstream msg;
        msg << "Unknown argument(s):";
        for (const auto& u : result.unknown) {
            msg << "\n  " << u;
        }
        msg << "\nRun with --help for usage.";
        throw std::invalid_argument(msg.str());
    }

    // Precedence: default < env var < command line
    apply_env_vars();
    apply_tokens(result);
}

// ── resolve() ────────────────────────────────────────────────────────────────

void SimulationArgumentParser::resolve() {
    for (auto& group : groups_) {
        group->post_parse_resolve();
    }
    for (auto& group : groups_) {
        group->post_parse_validate();
    }
    for (auto& group : groups_) {
        group->cross_validate(*this);
    }

    // dry-run: checked after all hooks so derived values (e.g. randomised seed)
    // are fully resolved and shown correctly in the output.
    if (const ArgumentDescriptor* d = registry_.find("dry-run");
        d != nullptr && d->serialise() == "true") {
        HelpFormatter::print_resolved(registry_);
        std::exit(0);
    }
}

void SimulationArgumentParser::apply_env_vars() {
    for (auto& desc : registry_.all()) {
        if (desc.source != ArgumentSource::Default) continue;
        const char* val = std::getenv(desc.env_var.c_str());
        if (val == nullptr) continue;
        desc.apply(val);
        desc.source = ArgumentSource::EnvVar;
    }
}

void SimulationArgumentParser::apply_tokens(const ArgumentTokeniser::Result& result) {
    for (const auto& token : result.tokens) {
        ArgumentDescriptor* desc = registry_.find(token.key);
        if (desc == nullptr) {
            throw std::invalid_argument("Unknown argument: --" + token.key);
        }
        if (!token.value.has_value()) {
            throw std::invalid_argument("--" + token.key + ": missing value.");
        }
        desc->apply(*token.value);
        desc->source = ArgumentSource::CommandLine;
    }

    // Validate required arguments after all tokens are applied.
    for (const auto& desc : registry_.all()) {
        if (desc.required && desc.source == ArgumentSource::Default) {
            throw std::invalid_argument(
                "Required argument --" + desc.full_name + " was not provided.");
        }
    }
}

}
