#include "simulation_args_help_formatter.h"
#include <iomanip>

void simulation::args::HelpFormatter::print_help(std::string_view program_name, std::string_view program_description, const ArgumentRegistry &registry, std::ostream& out) {
    out << "\nUsage: " << program_name << "[options]\n";
    if (!program_description.empty()) {
        out << "\n  " << program_description << "\n";
    }
    std::string current_prefix = "\x01"; // sentinel — never matches a real prefix

    for (const auto& d : registry.all()) {
        // Print section header when the group changes
        if (d.group_prefix != current_prefix) {
            current_prefix = d.group_prefix;
            out << "\n";
            if (current_prefix.empty()) {
                out << "Core options:\n";
            } else {
                out << current_prefix << " options (--" << current_prefix << ".*):\n";
            }
        }
        // Flag + type column
        std::string flag_col = "  " + d.cli_flag;
        if (!d.is_flag && !d.type_hint.empty()) {
            flag_col += " " + d.type_hint;
        }

        constexpr int kFlagWidth = 38;
        if (static_cast<int>(flag_col.size()) < kFlagWidth) {
            flag_col.append(static_cast<std::size_t>(kFlagWidth) - flag_col.size(), ' ');
        } else {
            flag_col += "\n" + std::string(kFlagWidth, ' ');
        }
        out << flag_col << d.description;

        if (!d.default_string.empty()) {
            out << " [default: " << d.default_string << "]";
        }
        if (d.required) {
            out << " [REQUIRED]";
        }
        out << "\n";

        // Valid values for enum options
        if (d.valid_values.has_value() && !d.valid_values->empty()) {
            out << std::string(kFlagWidth, ' ') << "Valid: ";
            const auto& vals = *d.valid_values;
            for (std::size_t k = 0; k < vals.size(); ++k) {
                if (k) out << ", ";
                out << vals[k];
            }
            out << "\n";
        }

        out << "\nEnvironment variable overrides:\n"
            << "  Every option exposes a SIM_<PREFIX>_<n> env var equivalent.\n"
            << "  Example: SIM_UART0_BAUD_RATE=115200\n"
            << "  Precedence: default < env var < command line\n\n";
    }
}

void simulation::args::HelpFormatter::print_resolved(const ArgumentRegistry& registry, std::ostream& out) {
    out << "\n[DRY-RUN] Resolved argument values:\n\n";

    std::string current_prefix = "\x01";

    for (const auto& d : registry.all()) {
        if (d.group_prefix != current_prefix) {
            current_prefix = d.group_prefix;
            out << (current_prefix.empty() ? "  [core]\n" : "  [" + current_prefix + "]\n");
        }

        constexpr int kKeyWidth = 36;
        const std::string key = "   --" + d.full_name;
        out << key;
        if (static_cast<int>(key.size() < kKeyWidth)) {
            out << std::string(static_cast<std::size_t>(kKeyWidth) - key.size(), ' ');
        } else {
            out << "\n" << std::string(kKeyWidth, ' ');
        }

        out << "= " << std::left << std::setw(20) << d.serialise()
             << "  [" << source_name(d.source) << "]\n";
    }
    out << "\n";
}
