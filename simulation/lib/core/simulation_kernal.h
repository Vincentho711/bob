#ifndef SIMULATION_KERNAL_H
#define SIMULATION_KERNAL_H
#include <vector>
#include <memory>
#include "simulation_context.h"
#include "simulation_clock.h"
#include "simulation_task_symmetric_transfer.h"

namespace simulation {

    template <typename DutType, typename TraceType>
    class SimulationKernal {
    public:
        uint64_t time = 0U;
        std::vector<std::shared_ptr<simulation::Clock<DutType>>> clocks;
        // Raw pointer reference to all the root tasks
        std::vector<simulation::Task<>>* root_tasks = nullptr;

        SimulationKernal(std::shared_ptr<DutType> dut, std::shared_ptr<TraceType> trace) :
            dut_(dut), trace_(trace) {};

        void register_clock(std::shared_ptr<simulation::Clock<DutType>> clk) {
            clocks.push_back(clk);
        }

        void run(uint64_t max_time) {
            while (time < max_time) {
                // Sync global time for reporting
                simulation::current_time_ps = time;

                // Check root tasks for exceptions before ticking clocks
                if (root_tasks) {
                    for (auto &root_task : *root_tasks) {
                        root_task.check_exception();
                    }
                }
                bool clk_ticked = false;
                for (auto &clk : clocks) {
                    clk_ticked |= clk->tick(time);
                }
                // Dump waveform
                if (clk_ticked && trace_) {
                    trace_->dump(time);
                }
                // Advance time only after all triggered coroutines yield
                time++;
            }
        }
    private:
        std::shared_ptr<DutType> dut_;
        std::shared_ptr<TraceType> trace_;
    };

}

#endif // SIMULATION_KERNAL_H
