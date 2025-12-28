#include "dual_port_ram_scoreboard.h"
#include "simulation_exceptions.h"
#include <string>

DualPortRamScoreboard::DualPortRamScoreboard(
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
    std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle,
    const std::string &name, bool debug_enabled)
    : tlm_wr_queue_(tlm_wr_queue), tlm_rd_queue_(tlm_rd_queue),
      wr_delay_cycle_(wr_delay_cycle), wr_clk_(wr_clk),
      BaseScoreboard(name, debug_enabled) {
    apply_index_ = 0U;
    circular_buffer_size_ = wr_delay_cycle_ + 1U;
    circular_buffer_.resize(circular_buffer_size_);
}

simulation::Task DualPortRamScoreboard::run_write_capture() {
    while (true) {
        TxnPtr write_txn = co_await tlm_wr_queue_->blocking_get();
        if (write_txn) {
            // Check whether it is a valid ptr and calculate the current staging index
            uint32_t staging_index = (apply_index_ + wr_delay_cycle_) % circular_buffer_size_;
            // Push captured transaction into circular buffer for pending ram model update
            circular_buffer_[staging_index].push_back(write_txn);
        }
    }
}

simulation::Task DualPortRamScoreboard::run_read_capture() {
    while (true) {
        TxnPtr read_txn = co_await tlm_rd_queue_->blocking_get();
        if (read_txn) {
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
            }
            bool matched = verify_with_ram_model(read_txn);
            if (!matched) {
                // Exception and should terminate simulations, including all running coroutines.
            }
        }
    }
}

simulation::Task DualPortRamScoreboard::update_ram_model() {
    while (true) {
        co_await wr_clk_->rising_edge(simulation::Phase::PreDrive);

        // Apply all pending write transactions from circular buffer to ram model
        while (!circular_buffer_[apply_index_].empty()) {
            write_txn = circular_buffer_[apply_index_].front();
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
    }
}
