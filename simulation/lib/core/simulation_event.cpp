#include "simulation_event.h"
#include <iostream>
#include <string>

namespace simulation {
    void Event::Awaiter::await_suspend(handle_t h) noexcept {
        event.waiters_.push_back(h);
    }

    void Event::trigger() noexcept {
        std::cout << "trigger() called.\n" ;
        triggered_ = true;
        // Move all the current waiters to a local list as some tasks might co_await the same event multiple times.
        // Only handle current batch
        std::vector<handle_t> current_batch;
        std::swap(current_batch, waiters_);
        std::cout << "current_batch.size() = " << std::to_string(current_batch.size()) << ".\n" ;
        for (auto &h : current_batch) {
            if (h && !h.done()) h.resume();
        }
    }

    void Event::reset() noexcept {
        triggered_ = false;
        waiters_.clear();
    }
}
