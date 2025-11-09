#pragma once

#include "checker.h"
#include "adder_transaction.h"
#include "adder_simulation_context.h"
#include "Vhello_world_top.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <chrono>

/**
 * @brief Configuration structure for AdderChecker
 */
struct AdderCheckerConfig : public CheckerConfig {
    bool enable_overflow_check = true;

    AdderCheckerConfig() {
        log_level = CheckerLogLevel::INFO;
    }
    explicit AdderCheckerConfig(const CheckerConfig &base)
        : CheckerConfig(base) {
        log_level = CheckerLogLevel::INFO;
    }
};

// Forward declarations of utility functions
void print_adder_checker_config(const AdderCheckerConfig& config, std::ostream& os);
AdderCheckerConfig create_basic_adder_config();
AdderCheckerConfig create_debug_adder_config();

/**
 * @brief Specialised checker for adder verification
 *
 */
class AdderChecker : public BaseChecker<Vhello_world_top, AdderTransaction> {
public:
    using DutPtr = std::shared_ptr<Vhello_world_top>;
    using Base = BaseChecker<Vhello_world_top, AdderTransaction>;
    using AdderTransactionPtr = std::shared_ptr<AdderTransaction>;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;

    /**
     * @brief Construct AdderChecker
     * 
     * @param name Checker instance name
     * @param dut Shared pointer to DUT
     * @param ctx Adder-specific simulation context for tracking global state
     * @param config Adder-specific configuration
     */
    AdderChecker(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderCheckerConfig& config = {});

    /**
     * @brief Destructor
     */
    virtual ~AdderChecker();

    // Overridden virtual methods from BaseChecker
    bool perform_check(AdderTransactionPtr expected_txn, AdderTransactionPtr actual_txn) override;

protected:
    void reset() override;
    // Specific checks for adders
    bool check_arithmetic(AdderTransactionPtr expected_txn, AdderTransactionPtr actual_txn);
    bool check_overflow(AdderTransactionPtr expected_txn_ptr, AdderTransactionPtr actual_txn_ptr);

private:
    // Simulation context
    AdderSimulationContextPtr ctx_;

    // Configuration
    AdderCheckerConfig adder_config_;

};
