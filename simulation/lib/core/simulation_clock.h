#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H
#include "simulation_phase_event.h"
#include <string>
#include <iostream>
#include <memory>
#include <functional>

namespace simulation {
    template <typename DutType>
    class EventSchedular;
    template <typename DutType>
    struct ClockEvent;

    enum class ClockStep : uint8_t {
        RisingEdge = 0U,
        PositiveMidPoint = 1U,
        FallingEdge = 2U,
        NegativeMidPoint = 3U,
    };

    constexpr std::size_t CLOCK_STEP_COUNT = 4;

    template <typename DutType>
    class Clock {
    public:
        std::string name;
        uint64_t period_ps;
        std::shared_ptr<DutType> dut;
        bool level = false;

        // Callback function to drive the DUT's clock input
        std::function<void(bool)> drive_clk_signal_fn;

        PhaseEvent rising_edge, positive_mid, falling_edge, negative_mid;

        Clock(const std::string &n, const uint64_t ps, std::shared_ptr<DutType> dut, std::function<void(bool)> clk_drive_fn, uint64_t initial_offset_ps =0)
            : name(n), period_ps(ps), dut(dut), drive_clk_signal_fn(clk_drive_fn), current_step_(ClockStep::RisingEdge), initial_offset_(initial_offset_ps) {}

        void initialise(EventSchedular<DutType>& scheduler) {
            scheduler_ = &scheduler;
            // Schedule first RisingEdge
            schedule_next_event(initial_offset_);
        }

        void execute_step(ClockStep step, uint64_t current_time) {
            if (step != current_step_) {
                throw std::runtime_error(
                    "Clock " + name + "step mismatch: expected " +
                    std::to_string(static_cast<int>(current_step_)) +
                    " got " + std::to_string(static_cast<int>(step))
                );
            }

            // Define the DUT evaluation lambda
            auto dut_eval_lambda = [this]() {
                if (this->dut) {
                    this->dut->eval();
                }
            };

            switch (step) {
                case ClockStep::RisingEdge:
                    level = true;
                    if (drive_clk_signal_fn) {
                        drive_clk_signal_fn(level);
                        dut->eval();
                    }
                    rising_edge.trigger(dut_eval_lambda);
                    break;
                case ClockStep::PositiveMidPoint:
                    level = true;
                    if (drive_clk_signal_fn) {
                        drive_clk_signal_fn(level);
                        dut->eval();
                    }
                    positive_mid.trigger(dut_eval_lambda);
                    break;
                case ClockStep::FallingEdge:
                    level = false;
                    if (drive_clk_signal_fn) {
                        drive_clk_signal_fn(level);
                        dut->eval();
                    }
                    falling_edge.trigger(dut_eval_lambda);
                    break;
                case ClockStep::NegativeMidPoint:
                    level = false;
                    if (drive_clk_signal_fn) {
                        drive_clk_signal_fn(level);
                        dut->eval();
                    }
                    negative_mid.trigger(dut_eval_lambda);
                    break;

            }

            // Final evaluation after all phase events complete
            dut->eval();

            // Advnace to next step
            advance_step();

            // Schedule the next event
            uint64_t next_time = current_time + (period_ps / CLOCK_STEP_COUNT);
            schedule_next_event(next_time);
        }

        uint64_t get_next_event_time(uint64_t current_time) const {
            return current_time + (period_ps / CLOCK_STEP_COUNT);
        }

        ClockStep get_current_step() const {
            return current_step_;
        }
    private:
        ClockStep current_step_;
        uint64_t initial_offset_;
        EventSchedular<DutType>* scheduler_ = nullptr;

        void advance_step() {
            current_step_ = static_cast<ClockStep>(
                (static_cast<unsigned>(current_step_) + 1U) % CLOCK_STEP_COUNT
            );
        }

        void schedule_next_event(uint64_t time_ps) {
            if (scheduler_) {
                scheduler_->schedule_event(time_ps, this, current_step_);
            }
        }
    };
}
#endif // SIMULATION_CLOCK_H
