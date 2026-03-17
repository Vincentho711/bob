#ifndef SIMULATION_KERNEL_H
#define SIMULATION_KERNEL_H
#include <vector>
#include <memory>
#include "simulation_context.h"
#include "simulation_clock.h"
#include "simulation_event_scheduler.h"
#include "simulation_task_symmetric_transfer.h"

namespace simulation {

    enum class RunResult : uint8_t {
      Completed,        // Scheduler drained naturally - all tasks finished
      MaxTimeReached    // Scheduler stopped because max-time is reached
    };

    template <typename DutType, typename TraceType>
    class SimulationKernel {
    public:

        uint64_t time = 0U;
        std::vector<std::shared_ptr<simulation::Clock<DutType>>> clocks;
        // Raw pointer references to active (finite) and reactive (infinite) tasks.
        // Simulation terminates when all active tasks complete and TLM queues drain.
        std::vector<simulation::Task<>>* active_tasks = nullptr;
        std::vector<simulation::Task<>>* reactive_tasks = nullptr;

        SimulationKernel(std::shared_ptr<DutType> dut, std::shared_ptr<TraceType> trace) :
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

        RunResult run(uint64_t max_time) {
            auto& drain_ctx = simulation::SimulationDrainContext::instance();
            drain_ctx.reset();
            const bool had_active = active_tasks && !active_tasks->empty();

            uint64_t last_waveform_time = 0;
            while (scheduler_.has_events()) {
                uint64_t next_time = scheduler_.peek_next_time();
                // Check if we have exceeded max time
                if (next_time >= max_time) {
                    time = max_time;
                    simulation::current_time_ps = time;
                    return RunResult::MaxTimeReached;
                }

                // Advance simulation time to next event
                time = next_time;
                simulation::current_time_ps = time;
                scheduler_.set_current_time(time);

                // Get all events at this time
                auto batch = scheduler_.get_next_batch();

                // Process all clock events at this time first
                for (const auto& clock_event : batch.clock_events) {
                    clock_event.clock->execute_step(clock_event.step, time);
                }
                check_all_exceptions_();

                // Process async events
                for (const auto& async_event : batch.async_events) {
                    async_event.callback();
                    // After each async event, eval to propagate combinational changes
                    dut_->eval();
                }
                check_all_exceptions_();

                // Process async immediate events
                scheduler_.process_async_immediate_events();

                // Final evaluation to ensure changes are reflected
                dut_->eval();
                check_all_exceptions_();

                // Dump waveform if enabled
                if (trace_) {
                    trace_->dump(time);
                    last_waveform_time = time;
                }

                // Termination check: arm drain once all active tasks have co_returned,
                // then exit as soon as every registered TLM queue is empty.
                if (had_active && all_active_tasks_done_()) {
                    if (!drain_ctx.is_draining()) drain_ctx.set_draining(true);
                    if (drain_ctx.all_drain_queues_empty()) break;
                }
            }

            // Destroy reactive task handles (suspended coroutines are torn down here).
            if (reactive_tasks) reactive_tasks->clear();
            return RunResult::Completed;
        }

        EventScheduler<DutType>& get_scheduler() {
            return scheduler_;
        }

    private:
        std::shared_ptr<DutType> dut_;
        std::shared_ptr<TraceType> trace_;
        EventScheduler<DutType> scheduler_;

        bool all_active_tasks_done_() const {
            if (!active_tasks) return true;
            for (const auto& t : *active_tasks) { if (!t.done()) return false; }
            return true;
        }

        void check_all_exceptions_() {
            if (active_tasks)   for (auto& t : *active_tasks)   t.check_exception();
            if (reactive_tasks) for (auto& t : *reactive_tasks) t.check_exception();
        }
    };

}

#endif // SIMULATION_KERNEL_H
