#ifndef MONITOR_H
#define MONITOR_H

#include <iostream>
#include <chrono>
#include "transaction.h"
#include "simulation_context.h"

/**
 * @brief Base monitor class providing common functionality for all monitors
 * 
 * This class implements the UVM monitor pattern, providing:
 * - Transaction handling and queueing
 * - Timing control and synchronization
 * - Generic interface for different DUT types
 * - RAII-based resource management
 */
template<typename DUT_TYPE, typename TXN_TYPE>
class BaseMonitor{
    static_assert(std::is_base_of_v<Transaction, TXN_TYPE>,
                  "TXN_TYPE must derive from Transaction");
public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using TransactionPtr = std::shared_ptr<TXN_TYPE>;
    using SimulationContextPtr = std::shared_ptr<SimulationContext>;

    /**
     * @brief Constructor
     * @param name Monitor instance name for debugging
     * @param dut Shared pointer to the DUT
     * @param ctx Shared pointer to the simulation context object
     */
    explicit BaseMonitor(const std::string& name, DutPtr dut, SimulationContextPtr ctx);

    virtual ~BaseMonitor() = default;

    // Disable copy semantics for RAII safety
    BaseMonitor(const BaseMonitor&) = delete;
    BaseMonitor& operator=(const BaseMonitor&) = delete;

    // Enable move semantics
    BaseMonitor(BaseMonitor&&) = default;
    BaseMonitor& operator=(BaseMonitor&&) = default;

    /**
     * @brief Get driver statistics
     */
    struct MonitorStats {
        uint64_t cycles_active = 0;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_activity;
    };

    const MonitorStats& get_stats() const { return stats_; }

    /**
     * @brief Reset monitor state
     */
    virtual void reset();

    /**
     * @brief Get monitor name
     */
    const std::string& get_name() const { return name_; }

protected:
    /**
     * @brief Pure virtual method to be implemented by derived classes for sampling the input of DUT
     */
    virtual TransactionPtr sample_input() = 0;

    /**
     * @brief Pure virtual method to be implemented by derived classes for sampling the output of DUT
     */
    virtual TransactionPtr sample_output() = 0;

    /**
     * @brief Access to DUT for derived classes
     */
    DutPtr get_dut() const { return dut_; }

    /**
     * @brief Log monitor activity
     */
    void log_info(const std::string& message) const;
    void log_error(const std::string& message) const;
    void log_debug(const std::string& message) const;

private:
    std::string name_;
    DutPtr dut_;
    SimulationContextPtr ctx_;
    MonitorStats stats_;
    bool debug_enabled_;

    void update_stats();
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename DUT_TYPE, typename TXN_TYPE>
BaseMonitor<DUT_TYPE, TXN_TYPE>::BaseMonitor(const std::string& name, DutPtr dut, SimulationContextPtr ctx)
    : name_(name), dut_(dut), ctx_(ctx), debug_enabled_(true) {
    if (!dut_) {
        throw std::invalid_argument("DUT pointer cannot be null");
    }
    stats_.start_time = std::chrono::steady_clock::now();
    log_info("Monitor initialised");
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseMonitor<DUT_TYPE, TXN_TYPE>::reset() {
    stats_.cycles_active = 0;
    stats_.start_time = std::chrono::steady_clock::now();
    stats_.last_activity = stats_.start_time;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseMonitor<DUT_TYPE, TXN_TYPE>::log_info(const std::string& message) const {
    std::cout << "[MONITOR:" << name_ << "] INFO: " << message << std::endl;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseMonitor<DUT_TYPE, TXN_TYPE>::log_error(const std::string& message) const {
    std::cout << "[MONITOR:" << name_ << "] ERROR: " << message << std::endl;
}

template<typename DUT_TYPE, typename TXN_TYPE>
void BaseMonitor<DUT_TYPE, TXN_TYPE>::log_debug(const std::string& message) const {
    if (debug_enabled_) {
        std::cout << "[MONITOR:" << name_ << "] DEBUG: " << message << std::endl;
    }
}

#endif
