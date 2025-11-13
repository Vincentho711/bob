#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H
#include "simulation_phase_event.h"
#include <verilated_vcd_c.h>
#include <string>
#include <memory>

namespace simulation {
    enum class ClockStep : uint8_t {
        RisingEdge = 0U,
        PositiveMidPoint = 1U,
        FallingEdge = 2U,
        NegativeMidPoint = 3U,
        Count
    };

    constexpr std::size_t CLOCK_STEP_COUNT = static_cast<std::size_t>(ClockStep::Count);

    template <typename DutType>
    struct Clock {
        std::string name;
        uint64_t period_ps;
        std::shared_ptr<DutType> dut;
        std::shared_ptr<VerilatedVcdC> trace;
        bool dump_trace;
        bool level = false;
        uint64_t next_event_time = 0U;
        ClockStep step = ClockStep::RisingEdge;

        PhaseEvent rising_edge, positive_mid, falling_edge, negative_mid;

        Clock(const std::string &n, uint64_t ps, std::shared_ptr<DutType> dut, std::shared_ptr<VerilatedVcdC>trace) : name(n), period_ps(ps), dut(dut), trace(trace) {}

        void tick(uint64_t time) {
            if (time >= next_event_time) {
                // Give Verilator to settle any combinational logic that might have changed since the last clock
                dut->eval();
                // if (dump_trace) {
                //     if (trace) {
                //         trace->dump(time);
                //     }
                // }
                switch (step) {
                    case ClockStep::RisingEdge:
                        level = true;
                        rising_edge.trigger();
                        break;
                    case ClockStep::PositiveMidPoint:
                        level = true;
                        positive_mid.trigger();
                        break;
                    case ClockStep::FallingEdge:
                        level = false;
                        falling_edge.trigger();
                        break;
                    case ClockStep::NegativeMidPoint:
                        level = false;
                        negative_mid.trigger();
                        break;
                }
                // Evaluate DUT
                dut->eval();
                if (dump_trace) {
                  if (trace) {
                    trace->dump(time);
                  }
                }
                step = static_cast<ClockStep>((static_cast<unsigned>(step) + 1U) % static_cast<unsigned>(CLOCK_STEP_COUNT));
                next_event_time = time + period_ps / CLOCK_STEP_COUNT;
            }
        }
    };
}
#endif
