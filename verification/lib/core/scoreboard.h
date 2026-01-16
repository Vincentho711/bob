#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#include "simulation_task_symmetric_transfer.h"
#include <iostream>
#include <memory>
#include <string>

template <typename TransactionT>
class BaseScoreboard {
public:
    using TxnPtr = std::shared_ptr<TransactionT>;

    explicit BaseScoreboard(const std::string &name, bool debug_enabled)
        : name(name), debug_enabled(debug_enabled) {}

    virtual simulation::Task<> run() = 0;

    void log_info(const std::string &message) const {
        std::cout << "[Scoreboard:" << name << "] INFO : " << message << "\n";
    }

    void log_error(const std::string &message) const {
        std::cout << "[Scoreboard:" << name << "] ERROR : " << message << "\n";
    }

    void log_debug(const std::string &message) const {
        if (debug_enabled) {
            std::cout << "[Scoreboard:" << name << "] DEBUG : " << message << "\n";
        }
    }

private:
    std::string name;
    bool debug_enabled = true;
};
#endif
