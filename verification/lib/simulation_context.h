#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include <cstdint>

class SimulationContext {
public:
    virtual ~SimulationContext();

    virtual uint64_t current_cycle() const;
    virtual void set_cycle(uint64_t c);

protected:
    uint64_t current_cycle_ = 0;
};
#endif
