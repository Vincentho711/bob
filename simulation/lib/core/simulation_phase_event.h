#ifndef SIMULATION_PHASE_EVENT_H
#define SIMULATION_PHASE_EVENT_H
#include <cassert>
#include <coroutine>
#include <vector>

namespace simulation {
    enum class Phase : uint8_t {
        PreDrive = 0U,
        Drive = 1U,
        Monitor = 2U,
        PostMonitor = 3U,
        Count
    };

    constexpr std::size_t PHASE_COUNT = static_cast<std::size_t>(Phase::Count);

    class PhaseEvent {
    public:
        struct Awaiter {
            PhaseEvent &event;
            Phase phase;

            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) noexcept {
                auto idx = static_cast<std::size_t>(phase);
                assert(idx < PHASE_COUNT);
                event.waiters_[idx].push_back(h);
            }
            constexpr void await_resume() const noexcept {}

        };

        /**
         * @brief Return an Awaiter object for a given phase
         *
         * @param phase An enum phase
         * @return An Awaiter object of the phase
         */
        Awaiter operator()(Phase phase) noexcept {
            return Awaiter{*this, phase};
        }

        void trigger() noexcept {
            // Execute coroutines in strict phase order
            for (std::size_t i = 0 ; i < PHASE_COUNT ; ++i) {
                auto &phase_vector = waiters_[i];
                for (auto h : phase_vector) {
                    assert(h);
                    h.resume();
                }
                phase_vector.clear();
            }
        }

        void clear() noexcept {
            for (auto &v : waiters_) {
                v.clear();
            }
        }

        bool empty() const noexcept {
            for (auto &v : waiters_) {
                if (!v.empty()) return false;
            }
            return true;
        }
    private:
        std::array<std::vector<std::coroutine_handle<>>, PHASE_COUNT> waiters_{};
    };

};
#endif // SIMULATION_PHASE_EVENT_H

