#include "dual_port_ram_scoreboard.h"
#include "simulation_exceptions.h"
#include <cmath>
#include <string>

DualPortRamScoreboard::DualPortRamScoreboard(
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle,
    const std::string &name, bool debug_enabled)
    : BaseScoreboard(name, debug_enabled),
      tlm_wr_queue_(tlm_wr_queue), tlm_rd_queue_(tlm_rd_queue),
      wr_clk_(wr_clk), wr_delay_cycle_(wr_delay_cycle),
      apply_index_(0U),
      circular_buffer_size_(wr_delay_cycle + 1U),
      circular_buffer_(circular_buffer_size_) {}

simulation::Task DualPortRamScoreboard::run() {
    co_return;
}

simulation::Task DualPortRamScoreboard::run_write_capture() {
    while (true) {
        TxnPtr write_txn = co_await tlm_wr_queue_->blocking_get();
        if (write_txn) {
            log_debug("write txn fetched from tlm_wr_queue.");
            // Check whether it is a valid ptr and calculate the current staging index
            uint32_t staging_index = (apply_index_ + wr_delay_cycle_) % circular_buffer_size_;
            // Push captured transaction into circular buffer for pending ram model update
            circular_buffer_[staging_index].push_back(write_txn);
            log_debug("write_txn added to circular_buffer at staging_index = " + std::to_string(staging_index) + ".");
        }
    }
}

simulation::Task DualPortRamScoreboard::run_read_capture() {
    while (true) {
        TxnPtr read_txn = co_await tlm_rd_queue_->blocking_get();

        // Since the read interface is always active, we should ignore reads before any writes have been done at start up
        if (ram_model_.empty()) {
            log_debug("Ignoring read: Ram model not yet initialised by first write.");
            continue;
        }

        if (read_txn) {
            log_debug("read txn fetched from tlm_rd_queue.");
            uint32_t addr = read_txn->payload.addr;
            uint32_t dut_data = read_txn->payload.data;

            // Look up in our ram model
            if (ram_model_.find(addr) == ram_model_.end()) {
                // Throw VerificationError
                simulation::report_fatal("Read from uninitialised address: " + std::to_string(addr));
            }
            uint32_t expected_data = ram_model_[addr];
            if (dut_data != expected_data) {
                std::string msg = "Mismatch at Addr " + std::to_string(addr) +
                                  " | Expected: " + std::to_string(expected_data) +
                                  " | Observed: " + std::to_string(dut_data);
                // Throw VerificationError
                simulation::report_fatal(msg);
            } else {
                std::string msg = "Match at Addr " + std::to_string(addr) +
                                  " | Expected: " + std::to_string(expected_data) +
                                  " | Observed: " + std::to_string(dut_data);
                log_debug("\033[1;32m" + msg + "\033[0m");
                // std::cout << msg << std::endl;
            }
        }
    }
}

simulation::Task DualPortRamScoreboard::update_ram_model() {
    while (true) {
        co_await wr_clk_->rising_edge(simulation::Phase::PreDrive);
        log_debug("wr_clk->rising_edge's PreDrive detected.");
        log_debug("circular_buffer_[apply_index_].empty() = " + std::to_string(circular_buffer_[apply_index_].empty()));
        // Apply all pending write transactions from circular buffer to ram model
        while (!circular_buffer_[apply_index_].empty()) {
            TxnPtr write_txn = circular_buffer_[apply_index_].front();
            circular_buffer_[apply_index_].pop_front();
            // Extract addr and data of the write transaction
            uint32_t addr = write_txn->payload.addr;
            uint32_t data = write_txn->payload.data;
            log_debug("Updating ram model with write txn, addr = " + std::to_string(addr) + " , data = " + std::to_string(data));
            // Update the map
            ram_model_[addr] = data;
        }
        // Advance the apply index by 1
        apply_index_ = (apply_index_ + 1) % circular_buffer_size_;
        log_debug("apply_index_ = " + std::to_string(apply_index_));
    }
}
