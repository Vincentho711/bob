#include <iostream>
#include <memory>
#include <random>
#include <cstdint>
#include <array>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

#include "tb_command_line_parser.h"
#include "tb_verification_framework.h"

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

        std::cout << "Verification environment initialised." << std::endl;
    }

    void run() {
        uint64_t cycle_count = 0;
        ve.start_test_timer();

        while (sim_time < max_sim_time && !Verilated::gotFinish()) {
            toggle_clock();

            if (dut->clk_i == 1 && (cycle_count > 0)) {
                apply_inputs();
            }

            dut->eval();
            trace->dump(sim_time);

            if (dut->clk_i == 1) {
                cycle_count++;
                log_outputs(cycle_count);

                // Perform verification checks
                // Note: Due to the pipeline nature (2 cycle delay), we need to account for this
                if (cycle_count >= 2) {
                    check_design(cycle_count);
                }
            }

            ++sim_time;
        }

        std::cout << "Simulation completed after " << cycle_count << " cycles." << std::endl;

        // Set simulation info for debug reporting
        ve.set_simulation_info(seed, max_sim_time / 2, "tb_hello_world_top.vcd");

        // Generate final verification report
        ve.final_report();
    }

    ~VerilatorSim() {
        trace->close();
    }

    bool passed() const {
        return ve.simulation_passed();
    }

private:
    const uint32_t seed;
    const uint64_t max_sim_time;
    std::mt19937 rng;  // Mersenne Twister engine
    std::uniform_int_distribution<uint8_t> dist;  // 0-255 for 8-bit inputs

    std::unique_ptr<Vhello_world_top> dut;
    std::unique_ptr<VerilatedVcdC> trace{std::make_unique<VerilatedVcdC>()};
    uint64_t sim_time;

    // Verification environment
    VerificationEnvironment ve;

    // History for pipeline delay tracking
    struct InputHistory {
        uint8_t a;
        uint8_t b;
        uint64_t cycle;
    };
    std::vector<InputHistory> input_history;

    void toggle_clock() {
        dut->clk_i = !dut->clk_i;
    }

    void apply_inputs() {
         // Simple test stimulus with some directed tests mixed in
        uint8_t a_val, b_val;

        // Mix random and directed tests
        if (input_history.size() < 20) {
            // First 20 cycles: use some directed corner cases
            switch (input_history.size()) {
                case 0: a_val = 0; b_val = 0; break;           // Min + Min
                case 1: a_val = 255; b_val = 255; break;       // Max + Max
                case 2: a_val = 0; b_val = 255; break;         // Min + Max
                case 3: a_val = 255; b_val = 0; break;         // Max + Min
                case 4: a_val = 128; b_val = 128; break;       // Mid + Mid
                case 5: a_val = 1; b_val = 1; break;           // Small values
                case 6: a_val = 254; b_val = 1; break;         // Near overflow
                case 7: a_val = 1; b_val = 254; break;         // Near overflow
                case 8: a_val = 127; b_val = 128; break;       // Around midpoint
                case 9: a_val = 128; b_val = 127; break;       // Around midpoint
                case 10: a_val = 254; b_val = 254; break;     // Near maximum
                case 11: a_val = 255; b_val = 1; break;       // Overflow boundary
                case 12: a_val = 1; b_val = 255; break;       // Overflow boundary
                default:
                    a_val = dist(rng);
                    b_val = dist(rng);
                    break;
            }
        } else {
            // Rest: random values
            a_val = dist(rng);
            b_val = dist(rng);
        }

        dut->a_i = a_val;
        dut->b_i = b_val;

        // Store input history for pipeline delay handling
        input_history.push_back({a_val, b_val, (sim_time >> 1) + 1});
    }

    void log_outputs(uint64_t cycle) {
          std::cout << "Cycle: "  << cycle
                    << " | a: "   << +dut->a_i
                    << " | b: "   << +dut->b_i
                    << " | c_o: " << +dut->c_o << '\n';
    }

    void check_design(uint64_t current_cycle) {
        // The design has a 2-cycle delay (adder + output register)
        // So output at cycle N corresponds to inputs from cycle N-2
        // Only start checking after pipeline delay + 1 to account for random initial values
        if (current_cycle > 2 && input_history.size() >= 2) {
            size_t history_index = input_history.size() - 2;  // 2 cycles ago
            const auto& hist = input_history[history_index];

            // Verify the adder functionality
            ve.check_adder(hist.a, hist.b, dut->c_o, current_cycle);
        }
    }
};

// Function to generate a truly random seed using hardware entropy
uint32_t generate_random_seed() {
    std::random_device rd;

    // Generate random data for all internal bits of the MT19937 engine
    std::array<uint32_t, std::mt19937::state_size> seed_data{};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));

    // Create a seed_seq object from the truly random data
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));

    // Create a properly seeded MT19937 engine
    std::mt19937 seeded_rng{seq};

    // Generate and return a truly random seed
    return seeded_rng();

}

int main(int argc, char** argv) {
    Verilated::randReset(2); // Equivalent to setting +verilator+rand+reset+2
    Verilated::commandArgs(argc, argv);  // Initialise Verilator CLI args

    TbCommandLineParser cli_parser;
    cli_parser.add_argument("--seed", "Simulation seed (integer in range 1 - 2^31 - 1)", false, true);
    cli_parser.add_argument("--cycles", "Maximum simulation cycles", false, true);
    cli_parser.set_default_value("--cycles", "50");

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
            if (seed == 0) {
                std::cerr << "Error: Seed cannot be 0\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "Invalid seed: " << *seed_opt << "\n";
            return 1;
        }
        std::cout << "Using user-provided seed: " << seed << "\n";
    } else {
        // Generate a random seed
        seed = generate_random_seed();
        std::cout << "Using generated random seed: " << seed << "\n";
    }

    uint64_t max_cycles = 50;
    if (auto cycles_opt = cli_parser.get("--cycles"); cycles_opt) {
        try {
            max_cycles = std::stoull(*cycles_opt);
            if (max_cycles == 0) {
                std::cerr << "Error: Cycles must be greater than 0\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "Invalid cycles value: " << *cycles_opt << "\n";
            return 1;
        }
    }
    std::cout << "Max cycles: " << max_cycles << "\n";

    // Convert cycles to simulation time (each cycle = 2 sim_time units due to clock toggle)
    uint64_t max_sim_time = max_cycles * 2;

    VerilatorSim sim(seed, max_sim_time);
    sim.run();

    // Return appropriate exit code based on verification results
    return sim.passed() ? 0 : 1;
}
