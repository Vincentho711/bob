#ifndef DUAL_PORT_RAM_DIRECTED_TESTCASES_H
#define DUAL_PORT_RAM_DIRECTED_TESTCASES_H
#include <cstdint>
#include <coroutine>
#include <string>
#include "dual_port_ram_sequence.h"
#include "simulation_task_symmetric_transfer.h"

class Init_Reset_Sequence : public DualPortRamBaseSequence {
public:
    Init_Reset_Sequence(uint32_t addr_width, uint32_t data_width, const std::string &name = "Init_Reset_Sequence")
        : DualPortRamBaseSequence(name, true, addr_width, data_width) {}
    simulation::Task body() override;
};

class Seq_Directed_WriteRead_All_Address : public DualPortRamBaseSequence {
public:
    Seq_Directed_WriteRead_All_Address(uint32_t addr_width, uint32_t data_width, const std::string &name = "Seq_Directed_WriteRead_All_Address")
        : DualPortRamBaseSequence(name, true, addr_width, data_width) {};

    simulation::Task body() override;

};

#endif // DUAL_PORT_RAM_DIRECTED_TESTCASES_H
