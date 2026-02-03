#include "dual_port_ram_top_sequence.h"
#include <memory>
#include <iostream>
#include <coroutine>
#include "testcases/directed/dual_port_ram_directed_testcases.h"
#include "testcases/random/dual_port_ram_random_testcases.h"

simulation::Task<> DualPortRamTopSequence::body() {
    auto init_reset_seq = std::make_unique<Init_Reset_Sequence>(wr_addr_width_, wr_addr_width_, global_seed_);

    co_await p_sequencer->start_sequence(std::move(init_reset_seq));

    auto directed_all_address_increment_wr_rd_seq = std::make_unique<Seq_Directed_WriteRead_All_Address_Increment>(wr_addr_width_, wr_data_width_, global_seed_);
    // Run directed_all_address_wr_rd_seq
    co_await p_sequencer->start_sequence(std::move(directed_all_address_increment_wr_rd_seq));
    log_info("---------------------------------------");

    co_await wait_wr_cycles(10U);

    directed_all_address_increment_wr_rd_seq = std::make_unique<Seq_Directed_WriteRead_All_Address_Increment>(wr_addr_width_, wr_data_width_, global_seed_);
    co_await p_sequencer->start_sequence(std::move(directed_all_address_increment_wr_rd_seq));
    log_info("---------------------------------------");

    co_await wait_wr_cycles(5U);

    auto directed_all_address_decrement_wr_rd_seq = std::make_unique<Seq_Directed_WriteRead_All_Address_Decrement>(wr_addr_width_, wr_data_width_, global_seed_);
    co_await p_sequencer->start_sequence(std::move(directed_all_address_decrement_wr_rd_seq));
    log_info("---------------------------------------");

    auto random_write_random_seq = std::make_unique<Seq_Random_Write_Random>(wr_addr_width_, wr_data_width_, global_seed_, 0.5f, 500);
    co_await p_sequencer->start_sequence(std::move(random_write_random_seq));
    log_info("---------------------------------------");

    auto random_read_random_seq = std::make_unique<Seq_Random_Read_Random>(wr_addr_width_, wr_data_width_, global_seed_, 0.5f, 500);
    co_await p_sequencer->start_sequence(std::move(random_read_random_seq));
    log_info("---------------------------------------");

    auto random_write_read_random_seq = std::make_unique<Seq_Random_Write_Read_Random>(wr_addr_width_, wr_data_width_, global_seed_, 0.9f, 0.8f, 1000);
    co_await p_sequencer->start_sequence(std::move(random_write_read_random_seq));
    log_info("---------------------------------------");
};
