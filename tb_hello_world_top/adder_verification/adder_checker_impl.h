#pragma once

// Template implementation for AdderChecker
// This file is included by adder_checker.h and should not be included directly

template<typename DUT_TYPE>
AdderChecker<DUT_TYPE>::AdderChecker(const std::string& name, DutPtr dut, const AdderCheckerConfig& config)
    : Base(name, dut), adder_config_(config) {
    
    // Validate configuration
    std::string error_msg;
    if (!validate_adder_checker_config(config, error_msg)) {
        this->log_error("Invalid AdderChecker configuration: " + error_msg);
    }
    
    // Initialize statistics
    adder_stats_ = AdderCheckerStats{};
    adder_stats_.first_check_time = std::chrono::high_resolution_clock::now();
    adder_stats_.last_check_time = adder_stats_.first_check_time;
    
    this->log_info("AdderChecker initialized with pipeline depth " + 
                   std::to_string(adder_config_.pipeline_depth));
}

template<typename DUT_TYPE>
AdderChecker<DUT_TYPE>::~AdderChecker() {
    if (this->get_stats().transactions_checked > 0) {
        print_detailed_report();
    }
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::update_config(const AdderCheckerConfig& new_config) {
    std::string error_msg;
    if (!validate_adder_checker_config(new_config, error_msg)) {
        this->log_error("Cannot update config: " + error_msg);
        return;
    }
    
    adder_config_ = new_config;
    this->log_info("AdderChecker configuration updated");
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::expect_transaction(TransactionPtr txn, uint64_t cycle) {
    if (!txn) {
        this->log_error("Cannot expect null transaction");
        return;
    }
    
    // Check if we're exceeding pending transaction limit
    if (pending_transactions_.size() >= adder_config_.max_pending_transactions) {
        adder_stats_.pipeline_errors++;
        this->log_error("Exceeded maximum pending transactions (" + 
                       std::to_string(adder_config_.max_pending_transactions) + ")");
        if (adder_config_.strict_mode) {
            return;
        }
    }
    
    // Create pending transaction with pipeline delay
    // Args : txn, current_cycle, pipeline_depth
    pending_transactions_.emplace(txn, cycle, adder_config_.pipeline_depth);
    
    this->log_debug("Expected transaction at cycle " + std::to_string(cycle + adder_config_.pipeline_depth));
}

// template<typename DUT_TYPE>
// void AdderChecker<DUT_TYPE>::expect_transaction(const AdderTransaction& txn, uint64_t cycle) {
//     auto txn_copy = std::make_shared<AdderTransaction>(txn);
//     expect_transaction(txn_copy, cycle);
// }

template<typename DUT_TYPE>
uint32_t AdderChecker<DUT_TYPE>::check_cycle() {
    start_timing();
    
    // Process any pending transactions that should have results by now
    process_pending_transactions();
    
    // Check pipeline health
    if (adder_config_.enable_timing_checks) {
        check_pipeline_health();
    }
    
    end_timing();
    this->current_cycle_++;
    return 1U;
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::perform_check(const AdderTransaction& txn) {
    // Read result from DUT
    uint16_t actual_result = read_dut_output();

    // Run all check stages
    bool passed = true;
    passed &= check_arithmetic(txn, actual_result);
    passed &= check_overflow_handling(txn, actual_result);

    // Possibly add more validation
    update_statistics(txn, actual_result, passed);
    if (!passed) {
        log_mismatch(txn, txn.get_expected_result(), actual_result);
    }
    return passed;
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::reset() {
    Base::reset();
    
    // Clear pending transactions
    while (!pending_transactions_.empty()) {
        pending_transactions_.pop();
    }
    
    this->current_cycle_ = 0;
    
    // Reset statistics if configured to do so
    if (adder_config_.strict_mode) {
        reset_statistics();
    }
    
    this->log_info("AdderChecker reset completed");
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::set_pipeline_depth(uint32_t depth) {
    if (depth == 0) {
        this->log_error("Pipeline depth must be greater than 0");
        return;
    }
    
    adder_config_.pipeline_depth = depth;
    this->log_info("Pipeline depth updated to " + std::to_string(depth));
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::enable_strict_mode(bool enable) {
    adder_config_.strict_mode = enable;
    this->log_info("Strict mode " + std::string(enable ? "enabled" : "disabled"));
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::reset_statistics() {
    adder_stats_ = AdderCheckerStats{};
    adder_stats_.first_check_time = std::chrono::high_resolution_clock::now();
    adder_stats_.last_check_time = adder_stats_.first_check_time;
    this->log_info("Statistics reset");
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::check_arithmetic(const AdderTransaction& txn, uint16_t actual_result) {
    uint16_t expected = txn.get_expected_result();
    
    if (actual_result != expected) {
        log_mismatch(txn, expected, actual_result);
        return false;
    }
    
    if (adder_config_.enable_value_logging) {
        this->log_debug("Arithmetic check passed: " + std::to_string(txn.get_a()) + 
                       " + " + std::to_string(txn.get_b()) + " = " + std::to_string(actual_result));
    }
    
    return true;
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::check_overflow_handling(const AdderTransaction& txn, uint16_t actual_result) {
    if (!adder_config_.enable_overflow_checking) {
        return true;
    }
    
    bool is_overflow = is_overflow_case(txn.get_a(), txn.get_b());
    
    if (is_overflow) {
        adder_stats_.overflow_cases_checked++;
        
        // For our 8-bit adder, overflow means result > 255
        // The DUT should still produce the correct mathematical result
        uint16_t expected_math_result = static_cast<uint16_t>(txn.get_a()) + static_cast<uint16_t>(txn.get_b());
        
        if (actual_result != expected_math_result) {
            this->log_error("Overflow case failed: " + format_transaction_error(txn, expected_math_result, actual_result));
            return false;
        }
        
        if (adder_config_.enable_value_logging) {
            this->log_debug("Overflow case handled correctly: result = " + std::to_string(actual_result));
        }
    }
    
    return true;
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::check_timing(const PendingTransaction& pending_txn) {
    if (!adder_config_.enable_timing_checks) {
        return true;
    }

    uint64_t cycles_elapsed = this->current_cycle_ - pending_txn.drive_cycle;

    if (cycles_elapsed > adder_config_.max_response_cycles) {
        adder_stats_.timing_violations++;
        this->log_error("Timing violation: transaction took " + std::to_string(cycles_elapsed) +
                       " cycles (max: " + std::to_string(adder_config_.max_response_cycles) + ")");
        return false;
    }

    return true;
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::log_mismatch(const AdderTransaction& txn, uint16_t expected, uint16_t actual) {
    adder_stats_.value_mismatches++;
    std::string error_msg = format_transaction_error(txn, expected, actual);
    this->log_error("Value mismatch: " + error_msg);
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::process_pending_transactions() {
    while (!pending_transactions_.empty()) {
        auto& pending = pending_transactions_.front();

        // Check if this transaction's result should be available
        if (this->current_cycle_ < pending.expected_result_cycle) {
            break; // Not ready yet
        }

        // Verify the transaction result
        bool passed = verify_transaction_result(pending);

        // Update statistics
        uint16_t actual_result = read_dut_output();
        update_statistics(*pending.transaction, actual_result, passed);

        // Remove processed transaction
        pending_transactions_.pop();

        // Stop on first error if in strict mode
        if (!passed && adder_config_.strict_mode) {
            this->log_fatal("Stopping on first error (strict mode enabled)");
            break;
        }
    }
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::verify_transaction_result(const PendingTransaction& pending_txn) {
    const auto& txn = *pending_txn.transaction;
    uint16_t actual_result = read_dut_output();
    
    // Perform all checks
    bool arithmetic_ok = check_arithmetic(txn, actual_result);
    bool overflow_ok = check_overflow_handling(txn, actual_result);
    bool timing_ok = check_timing(pending_txn);
    
    bool all_checks_passed = arithmetic_ok && overflow_ok && timing_ok;
    
    if (all_checks_passed) {
        this->log_debug("Transaction verification passed: " + txn.convert2string());
    }
    
    return all_checks_passed;
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::update_statistics(const AdderTransaction& txn, uint16_t actual_result, bool passed) {
    adder_stats_.last_check_time = std::chrono::high_resolution_clock::now();
    
    // Update result histogram
    adder_stats_.result_histogram[actual_result]++;
    adder_stats_.min_result_seen = std::min(adder_stats_.min_result_seen, actual_result);
    adder_stats_.max_result_seen = std::max(adder_stats_.max_result_seen, actual_result);
    
    // Classify transaction type
    classify_transaction_type(txn);
    
    // Update base class statistics
    this->update_stats(passed);
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::classify_transaction_type(const AdderTransaction& txn) {
    if (is_corner_case(txn)) {
        adder_stats_.corner_cases_checked++;
    }
    
    if (is_overflow_case(txn.get_a(), txn.get_b())) {
        adder_stats_.overflow_cases_checked++;
    }
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::check_pipeline_health() {
    size_t pending_count = pending_transactions_.size();

    if (pending_count > adder_config_.max_pending_transactions / 2) {
        this->log_warning("Pipeline depth concern: " + std::to_string(pending_count) + 
                         " pending transactions");
    }

    // Check for stalled transactions
    if (!pending_transactions_.empty()) {
        this->log_error("Current_cycle = " + std::to_string(this->current_cycle_) + ", Number of pending_transactions = " + std::to_string(pending_count));
        const auto& oldest = pending_transactions_.front();
        this->log_error("oldest.drive_cycle = " + std::to_string(oldest.drive_cycle));
        uint64_t age = this->current_cycle_ - oldest.drive_cycle;
        this->log_error("pending_transactions_.front()'s age = " + std::to_string(age));

        if (age > adder_config_.max_response_cycles * 2) {
            adder_stats_.pipeline_errors++;
            this->log_error("Potential pipeline stall: transaction age = " + std::to_string(age) + " cycles");
        }
    }
}

template<typename DUT_TYPE>
uint16_t AdderChecker<DUT_TYPE>::read_dut_output() const {
    auto dut = this->get_dut();
    return static_cast<uint16_t>(dut->c_o);
}


template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::is_overflow_case(uint8_t a, uint8_t b) const {
    return (static_cast<uint16_t>(a) + static_cast<uint16_t>(b)) > 255;
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::is_corner_case(const AdderTransaction& txn) const {
    uint8_t a = txn.get_a();
    uint8_t b = txn.get_b();
    
    // Corner cases for 8-bit adder
    return (a == 0 || b == 0) ||                    // Zero operands
           (a == 0xFF || b == 0xFF) ||               // Maximum values
           (a == 0x80 || b == 0x80) ||               // MSB set
           (a == b) ||                               // Equal operands
           ((a ^ b) == 0xFF);                        // Complementary values
}

template<typename DUT_TYPE>
std::string AdderChecker<DUT_TYPE>::format_transaction_error(const AdderTransaction& txn, 
                                                            uint16_t expected, 
                                                            uint16_t actual) const {
    std::ostringstream oss;
    oss << std::hex << std::uppercase
        << "0x" << std::setw(2) << std::setfill('0') << static_cast<int>(txn.get_a())
        << " + 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(txn.get_b())
        << " = 0x" << std::setw(3) << std::setfill('0') << actual
        << " (expected: 0x" << std::setw(3) << std::setfill('0') << expected << ")";
    return oss.str();
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::start_timing() {
    timing_start_ = std::chrono::high_resolution_clock::now();
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::end_timing() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - timing_start_).count();
    
    // Update average latency using exponential moving average
    if (adder_stats_.avg_check_latency_ns == 0.0) {
        adder_stats_.avg_check_latency_ns = static_cast<double>(duration);
    } else {
        // Use alpha = 0.1 for exponential moving average
        adder_stats_.avg_check_latency_ns = 0.9 * adder_stats_.avg_check_latency_ns + 0.1 * duration;
    }
}

// Analysis and reporting methods

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::print_detailed_report() const {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "ADDER CHECKER DETAILED REPORT: " << this->get_name() << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Basic statistics
    this->print_report();
    
    // Adder-specific statistics
    std::cout << "\nAdder-Specific Statistics:" << std::endl;
    print_adder_checker_stats(adder_stats_, std::cout);
    
    // Configuration
    std::cout << "\nAdder Configuration:" << std::endl;
    print_adder_checker_config(adder_config_, std::cout);
    
    // Result distribution if we have enough data
    if (adder_stats_.result_histogram.size() > 0 && 
        this->get_stats().transactions_checked >= adder_config_.min_transactions_for_stats) {
        std::cout << "\n";
        print_result_histogram(adder_stats_.result_histogram, std::cout);
    }
    
    std::cout << std::string(70, '=') << std::endl;
}

template<typename DUT_TYPE>
void AdderChecker<DUT_TYPE>::print_histogram() const {
    if (adder_stats_.result_histogram.empty()) {
        std::cout << "No histogram data available" << std::endl;
        return;
    }
    
    print_result_histogram(adder_stats_.result_histogram, std::cout);
}

template<typename DUT_TYPE>
double AdderChecker<DUT_TYPE>::get_current_pass_rate() const {
    return this->get_pass_rate();
}

template<typename DUT_TYPE>
bool AdderChecker<DUT_TYPE>::is_pass_rate_acceptable() const {
    return (this->get_stats().transactions_checked >= adder_config_.min_transactions_for_stats) &&
           (get_current_pass_rate() >= adder_config_.min_pass_rate);
}
