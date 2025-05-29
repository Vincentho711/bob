#ifndef COMMAND_LINE_PARSER_HPP
#define COMMAND_LINE_PARSER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <variant>

class CommandLineParser {
public:
    struct Argument {
        std::string name;
        std::string help;
        bool required;
        bool takes_value;
        std::optional<std::string> default_value;
    };

    CommandLineParser();

    void add_argument(const std::string& name, const std::string& help = "",
                      bool required = false, bool takes_value = true);

    void set_default_value(const std::string& name, const std::string& default_val);

    void parse(int argc, char** argv);

    std::optional<std::string> get(const std::string& name) const noexcept;
    bool has(const std::string& name) const noexcept;

    void print_help(const std::string& program_name) const;

private:
    std::unordered_map<std::string, Argument> arg_defs;
    std::unordered_map<std::string, std::string> arg_values;
    std::vector<std::string> flags;

    void validate_args() const;
};

#endif // COMMAND_LINE_PARSER_HPP
