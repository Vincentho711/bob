#ifndef DUAL_PORT_RAM_TEST_DEFAULT_TOP_SEQUENCE_H
#define DUAL_PORT_RAM_TEST_DEFAULT_TOP_SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "dual_port_ram_sequence.h"

class DualPortRamTestDefaultTopSequence : public DualPortRamBaseSequence {
public:
    explicit DualPortRamTestDefaultTopSequence(const std::string &name = "DualPortRamTestDefaultTopSequence")
        : DualPortRamBaseSequence(name) {}

    simulation::Task<> body() override;
};
#endif // DUAL_PORT_RAM_TEST_DEFAULT_TOP_SEQUENCE_H
