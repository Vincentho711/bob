#ifndef DRIVER_H
#define DRIVER_H

#include <queue>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include "transaction.h"

/**
 * @brief Base driver class providing common functionality for all drivers
 * 
 * This class implements the UVM driver pattern, providing:
 * - Transaction handling and queueing
 * - Timing control and synchronization
 * - Generic interface for different DUT types
 * - RAII-based resource management
 */
template<typename DUT_TYPE, typename TXN_TYPE>
class BaseDriver {
    static_assert(std::is_base_of_v<Transaction, TXN_TYPE>, 
                  "TXN_TYPE must derive from Transaction");

public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using TransactionPtr = std::shared_ptr<TXN_TYPE>;
    using ClockCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param name Driver instance name for debugging
     * @param dut Shared pointer to the DUT
     */
    explicit BaseDriver(const std::string& name, DutPtr dut);
    
    virtual ~BaseDriver() = default;

    // Disable copy semantics for RAII safety
    BaseDriver(const BaseDriver&) = delete;
    BaseDriver& operator=(const BaseDriver&) = delete;

    // Enable move semantics
    BaseDriver(BaseDriver&&) = default;
    BaseDriver& operator=(BaseDriver&&) = default;

    /**
     * @brief Add a transaction to the driver queue
     * @param txn Unique pointer to transaction
     */
    void add_transaction(TransactionPtr txn);

    /**
     * @brief Retrieve the next transaction from the driver queue without removing it
     * @return A pointer to the transaction at the front of the queue
     * @throws std::runtime_error if the transaction queue is empty
     */
    TransactionPtr get_next_transaction();

    /**
     * @brief Drive the next transaction in the queue
     * @param cycle Current simulation cycle
     * @return true if transaction was driven, false if queue empty
     */
    bool drive_next(uint64_t cycle);

    /**
     * @brief Check if there are pending transactions
     */
    bool has_pending_transactions() const;

    /**
     * @brief Get the count of pending transactions
     */
    size_t pending_count() const;

    /**
     * @brief Clear all pending transactions
     */
    void clear_transactions();

    /**
     * @brief Set callback for clock edge handling
     */
    void set_clock_callback(ClockCallback callback);

    /**
     * @brief Get driver statistics
     */
    struct DriverStats {
        uint64_t transactions_driven = 0;
        uint64_t cycles_active = 0;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_activity;
    };

    const DriverStats& get_stats() const { return stats_; }

    /**
     * @brief Reset driver state
     */
    virtual void reset();

    /**
     * @brief Get driver name
     */
    const std::string& get_name() const { return name_; }

protected:
    /**
     * @brief Pure virtual method to be implemented by derived classes
     * @param txn Transaction to drive
     * @param cycle Current cycle
     */
    virtual void drive_transaction(const TXN_TYPE& txn, uint64_t cycle) = 0;

    /**
     * @brief Virtual method called before driving a transaction
     * @param txn Transaction about to be driven
     * @param cycle Current cycle
     */
    virtual void pre_drive(const TXN_TYPE& txn, uint64_t cycle) {
        (void)txn;
        (void)cycle;
        // default empty implementation
    }

    /**
     * @brief Virtual method called after driving a transaction
     * @param txn Transaction that was driven
     * @param cycle Current cycle
     */
    virtual void post_drive(const TXN_TYPE& txn, uint64_t cycle) {
        (void)txn;
        (void)cycle;
        // default empty implementation
    }

    /**
     * @brief Access to DUT for derived classes
     */
    DutPtr get_dut() const { return dut_; }

    /**
     * @brief Log driver activity
     */
    void log_info(const std::string& message) const;
    void log_error(const std::string& message) const;
    void log_debug(const std::string& message) const;

private:
    std::string name_;
    DutPtr dut_;
    std::queue<TransactionPtr> transaction_queue_;
    ClockCallback clock_callback_;
    DriverStats stats_;
    bool debug_enabled_;

    void update_stats();
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename DUT_TYPE, typename TXN_TYPE>
BaseDriver<DUT_TYPE, TXN_TYPE>::BaseDriver(const std::string& name, DutPtr dut)
    : name_(name), dut_(std::move(dut)), debug_enabled_(false) {
    if (!dut_) {
        throw std::invalid_argument("DUT pointer cannot be null");
    }
    stats_.start_time = std::chrono::steady_clock::now();
    log_info("Driver initialized");
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::add_transaction(TransactionPtr txn) {
    if (!txn) {
        log_error("Cannot add null transaction");
        return;
    }
    
    log_debug("Added transaction: " + txn->convert2string());
    transaction_queue_.push(std::move(txn));
}

template<typename DUT_TYPE, typename TXN_TYPE>
typename BaseDriver<DUT_TYPE, TXN_TYPE>::TransactionPtr BaseDriver<DUT_TYPE, TXN_TYPE>::get_next_transaction() {
    if (transaction_queue_.empty()) {
        throw std::runtime_error("Transaction queue is empty.");
    }
    return transaction_queue_.front();
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseDriver<DUT_TYPE, TXN_TYPE>::drive_next(uint64_t cycle) {
    if (transaction_queue_.empty()) {
        return false;
    }

    auto txn = transaction_queue_.front();
    transaction_queue_.pop();

    try {
        pre_drive(*txn, cycle);
        drive_transaction(*txn, cycle);
        post_drive(*txn, cycle);
        
        update_stats();
        log_debug("Driven transaction at cycle " + std::to_string(cycle) + 
                 ": " + txn->convert2string());
        
        // Execute clock callback if set
        if (clock_callback_) {
            clock_callback_();
        }
        
        return true;
    } catch (const std::exception& e) {
        log_error("Error driving transaction: " + std::string(e.what()));
        return false;
    }
}

template<typename DUT_TYPE, typename TXN_TYPE>
bool BaseDriver<DUT_TYPE, TXN_TYPE>::has_pending_transactions() const {
    return !transaction_queue_.empty();
}

template<typename DUT_TYPE, typename TXN_TYPE>
size_t BaseDriver<DUT_TYPE, TXN_TYPE>::pending_count() const {
    return transaction_queue_.size();
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::clear_transactions() {
    while (!transaction_queue_.empty()) {
        transaction_queue_.pop();
    }
    log_info("Cleared all pending transactions");
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::set_clock_callback(ClockCallback callback) {
    clock_callback_ = std::move(callback);
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::reset() {
    clear_transactions();
    stats_ = DriverStats{};
    stats_.start_time = std::chrono::steady_clock::now();
    log_info("Driver reset");
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::log_info(const std::string& message) const {
    std::cout << "[DRIVER:" << name_ << "] INFO: " << message << std::endl;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::log_error(const std::string& message) const {
    std::cout << "[DRIVER:" << name_ << "] ERROR: " << message << std::endl;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::log_debug(const std::string& message) const {
    if (debug_enabled_) {
        std::cout << "[DRIVER:" << name_ << "] DEBUG: " << message << std::endl;
    }
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseDriver<DUT_TYPE, TXN_TYPE>::update_stats() {
    stats_.transactions_driven++;
    stats_.cycles_active++;
    stats_.last_activity = std::chrono::steady_clock::now();
}

#endif
