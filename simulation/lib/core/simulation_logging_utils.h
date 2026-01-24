#ifndef SIMULATION_LOGGING_UTILS_H
#define SIMULATION_LOGGING_UTILS_H
#include "simulation_context.h"
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <cstdlib>

namespace simulation {
    enum class LogLevel : uint8_t {
        DEBUG   = 0,
        INFO    = 1,
        WARNING = 2,
        ERROR   = 3,
        FATAL   = 4
    };
    // ANSI colours
    namespace colours {
        constexpr const char* RESET = "\033[0m";
        constexpr const char* BOLD  = "\033[1m";
        constexpr const char* DIM = "\033[2m";

        // Foreground colors
        constexpr const char* BLACK = "\033[30m";
        constexpr const char* RED = "\033[31m";
        constexpr const char* GREEN = "\033[32m";
        constexpr const char* YELLOW = "\033[33m";
        constexpr const char* BLUE = "\033[34m";
        constexpr const char* MAGENTA = "\033[35m";
        constexpr const char* CYAN = "\033[36m";
        constexpr const char* WHITE = "\033[37m";

        // Bright foreground colors
        constexpr const char* BRIGHT_BLACK = "\033[90m";
        constexpr const char* BRIGHT_RED = "\033[91m";
        constexpr const char* BRIGHT_GREEN = "\033[92m";
        constexpr const char* BRIGHT_YELLOW = "\033[93m";
        constexpr const char* BRIGHT_BLUE = "\033[94m";
        constexpr const char* BRIGHT_MAGENTA = "\033[95m";
        constexpr const char* BRIGHT_CYAN = "\033[96m";
        constexpr const char* BRIGHT_WHITE = "\033[97m";
    }

    class LoggerConfig {
    public:
        static LoggerConfig& instance() {
            static LoggerConfig config;
            return config;
        }

        // Check if output is to a TTY (terminal)
        bool is_tty() const {
            return is_tty_;
        }

        void set_colour_enabled(bool enabled) {
            colour_override_ = true;
            colour_enabled_ = enabled;
        }

        bool use_colours() const {
            if (colour_override_) {
                return colour_enabled_;
            }
            return is_tty_;
        }

        void set_min_log_level(LogLevel level) {
            min_log_level_ = level;
        }

        LogLevel get_min_log_level() const {
            return min_log_level_;
        }

        void set_show_timestamp(bool show) {
            show_timestamp_ = show;
        }

        bool show_timestamp() const {
            return show_timestamp_;
        }

        void set_timestamp_precision(int precision) {
            timestamp_precision_ = precision;
        }

        int get_timestamp_precision() const {
            return timestamp_precision_;
        }

        void set_strip_ansi_codes(bool strip) {
            strip_ansi_codes_ = strip;
        }

        bool should_strip_ansi_codes() const {
            return !use_colours() | strip_ansi_codes_;
        }
    private:
        LoggerConfig() {
            // Detect if stdout is a TTY
            is_tty_ = isatty(fileno(stdout)) != 0;

            // Check environment variables for colour preference
            const char* no_colour = std::getenv("NO_COLOR");
            const char* force_colour = std::getenv("FORCE_COLOR");

            if (no_colour != nullptr && no_colour[0] != '\0') {
                colour_override_ = true;
                colour_enabled_  = true;
            } else if (force_colour != nullptr && force_colour[0] != '\0') {
                colour_override_ = true;
                colour_enabled_  = true;
            }
        }

        bool is_tty_ = false;
        bool colour_override_ = false;
        bool colour_enabled_ = false;
        LogLevel min_log_level_ = LogLevel::DEBUG;
        bool show_timestamp_ = true;
        int timestamp_precision_ = 0;
        bool strip_ansi_codes_ = false;
    };

    // Strip ANSI escape codes from a string
    inline std::string strip_ansi_codes(const std::string& str) {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i) {
            // Check for ESC character (0x1B or '\033')
            if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') {
                // Found start of ANSI sequence, skip until 'm'
                i += 2; // Skip '\033['
                while (i < str.size() && str[i] != 'm') {
                    ++i;
                }
                // i now points to 'm' or end of string, loop will increment past it
            } else {
                result += str[i];
            }
        }

        return result;
    }

    inline const char* get_level_colour(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return colours::DIM;
            case LogLevel::INFO:    return colours::BRIGHT_CYAN;
            case LogLevel::WARNING: return colours::BRIGHT_YELLOW;
            case LogLevel::ERROR:   return colours::BRIGHT_RED;
            case LogLevel::FATAL:   return std::string(colours::BOLD).append(colours::BRIGHT_RED).c_str();
            default:                return colours::RESET;
        }
    }

    inline std::string_view get_level_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG  ";
            case LogLevel::INFO:    return "INFO   ";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR  ";
            case LogLevel::FATAL:   return "FATAL  ";
            default:                return "UNKNOWN";
        }
    }

    // Format timestamp with configurable precision
    inline std::string format_timestamp(uint64_t time_ps, int precision = 0) {
        if (precision <= 0) {
            return std::to_string(time_ps);
        }

        std::ostringstream oss;
        oss << time_ps;
        if (precision > 0) {
            oss << "." << std::string(precision, '0');
        }
        return oss.str();
    }

    inline void log_message(
        LogLevel level,
        const std::string& component_name,
        const std::string& message,
        uint64_t sim_time_ps = current_time_ps
    ) {
        auto& config = LoggerConfig::instance();

        // Check if this log level should be displayed
        if (level < config.get_min_log_level()) {
            return;
        }

        std::ostringstream log_stream;
        const bool use_colours = config.use_colours();

        // Timestamp
        if (config.show_timestamp()) {
            if (use_colours) {
                log_stream << colours::DIM;
            }
            log_stream << "@" << format_timestamp(sim_time_ps, config.get_timestamp_precision()) << "ps";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        // Log level with colour
        if (use_colours) {
            log_stream << get_level_colour(level);
        }
        log_stream << "[" << get_level_string(level) << "]";
        if (use_colours) {
            log_stream << colours::RESET;
        }
        log_stream << " ";

        // Component name
        if (!component_name.empty()) {
            if (use_colours) {
                log_stream << colours::BRIGHT_BLUE;
            }
            log_stream << "[" << component_name << "]";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        // Message (strip ANSI codes if needed)
        std::string processed_message = message;
        if (config.should_strip_ansi_codes()) {
            processed_message = strip_ansi_codes(message);
        }
        log_stream << processed_message;

        // Output to stdout
        std::cout << log_stream.str() << std::endl;

        // Handle FATAL: terminate simulation
        if (level == LogLevel::FATAL) {
            std::cerr << "\n[FATAL] Simulation terminated due to fatal error.\n";
            std::exit(EXIT_FAILURE);
        }
    }

    inline void log_debug(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::DEBUG, component_name, message);
    }

    inline void log_info(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::INFO, component_name, message);
    }

    inline void log_warning(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::WARNING, component_name, message);
    }

    inline void log_error(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::ERROR, component_name, message);
    }

    inline void log_fatal(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::FATAL, component_name, message);
    }

    inline void log_test_passed(const std::string& component_name, const std::string& message = "Simulation Passed") {
        auto& config = LoggerConfig::instance();
        std::ostringstream log_stream;
        const bool use_colours = config.use_colours();

        std::cout << "\n";

        // Timestamp
        if (config.show_timestamp()) {
            if (use_colours) {
                log_stream << colours::DIM;
            }
            log_stream << "@" << format_timestamp(current_time_ps, config.get_timestamp_precision()) << "ps";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        // Log level
        if (use_colours) {
            log_stream << colours::BRIGHT_CYAN;
        }

        log_stream << "[INFO   ]";
        if (use_colours) {
            log_stream << colours::RESET;
        }

        log_stream << " ";

        // Component name
        if (!component_name.empty()) {
            if (use_colours) {
                log_stream << colours::BRIGHT_BLUE;
            }
            log_stream << "[" << component_name << "]";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        if (use_colours) {
            log_stream << colours::BOLD << colours::BRIGHT_GREEN;
        }
        log_stream << "✓ " << message;
        if (use_colours) {
            log_stream << colours::RESET;
        }

        std::cout << log_stream.str() << std::endl;
    }

    inline void log_test_failed(const std::string& component_name, const std::string& message = "Simulation Failed") {
        auto& config = LoggerConfig::instance();
        std::ostringstream log_stream;
        const bool use_colours = config.use_colours();

        std::cout << "\n";

        // Timestamp
        if (config.show_timestamp()) {
            if (use_colours) {
                log_stream << colours::DIM;
            }
            log_stream << "@" << format_timestamp(current_time_ps, config.get_timestamp_precision()) << "ps";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        // Log level
        if (use_colours) {
            log_stream << colours::BRIGHT_RED;
        }
        log_stream << "[ERROR  ]";
        if (use_colours) {
            log_stream << colours::RESET;
        }
        log_stream << " ";

        // Component name
        if (!component_name.empty()) {
            if (use_colours) {
                log_stream << colours::BRIGHT_BLUE;
            }
            log_stream << "[" << component_name << "]";
            if (use_colours) {
                log_stream << colours::RESET;
            }
            log_stream << " ";
        }

        if (use_colours) {
            log_stream << colours::BOLD << colours::BRIGHT_RED;
        }
        log_stream << "✗ " << message;
        if (use_colours) {
            log_stream << colours::RESET;
        }

        std::cout << log_stream.str() << std::endl;
    }

    class Logger {
    public:
        explicit Logger(std::string component_name) 
            : component_name_(std::move(component_name)) {}

        void debug(const std::string& message) const {
            log_debug(component_name_, message);
        }

        void info(const std::string& message) const {
            log_info(component_name_, message);
        }

        void warning(const std::string& message) const {
            log_warning(component_name_, message);
        }

        void error(const std::string& message) const {
            log_error(component_name_, message);
        }

        void fatal(const std::string& message) const {
            log_fatal(component_name_, message);
        }

        const std::string& get_component_name() const {
            return component_name_;
        }

        void test_passed(const std::string& message = "Test Passed") const {
            log_test_passed(component_name_, message);
        }

        void test_failed(const std::string& message = "Test Failed") const {
            log_test_failed(component_name_, message);
        }

    private:
        std::string component_name_;
    };

}
#endif // SIMULATION_LOGGING_UTILS_H
