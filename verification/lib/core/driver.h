#ifndef DRIVER_H
#define DRIVER_H
template <typename PayloadT, typename DutType>
class BaseDriver {
protected:
    using TxnPtr = std::shared_ptr<BaseTransaction<PayloadT>>;
    std::shared_ptr<BaseSequencer<PayloadT>> p_base_sequencer_;
    std::shared_ptr<DutType> dut_;
public:
    BaseDriver(std::shared_ptr<BaseSequencer<PayloadT>> sequencer, std::shared_ptr<DutType> dut)
        : p_base_sequencer_(sequencer), dut_(dut) {}

    virtual simulation::Task run() = 0;
};
#endif

