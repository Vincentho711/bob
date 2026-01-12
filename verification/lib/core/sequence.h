#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "simulation_event.h"
#include <memory>
#include <iostream>
#include <atomic>
#include <random>

// Global counter shared across all template instantiations
struct BaseSequenceIdCounter {
    inline static std::atomic<uint64_t> counter;
};

template <typename TransactionT, typename ConcreteSequencerT>
class BaseSequence {
public:
    using TxnType = TransactionT;
    using TxnPtr = std::shared_ptr<TxnType>;
    using SequencerType = ConcreteSequencerT;
    using TxnDoneAwaiter = simulation::Event::Awaiter;

    std::shared_ptr<ConcreteSequencerT> p_sequencer = nullptr;

    BaseSequence(const std::string &name, bool debug_enabled, uint64_t global_seed) :
        name(name), debug_enabled(debug_enabled),
        sequence_id_(next_sequence_id()),
        seed_(derive_seed(global_seed, sequence_id_)),
        rng_(seed_) {}

    virtual ~BaseSequence() = default;
    virtual simulation::Task body() = 0;

    [[nodiscard]]
    uint64_t sequence_id() const noexcept { return sequence_id_; }

    [[nodiscard]]
    TxnPtr create_transaction() {
        auto ptr = std::static_pointer_cast<TransactionT>(p_sequencer->pool.acquire());
        return ptr;
    }

    TxnDoneAwaiter wait_for_txn_done(TxnPtr txn) {
        // Return the Awaiter object created by the Event's operator co_await()
        return txn->done_event.operator co_await();
    }

    simulation::Task wait_all(std::vector<TxnPtr> txns) {
        for (auto &t : txns) {
            co_await wait_for_txn_done(t);
        }
    }

    void log_info(const std::string& message) const {
        std::cout << "[Sequence:" << name << "] INFO : " << message << "\n";
    }

    void log_error(const std::string& message) const {
        std::cout << "[Sequence:" << name << "] ERROR : " << message << "\n";
    }

    void log_debug(const std::string& message) const {
        if (debug_enabled) {
            std::cout << "[Sequence:" << name << "] DEBUG : " << message << "\n";
        }
    }
protected:
    [[nodiscard]]
    uint32_t rand_uint(uint32_t min, uint32_t max) {
        if (min > max) {
            throw std::invalid_argument("rand_uint: min must be <= max");
        }
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(rng_);
    }

    [[nodiscard]]
    float rand_unit() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng_);
    }

    [[nodiscard]]
    bool rand_prob(float p) {
        if (p < 0.0f || p > 1.0f) {
            throw std::invalid_argument("rand_prob: p must be >= 0.0 and <= 1.0");
        }
        std::bernoulli_distribution dist(p);
        return dist(rng_);
    }

    static uint64_t derive_seed(uint64_t global_seed, uint64_t sequence_id) {
        // SplitMix64-style mixing
        uint64_t x = global_seed + 0x9e3779b97f4a7c15ULL;
        x ^= sequence_id + (x << 6) + (x >> 2);
        return x;
    }
protected:
    static uint64_t next_sequence_id() {
        return BaseSequenceIdCounter::counter.fetch_add(1, std::memory_order_relaxed);
    }
private:
    std::string name;
    bool debug_enabled = true;
    const uint64_t sequence_id_;
    const uint64_t seed_;
    std::mt19937_64 rng_;

    static std::atomic<uint64_t> global_sequence_counter_;
};
#endif  // SEQUENCE_H
