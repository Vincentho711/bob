#include "dual_port_ram_top_sequence.h"
#include <memory>
#include <iostream>
#include <coroutine>
#include "testcases/directed/dual_port_ram_directed_testcases.h"

// simulation::Task body() {
//     auto directed_all_address_wr_rd_seq = std::make_shared<Seq_Directed_WriteRead_All_Address>();
//     // Run directed_all_address_wr_rd_seq
//     co_await p_sequencer->start_sequence(directed_all_address_wr_rd_seq);
//     std::cout << "---------------------------------------\n";
// };
