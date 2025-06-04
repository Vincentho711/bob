#pragma once

#include "checker.h"
#include "adder_transaction.h"
#include "adder_simulation_context.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <chrono>

/**
 * @brief Configuration structure for AdderChecker
 */
struct AdderCheckerConfig {
    bool enable_overflow_checking = true;        ///< Check for overflow conditions
    bool enable_timing_checks = true;            ///< Verify timing requirements
    bool enable_value_logging = false;           ///< Log all input/output values
    bool strict_mode = false;                    ///< Fail on any discrepancy
    uint32_t pipeline_depth = 2;                 ///< Expected pipeline depth of DUT
    uint32_t max_pending_transactions = 1000;    ///< Maximum transactions in pipeline
    
    // Timing constraints
    uint32_t max_response_cycles = 10;           ///< Maximum cycles for response
    
    // Statistical thresholds
    double min_pass_rate = 0.99;                 ///< Minimum acceptable pass rate
    uint32_t min_transactions_for_stats = 100;   ///< Minimum transactions for statistics
};

/**
 * @brief Statistics specific to adder verification
 */
struct AdderCheckerStats {
    uint64_t overflow_cases_checked = 0;         ///< Number of overflow cases verified
    uint64_t corner_cases_checked = 0;           ///< Number of corner cases verified
    uint64_t timing_violations = 0;              ///< Number of timing violations detected
    uint64_t value_mismatches = 0;               ///< Number of value mismatches
    uint64_t pipeline_errors = 0;                ///< Number of pipeline ordering errors
    
    // Value distribution tracking
    std::unordered_map<uint16_t, uint64_t> result_histogram; ///< Histogram of results
    uint16_t min_result_seen = 0xFFFF;           ///< Minimum result observed
    uint16_t max_result_seen = 0;                ///< Maximum result observed
    
    // Timing statistics
    std::chrono::high_resolution_clock::time_point first_check_time;
    std::chrono::high_resolution_clock::time_point last_check_time;
    double avg_check_latency_ns = 0.0;           ///< Average check latency in nanoseconds
};

/**
 * @brief Transaction with expected result and timing information
 */
struct PendingTransaction {
    std::shared_ptr<AdderTransaction> transaction;
    uint64_t drive_cycle;                        ///< Cycle when transaction was driven
    uint64_t expected_result_cycle;              ///< Cycle when result is expected
    std::chrono::high_resolution_clock::time_point drive_time;

    PendingTransaction(std::shared_ptr<AdderTransaction> txn, uint64_t drive_cycle, uint32_t pipeline_depth)
        : transaction(txn)
        , drive_cycle(drive_cycle)
        , expected_result_cycle(drive_cycle + pipeline_depth)
        , drive_time(std::chrono::high_resolution_clock::now()) {}
};

// Forward declarations of utility functions
bool validate_adder_checker_config(const AdderCheckerConfig& config, std::string& error_msg);
void print_adder_checker_config(const AdderCheckerConfig& config, std::ostream& os);
void print_adder_checker_stats(const AdderCheckerStats& stats, std::ostream& os);
void print_result_histogram(const std::unordered_map<uint16_t, uint64_t>& histogram, 
                           std::ostream& os, size_t max_entries = 20);
AdderCheckerConfig create_basic_adder_config();
AdderCheckerConfig create_strict_adder_config();
AdderCheckerConfig create_performance_adder_config();

/**
 * @brief Specialized checker for adder verification
 * 
 * This checker verifies:
 * - Correct arithmetic operation (a + b = c)
 * - Proper handling of overflow cases
 * - Pipeline timing and ordering
 * - Corner case coverage
 * - Performance characteristics
 * 
 * @tparam DUT_TYPE The type of the DUT (e.g., Vhello_world_top)
 */
template<typename DUT_TYPE>
class AdderChecker : public BaseChecker<DUT_TYPE, AdderTransaction> {
public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using Base = BaseChecker<DUT_TYPE, AdderTransaction>;
    using TransactionPtr = std::shared_ptr<AdderTransaction>;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;

    /**
     * @brief Construct AdderChecker
     * 
     * @param name Checker instance name
     * @param dut Shared pointer to DUT
     * @param config Adder-specific configuration
     * @param simulation_context Adder-specific simulation context for tracking global state
     */
    AdderChecker(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderCheckerConfig& config = {});

    /**
     * @brief Destructor - prints final report
     */
    virtual ~AdderChecker();

    // Overridden virtual methods from BaseChecker
    uint32_t check_cycle() override;
    bool perform_check(const AdderTransaction& txn) override;

    // Configuration management
    void update_config(const AdderCheckerConfig& new_config);
    const AdderCheckerConfig& get_adder_config() const { return adder_config_; }

    // Transaction tracking
    void expect_transaction(TransactionPtr txn);
    // void expect_transaction(const AdderTransaction& txn, uint64_t cycle);

    // Verification control
    void set_pipeline_depth(uint32_t depth);
    void enable_strict_mode(bool enable = true);
    void reset_statistics();

    // Analysis and reporting
    void print_detailed_report() const;
    void print_histogram() const;
    double get_current_pass_rate() const;
    bool is_pass_rate_acceptable() const;

    // Statistics access
    const AdderCheckerStats& get_adder_stats() const { return adder_stats_; }

protected:
    void reset() override;
    // Additional virtual methods for extensibility
    virtual bool check_arithmetic(const AdderTransaction& txn, uint16_t actual_result);
    virtual bool check_overflow_handling(const AdderTransaction& txn, uint16_t actual_result);
    virtual bool check_timing(const PendingTransaction& pending_txn);
    virtual void log_mismatch(const AdderTransaction& txn, uint16_t expected, uint16_t actual);

private:
    // Simulation context
    AdderSimulationContextPtr ctx_;

    // Configuration
    AdderCheckerConfig adder_config_;

    // Statistics
    AdderCheckerStats adder_stats_;

    // Pipeline management
    std::queue<PendingTransaction> pending_transactions_;

    // Internal helper methods
    void process_pending_transactions();
    bool verify_transaction_result(const PendingTransaction& pending_txn);
    void update_statistics(const AdderTransaction& txn, uint16_t actual_result, bool passed);
    void classify_transaction_type(const AdderTransaction& txn);
    void check_pipeline_health();
    uint16_t read_dut_output() const;

    // Validation helpers
    bool is_overflow_case(uint8_t a, uint8_t b) const;
    bool is_corner_case(const AdderTransaction& txn) const;
    std::string format_transaction_error(const AdderTransaction& txn, uint16_t expected, uint16_t actual) const;

    // Performance monitoring
    void start_timing();
    void end_timing();
    std::chrono::high_resolution_clock::time_point timing_start_;
};

// Template implementation must be in header file
#include "adder_checker_impl.h"
