#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <concepts>
#include <cstdint>
#include <atomic>
#include "simulation_event.h"

// Global transaction ID counter shared across all template instantiations
struct BaseTransactionIdCounter {
    inline static std::atomic<uint64_t> counter{0};
};

template <typename PayloadT>
class BaseTransaction {
public:
    uint64_t txn_id;
    PayloadT payload;
    PayloadT response;
    simulation::Event done_event;

    void reset() {
        done_event.reset();
        if constexpr (requires { payload.reset();}) payload.reset();
    }

    void set_txn_id() {
        txn_id = next_txn_id();
    }

protected:
    static uint64_t next_txn_id() {
        return BaseTransactionIdCounter::counter.fetch_add(1, std::memory_order_relaxed);
    }
};

#endif // TRANSACTION_H
