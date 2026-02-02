#include "write_transaction_done_event.h"

void WriteTransactionDoneEvent::Awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
    event.suspended_coroutines.push_back(h);
}

void WriteTransactionDoneEvent::set_done() noexcept {
    // Resume all coroutines that are waiting for this event
    for (auto &handle: suspended_coroutines) {
        handle.resume();
    }
    suspended_coroutines.clear();
}
