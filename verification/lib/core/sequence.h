#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "simulation_task.h"
#include <memory>

template <typename PayloadT, typename ConcreteSequencerT>
class BaseSequence {
public:
    using TxnType = BaseTransaction<PayloadT>;
    using TxnPtr = std::shared_ptr<TxnType>;
    using SequencerType = ConcreteSequencerT;

    std::shared_ptr<ConcreteSequencerT> p_sequencer = nullptr;

    virtual ~BaseSequence() = default;
    virtual simulation::Task body() = 0;

    [[nodiscard]]
    TxnPtr create_transaction() {
        return p_sequencer->pool.acquire();
    }

    simulation::Task wait_for_txn_done(TxnPtr txn) {
        co_await txn->done_event;
    }

    simulation::Task wait_all(std::vector<TxnPtr> txns) {
        for (auto &t : txns) {
            co_await wait_for_txn_done(t);
        }
    }

};
#endif  // SEQUENCE_H
