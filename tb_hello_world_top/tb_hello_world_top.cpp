#include <iostream>
#include <memory>
#include <csdint>
#include <chrono>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

constexpr uint64_t MAX_SIM_TIME = 20;
constexpr int TRACE_DEPTH = 5;

class VerilatorSim{
public:
    VerilatorSim()
        : dut(std::make_unique<Vhello_world_top>()), trace(std::make_unique<VerilatedVcdC>()), sim_time(0) {
        Verilated::traceEverOn(true);
        dut->trace(trace.get(), TRACE_DEPTH);
        trace->open("tb_hello_world_top.vcd");

        // Initialize inputs
        dut->clk_i = 0;
        dut->a_i = 0;
        dut->b_i = 0;
    }

    void run() {
        while (sim_time < MAX_SIM_TIME && !Verilated::gotFinish()) {
            toggle_clock();

            if (dut->clk_i == 1) {
                apply_inputs();
            }

            dut->eval();
            trace->dump(sim_time);

            if (dut->clk_i == 1) {
                log_outputs();
            }

            ++sim_time;
        }

        std::cout << "Simulation completed." << std::endl;
    }

    ~VerilatorSim() {
        trace->close();
    }

private:
    std::unique_ptr<Valu> dut;
    std::unique_ptr<VerilatedVcdC> trace;
    uint64_t sim_time;

    void toggle_clock() {
        dut->clk_i = !dut->clk_i;
    }

    void apply_inputs() {
        // Simple test stimulus
        // Apply new inputs every few cycles
        if ((sim_time >> 1) < 5) {
            dut->a_i = sim_time;
            dut->b_i = sim_time * 2;
        }
    }
    void log_outputs() {
          std::cout << "Cycle: " << (sim_time >> 1)
                    << " | a: " << +dut->a_i
                    << " | b: " << +dut->b_i
                    << " | c_o: " << +dut->c_o << '\n';
    }
};
}
int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);  // Initialize Verilator CLI args

    VerilatorSim sim;
    sim.run();

    return 0;
}
