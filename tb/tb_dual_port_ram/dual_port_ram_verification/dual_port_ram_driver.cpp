#include "dual_port_ram_driver.h"

DualPortRamDriver::DualPortRamDriver(
    std::shared_ptr<DualPortRamSequencer> sequencer,
    std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<clock_t> wr_clk)
    : BaseDriver(sequencer, dut), p_sequencer(sequencer) {}

simulation::Task DualPortRamDriver::run() {
    co_return;
}

simulation::Task DualPortRamDriver::wr_driver_run() {
    while (true) {
        // Wait for wr_clk's rising edge
        co_await wr_clk->rising_edge(simulation::Phase::Drive);
        TxnPtr active_wr = nullptr;

        if (!p_sequencer->write_queue.empty()) {
            active_wr = p_sequencer->write_queue.front();
            p_sequencer->write_queue.pop_front();

            // Drive wr interface to DUT
            dut->wr_en_i = true;
            dut->wr_addr_i = active_wr->payload.addr;
            dut->wr_data_i = active_wr->payload.data;
        } else {
            dut->wr_en_i = false;
        }

        co_await wr_clk->rising_edge(simulation::Phase::Monitor);

        if (active_wr) {
            active_wr->done_event.trigger();
        }
    }
}

simulation::Task DualPortRamDriver::rd_driver_run() {
    while (true) {
        // Wait for wr_clk's rising edge
        co_await wr_clk->rising_edge(simulation::Phase::Drive);
        TxnPtr active_rd = nullptr;

        if (!p_sequencer->read_queue.empty()) {
            active_rd = p_sequencer->read_queue.front();
            p_sequencer->read_queue.pop_front();
            dut->rd_addr_i = active_rd->payload.addr;
        }

        co_await wr_clk->rising_edge(simulation::Phase::Monitor);

        if (active_rd) {
            active_rd->done_event.trigger();
        }

    }

}
