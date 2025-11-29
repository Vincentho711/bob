#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "simulation_task.h"
#include <memory>
#include <iostream>

template <typename TransactionT, typename ConcreteSequencerT>
class BaseSequence {
public:
    using TxnType = TransactionT;
    using TxnPtr = std::shared_ptr<TxnType>;
    using SequencerType = ConcreteSequencerT;

    std::shared_ptr<ConcreteSequencerT> p_sequencer = nullptr;

    explicit BaseSequence(const std::string &name, bool debug_enabled) :
        name(name), debug_enabled(debug_enabled) {}

    virtual ~BaseSequence() = default;
    virtual simulation::Task body() = 0;

    [[nodiscard]]
    TxnPtr create_transaction() {
        return std::static_pointer_cast<TransactionT>(p_sequencer->pool.acquire());
    }

    simulation::Task wait_for_txn_done(TxnPtr txn) {
        co_await txn->done_event;
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
            std::cout << "[Sequence:" << name << "] ERROR : " << message << "\n";
        }
    }
private:
    std::string name;
    bool debug_enabled = true;

};
#endif  // SEQUENCE_H
