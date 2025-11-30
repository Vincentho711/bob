#ifndef DUAL_PORT_RAM_DIRECTED_TESTCASES_H
#define DUAL_PORT_RAM_DIRECTED_TESTCASES_H
#include <cstdint>
#include <coroutine>
#include <string>
#include "dual_port_ram_sequence.h"
#include "simulation_task.h"

class Seq_Directed_WriteRead_All_Address : public DualPortRamBaseSequence {
private:
    uint32_t addr_width;
public:
    Seq_Directed_WriteRead_All_Address(uint32_t addr_width, const std::string &name = "Seq_Directed_WriteRead_All_Address")
        : DualPortRamBaseSequence(name, true), addr_width(addr_width) {};

    simulation::Task body() override;

};

#endif // DUAL_PORT_RAM_DIRECTED_TESTCASES_H
