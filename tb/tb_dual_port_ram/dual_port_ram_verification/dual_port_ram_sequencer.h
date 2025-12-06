#ifndef DUAL_PORT_RAM_SEQUENCER_H
#define DUAL_PORT_RAM_SEQUENCER_H
#include <deque>
#include <memory>
#include "simulation_clock.h"
#include "dual_port_ram_transaction.h"
#include "sequencer.h"
#include "Vdual_port_ram.h"

class DualPortRamSequencer : public BaseSequencer<DualPortRamTransaction> {
public:
    using TransactionType = DualPortRamTransaction;
    using TxnPtr = std::shared_ptr<TransactionType>;
    using ClockT = simulation::Clock<Vdual_port_ram>;
    std::deque<TxnPtr> write_queue;
    std::deque<TxnPtr> read_queue;

    // Resources available to all sequences running on this sequencer
    std::shared_ptr<ClockT> wr_clk;

    DualPortRamSequencer(std::shared_ptr<ClockT> wr_clk)
        : wr_clk(wr_clk) {}
};

#endif // DUAL_PORT_RAM_SEQUENCER_H
