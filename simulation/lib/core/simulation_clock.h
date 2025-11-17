#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H
#include "simulation_phase_event.h"
#include <string>
#include <iostream>
#include <memory>
#include <functional>

namespace simulation {
    enum class ClockStep : uint8_t {
        RisingEdge = 0U,
        PositiveMidPoint = 1U,
        FallingEdge = 2U,
        NegativeMidPoint = 3U,
    };

    constexpr std::size_t CLOCK_STEP_COUNT = 4;

    template <typename DutType>
    struct Clock {
        std::string name;
        uint64_t period_ps;
        std::shared_ptr<DutType> dut;
        bool level = false;
        uint64_t next_event_time = 0U;
        ClockStep step = ClockStep::RisingEdge;

        // Callback function to drive the DUT's clock input
        std::function<void(bool)> drive_clk_signal_fn;

        PhaseEvent rising_edge, positive_mid, falling_edge, negative_mid;

        Clock(const std::string &n, const uint64_t ps, std::shared_ptr<DutType> dut, std::function<void(bool)> clk_drive_fn)
            : name(n), period_ps(ps), dut(dut), drive_clk_signal_fn(clk_drive_fn) {}

        void tick(uint64_t time) {
            if (time >= next_event_time) {
                // Give Verilator to settle any combinational logic that might have changed since the last clock
                dut->eval();
                switch (step) {
                    case ClockStep::RisingEdge:
                        level = true;
                        if (drive_clk_signal_fn) {
                            drive_clk_signal_fn(level);
                        }
                        rising_edge.trigger();
                        break;
                    case ClockStep::PositiveMidPoint:
                        level = true;
                        if (drive_clk_signal_fn) {
                            drive_clk_signal_fn(level);
                        }
                        positive_mid.trigger();
                        break;
                    case ClockStep::FallingEdge:
                        level = false;
                        if (drive_clk_signal_fn) {
                            drive_clk_signal_fn(level);
                        }
                        falling_edge.trigger();
                        break;
                    case ClockStep::NegativeMidPoint:
                        level = false;
                        if (drive_clk_signal_fn) {
                            drive_clk_signal_fn(level);
                        }
                        negative_mid.trigger();
                        break;
                }
                // Evaluate DUT
                dut->eval();
                step = static_cast<ClockStep>((static_cast<unsigned>(step) + 1U) % static_cast<unsigned>(CLOCK_STEP_COUNT));
                next_event_time = time + period_ps / CLOCK_STEP_COUNT;
            }
        }
    };
}
#endif
