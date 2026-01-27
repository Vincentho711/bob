#ifndef SIMULATION_LOGGING_UTILS_H
#define SIMULATION_LOGGING_UTILS_H
#include "simulation_context.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <system_error>
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

    enum class OutputMode : uint8_t {
        STDOUT_ONLY     = 0,
        FILE_ONLY       = 1,
        BOTH            = 2,
        SEPARATE_LEVELS = 3
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

        LoggerConfig(const LoggerConfig&) = delete;
        LoggerConfig& operator=(const LoggerConfig&) = delete;
        LoggerConfig(LoggerConfig&&) = delete;
        LoggerConfig& operator=(LoggerConfig&&) = delete;

        bool set_log_file(const std::string& filename, OutputMode mode = OutputMode::BOTH, bool append = false) {
            close_log_file();
            auto flags = append ? (std::ios::out | std::ios::app) : std::ios::out;
            log_file_ = std::make_unique<std::ofstream>(filename, flags);

            if (!log_file_->is_open() || !log_file_->good()) {
                throw std::system_error(errno, std::generic_category(), "Failed to open log file: " + filename);
            }

            log_file_path_ = filename;
            output_mode_ = mode;
            file_output_enabled_ = true;
            return true;
        }

        void close_log_file() {
            if (log_file_ && log_file_->is_open()) {
                log_file_->flush();
                log_file_->close();
            }
            log_file_.reset();
            file_output_enabled_ = false;
            log_file_path_.clear();
        }

        void set_output_mode(OutputMode mode) {
            output_mode_ = mode;
        }

        OutputMode get_output_mode() const {
            return output_mode_;
        }

        bool is_file_output_enabled() const {
            return file_output_enabled_ && log_file_ && log_file_->is_open();
        }

        const std::string& get_log_file_path() const {
            return log_file_path_;
        }

        // Set minimum log level for stdout
        void set_stdout_min_level(LogLevel level) {
            stdout_min_level_ = level;
        }

        LogLevel get_stdout_min_level() const {
            return stdout_min_level_;
        }

        // Set minimum log level for file
        void set_file_min_level(LogLevel level) {
            file_min_level_ = level;
        }

        LogLevel get_file_min_level() const {
            return file_min_level_;
        }

        // Set both stdout and file to same level
        void set_min_log_level(LogLevel level) {
            stdout_min_level_ = level;
            file_min_level_ = level;
        }

        // Check if output is to a TTY (terminal)
        bool is_tty() const {
            return is_tty_;
        }

        void set_stdout_colour_enabled(bool enabled) {
            stdout_colour_override_ = true;
            stdout_colour_enabled_ = enabled;
        }

        bool use_stdout_colours() const {
            if (stdout_colour_override_) {
                return stdout_colour_enabled_;
            }
            return is_tty_;
        }

        bool use_file_colours() const {
            return false;
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

        void set_auto_flush(bool enable) {
            auto_flush_ = enable;
        }

        bool get_auto_flush() const {
            return auto_flush_;
        }

        void flush() {
            if (log_file_ && log_file_->is_open()) {
                log_file_->flush();
            }
            std::cout.flush();
        }

        void write_to_stdout(const std::string& message) {
            std::cout << message << std::endl;
            if (auto_flush_) {
                std::cout.flush();
            }
        }

        void write_to_file(const std::string& message) {
            if (log_file_ && log_file_->is_open()) {
                *log_file_ << message << std::endl;
                if (auto_flush_) {
                    log_file_->flush();
                }
            }
        }

    private:
        LoggerConfig() {
            // Detect if stdout is a TTY
            is_tty_ = isatty(fileno(stdout)) != 0;

            // Check environment variables for colour preference
            const char* no_colour = std::getenv("NO_COLOR");
            const char* force_colour = std::getenv("FORCE_COLOR");

            if (no_colour != nullptr && no_colour[0] != '\0') {
                stdout_colour_override_ = true;
                stdout_colour_enabled_ = false;
            } else if (force_colour != nullptr && force_colour[0] != '\0') {
                stdout_colour_override_ = true;
                stdout_colour_override_  = true;
            }
        }
        ~LoggerConfig() {
            close_log_file();
        }

        bool is_tty_ = false;
        bool stdout_colour_override_ = false;
        bool stdout_colour_enabled_ = false;
        std::unique_ptr<std::ofstream> log_file_;
        std::string log_file_path_;
        bool file_output_enabled_ = false;
        OutputMode output_mode_ = OutputMode::STDOUT_ONLY;
        LogLevel stdout_min_level_ = LogLevel::INFO;
        LogLevel file_min_level_ = LogLevel::DEBUG;
        bool show_timestamp_ = true;
        int timestamp_precision_ = 0;
        bool auto_flush_ = true;
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

    inline std::string get_level_colour(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return colours::DIM;
            case LogLevel::INFO:    return colours::BRIGHT_CYAN;
            case LogLevel::WARNING: return colours::BRIGHT_YELLOW;
            case LogLevel::ERROR:   return colours::BRIGHT_RED;
            case LogLevel::FATAL:   return std::string(colours::BOLD) + colours::BRIGHT_RED;
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
        uint64_t sim_time_ps = current_time_ps,
        const uint64_t* txn_id = nullptr,
        const std::vector<std::string>* context_stack = nullptr
    ) {
        auto& config = LoggerConfig::instance();

        // Determine which destinations should receive this message
        const bool send_to_stdout = [&]() {
            switch (config.get_output_mode()) {
                case OutputMode::STDOUT_ONLY:
                    return level >= config.get_stdout_min_level();
                case OutputMode::FILE_ONLY:
                    return false;
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_stdout_min_level();
                default:
                    return false;
            }
        }();

        const bool send_to_file = [&]() {
            if (!config.is_file_output_enabled()) {
                return false;
            }
            switch (config.get_output_mode()) {
                case OutputMode::STDOUT_ONLY:
                    return false;
                case OutputMode::FILE_ONLY:
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_file_min_level();
                default:
                    return false;
            }
        }();

        if (!send_to_file && !send_to_stdout) {
            return;
        }

        // Build the log message, will be formatted differently for stdout vs file
        auto build_message = [&](bool use_colours) -> std::string {
            std::ostringstream log_stream;

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
                // Add BOLD for FATAL
                if (level == LogLevel::FATAL) {
                    log_stream << colours::BOLD;
                }
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

            // Hierarchical context
            if (context_stack != nullptr && !context_stack->empty()) {
                if (use_colours) {
                    log_stream << colours::BRIGHT_MAGENTA;
                }
                log_stream << "[";
                for (size_t i = 0; i < context_stack->size(); ++i) {
                    if (i > 0) {
                        log_stream << "/";
                    }
                    log_stream << (*context_stack)[i];
                }
                log_stream << "]";
                if (use_colours) {
                    log_stream << colours::RESET;
                }
                log_stream << " ";
            }

            // Transaction ID
            if (txn_id != nullptr) {
                if (use_colours) {
                    log_stream << colours::BRIGHT_YELLOW;
                }
                log_stream << "[TXN:" << *txn_id << "]";
                if (use_colours) {
                    log_stream << colours::RESET;
                }
                log_stream << " ";
            }

            // Message (strip ANSI codes if no colours)
            if (use_colours) {
                log_stream << message;
            } else {
                log_stream << strip_ansi_codes(message);
            }

            return log_stream.str();
        };

        // Output to stdout (with colors if TTY)
        if (send_to_stdout) {
            const bool use_colours = config.use_stdout_colours();
            std::string formatted_msg = build_message(use_colours);
            config.write_to_stdout(formatted_msg);
        }

        // Output to file (always without colors)
        if (send_to_file) {
            std::string formatted_msg = build_message(false);  // No colors for file
            config.write_to_file(formatted_msg);
        }

        if (level == LogLevel::FATAL) {
            config.flush(); // Ensure everything is written before error
            throw std::runtime_error("Simulation terminated due to fatal error.\n");
        }

    }

    inline void log_debug(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::DEBUG, component_name, message, current_time_ps, nullptr, nullptr);
    }

    inline void log_info(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::INFO, component_name, message, current_time_ps, nullptr, nullptr);
    }

    inline void log_warning(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::WARNING, component_name, message, current_time_ps, nullptr, nullptr);
    }

    inline void log_error(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::ERROR, component_name, message, current_time_ps, nullptr, nullptr);
    }

    inline void log_fatal(const std::string& component_name, const std::string& message) {
        log_message(LogLevel::FATAL, component_name, message, current_time_ps, nullptr, nullptr);
    }

    inline void log_test_passed(const std::string& component_name, const std::string& message = "Simulation Passed") {
        auto& config = LoggerConfig::instance();

        // Determine destinations
        const LogLevel level = LogLevel::INFO;
        const bool send_to_stdout = [&]() {
            switch (config.get_output_mode()) {
                case OutputMode::STDOUT_ONLY:
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_stdout_min_level();
                default:
                    return false;
            }
        }();

        const bool send_to_file = [&]() {
            if (!config.is_file_output_enabled()) {
                return false;
            }
            switch (config.get_output_mode()) {
                case OutputMode::FILE_ONLY:
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_file_min_level();
                default:
                    return false;
            }
        }();

        if (!send_to_stdout && !send_to_file) {
            return;
        }

        auto build_message = [&](bool use_colours) -> std::string {
            std::ostringstream log_stream;

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

            // Success message with colour
            if (use_colours) {
                log_stream << colours::BOLD << colours::BRIGHT_GREEN;
            }
            log_stream << "✓ " << message;
            if (use_colours) {
                log_stream << colours::RESET;
            }

            return log_stream.str();
        };

        // Add visual separator and output
        if (send_to_stdout) {
            config.write_to_stdout("");
            config.write_to_stdout(build_message(config.use_stdout_colours()));
        }

        if (send_to_file) {
            config.write_to_file("");
            config.write_to_file(build_message(false));
        }
    }

    inline void log_test_failed(const std::string& component_name, const std::string& message = "Simulation Failed") {
        auto& config = LoggerConfig::instance();
        // Determine destinations
        const LogLevel level = LogLevel::ERROR;
        const bool send_to_stdout = [&]() {
            switch (config.get_output_mode()) {
                case OutputMode::STDOUT_ONLY:
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_stdout_min_level();
                default:
                    return false;
            }
        }();

        const bool send_to_file = [&]() {
            if (!config.is_file_output_enabled()) {
                return false;
            }
            switch (config.get_output_mode()) {
                case OutputMode::FILE_ONLY:
                case OutputMode::BOTH:
                case OutputMode::SEPARATE_LEVELS:
                    return level >= config.get_file_min_level();
                default:
                    return false;
            }
        }();

        if (!send_to_stdout && !send_to_file) {
            return;
        }

        auto build_message = [&](bool use_colours) -> std::string {
            std::ostringstream log_stream;

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

            // Failure message with colour
            if (use_colours) {
                log_stream << colours::BOLD << colours::BRIGHT_RED;
            }
            log_stream << "✗ " << message;
            if (use_colours) {
                log_stream << colours::RESET;
            }

            return log_stream.str();
        };

        // Add visual separator and output
        if (send_to_stdout) {
            config.write_to_stdout("");
            config.write_to_stdout(build_message(config.use_stdout_colours()));
        }

        if (send_to_file) {
            config.write_to_file("");
            config.write_to_file(build_message(false));
        }
    }

    class Logger {
    public:
        explicit Logger(std::string component_name) 
            : component_name_(std::move(component_name)) {}

        void debug(const std::string& message) const {
            log_message(LogLevel::DEBUG, component_name_, message, current_time_ps,
                        nullptr, &context_stack_);
        }

        void info(const std::string& message) const {
            log_message(LogLevel::INFO, component_name_, message, current_time_ps,
                        nullptr, &context_stack_);
        }

        void warning(const std::string& message) const {
            log_message(LogLevel::WARNING, component_name_, message, current_time_ps,
                        nullptr, &context_stack_);
        }

        void error(const std::string& message) const {
            log_message(LogLevel::ERROR, component_name_, message, current_time_ps,
                        nullptr, &context_stack_);
        }

        void fatal(const std::string& message) const {
            log_message(LogLevel::FATAL, component_name_, message, current_time_ps,
                        nullptr, &context_stack_);
        }

        void debug_txn(uint64_t txn_id, const std::string& message) const {
            log_message(LogLevel::DEBUG, component_name_, message, current_time_ps,
                       &txn_id, &context_stack_);
        }

        void info_txn(uint64_t txn_id, const std::string& message) const {
            log_message(LogLevel::INFO, component_name_, message, current_time_ps,
                       &txn_id, &context_stack_);
        }

        void warning_txn(uint64_t txn_id, const std::string& message) const {
            log_message(LogLevel::WARNING, component_name_, message, current_time_ps,
                       &txn_id, &context_stack_);
        }

        void error_txn(uint64_t txn_id, const std::string& message) const {
            log_message(LogLevel::ERROR, component_name_, message, current_time_ps,
                       &txn_id, &context_stack_);
        }

        void fatal_txn(uint64_t txn_id, const std::string& message) const {
            log_message(LogLevel::FATAL, component_name_, message, current_time_ps,
                       &txn_id, &context_stack_);
        }

        void push_context(const std::string& context) {
            context_stack_.push_back(context);
        }

        void pop_context() {
            if (!context_stack_.empty()) {
                context_stack_.pop_back();
            }
        }

        std::string get_full_context() const {
            if (context_stack_.empty()) {
                return "";
            }

            std::ostringstream oss;
            for (size_t i = 0; i < context_stack_.size(); ++i) {
                if (i > 0) {
                    oss << "/";
                }
                oss << context_stack_[i];
            }
            return oss.str();
        }

        class ScopedContext {
        public:
            ScopedContext(Logger& logger, std::string context)
                : logger_(logger), context_(std::move(context)) {
                logger_.push_context(context_);
            }

            ~ScopedContext() {
                logger_.pop_context();
            }

            // Prevent copying and moving
            ScopedContext(const ScopedContext&) = delete;
            ScopedContext& operator=(const ScopedContext&) = delete;
            ScopedContext(ScopedContext&&) = delete;
            ScopedContext& operator=(ScopedContext&&) = delete;

            const std::string& get_context() const {
                return context_;
            }

        private:
            Logger& logger_;
            std::string context_;
        };

        [[nodiscard]] ScopedContext scoped_context(std::string context) {
            return ScopedContext(*this, std::move(context));
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
        mutable std::vector<std::string> context_stack_;
    };

}
#endif // SIMULATION_LOGGING_UTILS_H
