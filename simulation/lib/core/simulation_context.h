#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include <memory>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <vector>

namespace simulation {
    inline uint64_t current_time_ps = 0;
}

namespace simulation {

// Interface implemented by TLMQueue<T> so the drain context can poll emptiness
// without knowing the transaction type.
struct IDrainableQueue {
    virtual ~IDrainableQueue() = default;
    virtual bool empty() const noexcept = 0;
};

// Non-templated singleton that tracks drain state and registered TLM queues.
// Accessible from any component without knowing the DUT type.
class SimulationDrainContext {
public:
    static SimulationDrainContext& instance() {
        static SimulationDrainContext ctx;
        return ctx;
    }

    // Called at the start of each kernel run to clear state from previous runs.
    void reset() { draining_ = false; drain_queues_.clear(); }

    void set_draining(bool v) noexcept { draining_ = v; }
    bool is_draining() const noexcept { return draining_; }

    void register_drain_queue(IDrainableQueue* q) { drain_queues_.push_back(q); }

    bool all_drain_queues_empty() const noexcept {
        for (const auto* q : drain_queues_) {
            if (!q->empty()) return false;
        }
        return true;
    }

private:
    SimulationDrainContext() = default;
    bool draining_ = false;
    std::vector<IDrainableQueue*> drain_queues_;
};

} // namespace simulation

// Forward declarations

namespace simulation {
    template <typename DutType>
    class EventScheduler;
}

namespace simulation {
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
