#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <concepts>
#include "simulation_event.h"

template <typename PayloadT>
class BaseTransaction {
public:
    PayloadT payload;
    PayloadT response;
    simulation::Event done_event;

    void reset() {
        done_event.reset();
        if constexpr (requires { payload.reset();}) payload.reset();
    }
};

#endif // TRANSACTION_H
