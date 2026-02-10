#ifndef SIMULATION_DELAY_H
#define SIMULATION_DELAY_H
#include "simulation_context.h"
#include <coroutine>
#include <cstdint>

namespace simulation {

template <typename DutType>
struct DelayAwaiter {
    uint64_t delay_ps;
    bool await_ready() const noexcept {
        return delay_ps == 0;
    }

    void await_suspend(std::coroutine_handle<> h) {
        auto& scheduler = SimulationContext<DutType>::current().scheduler();
        scheduler.schedule_async_delay(delay_ps, [h]() {
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

template<typename DutType>
inline auto delay_ps(uint64_t ps) {
    return DelayAwaiter<DutType>{ps};
}

template<typename DutType>
inline auto delay_ns(uint64_t ns) {
    return DelayAwaiter<DutType>{ns * 1000};
}

template<typename DutType>
inline auto delay_us(uint64_t us) {
    return DelayAwaiter<DutType>{us * 1000000};
}

template<typename DutType>
inline auto delay_ms(uint64_t ms) {
    return DelayAwaiter<DutType>{ms * 1000000000};
}

}
#endif // SIMULATION_DELAY_H
