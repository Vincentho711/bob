#include "simulation_event.h"

namespace simulation {
    void Event::Awaiter::await_suspend(handle_t h) noexcept {
        event.waiters_.push_back(h);
    }

    void Event::trigger() noexcept {
        triggered_ = true;
        for (auto &h : waiters_) h.resume();
        waiters_.clear();
    }

    void Event::reset() noexcept {
        triggered_ = false;
        waiters_.clear();
    }
}
