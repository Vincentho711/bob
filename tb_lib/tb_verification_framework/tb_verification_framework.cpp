#include "tb_verification_framework.h"

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
// VerificationEnvironment Implementation
// ============================================================================

VerificationEnvironment::VerificationEnvironment()
    : adder_checker(), coverage("Adder Coverage"), pipeline_delay(2), pipeline_flushed(false) {
    setup_coverage_points();
}

void VerificationEnvironment::set_pipeline_delay(uint32_t delay) {
    pipeline_delay = delay;
    pipeline_flushed = false;
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
}

void VerificationEnvironment::final_report() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "FINAL VERIFICATION REPORT" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    adder_checker.report();
    coverage.report();

    std::cout << "\n=== SUMMARY ===" << std::endl;
    bool overall_pass = adder_checker.all_passed();
    std::cout << "Overall Test Result: " << (overall_pass ? "PASSED" : "FAILED") << std::endl;

    if (!overall_pass) {
        std::cout << "Some functional checks failed!" << std::endl;
    }

    double corner_cov = coverage.get_corner_coverage();
    if (corner_cov < 100.0) {
        std::cout << "Warning: Corner case coverage incomplete (" << corner_cov << "%)" << std::endl;
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
    coverage.add_corner_case(127, 128);       // Around mid point
    coverage.add_corner_case(128, 127);       // Around mid point
    coverage.add_corner_case(1, 1);           // Small values
    coverage.add_corner_case(254, 254);       // Near maximum
    coverage.add_corner_case(255, 1);         // Overflow boundary
    coverage.add_corner_case(1, 255);         // Overflow boundary
}
