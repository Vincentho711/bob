#include "dual_port_ram_sequence.h"
#include "dual_port_ram_payload.h"

DualPortRamBaseSequence::DualPortRamBaseSequence(const std::string &name,
                                                 uint32_t wr_addr_width,
                                                 uint32_t wr_data_width,
                                                 uint64_t global_seed)
    : BaseSequence(name, global_seed), wr_addr_width_(wr_addr_width),
      wr_data_width_(wr_data_width) {}

[[nodiscard]]
DualPortRamBaseSequence::TxnPtr DualPortRamBaseSequence::dispatch_write(uint32_t addr, uint32_t data) {
    auto txn = create_transaction();
    // Fill payload
    txn->payload.type = DualPortRamPayload::Write;
    txn->payload.addr = addr;
    txn->payload.data = data;

    // Push to specific queue in sequencer
    p_sequencer->write_queue.push_back(txn);

    log_debug_txn(txn->txn_id, 
        std::format("Dispatched write: addr=0x{:X}, data=0x{:X}", addr, data));

    return txn;
}

[[nodiscard]]
DualPortRamBaseSequence::TxnPtr DualPortRamBaseSequence::dispatch_read(uint32_t addr) {
    TxnPtr txn = create_transaction();

    txn->payload.type = DualPortRamPayload::Read;
    txn->payload.addr = addr;

    p_sequencer->read_queue.push_back(txn);

    log_debug_txn(txn->txn_id,
        std::format("Dispatched read: addr=0x{:X}", addr));

    return txn;
}

simulation::Task<> DualPortRamBaseSequence::write(uint32_t addr, uint32_t data) {
    log_debug(std::format("Issuing write: addr=0x{:X}, data=0x{:X}", addr, data));
    auto txn = dispatch_write(addr, data);
    log_debug_txn(txn->txn_id, "Waiting for write completion");
    co_await wait_for_txn_done(txn);
    log_debug_txn(txn->txn_id, "Write completed");
}

simulation::Task<> DualPortRamBaseSequence::read(uint32_t addr) {
    log_debug(std::format("Issuing read: addr=0x{:X}", addr));
    auto txn = dispatch_read(addr);
    log_debug_txn(txn->txn_id, "Waiting for read completion");
    co_await wait_for_txn_done(txn);
}

simulation::Task<> DualPortRamBaseSequence::wait_wr_cycles(uint32_t n) {
    if (!p_sequencer) {
        log_error("Sequencer not connected. Cannot wait for clock.");
        co_return;
    }

    log_debug(std::format("Waiting for {} write clock cycles", n));

    // Access clk via p_sequencer
    auto clk = p_sequencer->wr_clk;

    for (uint32_t i = 0; i < n ; ++i) {
        co_await clk->rising_edge(simulation::Phase::Drive);
    }

    log_debug(std::format("Waited {} write clock cycles", n));
}

simulation::Task<> DualPortRamBaseSequence::wait_rd_cycles(uint32_t n) {
    if (!p_sequencer) {
        log_error("Sequencer not connected. Cannot wait for clock.");
        co_return;
    }

    log_debug(std::format("Waiting for {} read clock cycles", n));

    // Access clk via p_sequencer
    auto clk = p_sequencer->rd_clk;

    for (uint32_t i = 0; i < n ; ++i) {
        co_await clk->rising_edge(simulation::Phase::Drive);
    }
    log_debug(std::format("Waited {} read clock cycles", n));
}
