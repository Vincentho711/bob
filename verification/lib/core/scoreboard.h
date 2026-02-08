#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#include "simulation_task_symmetric_transfer.h"
#include "simulation_context.h"
#include "simulation_component.h"
#include "simulation_logging_utils.h"
#include <iostream>
#include <memory>
#include <string>

template <typename DutType, typename TransactionT>
class BaseScoreboard : public simulation::SimulationComponent<DutType>{
public:
    using TxnPtr = std::shared_ptr<TransactionT>;

    explicit BaseScoreboard(const std::string &name)
        : simulation::SimulationComponent<DutType>(name) {}

    virtual simulation::Task<> run_phase() = 0;

    void log_info(const std::string &message) const {
        this->logger_.info(message);
    }

    void log_error(const std::string &message) const {
        this->logger_.error(message);
    }

    void log_debug(const std::string &message) const {
        this->logger_.debug(message);
    }

    void log_warning(const std::string &message) const {
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
#endif
