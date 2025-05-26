#include <iostream>
#include <memory>
#include <random>
#include <cstdint>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

#include "tb_command_line_parser.h"

constexpr int TRACE_DEPTH = 5;

class VerilatorSim{
public:
    VerilatorSim(uint32_t seed, uint64_t max_sim_time)
        : seed(seed),
          max_sim_time(max_sim_time),
          rng(seed),
          dist(0, 255),
          sim_time(0) {

        Verilated::traceEverOn(true);
        Verilated::randSeed(seed);

        dut = std::make_unique<Vhello_world_top>();  // create DUT after seed is set

        dut->trace(trace.get(), TRACE_DEPTH);
        trace->open("tb_hello_world_top.vcd");

        // Initialize inputs
        dut->clk_i = 0;
        dut->a_i = 0;
        dut->b_i = 0;
    }

    void run() {
        while (sim_time < max_sim_time && !Verilated::gotFinish()) {
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
    const uint64_t max_sim_time;
    std::mt19937 rng;  // Mersenne Twister engine
    std::uniform_int_distribution<uint8_t> dist;  // 0-255 for 8-bit inputs

    std::unique_ptr<Vhello_world_top> dut;
    std::unique_ptr<VerilatedVcdC> trace{std::make_unique<VerilatedVcdC>()};
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
};

int main(int argc, char** argv) {
    Verilated::randReset(2); // Equivalent to setting +verilator+rand+reset+2
    Verilated::commandArgs(argc, argv);  // Initialise Verilator CLI args

    TbCommandLineParser cli_parser;
    cli_parser.add_argument("--seed", "Simulation seed (integer in range 1 - 2^31 - 1)", false, true);
    cli_parser.add_argument("--cycles", "Maximum simulation cycles", false, true);
    cli_parser.set_default_value("--cycles", "20");

    try {
        cli_parser.parse(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        cli_parser.print_help(argv[0]);
        return 1;
    }

    uint32_t seed;
    if (auto seed_opt = cli_parser.get("--seed"); seed_opt) {
        try {
            seed = std::stoul(*seed_opt);
        } catch (...) {
            std::cerr << "Invalid seed: " << *seed_opt << "\n";
            return 1;
        }
    } else {
        std::random_device rd;
        seed = rd();
    }

    uint64_t max_cycles = 20;
    if (auto cycles_opt = cli_parser.get("--cycles"); cycles_opt) {
        try {
            max_cycles = std::stoull(*cycles_opt);
        } catch (...) {
            std::cerr << "Invalid cycles value: " << *cycles_opt << "\n";
            return 1;
        }
    }

    std::cout << "Using seed: " << seed << "\n";
    std::cout << "Max cycles: " << max_cycles << "\n";

    VerilatorSim sim(seed, max_cycles);
    sim.run();

    return 0;
}
