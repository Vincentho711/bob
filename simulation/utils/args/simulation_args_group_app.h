#ifndef SIMULATION_ARGS_GROUP_APP_H
#define SIMULATION_ARGS_GROUP_APP_H
#include "simulation_args_argument_descriptor.h"
#include "simulation_args_argument_registry.h"
#include "simulation_args_type_converter.h"
#include <cctype>
#include <complex>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <ranges>
#include <string>

namespace simulation::args {

template <typename T>
concept BindableArgumentType =
    std::same_as<T, bool>         ||
    std::same_as<T, int32_t>      ||
    std::same_as<T, uint32_t>     ||
    std::same_as<T, int64_t>      ||
    std::same_as<T, uint64_t>     ||
    std::same_as<T, double>       ||
    std::same_as<T, std::string>;

class GroupApp {
public:
    GroupApp(std::string_view prefix, ArgumentRegistry& registry) noexcept
        : prefix_(prefix), registry_(registry) {}

    GroupApp(const GroupApp&)      = delete;
    GroupApp& operator=(const GroupApp&) = delete;
    GroupApp(GroupApp&&) = delete;
    GroupApp operator=(GroupApp&&) = delete;

    template<BindableArgumentType T>
    GroupApp& add_argument(
        std::string_view name,
        T& binding,
        std::string_view description,
        T default_value = T{}) {
        binding = default_value;

        ArgumentDescriptor arg_desc;
        arg_desc.full_name = make_full_name(name);
        arg_desc.cli_flag = "--" + arg_desc.full_name;
        arg_desc.env_var = make_env_var(name);
        arg_desc.description = std::string(description);
        arg_desc.group_prefix = std::string(prefix_);
        arg_desc.default_string = simulation::args::TypeConverter::str(default_value);
        arg_desc.type_hint = type_hint_for<T>();
        arg_desc.is_flag = false;
        arg_desc.required = false;
        arg_desc.source = ArgumentSource::Default;

        // Capture raw pointer of the default value string
        T* ptr = std::addressof(binding);
        arg_desc.apply = [ptr, fn = arg_desc.full_name](std::string_view sv) {
            *ptr = parse_as<T>(sv, fn);
        };
        arg_desc.serialise = [ptr]() -> std::string {
            return TypeConverter::str(*ptr);
        };

        registry_.register_argument(std::move(arg_desc));
        return *this;
    }

    GroupApp& add_flag(std::string_view name, bool& binding, std::string_view description, bool default_value = false) {
        binding = default_value;

        ArgumentDescriptor arg_desc;
        arg_desc.full_name = make_full_name(name);
        arg_desc.cli_flag = "--" + arg_desc.full_name;
        arg_desc.env_var = make_env_var(name);
        arg_desc.description = std::string(description);
        arg_desc.group_prefix = std::string(prefix_);
        arg_desc.default_string = TypeConverter::str(default_value);
        arg_desc.type_hint = {}; // flags have no value in --help
        arg_desc.is_flag = true;
        arg_desc.required = false;
        arg_desc.source = ArgumentSource::Default;

        bool* ptr = std::addressof(binding);
        arg_desc.apply = [ptr, fn = arg_desc.full_name](std::string_view sv) {
            *ptr = TypeConverter::parse_bool(sv, fn);
        };
        arg_desc.serialise = [ptr]() -> std::string {
            return TypeConverter::str(*ptr);
        };
        registry_.register_argument(std::move(arg_desc));
        return *this;
    }

    GroupApp& add_enum_argument(std::string_view name, std::string& binding, std::string_view description, std::vector<std::string> valid_values, std::string default_value = {}) {
        // Validate that the default value is one of valid_values
        if (!default_value.empty()) {
            if (std::ranges::find(valid_values, default_value) == valid_values.end()) {
                throw std::invalid_argument("Default value \"" + default_value + "\" for option \"--" + make_full_name(name) + "\" is not in valid_values.");
            }
        }
        binding = default_value;
        ArgumentDescriptor arg_desc;
        arg_desc.full_name = make_full_name(name);
        arg_desc.cli_flag = "--" + arg_desc.full_name;
        arg_desc.env_var = make_env_var(name);
        arg_desc.description = std::string(description);
        arg_desc.group_prefix = std::string(prefix_);
        arg_desc.default_string = default_value;
        arg_desc.type_hint = "<string>";
        arg_desc.is_flag = false;
        arg_desc.required = false;
        arg_desc.valid_values = valid_values;
        arg_desc.source = ArgumentSource::Default;

        std::string* ptr = std::addressof(binding);

        arg_desc.apply = [ptr, fn = arg_desc.full_name, vv = std::move(valid_values)](std::string_view sv) {
            std::string val{sv};
            if (std::ranges::find(vv, val) == vv.end()) {
                std::string msg = "--" + fn + ": \"" + val + "\" is not valid.\n Valid: ";
                for (std::size_t k = 0; k < vv.size(); ++k) {
                    if (k) msg += ", ";
                    msg += vv[k];
                }
                throw std::invalid_argument(msg + ".");
            }
            *ptr = std::move(val);
        };

        arg_desc.serialise = [ptr]() -> std::string { return *ptr; };
        registry_.register_argument(std::move(arg_desc));
    }

private:

    [[nodiscard]] std::string make_full_name(std::string_view name) const {
        return prefix_.empty() ? std::string(name) : std::string(prefix_) + "." + std::string(name);
    }

    // "uart0" + "baud-rate" -> "SIM_UART0_BAUD_RATE"
    // "" + "seed" -> "SIM_SEED"
    [[nodiscard]] std::string make_env_var(std::string_view name) const {
        std::string result = "SIM_";
        auto append_upper = [&](std::string_view sv) {
            for (const unsigned char c : sv) {
                result += (c == '-' || c == '.') ? '_' : static_cast<char>(std::toupper(c));
            }
        };
        if (!prefix_.empty()) {
            append_upper(prefix_);
            result += '_';
        }
        append_upper(name);
        return result;
    }

    template<BindableArgumentType T>
    [[nodiscard]] static std::string type_hint_for() {
        if constexpr (std::is_same_v<T, bool>) return "";
        else if constexpr (std::is_same_v<T, int32_t>) return "<int32_t>";
        else if constexpr (std::is_same_v<T, uint32_t>) return "<uint32_t>";
        else if constexpr (std::is_same_v<T, int64_t>) return "<int64_t>";
        else if constexpr (std::is_same_v<T, uint64_t>) return "<uint64_t>";
        else if constexpr (std::is_same_v<T, double>) return "<double>";
        else if constexpr (std::is_same_v<T, std::string>) return "<string>";
        return "<value>";
    }

    template<BindableArgumentType T>
    [[nodiscard]] static T parse_as(std::string_view sv, const std::string& name) {
        if constexpr (std::is_same_v<T, bool>) return TypeConverter::parse_bool(sv, name);
        else if constexpr (std::is_same_v<T, int32_t>) return TypeConverter::parse_int32(sv, name);
        else if constexpr (std::is_same_v<T, uint32_t>) return TypeConverter::parse_uint32(sv, name);
        else if constexpr (std::is_same_v<T, int64_t>) return TypeConverter::parse_int64(sv, name);
        else if constexpr (std::is_same_v<T, uint64_t>) return TypeConverter::parse_uint64(sv, name);
        else if constexpr (std::is_same_v<T, double>) return TypeConverter::parse_double(sv, name);
        else if constexpr (std::is_same_v<T, std::string>) return TypeConverter::parse_string(sv, name);
    }

    std::string_view prefix_;
    ArgumentRegistry& registry_;
};
}
#endif // SIMULATION_ARGS_GROUP_APP_H
