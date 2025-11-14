#include "simulation_phase_event.h"

namespace simulation {

    void PhaseEvent::Awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
        auto idx = static_cast<std::size_t>(phase);
        assert(idx < PHASE_COUNT);
        event.waiters_[idx].push_back(h);
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

    void PhaseEvent::trigger() noexcept {
        for (std::size_t i = 0; i < PHASE_COUNT; ++i) {
            auto &phase_vector = waiters_[i];
            for (auto h : phase_vector) {
                assert(h);
                h.resume();
            }
            phase_vector.clear();
        }
    }

    void PhaseEvent::clear() noexcept {
        for (auto &v : waiters_) {
            v.clear();
        }
    }

    bool PhaseEvent::empty() const noexcept {
        for (auto &v : waiters_) {
            if (!v.empty()) return false;
        }
        return true;
    }

} // namespace simulation
