#include "simulation_event.h"

namespace simulation {
    void Event::Awaiter::await_suspend(handle_t h) noexcept {
        event.waiters_.push_back(h);
    }

    void Event::trigger() noexcept {
        triggered_ = true;
        // Move all the current waiters to a local list as some tasks might co_await the same event multiple times.
        // Only handle current batch
        std::vector<handle_t> current_batch;
        std::swap(current_batch, waiters_);
        for (auto &h : waiters_) h.resume();
        // waiters_.clear();
    }

    void Event::reset() noexcept {
        triggered_ = false;
        waiters_.clear();
    }
}
