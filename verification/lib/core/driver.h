#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include <memory>
#include <iostream>
#include "simulation_component.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_logging_utils.h"

template <typename SequencerT, typename DutType>
class BaseDriver : public simulation::SimulationComponent<DutType> {
protected:
    using TransactionType = typename SequencerT::TransactionType;
    using TxnPtr = typename SequencerT::TxnPtr;

    std::shared_ptr<SequencerT> p_sequencer;
public:
    explicit BaseDriver(const std::string &name, std::shared_ptr<SequencerT> sequencer)
        : simulation::SimulationComponent<DutType>(name), p_sequencer(std::move(sequencer)) {};

    virtual simulation::Task<> run_phase() = 0;

    void log_info(const std::string& message) const {
        this->logger_.info(message);
    }

    void log_error(const std::string& message) const {
        this->logger_.error(message);
    }

    void log_debug(const std::string& message) const {
        this->logger_.debug(message);
    }

    void log_warning(const std::string& message) const {
        this->logger_.warning(message);
    }

    void log_info_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.info_txn(txn_id, message);
    }

    void log_debug_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.debug_txn(txn_id, message);
    }

    void log_error_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.error_txn(txn_id, message);
    }

    void log_warning_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.warning_txn(txn_id, message);
    }

};
#endif // DRIVER_H

