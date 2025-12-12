#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "simulation_context.h"
#include "transaction.h"
#include "checker.h"
#include <iomanip>
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
 * @brief Configuration structure for checker behavior
 */
struct ScoreboardConfig {
    uint32_t max_latency_cycles = 10;
    bool enable_out_of_order_matching = false;
    bool stop_on_first_error = true;
    bool use_checker = true;
    bool enable_timeout_checking = true;
    uint32_t timeout_cycles = 1000;
    ScoreboardLogLevel log_level = ScoreboardLogLevel::INFO;
    bool enable_detailed_logging = true;

    ScoreboardConfig() = default;
};

/**
 * @brief Base statistics structure for scoreboard
 */
struct ScoreboardStats {
    uint64_t total_expected = 0;
    uint64_t total_actual = 0;
    uint64_t matched = 0;
    uint64_t mismatch = 0;
    uint64_t timed_out = 0;
    uint64_t check_passed = 0;
    uint64_t check_failed = 0;

    ScoreboardStats() = default;
};

template<typename DUT_TYPE, typename TRANSACTION_TYPE>
class BaseScoreboard {
public:
  using DutPtr = std::shared_ptr<DUT_TYPE>;
  using TransactionPtr = std::shared_ptr<TRANSACTION_TYPE>;
  using SimulationContextPtr = std::shared_ptr<SimulationContext>;
  using CheckerPtr = std::shared_ptr<BaseChecker<DUT_TYPE, TRANSACTION_TYPE>>;
  using ScoreboardStatsPtr = std::shared_ptr<ScoreboardStats>;

  /**
    * @brief Structure to hold pending transactions with metadata
    */
  struct ExpectedTransaction {
      TransactionPtr transaction_ptr;
      uint64_t expected_cycle;
      uint64_t submitted_cycle;

      ExpectedTransaction(TransactionPtr txn_ptr, uint64_t exp_cycle, uint64_t sub_cycle)
          : transaction_ptr(txn_ptr), expected_cycle(exp_cycle), submitted_cycle(sub_cycle) {}
  };

protected:
    std::string name_;
    DutPtr dut_;
    ScoreboardConfig config_;
    ScoreboardStats stats_;
    SimulationContextPtr ctx_;
    CheckerPtr checker_;
    std::queue<ExpectedTransaction> expected_transactions_;

public:
    /**
     * @brief Construct a new BaseScoreboard
     * 
     * @param name Unique name for this scoreboard instance
     * @param dut Shared pointer to the DUT
     * @param config Configuration for scoreboard behaviour
     * @param checker A shared_ptr to a BaseChecker object
     */
    explicit BaseScoreboard(const std::string& name, DutPtr dut,
                        ScoreboardConfig config, SimulationContextPtr ctx, CheckerPtr checker)
        : name_(name), dut_(dut), config_(config), ctx_(ctx), checker_(checker) {
        if (!dut_) {
            throw std::invalid_argument("DUT pointer cannot be null");
        }
        if (!checker_) {
            throw std::invalid_argument("Checker pointer cannot be null");
        }
        log_info("BaseScoreboard '" + name_ + "' initialised");
    }

    virtual ~BaseScoreboard() = default;

    // Disable copy constructor and assignment (checkers should be unique)
    BaseScoreboard(const BaseScoreboard&) = delete;
    BaseScoreboard& operator=(const BaseScoreboard&) = delete;

    // Enable move constructor and assignment
    BaseScoreboard(BaseScoreboard&&) = default;
    BaseScoreboard& operator=(BaseScoreboard&&) = default;

    /**
     * @brief Add a transaction to be checked at a future cycle
     * 
     * @param transaction A transaction object to be checked in the future
     * @param expected_cycle The cycle when the result should checked
     */
    virtual void add_expected_transaction(TransactionPtr transaction_ptr, uint64_t expected_cycle) {
        uint64_t current_cycle = ctx_->current_cycle();
        if (!transaction_ptr) {
            log_error("Attempted to add null transaction");
            return;
        }

        // Create an ExpectedTransaction instance and add it to the expected_transactions_ queue
        expected_transactions_.emplace(transaction_ptr, expected_cycle, current_cycle);
        log_debug("Added expected transaction for cycle " + std::to_string(expected_cycle));
    }

    /**
     * @brief Check all transactions that should be ready at the current cycle
     * 
     * @return Number of checks performed
     */
    virtual std::uint32_t check_current_cycle(TransactionPtr actual_ptr) {
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
                handle_timeout(expected_txn);
            } else {
                // Perform the actual check
                bool check_result = checker_->perform_check(expected_ptr, actual_ptr);
                update_stats(check_result);
                checks_performed++;

                if (!check_result && config_.stop_on_first_error) {
                    log_fatal("Stopping on first error as configured");
                    break;
                }
            }

            expected_transactions_.pop();
        }

        return checks_performed;
    }

    /**
     * @brief Reset the scoreboard to initial state
     */
    virtual void reset() {
        while (!expected_transactions_.empty()) {
            expected_transactions_.pop();
        }
        stats_ = ScoreboardStats{};
        log_info("Scoreboard '" + name_ + "' reset");
    }

    /**
     * @brief Get a shared pointer to the ScoreboardStats object
     *
     * @return A shared pointer to the ScoreboardStats object
     */
    ScoreboardStatsPtr get_stats_ptr() {
        return std::make_shared<ScoreboardStats>(stats_);
    }

    /**
     * @brief Obtain the overall pass rate
     *
     * @return Pass rate in %
     */
    double get_pass_rate() const {
        return calculate_pass_rate();
    }

    /**
     * @brief Display the result using data from ScoreboardStats
     *
     * @param os output stream
     */
    void display_results(std::ostream& os) const {
        print_scoreboard_stats(os);
        diplay_pass_rate(os);
    }

    /**
     * @brief Get the number of expected transactions in the queue
     *
     * @return The number of expected transactions in the queue
     */
    uint32_t get_expected_transactions_queue_size() const {
        return expected_transactions_.size();
    }

protected:
    /**
     * @brief Get the DUT pointer (for derived classes)
     */
    DutPtr get_dut() const { return dut_; }

    /**
     * @brief Handle timeout situations
     * 
     * @param pending The pending transaction that timed out
     */
    virtual void handle_timeout(const ExpectedTransaction& expected) {
        std::uint64_t current_cycle = ctx_->current_cycle();
        log_error("Transaction timed out: " + expected.transaction_ptr->convert2string() +
                 " (waited " + std::to_string(current_cycle - expected.submitted_cycle) + " cycles)");
        stats_.total_expected++;
        stats_.total_actual++;
        stats_.matched++;
        stats_.timed_out++;
    }

    /**
     * @brief Update statistics based on check result
     * 
     * @param passed Whether the check passed
     */
    void update_stats(bool passed) {
        stats_.total_expected++;
        stats_.total_actual++;
        stats_.matched++;
        if (passed) {
            stats_.check_passed++;
        } else {
            stats_.check_failed++;
        }
    }

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
        std::uint64_t current_cycle = ctx_->current_cycle();
        if (config_.enable_detailed_logging) {
            std::cout << "[" << level << "] [" << name_ << "] [Cycle " 
                      << current_cycle << "] " << message << std::endl;
        }
    }

    void print_scoreboard_stats(std::ostream &os) const {
        os << "Scoreboard Stats:" << std::endl;
        os << " Total Expected: " << stats_.total_expected << std::endl;
        os << "  Total Actual:   " << stats_.total_actual << std::endl;
        os << "  Matched:        " << stats_.matched << std::endl;
        os << "  Mismatch:       " << stats_.mismatch << std::endl;
        os << "  Timed Out:      " << stats_.timed_out << std::endl;
        os << "  Check Passed:   " << stats_.check_passed << std::endl;
        os << "  Check Failed:   " << stats_.check_failed << std::endl;
    }
    void diplay_pass_rate(std::ostream &os) const {
        double pass_rate  = calculate_pass_rate();
        os << "Pass Rate: " << std::fixed << std::setprecision(2) << pass_rate << "%" << std::endl;
    }

    double calculate_pass_rate() const {
        uint64_t total_checks = stats_.check_passed + stats_.check_failed + stats_.mismatch + stats_.timed_out;
        if (total_checks == 0.0) {
            return 0.0;
        }
        return (static_cast<double>(stats_.check_passed) / total_checks) * 100.0;
    }

};

#endif
