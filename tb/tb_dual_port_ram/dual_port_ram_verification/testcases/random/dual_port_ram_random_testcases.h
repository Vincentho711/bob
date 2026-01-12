#ifndef DUAL_PORT_RAM_RANDOM_TESTCASES_H
#define DUAL_PORT_RAM_RANDOM_TESTCASES_H
#include <string>
#include "simulation_task_symmetric_transfer.h"
#include "dual_port_ram_sequence.h"

class Seq_Random_Write_Random : public DualPortRamBaseSequence {
public:
    explicit Seq_Random_Write_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float wr_en_rate, const std::string &name = "Seq_Directed_WriteRead_All_Address_Increment");

    simulation::Task body() override;
protected:
    float wr_en_rate_;
};
#endif // DUAL_PORT_RAM_RANDOM_TESTCASES_H
