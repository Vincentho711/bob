#ifndef DUAL_PORT_RAM_TOP_SEQUENCE_H
#define DUAL_PORT_RAM_TOP_SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "dual_port_ram_sequence.h"

class DualPortRamTopSequence : public DualPortRamBaseSequence {
public:
    DualPortRamTopSequence(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, const std::string &name = "DualPortRamTopSequence") :
        DualPortRamBaseSequence(name, addr_width, data_width, global_seed), global_seed_(global_seed) {};

    simulation::Task<> body() override;

private:
    uint64_t global_seed_;

};
#endif // DUAL_PORT_RAM_TOP_SEQUENCE_H
