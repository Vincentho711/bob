#ifndef SIMULATION_EXCEPTIONS_H
#define SIMULATION_EXCEPTIONS_H
#include <stdexcept>
#include <sstream>
#include <string>
#include <iostream>
#include <source_location>
#include <format>
#include "simulation_context.h"
#include "simulation_logging_utils.h"

namespace simulation {
    class VerificationError : public std::runtime_error{
    public:
        explicit VerificationError(const std::string &msg, const std::string& component_name = "", uint64_t timestamp_ps = 0, const std::source_location& location = std::source_location::current())
            : std::runtime_error(format_message(msg, component_name, timestamp_ps)), component_name_(component_name), timestamp_ps_(timestamp_ps), location_(location), raw_message_(msg) {}
    const std::string& get_component_name() const { return component_name_; }
    uint64_t get_timestamp() const { return timestamp_ps_; }
    const std::string& get_raw_message() const { return raw_message_; }
    const std::source_location& get_location() const { return location_; }

    virtual std::string get_error_type() const { return "VerificationError"; }

protected:
    static std::string format_message(
        const std::string& message,
        const std::string& component_name,
        uint64_t timestamp_ps) {
        std::ostringstream ss;
        ss << "[" << std::format("{:>10}ps", timestamp_ps) << "]";

        if (!component_name.empty()) {
            ss << " [" << component_name << "]";
        }

        ss << " " << message;
        return ss.str();
    }

    std::string component_name_;
    uint64_t timestamp_ps_;
    std::source_location location_;
    std::string raw_message_;
};

class DataMismatchError : public VerificationError {
public:
    DataMismatchError(
        const std::string& message,
        uint64_t expected,
        uint64_t actual,
        const std::string& component_name = "",
        uint64_t timestamp_ps = 0,
        const std::source_location& location = std::source_location::current()
    ) : VerificationError(
            std::format("{} (expected: 0x{:X}, actual: 0x{:X})",
                       message, expected, actual),
            component_name, timestamp_ps, location),
        expected_(expected),
        actual_(actual) {}

    uint64_t get_expected() const { return expected_; }
    uint64_t get_actual() const { return actual_; }
    std::string get_error_type() const override { return "DataMismatch"; }

private:
    uint64_t expected_;
    uint64_t actual_;
};

// Error reporting functions, integrated with logger
template<typename ErrorType = VerificationError, typename DutType = void>
[[noreturn]] void report_error(
    Logger& logger,
    const std::string& message,
    const std::source_location& location = std::source_location::current()) {
    // Capture current time from context (if available)
    uint64_t timestamp_ps = current_time_ps;

    // Log the error
    logger.error(message);

    // Throw with full context
    throw ErrorType(message, logger.get_component_name(), timestamp_ps, location);
}

/**
 * @brief Report a fatal verification error (logs and throws)
 */
template<typename ErrorType = VerificationError, typename DutType = void>
[[noreturn]] void report_fatal(
    Logger& logger,
    const std::string& message,
    const std::source_location& location = std::source_location::current()) {
    uint64_t timestamp_ps = current_time_ps;

    // Log as fatal
    logger.fatal(message);

    // Throw
    throw VerificationError(message, logger.get_component_name(), timestamp_ps, location);
    // throw ErrorType(message, logger.get_component_name(), timestamp_ps, location);
}

/**
 * @brief Report a data mismatch error
 */
template<typename DutType = void>
[[noreturn]] void report_mismatch(
    Logger& logger,
    const std::string& message,
    uint64_t expected,
    uint64_t actual,
    const std::source_location& location = std::source_location::current()) {
    uint64_t timestamp_ps = current_time_ps;

    // Log with formatted message
    std::string full_msg = std::format("{} (expected: 0x{:X}, actual: 0x{:X})",
                                      message, expected, actual);
    logger.error(full_msg);

    // Throw
    throw DataMismatchError(message, expected, actual,
                           logger.get_component_name(), timestamp_ps, location);
}


}
#endif // SIMULATION_EXCEPTIONS_H
