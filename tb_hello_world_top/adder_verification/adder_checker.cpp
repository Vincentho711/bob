#include "adder_checker.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// This file contains explicit template instantiations and non-template utility functions
// The main template implementation is in adder_checker_impl.h

// Example explicit instantiation (uncomment when you have the actual DUT type):
// #include "Vhello_world_top.h"
// template class AdderChecker<Vhello_world_top>;

/**
 * @brief Utility function to print AdderCheckerConfig
 */
void print_adder_checker_config(const AdderCheckerConfig& config, std::ostream& os) {
    os << "AdderChecker Configuration:" << std::endl;
    os << "  Overflow Checking: " << (config.enable_overflow_check ? "Enabled" : "Disabled") << std::endl;
    os << "  Log Level: " << (config.log_level) << std::endl;
}

/**
 * @brief Create default configuration for basic verification scenario
 */
AdderCheckerConfig create_basic_adder_config() {
    AdderCheckerConfig config;
    config.enable_overflow_check = true;
    config.log_level = CheckerLogLevel::INFO; ///< Set debug log level
    return config;
}

/**
 * @brief Create default configuration for debugging
 */
AdderCheckerConfig create_debug_adder_config() {
    AdderCheckerConfig config;
    config.enable_overflow_check = true;
    config.log_level = CheckerLogLevel::DEBUG; ///< Set debug log level
    return config;
}

AdderChecker::AdderChecker(const std::string &name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderCheckerConfig &config)
    : Base(name, dut, ctx, config),
    ctx_(ctx),
    adder_config_(config) {
    log_info("AdderChecker '" + name + "' constructed");
}

AdderChecker::~AdderChecker() {
    log_info("AdderChecker '" + get_name() + "' destroyed.");
}

bool AdderChecker::perform_check(AdderTransactionPtr expected_txn_ptr, AdderTransactionPtr actual_txn_ptr) {
    // Run all check stages
    bool passed = true;
    passed &= check_arithmetic(expected_txn_ptr, actual_txn_ptr);
    if (adder_config_.enable_overflow_check) {
        passed &= check_overflow(expected_txn_ptr, actual_txn_ptr);
    }

    return passed;
}

bool AdderChecker::check_arithmetic(AdderTransactionPtr expected_txn_ptr, AdderTransactionPtr actual_txn_ptr) {
    uint16_t expected_result = static_cast<uint16_t>(expected_txn_ptr->get_a()) + static_cast<uint16_t>(expected_txn_ptr->get_b());
    uint16_t actual_result = actual_txn_ptr->get_result();

    if (actual_result != expected_result) {
        this->log_debug("check_arithmetic() failed. expected_result=" + std::to_string(expected_result) + " actual_result=" + 
                  std::to_string(actual_result));
        return false;
    }

    this->log_debug("check_arithmetic() passed. expected_result=actual_result=" + std::to_string(actual_result));
    return true;
}

bool AdderChecker::check_overflow(AdderTransactionPtr expected_txn_ptr, AdderTransactionPtr actual_txn_ptr) {
    // Since output is 1 bit greater than the input bit-width, overflow is not possible.
    return true;
}

/**
 * @brief Reset scoreboard
 */
void AdderChecker::reset() {
    Base::reset();
    log_info("AdderScoreboard reset complete");
}
