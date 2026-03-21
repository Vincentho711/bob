#ifndef DUAL_PORT_RAM_TEST_DIRECTED_TOP_SEQUENCE_H
#define DUAL_PORT_RAM_TEST_DIRECTED_TOP_SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "dual_port_ram_sequence.h"

class DualPortRamTestDirectedTopSequence : public DualPortRamBaseSequence {
public:
    explicit DualPortRamTestDirectedTopSequence(const std::string &name = "DualPortRamTestDirectedTopSequence")
        : DualPortRamBaseSequence(name) {}

    simulation::Task<> body() override;
};
#endif // DUAL_PORT_RAM_TEST_DIRECTED_TOP_SEQUENCE_H
