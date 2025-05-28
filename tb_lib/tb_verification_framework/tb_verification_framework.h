#ifndef TB_VERIFICATION_FRAMEWORK_H
#define TB_VERIFICATION_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_set>
#include <iomanip>
#include <sstream>
#include <cstdint>

// Base class for all checkers
class BaseChecker {
public:
    BaseChecker(const std::string& name);
    virtual ~BaseChecker() = default;

    virtual void check() = 0;

    void report() const;
    bool all_passed() const;
    uint32_t get_pass_count() const;
    uint32_t get_fail_count() const;

protected:
    std::string checker_name;
    uint32_t pass_count;
    uint32_t fail_count;

    void log_pass(const std::string& message = "");
    void log_fail(const std::string& message);
    virtual void print_detailed_report() const;
};

// Adder-specific functional checker
class AdderChecker : public BaseChecker {
public:
    AdderChecker();

    void set_inputs(uint8_t a, uint8_t b, uint16_t actual_output, uint64_t cycle);
    void check() override;

private:
    uint8_t current_a;
    uint8_t current_b;
    uint16_t current_output;
    uint64_t current_cycle;
};

// Simple coverage tracker for input combinations
class CoverageTracker {
public:
    CoverageTracker(const std::string& name);

    void add_corner_case(uint8_t a, uint8_t b);
    void hit(uint8_t a, uint8_t b);
    void report() const;
    double get_corner_coverage() const;

private:
    std::string tracker_name;
    std::unordered_set<uint16_t> corner_cases;
    std::unordered_set<uint16_t> hit_cases;
    std::unordered_set<uint16_t> hit_corner_cases;

    uint16_t encode_values(uint8_t a, uint8_t b) const;
    std::pair<uint8_t, uint8_t> decode_values(uint16_t encoded) const;
};

// Verification environment that manages all checkers
class VerificationEnvironment {
public:
    VerificationEnvironment();

    void check_adder(uint8_t a, uint8_t b, uint16_t output, uint64_t cycle);
    void set_pipeline_delay(uint32_t delay);
    void final_report();
    void set_simulation_info(uint32_t seed, uint64_t max_cycles, const std::string& vcd_file = "");
    void start_test_timer();
    bool simulation_passed() const;

private:
    AdderChecker adder_checker;
    CoverageTracker coverage;
    uint32_t pipeline_delay;
    bool pipeline_flushed;
    uint32_t sim_seed;
    uint64_t max_sim_cycles;
    std::string vcd_filename;
    uint64_t total_cycles_run;
    std::chrono::system_clock::time_point test_start_time;
    std::chrono::system_clock::time_point test_end_time;
    bool test_timer_started;

    void setup_coverage_points();
};

#endif // TB_VERIFICATION_FRAMEWORK_H
