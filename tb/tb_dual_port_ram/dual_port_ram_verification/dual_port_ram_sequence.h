#ifndef DUAL_PORT_RAM_SEQUENCE_H
#define DUAL_PORT_RAM_SEQUENCE_H
#include "dual_port_ram_transaction.h"
#include "dual_port_ram_sequencer.h"
#include "sequence.h"
#include "simulation_task_symmetric_transfer.h"
#include "Vdual_port_ram.h"
#include <memory>

class DualPortRamBaseSequence : public BaseSequence<Vdual_port_ram, DualPortRamTransaction, DualPortRamSequencer> {
public:
    using TxnType = DualPortRamTransaction;
    using TxnPtr = std::shared_ptr<TxnType>;

    explicit DualPortRamBaseSequence(uint32_t wr_addr_width, uint32_t wr_data_width, uint64_t global_seed, const std::string& name);

    [[nodiscard]]
    TxnPtr dispatch_write(uint32_t addr, uint32_t data);

    [[nodiscard]]
    TxnPtr dispatch_read(uint32_t addr);

    simulation::Task<> write(uint32_t addr, uint32_t data);

    simulation::Task<> read(uint32_t addr);

    simulation::Task<> wait_wr_cycles(uint32_t n);

    simulation::Task<> wait_rd_cycles(uint32_t n);

protected:
    uint32_t wr_addr_width_;
    uint32_t wr_data_width_;
};
#endif // DUAL_PORT_RAM_SEQUENCE_H
