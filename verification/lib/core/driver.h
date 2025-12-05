#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include <memory>
#include <iostream>
#include "simulation_task_symmetric_transfer.h"

template <typename SequencerT, typename DutType>
class BaseDriver {
protected:
    using TransactionType = typename SequencerT::TransactionType;
    using TxnPtr = typename SequencerT::TxnPtr;

    std::shared_ptr<SequencerT> p_sequencer;
    std::shared_ptr<DutType> dut;
public:
    explicit BaseDriver(std::shared_ptr<SequencerT> sequencer, std::shared_ptr<DutType> dut, const std::string &name, bool debug_enabled)
        : p_sequencer(sequencer), dut(dut), name(name), debug_enabled(debug_enabled) {};

    virtual simulation::Task run() = 0;
    void log_info(const std::string& message) const {
        std::cout << "[Sequence:" << name << "] INFO : " << message << "\n";
    }

    void log_error(const std::string& message) const {
        std::cout << "[Sequence:" << name << "] ERROR : " << message << "\n";
    }

    void log_debug(const std::string& message) const {
        if (debug_enabled) {
            std::cout << "[Sequence:" << name << "] DEBUG : " << message << "\n";
        }
    }
private:
    std::string name;
    bool debug_enabled = true;
};
#endif

