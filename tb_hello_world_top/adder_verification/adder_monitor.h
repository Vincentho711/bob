#ifndef ADDER_MONITOR_H
#define ADDER_MONITOR_H

#include "monitor.h"
#include "adder_simulation_context.h"
#include "adder_transaction.h"
#include "Vhello_world_top.h"


/**
 * @brief Specialized monitor for adder DUT
 * 
 * This driver handles:
 * - Adder-specific input driving (a_i, b_i)
 * - Pipeline timing considerations
 * - Input validation and constraint handling
 * - Automatic transaction generation capabilities
 */
class AdderMonitor : public BaseMonitor<Vhello_world_top, AdderTransaction> {
public:
    using Base = BaseMonitor<Vhello_world_top, AdderTransaction>;
    using DutPtr = Base::DutPtr;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;
    using AdderTransactionPtr = std::shared_ptr<AdderTransaction>;

    /**
     * @brief Constructor with default configuration
     * @param name Monitor instance name
     * @param dut Shared pointer to the adder DUT
     * @param ctx Adder-specific simulation context for tracking global state
     */
    explicit AdderMonitor(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx);

    /**
     * @brief Sample the output of the adder DUT
     */
    AdderTransactionPtr sample_output() override;
protected:
    /**
     * @brief Reset the stats of monitor
     */
    void reset() override;

private:
    AdderSimulationContextPtr ctx_;
};

#endif // ADDER_MONITOR_H
