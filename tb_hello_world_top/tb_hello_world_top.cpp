#include <iostream>
#include <memory>
#include <random>
#include <cstdint>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

constexpr uint64_t MAX_SIM_TIME = 20;
constexpr int TRACE_DEPTH = 5;

class VerilatorSim{
public:
    VerilatorSim(int argc, char** argv)
        : seed(parse_verilator_seed(argc, argv)),
          rng(seed),
          dist(0, 255),
          dut(std::make_unique<Vhello_world_top>()),
          trace(std::make_unique<VerilatedVcdC>()),
          sim_time(0) {

        std::cout << "Using simulation seed: " << seed << std::endl;
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
    const uint32_t seed;
    std::mt19937 rng;  // Mersenne Twister engine
    std::uniform_int_distribution<uint8_t> dist;  // 0-255 for 8-bit inputs

    std::unique_ptr<Vhello_world_top> dut;
    std::unique_ptr<VerilatedVcdC> trace;
    uint64_t sim_time;

    void toggle_clock() {
        dut->clk_i = !dut->clk_i;
    }

    void apply_inputs() {
        // Simple test stimulus
        // Apply random values every rising clock edge
        dut->a_i = dist(rng);
        dut->b_i = dist(rng);
    }
    void log_outputs() {
          std::cout << "Cycle: " << (sim_time >> 1)
                    << " | a: " << +dut->a_i
                    << " | b: " << +dut->b_i
                    << " | c_o: " << +dut->c_o << '\n';
    }

    static uint32_t parse_verilator_seed(int argc, char** argv) {
        constexpr uint32_t MAX_VERILATOR_SEED = 2147483647u; // 2^31 - 1
        for (int i = 0; i < argc; ++i) {
            std::string arg(argv[i]);
            std::string prefix = "+verilator+seed+";
            if (arg.find(prefix) == 0) {
                try {
                    uint64_t val = std::stoull(arg.substr(prefix.length()));
                    if (val > 0 && val <= MAX_VERILATOR_SEED) {
                        return static_cast<uint32_t>(val);
                    } else {
                        std::cerr << "Warning: Verilator seed out of range (1 to "
                                  << MAX_VERILATOR_SEED << "): " << val << "\n";
                    }
                } catch (...) {
                    std::cerr << "Warning: Invalid seed format in " << arg << std::endl;
                }
            }
        }
        // Fallback: generate random seed in valid range
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(1, MAX_VERILATOR_SEED);
        return dist(gen);
    }
};

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);  // Initialise Verilator CLI args

    VerilatorSim sim(argc, argv);
    sim.run();

    return 0;
}
