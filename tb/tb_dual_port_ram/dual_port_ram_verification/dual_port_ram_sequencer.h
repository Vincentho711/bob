#ifndef DUAL_PORT_RAM_SEQUENCER_H
#define DUAL_PORT_RAM_SEQUENCER_H
#include "sequencer.h"

class DualPortRamSequencer : public BaseSequencer<DualPortRamPayload> {
public:
    std::deque<TxnPtr> write_queue;
    std::deque<TxnPtr> read_queue;
};

#endif // DUAL_PORT_RAM_SEQUENCER_H
