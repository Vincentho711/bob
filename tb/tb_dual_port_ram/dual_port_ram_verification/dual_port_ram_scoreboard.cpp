#include "dual_port_ram_scoreboard.h"

DualPortRamScoreboard::DualPortRamScoreboard(
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle,
    const std::string &name, bool debug_enabled)
    : tlm_wr_queue(tlm_wr_queue), tlm_rd_queue(tlm_rd_queue),
      wr_delay_cycle(wr_delay_cycle), wr_clk(wr_clk),
      BaseScoreboard(name, debug_enabled) {}

void DualPortRamScoreboard::update_ram_model(const TxnPtr &write_txn) {
    

}
