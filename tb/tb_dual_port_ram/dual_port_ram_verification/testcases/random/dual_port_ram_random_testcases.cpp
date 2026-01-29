#include "dual_port_ram_random_testcases.h"
#include <stdexcept>

Seq_Random_Write_Random::Seq_Random_Write_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float wr_en_rate, uint32_t iterations, const std::string &name)
        : DualPortRamBaseSequence(name, addr_width, data_width, global_seed), iterations_(iterations) {
    if (wr_en_rate < 0.0f || wr_en_rate > 1.0f) {
        log_error("wr_en_rate:" + std::to_string(wr_en_rate) +
                  " must be >= 0.0 and <= 1.0");
        throw std::out_of_range("wr_en_rate must be in [0.0, 1.0] for Seq_Random_Write_Random object.");
    }
    wr_en_rate_ = wr_en_rate;
};

simulation::Task<> Seq_Random_Write_Random::body() {
    auto rand_wr_ctx = logger_.scoped_context("Random Write Sequence");
    log_info("Starting Random Write Sequence");
    for (uint32_t iter = 0; iter < iterations_; ++iter) {
        if (rand_prob(wr_en_rate_)) {
            uint32_t addr = rand_uint(0, (1U << wr_addr_width_) - 1);
            uint32_t data = rand_uint(0, (1ULL << wr_data_width_) - 1);
            log_debug(std::format("Write transaction issued. addr=0x{:X}, data=0x{:X}", addr, data));
            co_await write(addr, data);
        } else {
            co_await wait_wr_cycles(1U);
        }
    }
    log_info("Finished Random Write Sequence");
}

Seq_Random_Read_Random::Seq_Random_Read_Random(uint32_t addr_width, uint32_t data_width, uint64_t global_seed, float change_rate, uint32_t iterations, const std::string &name)
        : DualPortRamBaseSequence(name, addr_width, data_width, global_seed), iterations_(iterations) {
    if (change_rate < 0.0f || change_rate > 1.0f) {
        log_error("change_rate:" + std::to_string(change_rate) +
                  " must be >= 0.0 and <= 1.0");
        throw std::out_of_range("change_rate must be in [0.0, 1.0] for Seq_Random_Read_Random object.");
    }
    change_rate_ = change_rate;
};

simulation::Task<> Seq_Random_Read_Random::body() {
    auto rand_rd_ctx = logger_.scoped_context("Random Read Sequence");
    log_info("Starting Random Read Sequence");
    for (uint32_t iter = 0; iter < iterations_; ++iter) {
        if (rand_prob(change_rate_)) {
            uint32_t addr = rand_uint(0, (1U << wr_addr_width_) - 1);
            log_debug(std::format("Read transaction issued. addr=0x{:X}", addr));
            co_await read(addr);
        } else {
            co_await wait_rd_cycles(1U);
        }
    }
    log_info("Finished Random Read Sequence");
}
