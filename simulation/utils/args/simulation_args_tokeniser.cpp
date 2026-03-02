#include "simulation_args_tokeniser.h"

simulation::args::ArgumentTokeniser::Result simulation::args::ArgumentTokeniser::tokenise(std::span<char* const> argv) const {
    simulation::args::ArgumentTokeniser::Result result;
    std::size_t i = 1; // skip argv[0]

    while (i < argv.size()) {
        const std::string_view arg = argv[i];
        // Help shortcut
        if (arg == "--help" || arg == "-h") {
            result.help_requested = true;
            ++i;
            continue;
        }

        // Must start with "--"
        if (!arg.starts_with("--")) {
            result.unknown.emplace_back(arg);
            ++i;
            continue;
        }

        const std::string_view body = arg.substr(2); // strip leading "--"
        // --key=value
        if (const auto eq = body.find('='); eq != std::string_view::npos) {
            ArgumentToken token;
            token.raw = std::string(arg);
            token.argv_index = i;
            token.key = std::string(body.substr(0, eq));
            token.value = std::string(body.substr(eq + 1));
            result.tokens.push_back(std::move(token));
            ++i;
            continue;
        }
        // --no-flag -> key="flag", value = "false"
        if (body.starts_with("no-")) {
            ArgumentToken token;
            token.raw = std::string(arg);
            token.argv_index = i;
            token.key = std::string(body.substr(3));
            token.value = "false";
            result.tokens.push_back(std::move(token));
            ++i;
            continue;
        }

        // --flag or --key value
        const std::string key_str{body};
        const ArgumentDescriptor* desc = registry_.find(key_str);

        if (desc && desc->is_flag) {
            // Register boolean flag - no following value token
            ArgumentToken token;
            token.raw = std::string(arg);
            token.argv_index = i;
            token.key = key_str;
            token.value = "true";
            result.tokens.push_back(token);
            ++i;
        } else if (desc && !desc->is_flag) {
            // Registered value argument - consume next argv element as value
            ++i;
            if (i >= argv.size() ||  std::string_view(argv[i]).starts_with("--")) {
                throw std::invalid_argument(
                    "--" + key_str + ": expected a value but found " +
                    (i >= argv.size() ? "end of arguments" : "another flag") + ".\n"
                    " Use: --" + key_str + "=<value> or --" + key_str + " <value>");
            }
            ArgumentToken token;
            token.raw = std::string(arg);
            token.argv_index = i - 1;
            token.key = key_str;
            token.value = std::string(argv[i]);
            result.tokens.push_back(std::move(token));
            ++i;

        } else {
            // Not in registry
            result.unknown.push_back(std::string(arg));
            ++i;
        }
    }
    return result;
}
