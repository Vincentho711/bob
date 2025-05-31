#include "tb_verification_framework.h"

#include <chrono>

// ============================================================================
// BaseChecker Implementation
// ============================================================================

BaseChecker::BaseChecker(const std::string& name)
    : checker_name(name), pass_count(0), fail_count(0) {
}

void BaseChecker::report() const {
    std::cout << "\n=== " << checker_name << " Report ===" << std::endl;
    std::cout << "Passed: " << pass_count << std::endl;
    std::cout << "Failed: " << fail_count << std::endl;
    std::cout << "Total:  " << (pass_count + fail_count) << std::endl;
    if (pass_count + fail_count > 0) {
        double pass_rate = (double)pass_count / (pass_count + fail_count) * 100.0;
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(2) << pass_rate << "%" << std::endl;
    }
    print_detailed_report();
}

bool BaseChecker::all_passed() const {
    return fail_count == 0 && pass_count > 0;
}

uint32_t BaseChecker::get_pass_count() const {
    return pass_count;
}

uint32_t BaseChecker::get_fail_count() const {
    return fail_count;
}

void BaseChecker::log_pass(const std::string& message) {
    pass_count++;
    if (!message.empty()) {
        std::cout << "[PASS] " << checker_name << ": " << message << std::endl;
    }
}

void BaseChecker::log_fail(const std::string& message) {
    fail_count++;
    std::cout << "[FAIL] " << checker_name << ": " << message << std::endl;
}

void BaseChecker::print_detailed_report() const {
    // Default implementation - can be overridden by derived classes
}

// ============================================================================
// AdderChecker Implementation
// ============================================================================

AdderChecker::AdderChecker()
    : BaseChecker("Adder Functional Checker"),
      current_a(0), current_b(0), current_output(0), current_cycle(0) {
}

void AdderChecker::set_inputs(uint8_t a, uint8_t b, uint16_t actual_output, uint64_t cycle) {
    current_a = a;
    current_b = b;
    current_output = actual_output;
    current_cycle = cycle;
}

void AdderChecker::check() {
    uint16_t expected = static_cast<uint16_t>(current_a) + static_cast<uint16_t>(current_b);

    std::stringstream msg;
    msg << "Cycle " << current_cycle
        << ": a=" << +current_a
        << ", b=" << +current_b
        << ", expected=" << expected
        << ", actual=" << current_output;

    if (current_output == expected) {
        log_pass();
    } else {
        log_fail(msg.str());
    }
}

// ============================================================================
// CoverageTracker Implementation
// ============================================================================

CoverageTracker::CoverageTracker(const std::string& name) : tracker_name(name) {
}

void CoverageTracker::add_corner_case(uint8_t a, uint8_t b) {
    corner_cases.insert(encode_values(a, b));
}

void CoverageTracker::hit(uint8_t a, uint8_t b) {
    uint16_t encoded = encode_values(a, b);
    hit_cases.insert(encoded);

    // Check if this is a corner case
    if (corner_cases.find(encoded) != corner_cases.end()) {
        hit_corner_cases.insert(encoded);
    }
}

void CoverageTracker::report() const {
    std::cout << "\n=== " << tracker_name << " Report ===" << std::endl;

    // Corner case coverage
    if (!corner_cases.empty()) {
        double corner_coverage = (double)hit_corner_cases.size() / corner_cases.size() * 100.0;
        std::cout << "Corner Cases Hit: " << hit_corner_cases.size()
                  << "/" << corner_cases.size()
                  << " (" << std::fixed << std::setprecision(2) << corner_coverage << "%)" << std::endl;

        // List missed corner cases
        if (hit_corner_cases.size() < corner_cases.size()) {
            std::cout << "Missed corner cases: ";
            for (uint16_t encoded : corner_cases) {
                if (hit_corner_cases.find(encoded) == hit_corner_cases.end()) {
                    auto decoded = decode_values(encoded);
                    std::cout << "(" << +decoded.first << "," << +decoded.second << ") ";
                }
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Total unique combinations tested: " << hit_cases.size() << std::endl;
}

double CoverageTracker::get_corner_coverage() const {
    if (corner_cases.empty()) return 100.0;
    return (double)hit_corner_cases.size() / corner_cases.size() * 100.0;
}

uint16_t CoverageTracker::encode_values(uint8_t a, uint8_t b) const {
    return (static_cast<uint16_t>(a) << 8) | b;
}

std::pair<uint8_t, uint8_t> CoverageTracker::decode_values(uint16_t encoded) const {
    return {static_cast<uint8_t>(encoded >> 8), static_cast<uint8_t>(encoded & 0xFF)};
}

// ============================================================================
// TransactionTracker Implementation
// ============================================================================

TransactionTracker::TransactionTracker(const std::string& name) 
    : tracker_name(name), corner_case_count(0), random_count(0), directed_count(0) {
}

void TransactionTracker::add_transaction(const std::string& txn_name, TransactionType type) {
    transaction_names.push_back(txn_name);
    
    switch (type) {
        case TransactionType::CORNER_CASE:
            corner_case_count++;
            break;
        case TransactionType::RANDOM:
            random_count++;
            break;
        case TransactionType::DIRECTED:
            directed_count++;
            break;
    }
}

void TransactionTracker::report() const {
    std::cout << "\n=== " << tracker_name << " Report ===" << std::endl;
    std::cout << "Total Transactions: " << transaction_names.size() << std::endl;
    std::cout << "  - Corner Cases: " << corner_case_count << std::endl;
    std::cout << "  - Random: " << random_count << std::endl;
    std::cout << "  - Directed: " << directed_count << std::endl;
    
    if (!transaction_names.empty()) {
        std::cout << "Transaction Types Distribution:" << std::endl;
        double corner_pct = (double)corner_case_count / transaction_names.size() * 100.0;
        double random_pct = (double)random_count / transaction_names.size() * 100.0;
        double directed_pct = (double)directed_count / transaction_names.size() * 100.0;
        
        std::cout << "  - Corner Cases: " << std::fixed << std::setprecision(1) << corner_pct << "%" << std::endl;
        std::cout << "  - Random: " << std::fixed << std::setprecision(1) << random_pct << "%" << std::endl;
        std::cout << "  - Directed: " << std::fixed << std::setprecision(1) << directed_pct << "%" << std::endl;
    }
}

uint32_t TransactionTracker::get_total_count() const {
    return transaction_names.size();
}

uint32_t TransactionTracker::get_corner_case_count() const {
    return corner_case_count;
}

// ============================================================================
// VerificationEnvironment Implementation
// ============================================================================

VerificationEnvironment::VerificationEnvironment()
    : adder_checker(), coverage("Adder Coverage"), transaction_tracker("Transaction Tracker"),
      pipeline_delay(2), pipeline_flushed(false), sim_seed(0), max_sim_cycles(0), 
      vcd_filename(""), total_cycles_run(0), test_timer_started(false) {
    setup_coverage_points();
}

void VerificationEnvironment::set_pipeline_delay(uint32_t delay) {
    pipeline_delay = delay;
    pipeline_flushed = false;
}

void VerificationEnvironment::set_simulation_info(uint32_t seed, uint64_t max_cycles, const std::string& vcd_file) {
    sim_seed = seed;
    max_sim_cycles = max_cycles;
    vcd_filename = vcd_file;
}

void VerificationEnvironment::start_test_timer() {
    test_start_time = std::chrono::system_clock::now();
    test_timer_started = true;
}

void VerificationEnvironment::check_adder(uint8_t a, uint8_t b, uint16_t output, uint64_t cycle) {
    // Don't start checking until pipeline is flushed with known values
    if (cycle <= pipeline_delay) {
        std::cout << "[INFO] Cycle " << cycle << ": Pipeline flushing, skipping verification" << std::endl;
        return;
    }

    if (!pipeline_flushed) {
        std::cout << "[INFO] Pipeline flushed, starting verification from cycle " << cycle << std::endl;
        pipeline_flushed = true;
    }

    adder_checker.set_inputs(a, b, output, cycle);
    adder_checker.check();
    coverage.hit(a, b);
    total_cycles_run = cycle;
}

void VerificationEnvironment::add_transaction(const std::string& txn_name, const std::string& txn_type) {
    TransactionTracker::TransactionType type;
    
    if (txn_name.find("corner_case") != std::string::npos) {
        type = TransactionTracker::TransactionType::CORNER_CASE;
    } else if (txn_name.find("random") != std::string::npos) {
        type = TransactionTracker::TransactionType::RANDOM;
    } else {
        type = TransactionTracker::TransactionType::DIRECTED;
    }
    
    transaction_tracker.add_transaction(txn_name, type);
}

void VerificationEnvironment::final_report() {
    auto test_end_time = std::chrono::system_clock::now();

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "SIMULATION DEBUG INFORMATION" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Seed Used: " << sim_seed << std::endl;
    std::cout << "Max Cycles Configured: " << max_sim_cycles << std::endl;
    std::cout << "Actual Cycles Run: " << total_cycles_run << std::endl;
    std::cout << "Pipeline Delay: " << pipeline_delay << " cycles" << std::endl;
    std::cout << "Effective Verification Cycles: " << (total_cycles_run > pipeline_delay ? total_cycles_run - pipeline_delay : 0) << std::endl;
    if (!vcd_filename.empty()) {
        std::cout << "Waveform File: " << vcd_filename << std::endl;
    }

    // Show test execution time
    if (test_timer_started) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end_time - test_start_time);
        std::cout << "Test Execution Time: " << duration.count() << " ms" << std::endl;
    } else {
        std::cout << "Test Execution Time: Not measured (timer not started)" << std::endl;
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "FINAL VERIFICATION REPORT" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Show transaction statistics
    transaction_tracker.report();
    
    // Show functional verification results
    adder_checker.report();
    
    // Show coverage results
    coverage.report();

    std::cout << "\n=== SUMMARY ===" << std::endl;
    bool overall_pass = adder_checker.all_passed();
    std::cout << "Overall Test Result: " << (overall_pass ? "PASSED" : "FAILED") << std::endl;

    if (!overall_pass && adder_checker.get_fail_count() > 0) {
        std::cout << "❌ " << adder_checker.get_fail_count() << " functional check(s) failed!" << std::endl;
    } else if (overall_pass && adder_checker.get_pass_count() > 0) {
        std::cout << "✅ All " << adder_checker.get_pass_count() << " functional checks passed!" << std::endl;
    }

    double corner_cov = coverage.get_corner_coverage();
    if (corner_cov < 100.0) {
        std::cout << "⚠️  Corner case coverage incomplete (" << std::fixed << std::setprecision(1) << corner_cov << "%)" << std::endl;
    } else {
        std::cout << "✅ All corner cases covered (100%)" << std::endl;
    }

    // Additional statistics
    uint32_t total_transactions = transaction_tracker.get_total_count();
    uint32_t corner_transactions = transaction_tracker.get_corner_case_count();
    if (total_transactions > 0) {
        std::cout << "📊 Executed " << total_transactions << " transactions (" 
                  << corner_transactions << " corner cases)" << std::endl;
    }

    std::cout << std::string(60, '=') << std::endl;
}

bool VerificationEnvironment::simulation_passed() const {
    return adder_checker.all_passed();
}

void VerificationEnvironment::setup_coverage_points() {
    // Define corner cases for 8-bit adder
    coverage.add_corner_case(0, 0);           // Minimum values
    coverage.add_corner_case(255, 255);       // Maximum values
    coverage.add_corner_case(0, 255);         // Min + Max
    coverage.add_corner_case(255, 0);         // Max + Min
    coverage.add_corner_case(128, 128);       // Mid values
    coverage.add_corner_case(1, 1);           // Small values
    coverage.add_corner_case(254, 1);         // Near overflow
    coverage.add_corner_case(255, 1);         // Overflow boundary
    coverage.add_corner_case(127, 128);       // Around mid point
    coverage.add_corner_case(128, 127);       // Around mid point
    coverage.add_corner_case(254, 254);       // Near maximum
}
