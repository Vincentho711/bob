#ifndef SIMULATION_PHASE_EVENT_H
#define SIMULATION_PHASE_EVENT_H
#include <cassert>
#include <coroutine>
#include <vector>
#include <cstdint>
#include <array>
#include <functional>

namespace simulation {
    enum class Phase : uint8_t {
        PreDrive = 0U,
        Drive = 1U,
        Monitor = 2U,
        PostMonitor = 3U,
    };

    constexpr std::size_t PHASE_COUNT = 4;

    class PhaseEvent {
    public:
        using handle_t = std::coroutine_handle<>;
        PhaseEvent() {
            // Reserve space for awaiters within each phases upfront
            for (std::size_t i = 0 ; i < PHASE_COUNT ; ++i) {
                current_waiters_[i].reserve(8);
                next_waiters_[i].reserve(8);
            }
        }
        struct Awaiter {
            PhaseEvent &event;
            Phase phase;

            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) noexcept;
            constexpr void await_resume() const noexcept {}

        };

        Awaiter operator()(Phase phase) noexcept;

        void trigger(std::function<void()> dut_eval_fn = nullptr);

        void clear() noexcept;

        bool empty() const noexcept;

    private:
        std::vector<handle_t> current_waiters_[PHASE_COUNT];
        std::vector<handle_t> next_waiters_[PHASE_COUNT];
    };

}; // namespace simulation
#endif // SIMULATION_PHASE_EVENT_H

