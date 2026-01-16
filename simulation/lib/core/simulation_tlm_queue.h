#ifndef SIMULATION_TLM_QUEUE_H
#define SIMULATION_TLM_QUEUE_H
#include <deque>
#include <memory>
#include <coroutine>
#include "simulation_task_symmetric_transfer.h"

namespace simulation {

template <typename TransactionT>
class TLMQueue {
public:
    using TxnPtr= std::shared_ptr<TransactionT>;
    using handle_t = std::coroutine_handle<>;

    explicit TLMQueue(const std::string& name) : name_(name) {}

    // --- Non-blocking interface ---

    // Add transaction to tlm queue immediately
    void put(TxnPtr txn) {
        txn_queue_.push_back(txn);
        // Check if a consumer is waiting on a put. If so, wake it up.
        // Only 1 consumer is supported for now.
        if (!waiting_handle_queue_.empty()) {
            waiting_handle_queue_.front().resume();
            waiting_handle_queue_.pop_front();
        }
    }

    // Retrieve a transaction
    TxnPtr get() {
        if (txn_queue_.empty()) {
            return nullptr;
        }
        TxnPtr txn = txn_queue_.front();
        txn_queue_.pop_front();
        return txn;
    }

    // --- Coroutine Blocking interface ---

    // Returns an Awaiter for co_await()
    auto blocking_get() {
        struct Awaiter {
          TLMQueue<TransactionT> &tlm_queue;
          TxnPtr result_txn = nullptr;

          bool await_ready() const noexcept {
            // Ready if txn is available within the txn_queue_
            return !tlm_queue.txn_queue_.empty();
          }

          void await_suspend(handle_t h) noexcept {
            // Store the suspended coroutine handle
            tlm_queue.waiting_handle_queue_.push_back(h);
          }
          TxnPtr await_resume() noexcept {
            result_txn = tlm_queue.txn_queue_.front();
            tlm_queue.txn_queue_.pop_front();
            return result_txn;
          }

        };
        return Awaiter{*this};
    }

    // Currently, this is not really a blocking because flow control is not required.
    // Simply implement it as a non-blocking put for now
    simulation::Task<> blocking_put(TxnPtr txn) {
        // Since we don't need to handle backpressure / capacity, blocking put is equivalent to put()
        put(txn);
        co_return;
    }

private:
    std::string name_;
    std::deque<TxnPtr> txn_queue_;
    std::deque<handle_t> waiting_handle_queue_;
};
}
#endif // SIMULATION_TLM_QUEUE_H
