#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include "simulation_event_scheduler.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdint>

namespace simulation {

template <typename DutType>
class EventScheduler;

inline uint64_t current_time_ps = 0;

template <typename DutType>
class SimulationContext {
public:
    static SimulationContext& current() {
        if (!instance_) {
            throw std::runtime_error(
                "SimulationContext not initialised. "
                "Call SimulationContext::set_context() during kernel initialisation"
            );
        }
        return *instance_;
    }

    static void set_current(SimulationContext* ctx) {
        instance_ = ctx;
    }

    SimulationContext(EventScheduler<DutType>& scheduler, std::shared_ptr<DutType> dut) : scheduler_(scheduler), dut_(dut) {}
    EventScheduler<DutType>& scheduler() { return scheduler_; }
    std::shared_ptr<DutType> dut() { return dut_; }
    uint64_t time() const { return current_time_ps; }
private:
    EventScheduler<DutType>& scheduler_;
    std::shared_ptr<DutType> dut_;

    static inline SimulationContext* instance_ = nullptr;
};
};
#endif // SIMULATION_CONTEXT_H
