#include "simulation_context.h"

SimulationContext::~SimulationContext() = default;

uint64_t SimulationContext::current_cycle() const {
    return current_cycle_;
}

void SimulationContext::set_cycle(uint64_t c) {
    current_cycle_ = c;
}
