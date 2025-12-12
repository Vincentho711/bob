#ifndef DUAL_PORT_RAM_TOP_SEQUENCE_H
#define DUAL_PORT_RAM_TOP_SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "dual_port_ram_sequence.h"

class DualPortRamTopSequence : public DualPortRamBaseSequence {
public:
    DualPortRamTopSequence(uint32_t addr_width, uint32_t data_width, const std::string &name = "DualPortRamTopSequence", const bool enabled_debug = true) :
        DualPortRamBaseSequence(name, enabled_debug, addr_width, data_width) {};

    simulation::Task body() override;

};
#endif // DUAL_PORT_RAM_TOP_SEQUENCE_H
