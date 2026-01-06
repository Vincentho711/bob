#ifndef SIMULATION_EXCEPTIONS_H
#define SIMULATION_EXCEPTIONS_H
#include <stdexcept>
#include <sstream>
#include <string>
#include <iostream>
#include <format>
#include "simulation_context.h"

namespace simulation {
    class VerificationError : public std::runtime_error{
    public:
        explicit VerificationError(const std::string &msg)
            : std::runtime_error("Verification Error: " + msg) {}
    };

    static void report_fatal(const std::string &msg) {
        std::stringstream ss;
        // Log to console with formatting
        ss << "\033[1;31m[" << std::format("{:^10} ps] ", current_time_ps);
        ss << "[FATAL]: " << msg << "\033[0m";
        throw VerificationError(ss.str());
    }
}
#endif // SIMULATION_EXCEPTIONS_H
