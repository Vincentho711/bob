#ifndef SIMULATION_KERNAL_H
#define SIMULATION_KERNAL_H
#include <vector>
#include <memory>
#include "simulation_clock.h"

namespace simulation {
    class SimulationKernal {
    public:
        uint64_t time = 0U;
        std::vector<std::unqiue_ptr<simulation::Clock>> clocks;
        std::function<void()> eval_dut;

        void register_clock(std::unique_ptr<simulation::Clock> clk) {
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
