#ifndef SIMULATION_COMPONENT_H
#define SIMULATION_COMPONENT_H

#include "simulation_context.h"
#include "simulation_logging_utils.h"
#include "simulation_task_symmetric_transfer.h"
#include <string>
#include <coroutine>
#include <vector>
#include <memory>

namespace simulation {
template <typename DutType>
class SimulationComponent {
public:
    SimulationComponent(const std::string& name) : name_(name), logger_(name) {}

    virtual ~SimulationComponent() = default;

    SimulationComponent(const SimulationComponent&) = delete;
    SimulationComponent& operator=(const SimulationComponent&) = delete;
    SimulationComponent(const SimulationComponent&&) = delete;
    SimulationComponent& operator=(const SimulationComponent&&) = delete;

    virtual void build_phase() {}
    virtual void connect_phase() {}
    virtual Task<> run_phase() { co_return; }

    EventScheduler<DutType>& scheduler() {
        return SimulationContext<DutType>::current().scheduler();
    }

    std::shared_ptr<DutType> dut() {
        return SimulationContext<DutType>::current().dut();
    }

    uint64_t current_time() const {
        return current_time_ps;
    }

    const std::string& get_name() const { return name_; }

    simulation::Logger& get_logger() {
        return this-logger_;
    }

    const simulation::Logger& get_logger() const {
        return logger_;
    }


private:
  std::string name_;
protected:
    simulation::Logger logger_;
};
}
#endif // SIMULATION_COMPONENT_H
