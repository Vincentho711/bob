#ifndef MONITOR_H
#define MONITOR_H
#include <string>
#include <memory>
#include <iostream>
#include "simulation_task_symmetric_transfer.h"

template <typename TransactionT, typename DutType>
class BaseMonitor {
protected:
    std::shared_ptr<DutType> dut;
public:
    using TxnPtr = std::shared_ptr<TransactionT>;
    explicit BaseMonitor(std::shared_ptr<DutType> dut, const std::string &name, bool debug_enabled)
        : dut(dut), name(name), debug_enabled(debug_enabled) {};

    virtual simulation::Task<> run() = 0;

    void log_info(const std::string& message) const {
        std::cout << "[Monitor:" << name << "] INFO : " << message << "\n";
    }

    void log_error(const std::string& message) const {
        std::cout << "[Monitor:" << name << "] ERROR : " << message << "\n";
    }

    void log_debug(const std::string& message) const {
        if (debug_enabled) {
            std::cout << "[Monitor:" << name << "] DEBUG : " << message << "\n";
        }
    }
private:
    std::string name;
    bool debug_enabled = true;
};
#endif

