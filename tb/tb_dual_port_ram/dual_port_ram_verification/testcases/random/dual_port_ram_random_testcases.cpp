#include "dual_port_ram_random_testcases.h"
#include <stdexcept>

Seq_Random_Write_Random::Seq_Random_Write_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float wr_en_rate, uint32_t iterations, const std::string &name)
        : DualPortRamBaseSequence(name, true, addr_width, data_width, global_seed), iterations_(iterations) {
    if (wr_en_rate < 0.0f || wr_en_rate > 1.0f) {
        throw std::out_of_range("wr_en_rate must be in [0.0, 1.0] for Seq_Random_Write_Random object.");
    }
    wr_en_rate_ = wr_en_rate;
};

simulation::Task Seq_Random_Write_Random::body() {
    log_info("Starting Seq_Random_Write_Random sequence.");
    for (uint32_t iter = 0; iter < iterations_; ++iter) {
        if (rand_prob(wr_en_rate_)) {
            uint32_t addr = rand_uint(0, (1U << wr_addr_width_) - 1);
            uint32_t data = rand_uint(0, (1ULL << wr_data_width_) - 1);
            log_debug("Write transaction issued. addr = " + std::to_string(addr) + " , data = " + std::to_string(data));
            co_await write(addr, data);
        } else {
            co_await wait_wr_cycles(1U);
        }
    }
    log_info("Finished Seq_Random_Write_Random sequence.");
}

Seq_Random_Read_Random::Seq_Random_Read_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float change_rate, uint32_t iterations, const std::string &name)
        : DualPortRamBaseSequence(name, true, addr_width, data_width, global_seed), iterations_(iterations) {
    if (change_rate < 0.0f || change_rate > 1.0f) {
        throw std::out_of_range("change_rate must be in [0.0, 1.0] for Seq_Random_Read_Random object.");
    }
    change_rate_ = change_rate;
};

simulation::Task Seq_Random_Read_Random::body() {
    log_info("Starting Seq_Random_Read_Random sequence.");
    for (uint32_t iter = 0; iter < iterations_; ++iter) {
        if (rand_prob(change_rate_)) {
            uint32_t addr = rand_uint(0, (1U << wr_addr_width_) - 1);
            log_debug("Read transaction issued. addr = " + std::to_string(addr));
            co_await read(addr);
        } else {
            co_await wait_rd_cycles(1U);
        }
    }
    log_info("Finished Seq_Random_Read_Random sequence.");
}
