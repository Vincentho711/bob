#include "dual_port_ram_sequence.h"

[[nodiscard]]
TxnPtr DualPortRamBaseSequence::dispatch_write(uint32_t addr, uint32_t data) {
    auto txn = create_transaction();
    // Fill payload
    txn->payload.type = DualPortRamPayload::Write;
    txn->payload.addr = addr;
    txn->payload.data = data;

    // Push to specific queue in sequencer
    p_sequencer->write_queue.push_back(txn);

    return txn;
}
