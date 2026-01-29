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

    BaseTransaction() : txn_id(next_txn_id()) {}

    // Copy constructor gets new txn_id
    BaseTransaction(const BaseTransaction& other)
        : txn_id(next_txn_id()),
          payload(other.payload),
          response(other.response) {}

    // Move constructor
    BaseTransaction(BaseTransaction&& other) noexcept
        : txn_id(next_txn_id()),
          payload(std::move(other.payload),
          response(other.response)) {}

    // Copy assignment - txn_id remains unchanged
    BaseTransaction& operator=(const BaseTransaction& other) {
        if (this != &other) {
            payload = other.payload;
            response = other.response;
            // txn_id is not copied - each transaction keeps its own ID
        }
        return *this;
    }

    // Move assignment - txn_id remains unchanged
    BaseTransaction& operator=(BaseTransaction&& other) noexcept {
        if (this != &other) {
            payload = std::move(other.payload);
            response = std::move(other.response);
            // txn_id is not moved - each transaction keeps its own ID
        }
        return *this;
    }

    // Method to get fresh ID when reusing from pool
    void renew_txn_id() {
        txn_id = next_txn_id();
    }

    void reset() {
        done_event.reset();
        if constexpr (requires { payload.reset();}) payload.reset();
    }

protected:
    static uint64_t next_txn_id() {
        return BaseTransactionIdCounter::counter.fetch_add(1, std::memory_order_relaxed);
    }
};

#endif // TRANSACTION_H
