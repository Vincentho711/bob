#include "dual_port_ram_driver.h"
#include <format>

DualPortRamDriver::DualPortRamDriver(
    std::shared_ptr<DualPortRamSequencer> sequencer,
    std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<clock_t> wr_clk,
    std::shared_ptr<clock_t> rd_clk,
    const std::string &name)
    : BaseDriver(sequencer, dut, name), wr_clk(wr_clk), rd_clk(rd_clk), wr_logger_(name + "_WrPort"), rd_logger_(name + "_RdPort") {}

simulation::Task<> DualPortRamDriver::run() {
    co_return;
}

simulation::Task<> DualPortRamDriver::wr_driver_run() {
    auto wr_ctx = wr_logger_.scoped_context("WriteDriver");

    while (true) {
        // Wait for wr_clk's rising edge
        co_await wr_clk->rising_edge(simulation::Phase::Drive);
        wr_logger_.debug("Waiting for write transaction");
        TxnPtr active_wr = nullptr;

        if (!p_sequencer->write_queue.empty()) {
            active_wr = p_sequencer->write_queue.front();
            p_sequencer->write_queue.pop_front();

            wr_logger_.debug_txn(active_wr->txn_id, "Fetched write transaction from queue");
            wr_logger_.info_txn(active_wr->txn_id,
                        std::format("Driving write: addr=0x{:X}, data=0x{:X}",
                                    active_wr->payload.addr,
                                    active_wr->payload.data));
            // Drive wr interface to DUT
            dut->wr_en_i = true;
            dut->wr_addr_i = active_wr->payload.addr;
            dut->wr_data_i = active_wr->payload.data;
        } else {
            wr_logger_.debug("Write queue empty, driving wr_en=0");
            dut->wr_en_i = false;
        }

        co_await wr_clk->rising_edge(simulation::Phase::Monitor);

        if (active_wr) {
            wr_logger_.debug_txn(active_wr->txn_id, "Write transaction complete, triggering done event.");
            active_wr->done_event.trigger();
        }
    }
}

simulation::Task<> DualPortRamDriver::rd_driver_run() {
    auto rd_ctx = rd_logger_.scoped_context("ReadDriver");
    while (true) {
        // Wait for wr_clk's rising edge
        co_await rd_clk->rising_edge(simulation::Phase::Drive);
        rd_logger_.debug("Waiting for read transaction");
        TxnPtr active_rd = nullptr;

        if (!p_sequencer->read_queue.empty()) {
            active_rd = p_sequencer->read_queue.front();
            p_sequencer->read_queue.pop_front();
            rd_logger_.debug_txn(active_rd->txn_id, "Fetched read transaction from queue");
            rd_logger_.info_txn(active_rd->txn_id,
                        std::format("Driving read: addr=0x{:X}", active_rd->payload.addr));
            dut->rd_addr_i = active_rd->payload.addr;
            rd_logger_.debug_txn(active_rd->txn_id, "Read address driven to DUT");
        } else {
            rd_logger_.debug("Read queue empty");
        }

        co_await rd_clk->rising_edge(simulation::Phase::Monitor);

        if (active_rd) {
            rd_logger_.debug_txn(active_rd->txn_id, "Read transaction complete, triggering done event");
            active_rd->done_event.trigger();
        }

    }

}
