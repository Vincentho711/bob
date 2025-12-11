#ifndef DUAL_PORT_RAM_SCOREBOARD_H
#define DUAL_PORT_RAM_SCOREBOARD_H
#include <map>
#include <memory>
#include "scoreboard.h"
#include "simulation_clock.h"
#include "Vdual_port_ram.h"
#include "dual_port_ram_transaction.h"
#include "dual_port_ram_tlm_queue.h"

class DualPortRamScoreboard : public BaseScoreboard<DualPortRamTransaction> {
public:
    using ClockT = simulation::Clock<Vdual_port_ram>;
    using TxnPtr = std::shared_ptr<DualPortRamTransaction>;
    explicit DualPortRamScoreboard(std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue, std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue, std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle, const std::string &name, bool debug_enabled){};

    void update_ram_model(const TxnPtr &write_txn);

protected:
    std::map<uint32_t, uint32_t> ram_model_;
private:
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue;
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue;
    uint32_t wr_delay_cycle;
};

#endif // DUAL_PORT_RAM_SCOREBOARD_H


