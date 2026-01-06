#include "simulation_phase_event.h"

namespace simulation {

    void PhaseEvent::Awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
        auto idx = static_cast<std::size_t>(phase);
        assert(idx < PHASE_COUNT);
        event.next_waiters_[idx].push_back(h);
    }

    /**
      * @brief Return an Awaiter object for a given phase
      *
      * @param phase An enum phase
      * @return An Awaiter object of the phase
      */
    PhaseEvent::Awaiter PhaseEvent::operator()(Phase phase) noexcept {
        return Awaiter{*this, phase};
    }

    void PhaseEvent::trigger(std::function<void()> dut_eval_fn) {
        for (std::size_t i = 0; i < PHASE_COUNT; ++i) {
            // To support co-awaiting the same clk multiple times in a Task
            std::swap(current_waiters_[i], next_waiters_[i]);
            auto &phase_vector = current_waiters_[i];
            for (auto h : phase_vector) {
                assert(h);
                h.resume();
            }
            phase_vector.clear();
            // Evaluate DUT after all tasks in the current phase yield to ensure
            // actions concerning combinational logic done in Drive phase is reflected in the Monitor phase
            if (dut_eval_fn) {
                dut_eval_fn();
            }
        }
    }

    void PhaseEvent::clear() noexcept {
        for (std::size_t i = 0 ; i < PHASE_COUNT; ++i) {
            current_waiters_[i].clear();
            next_waiters_[i].clear();
        }
    }

    bool PhaseEvent::empty() const noexcept {
        for (std::size_t i = 0 ; i < PHASE_COUNT; ++i) {
            if (!current_waiters_[i].empty() || !next_waiters_[i].empty()) return false;
        }
        return true;
    }

} // namespace simulation
