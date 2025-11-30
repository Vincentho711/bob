#ifndef DRIVER_H
#define DRIVER_H
template <typename SequencerT, typename DutType>
class BaseDriver {
protected:
    using TransactionType = typename SequencerT::TransactionType;
    using TxnPtr = typename SequencerT::TxnPtr;

    std::shared_ptr<SequencerT> p_sequencer;
    std::shared_ptr<DutType> dut;
public:
    explicit BaseDriver(std::shared_ptr<SequencerT> sequencer, std::shared_ptr<DutType> dut)
        : p_sequencer(sequencer), dut(dut) {}

    virtual simulation::Task run() = 0;
};
#endif

