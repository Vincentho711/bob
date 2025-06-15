#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "simulation_context.h"
#include "transaction.h"
#include "checker.h"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>

/**
 * @brief Enumeration for scoreboard log levels
 */
enum class ScoreboardLogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Configuration structure for scoreboard behavior
 */
struct ScoreboardConfig {
    uint32_t max_latency_cycles = 10;
    bool enable_out_of_order_matching = false;
    bool stop_on_first_error = true;
    bool enable_detailed_logging = true;
    bool enable_timeout_checking = true;
    bool use_chceker = true;
    ScoreboardLogLevel log_level = ScoreboardLogLevel::INFO;
    uint32_t max_pending_transactions = 1000; ///< Maximum pending transactions
    uint32_t timeout_cycles = 100;            ///< Transaction timeout in cycles
    double min_match_rate = 0.95;             ///< Minimum acceptable match rate

    ScoreboardConfig() = default;
};

/**
 * @brief Base statistics structure for scoreboard
 */
struct ScoreboardStats {
    uint64_t total_expected = 0;
    uint64_t total_actual = 0;
    uint64_t expected_transaction_dropped = 0;
    uint64_t actual_transaction_dropped = 0;
    uint64_t matched = 0;
    uint64_t mismatch = 0;
    uint64_t timed_out = 0;
    uint64_t checks_performed = 0;
    uint64_t checks_passed = 0;

    std::chrono::high_resolution_clock::time_point first_transaction_time;
    std::chrono::high_resolution_clock::time_point last_transaction_time;

    ScoreboardStats() = default;

    void reset() {
        *this = ScoreboardStats{};
        first_transaction_time = std::chrono::high_resolution_clock::now();
        last_transaction_time = first_transaction_time;
    }
};

template<typename DUT_TYPE, typename TXN_TYPE>
class BaseScoreboard {
public:
  using DutPtr = std::shared_ptr<DUT_TYPE>;
  using TransactionPtr = std::shared_ptr<TXN_TYPE>;
  using SimulationContextPtr = std::shared_ptr<SimulationContext>;
  using CheckerPtr = std::shared_ptr<BaseChecker<DUT_TYPE, TXN_TYPE>>;

  /**
    * @brief Structure to hold pending transactions with metadata
    */
  struct PendingTransaction {
      TransactionPtr transaction;
      uint64_t expected_cycle;
      uint64_t submitted_cycle;

      PendingTransaction(TransactionPtr txn, uint64_t exp_cycle, uint64_t sub_cycle)
          : transaction(txn), expected_cycle(exp_cycle), submitted_cycle(sub_cycle) {}
  };

protected:
    std::string name_;
    DutPtr dut_;
    CheckerPtr checker_;
    ScoreboardConfig config_;
    ScoreboardStats stats_;
    SimulationContextPtr ctx_;
    std::queue<PendingTransaction> expected_queue_;
    std::queue<PendingTransaction> actual_queue_;

public:
    /**
     * @brief Construct a new BaseScoreboard
     * 
     * @param name Unique name for this scoreboard instance
     * @param dut Shared pointer to the DUT
     * @param checker Shared pointer to the checker
     * @param ctx Shared pointer to the simulation context
     * @param config Configuration for scoreboard behaviour
     */
    explicit BaseScoreboard(const std::string& name, DutPtr dut, CheckerPtr checker,
                            SimulationContextPtr ctx, const ScoreboardConfig& config = ScoreboardConfig{})
        : name_(name), dut_(dut), checker_(checker), config_(config), ctx_(ctx) {
        if (!dut_) {
            throw std::invalid_argument("DUT pointer cannot be null");
        }
        if (!checker_) {
            throw std::invalid_argument("Checker potiner cannot be null");
        }
        if (!ctx_) {
            throw std::invalid_argument("SimulationContext pointer cannot be null");
        }
        log_info("BaseScoreboard '" + name_ + "' initialised");
    }

    /**
     * @brief Virtual destructor
     */
    virtual ~BaseScoreboard() = default;

    // Disable copy constructor and assignment (scoreboard should be unique)
    BaseScoreboard(const BaseScoreboard&) = delete;
    BaseScoreboard& operator=(const BaseScoreboard&) = delete;

    // Enable move constructor and assignment
    BaseScoreboard(BaseScoreboard&&) = default;
    BaseScoreboard& operator=(BaseScoreboard&&) = default;

    /**
     * @brief Add expected transaction to expected_queue_
     * 
     * @param transaction Expected transaction
     * @param expected_cycle Cycle expected to be popped from queue
     */
    virtual void add_expected(TransactionPtr transaction, uint64_t expected_cycle);

    /**
     * @brief Add actual transaction to actual_queue_
     * 
     * @param transaction Expected transaction
     * @param expected_cycle Cycle expected to be popped from queue
     */
    virtual void add_actual(TransactionPtr transaction, uint64_t expected_cycle);
    
    /**
     * @brief Match 2 pending transactions
     * 
     * @return true if match, false if they do not
     */
    virtual bool compare_transactions(const PendingTransaction& expected_transaction, const PendingTransaction& actual_transaction);

    /**
     * @brief Handle transaction timeouts
     */
    virtual void handle_timeout(const PendingTransaction& timed_out_transaction);

    /**
     * @brief Check for whether there are time-outed transactions, pop them
     */
    virtual void check_for_timeouts();

    /**
     * @brief Process pending transactions and perform matching
     * Called periodically to handle timeouts and matching
     * @return true if success, false if fail
     */
    virtual bool process_transactions();

    /**
     * @brief Check if all transactions have been processed in the pending queues
     * 
     * @return true if scoreboard is empty (all transactions matched/dropped)
     */
    virtual bool empty_queues() const;

    /**
     * @brief Get current match rate
     * 
     * @return Match rate as a value between 0.0 and 1.0
     */
    virtual double get_match_rate() const;
    
    /**
     * @brief Check if scoreboard performance is acceptable
     * 
     * @return true if match rate meets minimum requirements
     */
    virtual bool is_passing() const;

    // Configuration management

    /**
     * @brief Update scoreboard configuration
     * 
     * @param new_config New configuration to apply
     */
    void update_config(const ScoreboardConfig& new_config);

    /**
     * @brief Get current configuration
     */
    const ScoreboardConfig& get_config() const { return config_; }

    // Statistics and reporting

    /**
     * @brief Get current statistics
     */
    const ScoreboardStats& get_stats() const { return stats_; }

    /**
     * @brief Reset all statistics
     */
    void reset_stats();

    /**
     * @brief Print comprehensive report
     */
    virtual void print_report() const;

    /**
     * @brief Print summary report
     */
    virtual void print_summary() const;

protected:
    // Logging methods
    void log_debug(const std::string& message) const {
        if (config_.log_level <= ScoreboardLogLevel::DEBUG) {
            log_message("DEBUG", message);
        }
    }

    void log_info(const std::string& message) const {
        if (config_.log_level <= ScoreboardLogLevel::INFO) {
            log_message("INFO", message);
        }
    }

    void log_warning(const std::string& message) const {
        if (config_.log_level <= ScoreboardLogLevel::WARNING) {
            log_message("WARNING", message);
        }
    }

    void log_error(const std::string& message) const {
        if (config_.log_level <= ScoreboardLogLevel::ERROR) {
            log_message("ERROR", message);
        }
    }

    void log_fatal(const std::string& message) const {
        log_message("FATAL", message);
    }

private:
    void log_message(const std::string& level, const std::string& message) const {
        uint64_t current_cycle = ctx_->current_cycle();
        if (config_.enable_detailed_logging) {
            std::cout << "[" << level << "] [" << name_ << "] [Cycle " 
                      << current_cycle << "] " << message << std::endl;
        }
    }

    bool validate_scoreboard_config(const ScoreboardConfig& config, std::string& error_msg) const;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename DUT_TYPE, typename TXN_TYPE>
BaseScoreboard<DUT_TYPE, TXN_TYPE>::BaseScoreboard(const std::string& name, DutPtr dut, CheckerPtr checker, SimulationContextPtr ctx)
    : name_(name), dut_(dut), checker_(checker), ctx_(ctx) {
    if (!dut) {
        throw std::invalid_argument("DUT pointer cannot be null");
    }
    if (!checker_) {
        throw std::invalid_argument("Checker potiner cannot be null");
    }
    if (!ctx_) {
        throw std::invalid_argument("SimulationContext pointer cannot be null");
    }
    log_info("Scoreboard initialised.");
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::add_expected(TransactionPtr transaction, uint64_t expected_cycle) {
    if (!transaction) {
        log_error("Cannot add null expected transaction");
        return;
    }
    if (expected_queue_.size() >= config_.max_pending_transactions) {
        stats_.expected_transaction_dropped++;
        log_warning("Dropping expected transaction - expected queue is full");
        return;
    }

    uint64_t current_cycle = ctx_->current_cycle();
    PendingTransaction pending(transaction, expected_cycle, current_cycle);

    expected_queue_.push(pending);
    log_debug("Added expected transaction: " + transaction->convert2string());
    stats_.total_expected++;
    if (stats_.total_expected == 1) {
        stats_.first_transaction_time = std::chrono::high_resolution_clock::now();
    }
    stats_.last_transaction_time = std::chrono::high_resolution_clock::now();
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::add_actual(TransactionPtr transaction, uint64_t expected_cycle) {
    if (!transaction) {
        log_error("Cannot add nll expected transation");
        return;
    }
    if (actual_queue_.size() >= config_.max_pending_transactions) {
        stats_.actual_transaction_dropped++;
        log_warning("Dropping actual transaction - actual queue is full");
        return;
    }

    uint64_t current_cycle = ctx_->current_cycle();
    PendingTransaction pending(transaction, expected_cycle, current_cycle);

    actual_queue_.push(pending);
    log_debug("Added actual transaction: " + transaction->convert2string());
    stats_.total_actual++;
    stats_.last_transaction_time = std::chrono::high_resolution_clock::now();
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseScoreboard<DUT_TYPE, TXN_TYPE>::compare_transactions(PendingTransaction& expected_transaction, PendingTransaction& actual_transaction) {
    if (!expected_transaction) {
        log_error("Cannot compare null expected transaction.");
        return false;
    }
    if (!actual_transaction) {
        log_error("Cannot compare null actual transaction.");
        return false;
    }
    // Compare transaction based on their expected cycle based on each cycle 1 transaction in and out of DUT assumption
    return (expected_transaction.expected_cycle == actual_transaction.expected_cycle);
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::check_for_timeouts() {
    if (config_.enable_timeout_checking) {
        uint64_t current_cycle = ctx_->current_cycle();
        // Check whether there are timeout transactions in expected queue first, log errors and pop them
        while (!expected_queue_.empty()) {
            const auto& expected_transaction = expected_queue_.front();
            if ((current_cycle - expected_transaction.submitted_cycle) > config_.timeout_cycles) {
                handle_timeout(expected_transaction);
                expected_queue_.pop();
            }else{
                break; // Remaining transactions are newer
            }
        }

        while (!actual_queue_.empty()) {
            const auto& actual_transaction = actual_queue_.front();
            if ((current_cycle - actual_transaction.submitted_cycle) > config_.timeout_cycles) {
                handle_timeout(actual_transaction);
                actual_queue_.pop();
            }else{
                break; // Remaining transactions are newer
            }
        }
    }
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseScoreboard<DUT_TYPE, TXN_TYPE>::process_transactions() {
    uint64_t current_cycle = ctx_->current_cycle();

    // Process all transactions ready for checking
    while (!actual_queue_.empty() && !expected_queue_.empty()) {
        auto& expected_transaction = expected_queue_.front();
        auto& actual_transaction = actual_queue_.front();

        // Compare them to see whether they are a pair
        if ((actual_transaction.expected_cycle <= current_cycle) &&compare_transactions(expected_transaction)) {
            stats_.matched++;
            stats_.checks_performed++;
            // Perform the actual check with checker
            bool pass = checker_->perform_check(expected_transaction.transaction, actual_transaction.transaction);
            if (!pass && config_.stop_on_first_error) {
                log_fatal("Checker performed check and it failed. Stopping on first error.");
                return false;
            }else if (pass){
                log_debug("Checker performed check and check passed.");
                stats_.checks_passed++;
            }
        }else{
            log_debug("Expected and actual transaction did not match. No comparison was done.");
            stats_.mismatch++;
        }
        expected_queue_.pop();
        actual_queue_.pop();

        // Check whether the next transaction is timed out
        if (config_.enable_timeout_checking) {
            check_for_timeouts();
        }
    }
    return true;
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseScoreboard<DUT_TYPE, TXN_TYPE>::empty_queues() {
    log_debug("expected_queue_.size() = " + expected_queue_.size());
    log_debug("actual_queue_.size() = " + actual_queue_.size());
    return expected_queue_.empty() && actual_queue_.empty();
}

template<typename DUT_TYPE, typename TXN_TYPE>
double BaseScoreboard<DUT_TYPE, TXN_TYPE>::get_match_rate() {
    return static_cast<double>(stats_.matched / (stats_.matched + stats_.mismatch));
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseScoreboard<DUT_TYPE, TXN_TYPE>::is_passing() {
    return get_match_rate() >= config_.min_match_rate;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::update_config(const ScoreboardConfig& new_config) {
    std::string error_msg;
    if (!validate_scoreboard_config(new_config, error_msg)) {
        log_error("Cannot update config: " + error_msg);
        return;
    }
    config_ = new_config;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::reset_stats() {
    stats_.reset();
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseScoreboard<DUT_TYPE, TXN_TYPE>::handle_timeout(const PendingTransaction& pending) {
    uint64_t current_cycle = ctx_->current_cycle();
    stats_.timed_out++;
    log_error("Transaction timeout (cycle " + std::to_string(current_cycle - pending.expected_cycle) +
              "): " + pending.transaction->convert2string());
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseScoreboard<DUT_TYPE, TXN_TYPE>::validate_scoreboard_config(const ScoreboardConfig& config, std::string& error_msg) {
    if (config.max_latency_cycles == 0) {
        error_msg = "max_latency_cycles must be greater than 0";
        return false;
    }

    if (config.max_pending_transactions == 0) {
        error_msg = "max_pending_transactions must be greater than 0";
        return false;
    }

    if (config.timeout_cycles == 0) {
        error_msg = "timeout_cycles must be greater than 0";
        return false;
    }

    if (config.min_match_rate < 0.0 || config.min_match_rate > 1.0) {
        error_msg = "min_match_rate must be between 0.0 and 1.0";
        return false;
    }

    return true;
}
