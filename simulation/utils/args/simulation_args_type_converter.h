#ifndef SIMULATION_ARGS_TYPE_CONVERTER_H
#define SIMULATION_ARGS_TYPE_CONVERTER_H

#include <string_view>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ranges>
#include <concepts>
#include <limits>

namespace simulation::args {
class TypeConverter {
public:
    TypeConverter() = delete;

    static bool parse_bool(std::string_view sv, std::string_view opt_name) {
        const auto lower = to_lower(sv);
        if (lower == "true"  || lower == "1" || lower == "yes" || lower == "on")  return true;
        if (lower == "false" || lower == "0" || lower == "no"  || lower == "off") return false;
        throw std::invalid_argument(
            flag(opt_name) + ": cannot convert \"" + std::string(sv) + "\" to bool.\n"
            "  Accepted: true/false, 1/0, yes/no, on/off."
        );
    }

    static int32_t  parse_int32 (std::string_view sv, std::string_view n) { return parse_int<int32_t> (trim(sv), n, "int32");  }
    static uint32_t parse_uint32(std::string_view sv, std::string_view n) { return parse_int<uint32_t>(trim(sv), n, "uint32"); }
    static int64_t  parse_int64 (std::string_view sv, std::string_view n) { return parse_int<int64_t> (trim(sv), n, "int64");  }
    static uint64_t parse_uint64(std::string_view sv, std::string_view n) { return parse_int<uint64_t>(trim(sv), n, "uint64"); }
    static double parse_double(std::string_view sv, std::string_view opt_name) {
        // std::stod is locale-sensitive but from_chars float is not universally
        // available in C++20 toolchains. For simulation args (startup-only,
        // non-hot-path), this is acceptable.
        try {
            std::size_t consumed = 0;
            double result = std::stod(std::string(sv), &consumed);
            if (consumed != sv.size())
                throw std::invalid_argument{"trailing characters"};
            return result;
        } catch (...) {
            throw std::invalid_argument(flag(opt_name) + ": cannot convert \"" +
                std::string(sv) + "\" to double.");
        }
    }

    static std::string parse_string(std::string_view sv, std::string_view /*opt_name*/) {
        return std::string(sv);
    }

    static std::string str(bool v)               { return v ? "true" : "false"; }
    static std::string str(int32_t v)            { return std::to_string(v); }
    static std::string str(uint32_t v)           { return std::to_string(v); }
    static std::string str(int64_t v)            { return std::to_string(v); }
    static std::string str(uint64_t v)           { return std::to_string(v); }
    static std::string str(const std::string& v) { return v; }

private:
    template<std::integral T>
    static T parse_int(std::string_view sv, std::string_view opt_name, std::string_view type_name) {
        if (sv.empty()) {
            throw std::invalid_argument(flag(opt_name) + ": empty value for " + std::string(type_name) + ".");
        }

        try {
            std::size_t consumed = 0;
            if constexpr (std::is_unsigned_v<T>) {
                if (sv.front() == '-')
                    throw std::invalid_argument{"negative value"};
                const unsigned long long val = std::stoull(std::string(sv), &consumed);
                if (consumed != sv.size() || val > std::numeric_limits<T>::max())
                    throw std::invalid_argument{"out of range or trailing characters"};
                return static_cast<T>(val);
            } else {
                const long long val = std::stoll(std::string(sv), &consumed);
                if (consumed != sv.size() ||
                    val < std::numeric_limits<T>::min() ||
                    val > std::numeric_limits<T>::max())
                    throw std::invalid_argument{"out of range or trailing characters"};
                return static_cast<T>(val);
            }
        } catch (...) {
            throw std::invalid_argument(flag(opt_name) + ": cannot convert \"" +
                std::string(sv) + "\" to " + std::string(type_name) + ".");
        }
    }

    // Returns a copy with all characters lowercased
    [[nodiscard]] static std::string to_lower(std::string_view sv) {
        std::string s(sv);
        std::ranges::transform(s, s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Returns a string_view with leading/trailing ASCII whitespace removed.
    [[nodiscard]] static std::string_view trim(std::string_view sv) noexcept {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))  sv.remove_suffix(1);
        return sv;
    }

    [[nodiscard]] static std::string flag(std::string_view name) {
        return "--" + std::string(name);
    }
};
}
#endif
