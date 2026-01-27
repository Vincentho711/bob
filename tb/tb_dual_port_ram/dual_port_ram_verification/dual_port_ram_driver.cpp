#include "dual_port_ram_driver.h"

DualPortRamDriver::DualPortRamDriver(
    std::shared_ptr<DualPortRamSequencer> sequencer,
    std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<clock_t> wr_clk,
    std::shared_ptr<clock_t> rd_clk,
    const std::string &name,
    bool debug_enabled)
    : BaseDriver(sequencer, dut, name, debug_enabled), wr_clk(wr_clk), rd_clk(rd_clk) {}

simulation::Task<> DualPortRamDriver::run() {
    co_return;
}

simulation::Task<> DualPortRamDriver::wr_driver_run() {
    while (true) {
        log_debug("wr_driver_run() started.");
        // Wait for wr_clk's rising edge
        co_await wr_clk->rising_edge(simulation::Phase::Drive);
        log_debug("wr_clk->rising_edge(Drive) seen within wr_driver_run().");
        TxnPtr active_wr = nullptr;

        if (!p_sequencer->write_queue.empty()) {
            log_debug("p_sequencer->write_queue.empty() = 0. Fetch the top transaction.");
            active_wr = p_sequencer->write_queue.front();
            p_sequencer->write_queue.pop_front();

            log_debug("Driving dut->wr_en_i = 1.");
            log_debug("[TXN:"+ std::to_string(active_wr->txn_id) +"] " + "Driving dut->wr_addr_i = " + std::to_string(active_wr->payload.addr) + " , dut->wr_data_i = " + std::to_string(active_wr->payload.data));
            // Drive wr interface to DUT
            dut->wr_en_i = true;
            dut->wr_addr_i = active_wr->payload.addr;
            dut->wr_data_i = active_wr->payload.data;
        } else {
            log_debug("p_sequencer->write_queue.empty() = 1.");
            dut->wr_en_i = false;
        }

        co_await wr_clk->rising_edge(simulation::Phase::Monitor);
        log_debug("wr_clk->rising_edge(Monitor) seen within wr_driver_run().");

        if (active_wr) {
            log_debug("active_wr is not nullptr, triggering done_event.");
            active_wr->done_event.trigger();
        }
        log_debug("wr_driver_run() ended.");
    }
}

simulation::Task<> DualPortRamDriver::rd_driver_run() {
    while (true) {
        // Wait for wr_clk's rising edge
        co_await rd_clk->rising_edge(simulation::Phase::Drive);
        TxnPtr active_rd = nullptr;

        if (!p_sequencer->read_queue.empty()) {
            active_rd = p_sequencer->read_queue.front();
            p_sequencer->read_queue.pop_front();
            dut->rd_addr_i = active_rd->payload.addr;
        }

        co_await rd_clk->rising_edge(simulation::Phase::Monitor);

        if (active_rd) {
            active_rd->done_event.trigger();
        }

    }

}
