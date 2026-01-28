#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include <memory>
#include <iostream>
#include "simulation_task_symmetric_transfer.h"
#include "simulation_logging_utils.h"

template <typename SequencerT, typename DutType>
class BaseDriver {
protected:
    using TransactionType = typename SequencerT::TransactionType;
    using TxnPtr = typename SequencerT::TxnPtr;

    std::shared_ptr<SequencerT> p_sequencer;
    std::shared_ptr<DutType> dut;
    simulation::Logger logger_;
public:
    explicit BaseDriver(std::shared_ptr<SequencerT> sequencer, std::shared_ptr<DutType> dut, const std::string &name)
        : p_sequencer(sequencer), dut(dut), logger_(name) {};

    virtual simulation::Task<> run() = 0;

    void log_info(const std::string& message) const {
        logger_.info(message);
    }

    void log_error(const std::string& message) const {
        logger_.error(message);
    }

    void log_debug(const std::string& message) const {
        logger_.debug(message);
    }

    void log_warning(const std::string& message) const {
        logger_.warning(message);
    }

    void log_info_txn(uint64_t txn_id, const std::string& message) const {
        logger_.info_txn(txn_id, message);
    }

    void log_debug_txn(uint64_t txn_id, const std::string& message) const {
        logger_.debug_txn(txn_id, message);
    }

    void log_error_txn(uint64_t txn_id, const std::string& message) const {
        logger_.error_txn(txn_id, message);
    }

    void log_warning_txn(uint64_t txn_id, const std::string& message) const {
        logger_.warning_txn(txn_id, message);
    }

    simulation::Logger& get_logger() {
        return logger_;
    }

    const simulation::Logger& get_logger() const {
        return logger_;
    }
};
#endif

