#include "checker.h"
#include <iostream>
#include <iomanip>

// This file contains explicit template instantiations for common checker types
// and any non-template utility functions if needed

// Note: Since BaseChecker is a template class, most of its implementation
// is in the header file. This file serves as a placeholder for:
// 1. Explicit template instantiations (if needed for compilation speed)
// 2. Non-template utility functions
// 3. Static member definitions

// Example explicit instantiation (uncomment if needed):
// #include "Vhello_world_top.h"
// #include "adder_verification/adder_transaction.h"
// template class BaseChecker<Vhello_world_top, AdderTransaction>;

// Utility functions that don't depend on template parameters

/**
 * @brief Convert CheckerLogLevel enum to string
 * 
 * @param level The log level to convert
 * @return String representation of the log level
 */
std::string checker_log_level_to_string(CheckerLogLevel level) {
    switch (level) {
        case CheckerLogLevel::DEBUG:   return "DEBUG";
        case CheckerLogLevel::INFO:    return "INFO";
        case CheckerLogLevel::WARNING: return "WARNING";
        case CheckerLogLevel::ERROR:   return "ERROR";
        case CheckerLogLevel::FATAL:   return "FATAL";
        default:                       return "UNKNOWN";
    }
}

/**
 * @brief Convert string to CheckerLogLevel enum
 * 
 * @param level_str String representation of log level
 * @return CheckerLogLevel enum value
 */
CheckerLogLevel string_to_checker_log_level(const std::string& level_str) {
    if (level_str == "DEBUG") return CheckerLogLevel::DEBUG;
    if (level_str == "INFO") return CheckerLogLevel::INFO;
    if (level_str == "WARNING") return CheckerLogLevel::WARNING;
    if (level_str == "ERROR") return CheckerLogLevel::ERROR;
    if (level_str == "FATAL") return CheckerLogLevel::FATAL;
    return CheckerLogLevel::INFO; // Default
}

/**
 * @brief Print checker configuration in a formatted way
 * 
 * @param config The configuration to print
 * @param os Output stream to print to
 */
void print_checker_config(const CheckerConfig& config, std::ostream& os) {
    os << "Checker Configuration:" << std::endl;
    os << "  Detailed Logging: " << (config.enable_detailed_logging ? "Enabled" : "Disabled") << std::endl;
    os << "  Stop on First Error: " << (config.stop_on_first_error ? "Enabled" : "Disabled") << std::endl;
    os << "  Timeout Checking: " << (config.enable_timeout_checking ? "Enabled" : "Disabled") << std::endl;
    os << "  Timeout Cycles: " << config.timeout_cycles << std::endl;
    os << "  Log Level: " << checker_log_level_to_string(config.log_level) << std::endl;
    os << "  Statistics: " << (config.enable_statistics ? "Enabled" : "Disabled") << std::endl;
}

/**
 * @brief Print checker statistics in a formatted way
 * 
 * @param stats The statistics to print
 * @param os Output stream to print to
 */
void print_checker_stats(const CheckerStats& stats, std::ostream& os) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        stats.last_activity - stats.start_time);
    
    double pass_rate = stats.transactions_checked > 0 ? 
                      static_cast<double>(stats.checks_passed) / stats.transactions_checked : 0.0;
    
    os << "Checker Statistics:" << std::endl;
    os << "  Transactions Checked: " << stats.transactions_checked << std::endl;
    os << "  Checks Passed: " << stats.checks_passed << std::endl;
    os << "  Checks Failed: " << stats.checks_failed << std::endl;
    os << "  Pass Rate: " << std::fixed << std::setprecision(2) 
       << (pass_rate * 100.0) << "%" << std::endl;
    os << "  Active Cycles: " << stats.cycles_active << std::endl;
    os << "  Runtime: " << duration.count() << " ms" << std::endl;
}

// Future utility functions can be added here
