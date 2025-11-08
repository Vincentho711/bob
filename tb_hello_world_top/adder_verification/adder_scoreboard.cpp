#include "adder_scoreboard.h"
#include <iostream>

AdderScoreboard::AdderScoreboard(const std::string &name, DutPtr dut, AdderScoreboardConfig config, AdderSimulationContextPtr ctx, AdderCheckerPtr checker)
    : Base(name, dut, config, ctx, checker),
      ctx_(ctx),
      adder_config_(config),
      adder_stats_() {
    log_info("AdderScoreboard '" + name + "' constructed");
}

/**
 * @brief Add an expected transaction to the queue in scoreboard
 *
 * @param transaction_ptr A pointer to an expected transaction object
 * @param expected_cycle The cycle to be checked
 */
void AdderScoreboard::add_expected_transaction(AdderTransactionPtr transaction_ptr, uint64_t expected_cycle) {
    if (!transaction_ptr) {
        log_error("AdderScoreboard: Attempted to add null AdderTransaction");
        return;
    }
    log_debug("AdderScoreboard: Adding expected transaction " + transaction_ptr->convert2string());

    // Delegate to base class to actually queue it
    Base::add_expected_transaction(transaction_ptr, expected_cycle);
}

/**
 * @brief Check all expected transaction fo the current cycle
 *
 * @param actual_ptr A pointer to an actual transaction object
 * @return Returns the number of checks done
 */
std::uint32_t AdderScoreboard::check_current_cycle(AdderTransactionPtr actual_ptr) {
    if (!actual_ptr) {
        log_error("AdderScoreboard: Null actual transaction");
        return 0;
    }

    log_debug("AdderScoreboard: Checking actual transaction " + actual_ptr->convert2string());

    // Reuse BaseScoreboard logic
    std::uint32_t checks = Base::check_current_cycle(actual_ptr);

    // Perform coverage sampling
    adder_stats_.sample(actual_ptr->get_a(), actual_ptr->get_b(), actual_ptr->get_result());

    return checks;
}

/**
 * @brief Reset scoreboard
 */
void AdderScoreboard::reset() {
    Base::reset();
    adder_stats_ = AdderScoreboardStats{}; // Reset coverage group
    log_info("AdderScoreboard reset complete");
}
