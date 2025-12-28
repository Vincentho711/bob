#ifndef SIMULATION_EXCEPTIONS_H
#define SIMULATION_EXCEPTIONS_H
#include <stdexcept>
#include <string>
#include <iostream>

namespace simulation {
    class VerificationError : public std::runtime_error{
    public:
        explicit VerificationError(const std::string &msg)
            : std::runtime_error("Verification Error: " + msg) {}
    };

    static void report_fatal(const std::string &msg) {
        // Log to console with formatting
        std::cerr << "\033[1;31m[FATAL] " << msg << "\033[0m" << std::endl;
        throw VerificationError(msg);
    }
}
#endif // SIMULATION_EXCEPTIONS_H
