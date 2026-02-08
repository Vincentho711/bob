#include "dual_port_ram_scoreboard.h"
#include "simulation_exceptions.h"
#include "simulation_when_all.h"
#include <cmath>
#include <string>
#include <format>

DualPortRamScoreboard::DualPortRamScoreboard(
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle,
    const std::string &name)
    : BaseScoreboard(name),
      tlm_wr_queue_(tlm_wr_queue), tlm_rd_queue_(tlm_rd_queue),
      wr_clk_(wr_clk), wr_delay_cycle_(wr_delay_cycle),
      apply_index_(0U),
      circular_buffer_size_(wr_delay_cycle + 1U),
      circular_buffer_(circular_buffer_size_) {}

simulation::Task<> DualPortRamScoreboard::run_phase() {
    std::vector<simulation::Task<>> tasks;
    tasks.emplace_back(run_write_capture());
    tasks.emplace_back(run_read_capture());
    tasks.emplace_back(update_ram_model());
    co_await simulation::when_all(std::move(tasks));
}

simulation::Task<> DualPortRamScoreboard::run_write_capture() {
    while (true) {
        TxnPtr write_txn = co_await tlm_wr_queue_->blocking_get();
        if (write_txn) {
            auto wr_cap_ctx = this->logger_.scoped_context("Run Write Capture");
            this->log_debug_txn(write_txn->txn_id, "Write txn fetched from tlm wr queue");
            this->log_debug_txn(write_txn->txn_id, "Current apply index = " + std::to_string(apply_index_));
            // Check whether it is a valid ptr and calculate the current staging index
            uint32_t staging_index = (apply_index_ + wr_delay_cycle_) % circular_buffer_size_;
            this->log_debug_txn(write_txn->txn_id, "Current staging index = " + std::to_string(staging_index));
            // Push captured transaction into circular buffer for pending ram model update
            circular_buffer_[staging_index].push_back(write_txn);
            this->log_debug_txn(write_txn->txn_id, "Write transaction added to circular_buffer at staging index = " + std::to_string(staging_index));
        }
    }
}

simulation::Task<> DualPortRamScoreboard::run_read_capture() {
    while (true) {
        TxnPtr read_txn = co_await tlm_rd_queue_->blocking_get();

        // Since the read interface is always active, we should ignore reads before any writes have been done at start up
        if (ram_model_.empty()) {
            this->log_debug("Ignoring read: Ram model not yet initialised by first write.");
            continue;
        }

        if (read_txn) {
            auto rd_cap_ctx = this->logger_.scoped_context("Run Read Capture");
            log_debug_txn(read_txn->txn_id, "Read txn fetched from tlm rd queue");
            uint32_t addr = read_txn->payload.addr;
            uint32_t dut_data = read_txn->payload.data;

            // Look up in our ram model
            if (ram_model_.find(addr) == ram_model_.end()) {
                // Throw VerificationError
                simulation::report_fatal("Read from uninitialised address: " + std::to_string(addr));
            }
            uint32_t expected_data = ram_model_[addr];
            if (dut_data != expected_data) {
                std::string msg = std::format(
                    "{}Mismatch at addr: 0x{:X} | Expected data: 0x{:X} | Observed data: 0x{:X}{}",
                    simulation::colours::RED,
                    addr, expected_data, dut_data,
                    simulation::colours::RESET
                );
                // Throw VerificationError
                simulation::report_fatal(msg);
            } else {
                std::string msg = std::format(
                    "{}Match at addr: 0x{:X} | Expected data: 0x{:X} | Observed data: 0x{:X}{}",
                    simulation::colours::GREEN,
                    addr, expected_data, dut_data,
                    simulation::colours::RESET
                );
                this->log_debug_txn(read_txn->txn_id, msg);
            }
        }
    }
}

simulation::Task<> DualPortRamScoreboard::update_ram_model() {
    while (true) {
        co_await wr_clk_->rising_edge(simulation::Phase::PreDrive);
        auto update_model_ctx = logger_.scoped_context("Update ram model");
        // Advance the apply index by 1
        apply_index_ = (apply_index_ + 1) % circular_buffer_size_;
        this->log_debug(std::format(
            "Current apply index = {}",
            apply_index_
        ));
        this->log_debug("Circular_buffer_[apply_index_].empty() = " + std::to_string(circular_buffer_[apply_index_].empty()));
        // Apply all pending write transactions from circular buffer to ram model
        while (!circular_buffer_[apply_index_].empty()) {
            TxnPtr write_txn = circular_buffer_[apply_index_].front();
            circular_buffer_[apply_index_].pop_front();
            // Extract addr and data of the write transaction
            uint32_t addr = write_txn->payload.addr;
            uint32_t data = write_txn->payload.data;
            this->log_debug_txn(write_txn->txn_id, std::format("Updating ram model with write txn, addr: 0x{:X} , data: 0x{:X}", addr, data));
            // Update the map
            ram_model_[addr] = data;
        }
    }
}
