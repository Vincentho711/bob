#ifndef SIMULATION_KERNAL_H
#define SIMULATION_KERNAL_H
#include <vector>
#include <memory>
#include "simulation_context.h"
#include "simulation_clock.h"
#include "simulation_event_schedular.h"
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

        void initialise() {
            // Initialise all clocks, they will self schedule their first events
            for (auto& clk : clocks) {
                clk->initialise(scheduler_);
            }
        }

        void run(uint64_t max_time) {
            uint64_t last_waveform_time = 0;
            while (scheduler_.has_events()) {
                uint64_t next_time = scheduler_.peek_next_time();
                // Check if we have exceeded max time
                if (next_time >= max_time) {
                    time = max_time;
                    simulation::current_time_ps = time;
                    break;
                }

                // Advnace simulation time to next event
                time = next_time;
                simulation::current_time_ps = time;

                // Get all events at this time
                auto event_batch = scheduler_.get_next_batch();

                // Process all events at  this time
                for (const auto& event : event_batch) {
                    event.clock->execute_step(event.step, time);
                }

                // Check root tasks for exceptions after processing events
                if (root_tasks) {
                    for (auto& root_task : *root_tasks) {
                        root_task.check_exception();
                    }
                }

                // Dump waveform if enabled
                if (trace_) {
                    trace_->dump(time);
                    last_waveform_time = time;
                }
            }
        }

    private:
        std::shared_ptr<DutType> dut_;
        std::shared_ptr<TraceType> trace_;
        EventSchedular<DutType> scheduler_;
    };

}

#endif // SIMULATION_KERNAL_H
