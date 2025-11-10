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
    bool check_result = false;
    std::uint64_t current_cycle = ctx_->current_cycle();
    std::uint32_t checks_performed = 0;

    // Process all transactions ready for checking
    while (!expected_transactions_.empty() &&
           expected_transactions_.front().expected_cycle <= current_cycle) {
        ExpectedTransaction &expected_txn = expected_transactions_.front();
        TransactionPtr expected_ptr = expected_txn.transaction_ptr;

        // Check for timeout
        if (config_.enable_timeout_checking &&
            (current_cycle - expected_txn.submitted_cycle) > config_.timeout_cycles) {
            Base::handle_timeout(expected_txn);
            if (config_.stop_on_first_error) {
                log_fatal("Fatal error: Transaction timed out at cycle " + std::to_string(current_cycle));
                throw std::runtime_error("Fatal error: Transaction timed out at cycle " + std::to_string(current_cycle));
            }
        } else {
            check_result = checker_->perform_check(expected_ptr, actual_ptr);
            Base::update_stats(check_result);
            checks_performed++;
            if (!check_result) {
                log_fatal("Transaction mismatch detected! Expected transaction: " + expected_ptr->convert2string() +
                          " | Actual transaction: " + actual_ptr->convert2string());
                if (config_.stop_on_first_error) {
                    throw std::runtime_error("Fatal error: Transaction mismatch at cycle " + std::to_string(current_cycle));
                }
            } else {
                adder_stats_.sample(expected_ptr->get_a(), expected_ptr->get_b(), actual_ptr->get_result());
            }
            // Pop the expected transaction
            expected_transactions_.pop();
        }
    }

    return checks_performed;
}

/**
 * @brief Get a shared pointer to an AdderScoreboardStats object
 *
 * @return A shared pointer to the AdderScoreboardStats object
 */
AdderScoreboardStatsPtr AdderScoreboard::get_stats_ptr() {
    return std::make_shared<AdderScoreboardStats>(adder_stats_);
}

/**
 * @brief Reset scoreboard
 */
void AdderScoreboard::reset() {
    Base::reset();
    adder_stats_ = AdderScoreboardStats{}; // Reset coverage group
    log_info("AdderScoreboard reset complete");
}
