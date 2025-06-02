#include <iostream>
#include <memory>
#include <random>
#include <cstdint>
#include <array>
#include <queue>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

#include "command_line_parser.h"
#include "tb_verification_framework.h"
#include "adder_verification/adder_transaction.h"
#include "adder_verification/adder_driver.h"

constexpr int TRACE_DEPTH = 5;

class VerilatorSim{
public:
    VerilatorSim(uint32_t seed, uint64_t max_sim_time)
        : seed(seed),
          max_sim_time(max_sim_time),
          rng(seed) {

        Verilated::traceEverOn(true);
        Verilated::randSeed(seed);

        // Create DUT as shared_ptr for driver
        dut_ = std::make_shared<Vhello_world_top>();  // create DUT after seed is set

        // Setup tracing
        trace_ = std::make_unique<VerilatedVcdC>();
        dut_->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_hello_world_top.vcd");

        // Initialize DUT
        dut_->clk_i = 0;
        dut_->a_i = 0;
        dut_->b_i = 0;

        driver = std::make_unique<AdderDriver>("main_adder_driver", dut, driver_config);

        // Generate test stimulus using the driver
        generate_test_stimulus();

        // Set up verification environment
        ve.set_pipeline_delay(2);
        register_transactions_with_ve();
        std::cout << "Verification environment initialised with " << transactions.size() << std::endl;
    }

    void check_design(uint64_t current_cycle) {
    // The design has a 2-cycle delay (adder + output register)
    // So output at cycle N corresponds to inputs from cycle N-2
    if (current_cycle > 2 && input_history.size() >= 2) {
        size_t history_index = input_history.size() - 2;  // 2 cycles ago
        const auto& hist = input_history[history_index];

        if (hist.txn) {
            // Verify that the stored expected result matches our calculation
            uint16_t expected = hist.txn->get_expected_result();
            if (expected != (static_cast<uint16_t>(hist.a) + static_cast<uint16_t>(hist.b))) {
                std::cout << "[WARNING] Transaction expected result mismatch!" << std::endl;
            }

            std::cout << "[VERIFY] Checking transaction: " << hist.txn->get_name()
                      << " (ID: " << hist.txn->get_transaction_id() << ")" << std::endl;
        }

        // Perform verification checks using the verification environment
        ve.check_adder(hist.a, hist.b, dut->c_o, current_cycle);
    }
}

    void run() {
        uint64_t cycle_count = 0;
        ve.start_test_timer();

        while (sim_time < max_sim_time && !Verilated::gotFinish()) {
            toggle_clock();

            if (dut->clk_i == 1 && (cycle_count > 0)) {
                apply_inputs(cycle_count);
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
    // DUT and simulation infrastructure
    std::shared_ptr<Vhello_world_top> dut_;
    std::unqiue_ptr<VerilatedVcdC> trace_;
    // Driver
    std::unique_ptr<AdderDriver> driver_;

    // Transaction management
    std::vector<std::unique_ptr<AdderTransaction>> transactions_;

    // Simulation parameters
    const uint32_t seed;
    const uint64_t max_sim_time;
    uint64_t sim_time{0};
    uint64_t cycle_count_{0};
    size_t current_txn_index_{0};

    // Random number generation
    std::mt19937 rng;  // Mersenne Twister engine
    std::uniform_int_distribution<uint8_t> dist;  // 0-255 for 8-bit inputs


    // Verification environment
    VerificationEnvironment ve;

    // History for pipeline delay tracking
    struct InputHistory {
        uint8_t a;
        uint8_t b;
        uint64_t cycle;
        std::unique_ptr<AdderTransaction> txn;
    };
    std::vector<InputHistory> input_history;

    void toggle_clock() {
        dut->clk_i = !dut->clk_i;
    }

    void generate_transactions() {
        // First, add all corner cases for comprehensive coverage
        driver_->generate_corner_cases();
        size_t corner_case_transactions = driver_.pending_count();

        // Then add random transactions to fill remaining cycles
        size_t total_cycles = max_sim_time / 2;  // Convert sim_time to cycles
        size_t remaining_cycles = (total_cycles > corner_case_transactions) ? 
                                  (total_cycles - corner_case_transactions) : 0;
        driver_->generate_random_transactions(remaining_cycles, rng);
        size_t random_transactions = driver_.pending_count() - corner_case_transactions;

        std::cout << "Generated " << corner_case_transactions << " corner case transactions and " 
                  << random_transactions << " random transactions." << std::endl;
    }

    void apply_inputs(uint64_t cycle_count) {
        uint8_t a_val = 0, b_val = 0;
        std::unique_ptr<AdderTransaction> current_txn = nullptr;
  
        if (current_txn_index < transactions.size()) {
            // Use transaction-based stimulus
            const auto& txn = transactions[current_txn_index];
            a_val = txn->get_a();
            b_val = txn->get_b();

            // Clone the transaction for history tracking
            current_txn = std::unique_ptr<AdderTransaction>(
                static_cast<AdderTransaction*>(txn->clone().release())
            );

            std::cout << "[TXN] Cycle " << cycle_count << ": " << txn->convert2string() << std::endl;

            current_txn_index++;
        }else{
            // Fallback to random if we run out of transactions
            std::uniform_int_distribution<uint8_t> dist(0, 255);
            a_val = dist(rng);
            b_val = dist(rng);

            std::string name = "fallback_random_" + std::to_string(cycle_count);
            current_txn = AdderTransactionFactory::create_directed(a_val, b_val, name);
        }

        dut->a_i = a_val;
        dut->b_i = b_val;

        // Store input history with transaction for pipeline delay handling
        InputHistory hist;
        hist.a = a_val;
        hist.b = b_val;
        hist.cycle = cycle_count;
        hist.txn = std::move(current_txn);
        input_history.push_back(std::move(hist));
    }

    void log_outputs(uint64_t cycle) {
          std::cout << "Cycle: "  << cycle
                    << " | a: "   << +dut->a_i
                    << " | b: "   << +dut->b_i
                    << " | c_o: " << +dut->c_o << '\n';
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

    CommandLineParser cli_parser;
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
