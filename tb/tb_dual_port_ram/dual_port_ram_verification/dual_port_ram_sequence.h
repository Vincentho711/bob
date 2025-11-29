#ifndef DUAL_PORT_RAM_SEQUENCE_H
#define DUAL_PORT_RAM_SEQUENCE_H
#include "dual_port_ram_transaction.h"
#include "dual_port_ram_sequencer.h"
#include "sequence.h"
#include "simulation_task.h"
#include <memory>

class DualPortRamBaseSequence : public BaseSequence<DualPortRamTransaction, DualPortRamSequencer> {
public:
    using TxnType = DualPortRamTransaction;
    using TxnPtr = std::shared_ptr<TxnType>;

    [[nodiscard]]
    TxnPtr dispatch_write(uint32_t addr, uint32_t data);

    [[nodiscard]]
    TxnPtr dispatch_read(uint32_t addr);

    simulation::Task write(uint32_t addr, uint32_t data);

    simulation::Task read(uint32_t addr);

};
#endif // DUAL_PORT_RAM_SEQUENCE_H
