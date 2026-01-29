#include "dual_port_ram_monitor.h"
#include "dual_port_ram_payload.h"
#include "simulation_phase_event.h"
#include <format>

DualPortRamMonitor::DualPortRamMonitor(
    std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<ClockT> wr_clk,
    std::shared_ptr<ClockT> rd_clk,
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    const std::string &name)
    : BaseMonitor<DualPortRamTransaction, Vdual_port_ram>(dut, name),
      wr_clk_(wr_clk), rd_clk_(rd_clk),
      tlm_wr_queue_(tlm_wr_queue), tlm_rd_queue_(tlm_rd_queue),
      wr_logger_(name + "_WrPort"), rd_logger_(name + "_RdPort") {}

simulation::Task<> DualPortRamMonitor::run() {
    co_return;
}

simulation::Task<> DualPortRamMonitor::wr_port_run() {
    auto wr_ctx = wr_logger_.scoped_context("WriteMonitor");
    while (true) {
        co_await wr_clk_->rising_edge(simulation::Phase::Monitor);
        wr_logger_.debug("Waiting for capturing write transaction");

        if (dut->wr_en_i) {
            wr_logger_.debug("Capturing write transaction");
            TxnPtr wr_txn = std::make_shared<DualPortRamTransaction>();
            wr_txn->payload.type = DualPortRamPayload::Write;
            wr_txn->payload.addr = dut->wr_addr_i;
            wr_txn->payload.data = dut->wr_data_i;
            wr_logger_.info_txn(wr_txn->txn_id,
                                std::format("Capturing write: addr=0x{:X}, data=0x{:X}",
                                            wr_txn->payload.addr,
                                            wr_txn->payload.data));
            co_await put_wr_transaction(wr_txn);
        }
    }
}

simulation::Task<> DualPortRamMonitor::rd_port_run() {
    auto rd_ctx = rd_logger_.scoped_context("ReadMonitor");
    while (true) {
        co_await rd_clk_->rising_edge(simulation::Phase::Monitor);
        rd_logger_.debug("Waiting for capturing read transaction");

        // Read port is asynchronous
        TxnPtr rd_txn = std::make_shared<DualPortRamTransaction>();
        rd_txn->payload.type = DualPortRamPayload::Read;
        rd_txn->payload.addr = dut->rd_addr_i;
        rd_txn->payload.data = dut->rd_data_o;

        rd_logger_.info_txn(rd_txn->txn_id,
                            std::format("Capturing read: addr=0x{:X}, data=0x{:X}",
                                        rd_txn->payload.addr,
                                        rd_txn->payload.data));

        co_await put_rd_transaction(rd_txn);
    }
}

simulation::Task<> DualPortRamMonitor::put_wr_transaction(
    TxnPtr wr_txn) {
    wr_logger_.debug_txn(wr_txn->txn_id, "Putting write transaction to tlm wr queue");
    co_await tlm_wr_queue_->blocking_put(wr_txn);
}

simulation::Task<> DualPortRamMonitor::put_rd_transaction(
    TxnPtr rd_txn) {
    rd_logger_.debug_txn(rd_txn->txn_id, "Putting read transaction to tlm rd queue");
    co_await tlm_rd_queue_->blocking_put(rd_txn);
}
