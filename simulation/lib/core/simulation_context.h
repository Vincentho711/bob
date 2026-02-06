#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include "simulation_event_scheduler.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdint>

namespace simulation {

template <typename DutType>
class EventScheduler;

inline uint64_t current_time_ps = 0;

};
#endif // SIMULATION_CONTEXT_H
