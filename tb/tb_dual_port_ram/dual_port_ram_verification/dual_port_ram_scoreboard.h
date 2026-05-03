#ifndef DUAL_PORT_RAM_SCOREBOARD_H
#define DUAL_PORT_RAM_SCOREBOARD_H
#include <map>
#include <memory>
#include <deque>
#include <string>
#include <vector>
#include "scoreboard.h"
#include "simulation_clock.h"
#include "simulation_task_symmetric_transfer.h"
#include "Vdual_port_ram.h"
#include "dual_port_ram_transaction.h"
#include "dual_port_ram_tlm_queue.h"
#include "covergroup.h"

class DualPortRamScoreboard : public BaseScoreboard<Vdual_port_ram, DualPortRamTransaction> {
public:
    using ClockT = simulation::Clock<Vdual_port_ram>;
    using TxnPtr = std::shared_ptr<DualPortRamTransaction>;
    explicit DualPortRamScoreboard(
        std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
        std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
        std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle,
        std::shared_ptr<Covergroup> cg = nullptr,
        const std::string &name = "DualPortRamScoreboard"
    );

    simulation::Task<> run_phase() override;

    simulation::Task<> update_ram_model();

    simulation::Task<> run_write_capture();

    simulation::Task<> run_read_capture();

protected:
    std::map<uint32_t, uint32_t> ram_model_;
private:
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue_;
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue_;
    std::shared_ptr<ClockT>     wr_clk_;
    uint32_t                    wr_delay_cycle_;
    uint32_t                    apply_index_;
    uint32_t                    circular_buffer_size_;
    std::vector<std::deque<TxnPtr>> circular_buffer_;
    std::shared_ptr<Covergroup> cg_;
    uint64_t                    last_rd_addr_ = 0;
};

#endif // DUAL_PORT_RAM_SCOREBOARD_H


