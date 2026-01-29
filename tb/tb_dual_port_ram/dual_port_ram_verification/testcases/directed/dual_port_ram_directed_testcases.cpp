#include "dual_port_ram_directed_testcases.h"
#include <algorithm>
#include <limits>
#include <format>

simulation::Task<> Init_Reset_Sequence::body() {
    auto init_reset_ctx = logger_.scoped_context("Init Reset Sequence");
    log_info("Starting Init Reset Sequence");
    std::vector<TxnPtr> wr_txns;
    for (uint32_t i = 0; i < 5; ++i) {
        uint32_t addr = 0U;
        uint32_t data = 0U;
        TxnPtr wr_txn = dispatch_write(addr, data);
        wr_txns.emplace_back(wr_txn);
    }
    co_await wait_all(wr_txns);
    log_info("Finished Init Reset Sequence");
}

simulation::Task<> Seq_Directed_WriteRead_All_Address_Increment::body() {
    auto wr_rd_inc_ctx = logger_.scoped_context("Write Read All Address Increment");
    log_info("Starting Write Read All Address Increment Sequence");
    for (uint32_t i = 0; i < (1U << wr_addr_width_); ++i) {
        uint32_t addr = i;
        uint32_t data = std::min(0x100U + i, static_cast<uint32_t>((1ULL << wr_data_width_) - 1));
        log_debug(std::format("Write transaction issued. addr=0x{:X}, data=0x{:X}", addr, data));
        co_await write(addr, data);
    }
    for (uint32_t i = 0 ; i < (1U << wr_addr_width_); ++i) {
        uint32_t addr = i;
        log_debug(std::format("Read transaction issued. addr=0x{:X}", addr));
        co_await read(addr);
    }
    log_info("Finished Write Read All Address Increment Sequence");
}

simulation::Task<> Seq_Directed_WriteRead_All_Address_Decrement::body() {
    auto wr_rd_dec_ctx = logger_.scoped_context("Write Read All Address Decrement");
    log_info("Starting Write Read All Address Decrement Sequence");
    for (uint32_t i = ((1U << wr_addr_width_) - 1); i != std::numeric_limits<uint32_t>::max(); --i) {
        uint32_t addr = i;
        uint32_t data = std::min(0x200U + i, static_cast<uint32_t>((1ULL << wr_data_width_) - 1));
        log_debug(std::format("Write transaction issued. addr=0x{:X}, data=0x{:X}", addr, data));
        co_await write(addr, data);
    }
    for (uint32_t i = ((1U << wr_addr_width_) - 1); i != std::numeric_limits<uint32_t>::max(); --i) {
        uint32_t addr = i;
        log_debug(std::format("Read transaction issued. addr=0x{:X}", addr));
        co_await read(addr);
    }
    log_info("Finished Write Read All Address Decrement Sequence");
}
