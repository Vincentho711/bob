#include "dual_port_ram_test_directed_top_sequence.h"
#include <memory>
#include <iostream>
#include <coroutine>
#include "testcases/directed/dual_port_ram_directed_testcases.h"
#include "testcases/random/dual_port_ram_random_testcases.h"

simulation::Task<> DualPortRamTestDirectedTopSequence::body() {
    co_await p_sequencer->start_sequence(
         std::make_unique<Init_Reset_Sequence>(wr_addr_width_, wr_data_width_, global_seed_));
    log_info("---------------------------------------");

    co_await p_sequencer->start_sequence(
        std::make_unique<Seq_Directed_WriteRead_All_Address_Increment>(
            wr_addr_width_, wr_data_width_, global_seed_));
    log_info("---------------------------------------");

    co_await wait_wr_cycles(10U);

    co_await p_sequencer->start_sequence(
        std::make_unique<Seq_Directed_WriteRead_All_Address_Decrement>(
            wr_addr_width_, wr_data_width_, global_seed_));
    log_info("---------------------------------------");

    // Flush pipeline to ensure that all write transactions propagate to output
    // Design has a pipeline depth of 1, wait 5 rd cycles.
    log_info("------End of DualPortRamTopSequence------");
    log_debug("Wait for 5 read cycles to flush out transactions.");
    co_await wait_rd_cycles(5U);  // pipeline flush
    log_info("---------------------------------------");
};
