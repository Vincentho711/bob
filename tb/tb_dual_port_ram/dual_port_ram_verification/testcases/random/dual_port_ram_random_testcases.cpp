#include "dual_port_ram_random_testcases.h"
#include <stdexcept>

Seq_Random_Write_Random::Seq_Random_Write_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float wr_en_rate, const std::string &name)
        : DualPortRamBaseSequence(name, true, addr_width, data_width, global_seed) {
    if (wr_en_rate < 0.0f || wr_en_rate > 1.0f) {
        throw std::out_of_range("wr_en_rate must be in [0.0, 1.0] for Seq_Random_Write_Random object.");
    }
    wr_en_rate_ = wr_en_rate;
};

Seq_Random_Write_Random::body() {

}
