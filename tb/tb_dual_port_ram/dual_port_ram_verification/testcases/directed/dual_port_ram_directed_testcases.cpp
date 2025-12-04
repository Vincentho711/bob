#include "dual_port_ram_directed_testcases.h"

simulation::Task Seq_Directed_WriteRead_All_Address::body() {
    log_info("Starting Seq_Directed_WriteRead_All_Address sequence.");
    for (uint32_t i = 0; i < (2 << addr_width); ++i) {
        uint32_t addr = i;
        uint32_t data = 0x100U + i;
        log_debug("Write transaction issued. addr = " + std::to_string(addr) + " , data = " + std::to_string(data));
        co_await write(addr, data);
    }
    for (uint32_t i = 0 ; i < (2 << addr_width); ++i) {
        uint32_t addr = i;
        log_debug("Read transaction issued. addr = " + std::to_string(addr));
        co_await read(addr);
    }
    log_info("Finished Seq_Directed_WriteRead_All_Address sequence.");
}
