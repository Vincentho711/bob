#ifndef SIMULATION_KERNAL_H
#define SIMULATION_KERNAL_H
#include <vector>
#include <memory>
#include "simulation_clock.h"

namespace simulation {

    template <typename DutType, typename TraceType>
    class SimulationKernal {
    public:
        uint64_t time = 0U;
        std::vector<std::shared_ptr<simulation::Clock<DutType, TraceType>>> clocks;

        void register_clock(std::shared_ptr<simulation::Clock<DutType, TraceType>> clk) {
            clocks.push_back(clk);
        }

        void run(uint64_t max_time) {
            while (time < max_time) {
                for (auto &clk : clocks) {
                    clk->tick(time);
                }
                // Advance time only after all triggered coroutines yield
                time++;
            }
        }

    };

}

#endif // SIMULATION_KERNAL_H
