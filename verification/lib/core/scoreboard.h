#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#include "simulation_task_symmetric_transfer.h"
#include "simulation_context.h"
#include "simulation_logging_utils.h"
#include <iostream>
#include <memory>
#include <string>

template <typename TransactionT>
class BaseScoreboard {
public:
    using TxnPtr = std::shared_ptr<TransactionT>;

    explicit BaseScoreboard(const std::string &name, bool debug_enabled = false)
        : name_(name), logger_(name), debug_enabled_(debug_enabled) {}

    virtual simulation::Task<> run() = 0;

    void log_info(const std::string &message) const {
        logger_.info(message);
    }

    void log_error(const std::string &message) const {
        logger_.error(message);
    }

    void log_debug(const std::string &message) const {
        if (debug_enabled_) {
            logger_.debug(message);
        }
    }

    void log_warning(const std::string &message) const {
          logger_.warning(message);
    }

    const simulation::Logger& get_logger() const {
        return logger_;
    }

private:
    std::string name_;
    simulation::Logger logger_;
    bool debug_enabled_ = true;
};
#endif
