#ifndef DUAL_PORT_RAM_SEQUENCE_H
#define DUAL_PORT_RAM_SEQUENCE_H
class DualPortRamBaseSequence : public BaseSequence<DualPortRamPayload, DualPortRamSequencer> {
public:
    [[nodiscard]]
    TxnPtr dispatch_write(uint32_t addr, uint32_t data);

    [[nodiscard]]
    TxnPtr dispatch_read(uint32_t addr);

    simulation::Task write(uint32_t addr, uint32_t data); {
        auto t = dispatch_write(addr, data);
        co_await wait_for_txn_done(t);
    }

};
#endif // DUAL_PORT_RAM_SEQUENCE_H
