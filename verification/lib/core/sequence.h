#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "simulation_task_symmetric_transfer.h"
#include "simulation_event.h"
#include "simulation_logging_utils.h"
#include "simulation_component.h"
#include <memory>
#include <iostream>
#include <atomic>
#include <random>

// Global counter shared across all template instantiations
struct BaseSequenceIdCounter {
    inline static std::atomic<uint64_t> counter{0};
};

template <typename DutType, typename TransactionT, typename ConcreteSequencerT>
class BaseSequence : public simulation::SimulationComponent<DutType> {
public:
    using TxnType = TransactionT;
    using TxnPtr = std::shared_ptr<TxnType>;
    using SequencerType = ConcreteSequencerT;
    using TxnDoneAwaiter = simulation::Event::Awaiter;

    std::shared_ptr<ConcreteSequencerT> p_sequencer = nullptr;

    BaseSequence(const std::string &name, uint64_t global_seed) :
        simulation::SimulationComponent<DutType>(name),
        sequence_id_(next_sequence_id()),
        seed_(derive_seed(global_seed, sequence_id_)),
        rng_(seed_) {}

    virtual ~BaseSequence() = default;
    virtual simulation::Task<> body() = 0;

    [[nodiscard]]
    uint64_t sequence_id() const noexcept { return sequence_id_; }

    [[nodiscard]]
    uint64_t seed() const noexcept {
        return seed_;
    }

    [[nodiscard]]
    TxnPtr create_transaction() {
        auto ptr = std::static_pointer_cast<TransactionT>(p_sequencer->pool.acquire());
        ptr->renew_txn_id(); // Get fresh ID when acquired from pool
        this->log_debug_txn(ptr->txn_id, "Transaction created.");
        return ptr;
    }

    TxnDoneAwaiter wait_for_txn_done(TxnPtr txn) {
        this->log_debug_txn(txn->txn_id, "Waiting for transaction completion");
        // Return the Awaiter object created by the Event's operator co_await()
        return txn->done_event.operator co_await();
    }

    simulation::Task<> wait_all(std::vector<TxnPtr> txns) {
        auto wait_ctx = this->logger_.scoped_context("WaitAll");
        this->log_debug("Waiting for " + std::to_string(txns.size()) + " transactions");
        for (auto &t : txns) {
            co_await wait_for_txn_done(t);
        }
    }

    void log_info(const std::string& message) const {
        this->logger_.info(message);
    }

    void log_error(const std::string& message) const {
        this->logger_.error(message);
    }

    void log_debug(const std::string& message) const {
        this->logger_.debug(message);
    }

    void log_warning(const std::string& message) const {
        this->logger_.warning(message);
    }

    void log_info_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.info_txn(txn_id, message);
    }

    void log_debug_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.debug_txn(txn_id, message);
    }

    void log_error_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.error_txn(txn_id, message);
    }

    void log_warning_txn(uint64_t txn_id, const std::string& message) const {
        this->logger_.warning_txn(txn_id, message);
    }

protected:
    [[nodiscard]]
    uint32_t rand_uint(uint32_t min, uint32_t max) {
        if (min > max) {
            this->log_error("rand_uint: min (" + std::to_string(min) +
                     ") must be <= max (" + std::to_string(max) + ")");
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
            this->log_error("rand_prob: p (" + std::to_string(p) +
                     ") must be >= 0.0 and <= 1.0");
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
private:
    const uint64_t sequence_id_;
    const uint64_t seed_;
    std::mt19937_64 rng_;

    static std::atomic<uint64_t> global_sequence_counter_;

protected:
    static uint64_t next_sequence_id() {
        return BaseSequenceIdCounter::counter.fetch_add(1, std::memory_order_relaxed);
    }
};
#endif  // SEQUENCE_H
