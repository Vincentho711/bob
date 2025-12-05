#include "dual_port_ram_sequence.h"
#include "dual_port_ram_payload.h"

DualPortRamBaseSequence::DualPortRamBaseSequence(const std::string& name, const bool enabled_debug, uint32_t wr_addr_width, uint32_t wr_data_width)
    : BaseSequence(name, enabled_debug), wr_addr_width_(wr_addr_width), wr_data_width_(wr_data_width) {}

[[nodiscard]]
DualPortRamBaseSequence::TxnPtr DualPortRamBaseSequence::dispatch_write(uint32_t addr, uint32_t data) {
    auto txn = create_transaction();
    // Fill payload
    txn->payload.type = DualPortRamPayload::Write;
    txn->payload.addr = addr;
    txn->payload.data = data;

    // Push to specific queue in sequencer
    p_sequencer->write_queue.push_back(txn);

    return txn;
}

[[nodiscard]]
DualPortRamBaseSequence::TxnPtr DualPortRamBaseSequence::dispatch_read(uint32_t addr) {
    TxnPtr txn = create_transaction();

    txn->payload.type = DualPortRamPayload::Read;
    txn->payload.addr = addr;

    p_sequencer->read_queue.push_back(txn);

    return txn;
}

simulation::Task DualPortRamBaseSequence::write(uint32_t addr, uint32_t data) {
    auto t = dispatch_write(addr, data);
    co_await wait_for_txn_done(t);
}

simulation::Task DualPortRamBaseSequence::read(uint32_t addr) {
    auto t = dispatch_read(addr);
    co_await wait_for_txn_done(t);
}
