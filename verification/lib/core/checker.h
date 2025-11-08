#ifndef CHECKER_H
#define CHECKER_H

#include "simulation_context.h"
#include "transaction.h"
#include <memory>
#include <queue>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>

/**
 * @brief Base statistics structure for checker operations
 */
struct CheckerStats {
    std::uint64_t transactions_checked = 0;
    std::uint64_t checks_passed = 0;
    std::uint64_t checks_failed = 0;
    std::uint64_t cycles_active = 0;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_activity;

    CheckerStats() : start_time(std::chrono::steady_clock::now()), 
                     last_activity(start_time) {}
};

/**
 * @brief Enumeration for checker log levels
 */
enum class CheckerLogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Configuration structure for checker behavior
 */
struct CheckerConfig {
    bool enable_detailed_logging = true;
    bool stop_on_first_error = false;
    bool enable_timeout_checking = true;
    std::uint32_t timeout_cycles = 1000;
    CheckerLogLevel log_level = CheckerLogLevel::INFO;
    bool enable_statistics = true;

    CheckerConfig() = default;
};

/**
 * @brief Template base class for all checkers
 * 
 * @tparam DUT_TYPE The Verilated DUT type
 * @tparam TRANSACTION_TYPE The specific transaction type to check
 * 
 * This class provides the common infrastructure for checking DUT outputs
 * against expected results. It includes transaction queuing, statistics
 * tracking, logging, and extensible checking mechanisms.
 */
template<typename DUT_TYPE, typename TRANSACTION_TYPE>
class BaseChecker {
public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using TransactionPtr = std::shared_ptr<TRANSACTION_TYPE>;
    using SimulationContextPtr = std::shared_ptr<SimulationContext>;
    using CheckFunction = std::function<bool(const TRANSACTION_TYPE&, const DUT_TYPE&)>;

    /**
     * @brief Structure to hold pending transactions with metadata
     */
    struct PendingTransaction {
        TransactionPtr transaction;
        std::uint64_t expected_cycle;
        std::uint64_t submitted_cycle;

        PendingTransaction(TransactionPtr txn, std::uint64_t exp_cycle, std::uint64_t sub_cycle)
            : transaction(txn), expected_cycle(exp_cycle), submitted_cycle(sub_cycle) {}
    };

protected:
    std::string name_;
    DutPtr dut_;
    CheckerConfig config_;
    CheckerStats stats_;
    SimulationContextPtr ctx_;
    std::queue<PendingTransaction> pending_transactions_;
    std::unordered_map<std::string, CheckFunction> custom_checks_;

public:
    /**
     * @brief Construct a new BaseChecker
     * 
     * @param name Unique name for this checker instance
     * @param dut Shared pointer to the DUT
     * @param config Configuration for checker behavior
     */
    explicit BaseChecker(const std::string& name, DutPtr dut, 
                        SimulationContextPtr ctx, const CheckerConfig& config = CheckerConfig{})
        : name_(name), dut_(dut), config_(config), ctx_(ctx) {
        if (!dut_) {
            throw std::invalid_argument("DUT pointer cannot be null");
        }
        log_info("BaseChecker '" + name_ + "' initialized");
    }

    virtual ~BaseChecker() = default;

    // Disable copy constructor and assignment (checkers should be unique)
    BaseChecker(const BaseChecker&) = delete;
    BaseChecker& operator=(const BaseChecker&) = delete;

    // Enable move constructor and assignment
    BaseChecker(BaseChecker&&) = default;
    BaseChecker& operator=(BaseChecker&&) = default;

    /**
     * @brief Add a transaction to be checked at a future cycle
     * 
     * @param transaction The transaction to check
     * @param expected_cycle The cycle when the result should be available
     */
    void add_expected_transaction(TransactionPtr transaction, uint64_t expected_cycle) {
        uint64_t current_cycle = ctx_->current_cycle();
        if (!transaction) {
            log_error("Attempted to add null transaction");
            return;
        }

        pending_transactions_.emplace(transaction, expected_cycle, current_cycle);
        log_debug("Added expected transaction for cycle " + std::to_string(expected_cycle));
    }

    /**
     * @brief Check all transactions that should be ready at the current cycle
     * 
     * @return Number of checks performed
     */
    virtual std::uint32_t check_cycle() {
        /*
        std::uint64_t current_cycle = ctx_->current_cycle();
        std::uint32_t checks_performed = 0;
        stats_.last_activity = std::chrono::steady_clock::now();

        // Process all transactions ready for checking
        while (!pending_transactions_.empty() && 
               pending_transactions_.front().expected_cycle <= current_cycle) {

            auto& pending = pending_transactions_.front();

            // Check for timeout
            if (config_.enable_timeout_checking && 
                (current_cycle - pending.submitted_cycle) > config_.timeout_cycles) {
                handle_timeout(pending);
            } else {
                // Perform the actual check
                bool check_result = perform_check(*pending.transaction);
                update_stats(check_result);
                checks_performed++;

                if (!check_result && config_.stop_on_first_error) {
                    log_fatal("Stopping on first error as configured");
                    break;
                }
            }

            pending_transactions_.pop();
        }

        stats_.cycles_active++;

        return checks_performed;
        */
        return 0U;
    }

    /**
     * @brief Reset the checker to initial state
     */
    virtual void reset() {
        while (!pending_transactions_.empty()) {
            pending_transactions_.pop();
        }
        stats_ = CheckerStats{};
        log_info("Checker '" + name_ + "' reset");
    }

    /**
     * @brief Pure virtual function to perform the actual checking
     * Must be implemented by derived classes
     * 
     * @param expected_txn A pointer to the expected transaction object
     * @param actual_txn A pointer to the actual transaction object
     * @return true if check passed, false otherwise
     */
    virtual bool perform_check(TransactionPtr expected_txn, TransactionPtr actual_txn) = 0;

    /**
     * @brief Add a custom check function
     * 
     * @param name Name of the custom check
     * @param check_func Function to perform the check
     */
    void add_custom_check(const std::string& name, CheckFunction check_func) {
        custom_checks_[name] = check_func;
        log_info("Added custom check: " + name);
    }

    /**
     * @brief Remove a custom check function
     * 
     * @param name Name of the custom check to remove
     */
    void remove_custom_check(const std::string& name) {
        auto it = custom_checks_.find(name);
        if (it != custom_checks_.end()) {
            custom_checks_.erase(it);
            log_info("Removed custom check: " + name);
        }
    }

    // Getters
    const std::string& get_name() const { return name_; }
    const CheckerStats& get_stats() const { return stats_; }
    const CheckerConfig& get_config() const { return config_; }
    std::uint64_t get_current_cycle() const { return ctx_->current_cycle(); }
    size_t pending_count() const { return pending_transactions_.size(); }
    bool has_failures() const { return stats_.checks_failed > 0; }
    double get_pass_rate() const { 
        return stats_.transactions_checked > 0 ? 
               static_cast<double>(stats_.checks_passed) / stats_.transactions_checked : 0.0; 
    }

    // Setters
    void update_config(const CheckerConfig& new_config) { 
        config_ = new_config; 
        log_info("Configuration updated");
    }

    /**
     * @brief Print a comprehensive report of checker activity
     */
    virtual void print_report() const {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "CHECKER REPORT: " << name_ << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            stats_.last_activity - stats_.start_time);

        std::cout << "Statistics:" << std::endl;
        std::cout << "  Transactions Checked: " << stats_.transactions_checked << std::endl;
        std::cout << "  Checks Passed: " << stats_.checks_passed << std::endl;
        std::cout << "  Checks Failed: " << stats_.checks_failed << std::endl;
        std::cout << "  Pass Rate: " << std::fixed << std::setprecision(2) 
                  << (get_pass_rate() * 100.0) << "%" << std::endl;
        std::cout << "  Active Cycles: " << stats_.cycles_active << std::endl;
        std::cout << "  Runtime: " << duration.count() << " ms" << std::endl;
        std::cout << "  Pending: " << pending_transactions_.size() << " transactions" << std::endl;

        std::cout << "\nConfiguration:" << std::endl;
        std::cout << "  Detailed Logging: " << (config_.enable_detailed_logging ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  Stop on Error: " << (config_.stop_on_first_error ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  Timeout Checking: " << (config_.enable_timeout_checking ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  Timeout Cycles: " << config_.timeout_cycles << std::endl;

        std::cout << std::string(50, '=') << std::endl;
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
    virtual void handle_timeout(const PendingTransaction& pending) {
        std::uint64_t current_cycle = ctx_->current_cycle();
        log_error("Transaction timed out: " + pending.transaction->convert2string() +
                 " (waited " + std::to_string(current_cycle - pending.submitted_cycle) + " cycles)");
        stats_.checks_failed++;
        stats_.transactions_checked++;
    }

    /**
     * @brief Update statistics based on check result
     * 
     * @param passed Whether the check passed
     */
    void update_stats(bool passed) {
        stats_.transactions_checked++;
        if (passed) {
            stats_.checks_passed++;
        } else {
            stats_.checks_failed++;
        }
    }

    /**
     * @brief Execute all custom checks
     * 
     * @param transaction The transaction to check
     * @return true if all custom checks passed
     */
    bool execute_custom_checks(const TRANSACTION_TYPE& transaction) {
        bool all_passed = true;
        for (const auto& [name, check_func] : custom_checks_) {
            try {
                bool result = check_func(transaction, *dut_);
                if (!result) {
                    log_error("Custom check '" + name + "' failed for transaction: " + 
                             transaction.convert2string());
                    all_passed = false;
                }
            } catch (const std::exception& e) {
                log_error("Custom check '" + name + "' threw exception: " + e.what());
                all_passed = false;
            }
        }
        return all_passed;
    }

    // Logging methods
    void log_debug(const std::string& message) const {
        if (config_.log_level <= CheckerLogLevel::DEBUG) {
            log_message("DEBUG", message);
        }
    }
    
    void log_info(const std::string& message) const {
        if (config_.log_level <= CheckerLogLevel::INFO) {
            log_message("INFO", message);
        }
    }
    
    void log_warning(const std::string& message) const {
        if (config_.log_level <= CheckerLogLevel::WARNING) {
            log_message("WARNING", message);
        }
    }
    
    void log_error(const std::string& message) const {
        if (config_.log_level <= CheckerLogLevel::ERROR) {
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
};

#endif
