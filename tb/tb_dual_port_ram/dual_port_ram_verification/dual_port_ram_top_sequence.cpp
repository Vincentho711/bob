#include "dual_port_ram_top_sequence.h"
#include "Vdual_port_ram_dual_port_ram.h"
#include <memory>
#include <iostream>
#include <coroutine>
#include "testcases/directed/dual_port_ram_directed_testcases.h"

simulation::Task DualPortRamTopSequence::body() {
    auto directed_all_address_wr_rd_seq = std::make_unique<Seq_Directed_WriteRead_All_Address>(addr_width_);
    // Run directed_all_address_wr_rd_seq
    co_await p_sequencer->start_sequence(std::move(directed_all_address_wr_rd_seq));
    std::cout << "---------------------------------------\n";
};
