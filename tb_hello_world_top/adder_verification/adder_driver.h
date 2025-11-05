#ifndef ADDER_DRIVER_H
#define ADDER_DRIVER_H

#include "driver.h"
#include "adder_transaction.h"
#include "adder_simulation_context.h"
#include "Vhello_world_top.h"
#include <vector>
#include <random>

/**
 * @brief Specialised driver for adder DUT
 * 
 * This driver handles:
 * - Adder-specific input driving (a_i, b_i)
 * - Pipeline timing considerations
 * - Input validation and constraint handling
 * - Automatic transaction generation capabilities
 */
class AdderDriver : public BaseDriver<Vhello_world_top, AdderTransaction> {
public:
    using Base = BaseDriver<Vhello_world_top, AdderTransaction>;
    using DutPtr = Base::DutPtr;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;

    /**
     * @brief Configuration for the AdderDriver
     */
    struct Config {
        bool enable_input_validation = true;
        bool enable_pipeline_tracking = true;
        uint32_t pipeline_depth = 2;
        bool auto_generate_idle_cycles = false;
        uint8_t idle_value_a = 0;
        uint8_t idle_value_b = 0;
    };

    /**
     * @brief Constructor with default configuration
     * @param name Driver instance name
     * @param dut Shared pointer to the adder DUT
     * @param ctx Adder-specific simulation context for tracking global state
     */
    explicit AdderDriver(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx);

    /**
     * @brief Constructor with custom configuration
     * @param name Driver instance name
     * @param dut Shared pointer to the adder DUT
     * @param ctx Adder-specific simulation context for tracking global state
     * @param config Driver configuration
     */
    explicit AdderDriver(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const Config& config);

    /**
     * @brief Generate and add a set of corner case transactions
     * @param name_prefix Prefix for transaction names
     */
    void generate_corner_cases(const std::string& name_prefix = "corner");

    /**
     * @brief Generate and add random transactions
     * @param count Number of random transactions to generate
     * @param rng Random number generator
     * @param name_prefix Prefix for transaction names
     */
    void generate_random_transactions(size_t count, std::mt19937& rng,
                                    const std::string& name_prefix = "random");

    /**
     * @brief Add a directed transaction with specific values
     * @param a Input A value
     * @param b Input B value
     * @param name Transaction name
     */
    void add_directed_transaction(uint8_t a, uint8_t b, const std::string& name);

    /**
     * @brief Drive idle values (useful for pipeline flushing)
     * @param cycles Number of cycles to drive idle
     */
    void drive_idle_cycles(uint32_t cycles);

    /**
     * @brief Set the values to use during idle cycles
     */
    void set_idle_values(uint8_t a, uint8_t b);

    /**
     * @brief Get current input values being driven
     */
    struct CurrentInputs {
        uint8_t a;
        uint8_t b;
        uint64_t cycle;
        bool valid;
    };
    CurrentInputs get_current_inputs() const { return current_inputs_; }

    /**
     * @brief Get configuration
     */
    const Config& get_config() const { return config_; }

    /**
     * @brief Update configuration (some settings may require reset)
     */
    void update_config(const Config& new_config);

    /**
     * @brief Reset driver and DUT inputs to known state
     */
    void reset() override;

protected:
    /**
     * @brief Drive transaction to DUT inputs
     */
    void drive_transaction(const AdderTransaction& txn) override;

    /**
     * @brief Pre-drive hook for additional processing
     */
    void pre_drive(const AdderTransaction& txn) override;

    /**
     * @brief Post-drive hook for additional processing
     */
    void post_drive(const AdderTransaction& txn) override;

private:
    AdderSimulationContextPtr ctx_;
    Config config_;
    CurrentInputs current_inputs_;

    // Statistics and tracking
    struct AdderStats {
        uint64_t corner_cases_driven = 0;
        uint64_t random_cases_driven = 0;
        uint64_t directed_cases_driven = 0;
        uint64_t idle_cycles_driven = 0;
        uint64_t validation_failures = 0;
    };
    AdderStats adder_stats_;

    // Input validation
    bool validate_inputs(uint8_t a, uint8_t b) const;
    void update_current_inputs(uint8_t a, uint8_t b);
    
    // Transaction classification
    void classify_and_update_stats(const AdderTransaction& txn);
    
    // Idle transaction creation
    std::shared_ptr<AdderTransaction> create_idle_transaction(const std::string& name);

public:
    /**
     * @brief Get adder-specific statistics
     */
    const AdderStats& get_adder_stats() const { return adder_stats_; }

    /**
     * @brief Print comprehensive driver report
     */
    void print_report() const;
};


#endif
