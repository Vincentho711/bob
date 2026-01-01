#include "dual_port_ram_monitor.h"
#include "dual_port_ram_payload.h"
#include "simulation_phase_event.h"

DualPortRamMonitor::DualPortRamMonitor(
    std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<ClockT> wr_clk,
    std::shared_ptr<ClockT> rd_clk,
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    const std::string &name, bool debug_enabled)
    : BaseMonitor<DualPortRamTransaction, Vdual_port_ram>(dut, name,
                                                          debug_enabled),
      wr_clk_(wr_clk), rd_clk_(rd_clk), tlm_wr_queue_(tlm_wr_queue),
      tlm_rd_queue_(tlm_rd_queue) {}

simulation::Task DualPortRamMonitor::run() {
    co_return;
}

simulation::Task DualPortRamMonitor::wr_port_run() {
    while (true) {
        co_await wr_clk_->rising_edge(simulation::Phase::Monitor);

        if (dut_->wr_en_i) {
            TxnPtr wr_txn =
                std::make_shared<DualPortRamTransaction>();
            wr_txn->payload.type = DualPortRamPayload::Write;
            wr_txn->payload.addr = dut_->wr_addr_i;
            wr_txn->payload.data = dut_->wr_data_i;

            co_await put_wr_transaction(wr_txn);
        }
    }
}

simulation::Task DualPortRamMonitor::rd_port_run() {
    while (true) {
        co_await rd_clk_->rising_edge(simulation::Phase::Monitor);

        // Read port is asynchronous
        TxnPtr rd_txn = std::make_shared<DualPortRamTransaction>();
        rd_txn->payload.type = DualPortRamPayload::Read;
        rd_txn->payload.addr = dut_->rd_addr_i;
        rd_txn->payload.data = dut_->rd_data_o;

        log_debug("Capturing read transaction: Addr=" + std::to_string(rd_txn->payload.addr));
        co_await put_rd_transaction(rd_txn);
    }
}

simulation::Task DualPortRamMonitor::put_wr_transaction(
    TxnPtr wr_txn) {
    log_debug("tlm_wr_queue_->blocking_put(). in put_wr_transaction.");
    co_await tlm_wr_queue_->blocking_put(wr_txn);
}

simulation::Task DualPortRamMonitor::put_rd_transaction(
    TxnPtr rd_txn) {
    log_debug("tlm_rd_queue_->blocking_put() in put_rd_transaction.");
    co_await tlm_rd_queue_->blocking_put(rd_txn);
}
