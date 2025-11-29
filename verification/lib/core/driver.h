#ifndef DRIVER_H
#define DRIVER_H
template <typename PayloadT, typename DutType>
class BaseDriver {
protected:
    using TxnPtr = std::shared_ptr<BaseTransaction<PayloadT>>;
    std::shared_ptr<BaseSequencer<PayloadT>> p_base_sequencer;
    std::shared_ptr<DutType> dut;
public:
    explicit BaseDriver(std::shared_ptr<BaseSequencer<PayloadT>> sequencer, std::shared_ptr<DutType> dut)
        : p_base_sequencer(sequencer), dut(dut) {}

    virtual simulation::Task run() = 0;
};
#endif

