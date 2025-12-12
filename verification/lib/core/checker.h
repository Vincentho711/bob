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
    CheckerLogLevel log_level = CheckerLogLevel::INFO;

    CheckerConfig() = default;
};

inline std::ostream& operator<<(std::ostream& os, CheckerLogLevel level) {
    switch (level) {
        case CheckerLogLevel::DEBUG:   return os << "DEBUG";
        case CheckerLogLevel::INFO:    return os << "INFO";
        case CheckerLogLevel::WARNING: return os << "WARNING";
        case CheckerLogLevel::ERROR:   return os << "ERROR";
        default:                       return os << "UNKNOWN";
    }
}

/**
 * @brief Template base class for all checkers
 * 
 * @tparam DUT_TYPE The Verilated DUT type
 * @tparam TRANSACTION_TYPE The specific transaction type to check
 * 
 * This class provides the common infrastructure for checking DUT outputs
 * against expected results.
 */
template<typename DUT_TYPE, typename TRANSACTION_TYPE>
class BaseChecker {
public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using TransactionPtr = std::shared_ptr<TRANSACTION_TYPE>;
    using SimulationContextPtr = std::shared_ptr<SimulationContext>;

protected:
    std::string name_;
    DutPtr dut_;
    CheckerConfig config_;
    SimulationContextPtr ctx_;

public:
    /**
     * @brief Construct a new BaseChecker
     * 
     * @param name Unique name for this checker instance
     * @param dut Shared pointer to the DUT
     * @param config Configuration for checker behaviour
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
     * @brief Reset the checker to initial state
     */
    virtual void reset() {
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

    // Getters
    const std::string& get_name() const { return name_; }
    const CheckerConfig& get_config() const { return config_; }

    // Setters
    void update_config(const CheckerConfig& new_config) {
        config_ = new_config;
        log_info("Configuration updated");
    }

protected:
    /**
     * @brief Get the DUT pointer (for derived classes)
     */
    DutPtr get_dut() const { return dut_; }

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
        std::cout << "[" << level << "] [" << name_ << "] [Cycle "
                  << current_cycle << "] " << message << std::endl;
    }
};

#endif
