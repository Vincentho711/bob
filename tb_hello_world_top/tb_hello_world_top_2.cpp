#include <iostream>
#include <memory>
#include <random>
#include <cstdint>
#include <array>
#include <chrono>
#include <cassert>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include <Vhello_world_top.h>

#include "command_line_parser.h"
#include "adder_verification/adder_transaction.h"
#include "adder_verification/adder_driver.h"
#include "adder_verification/adder_checker.h"
#include "adder_verification/adder_simulation_context.h"

constexpr int TRACE_DEPTH = 5;
constexpr uint32_t PIPELINE_DEPTH = 2;

/**
 * @brief Modern Verilator simulation class using adder verification components
 */
class VerilatorSim {
public:
    /**
     * @brief Constructor - Initialize simulation and verification components
     */
    VerilatorSim(uint32_t seed, uint64_t max_cycles) 
        : seed_(seed)
        , max_cycles_(max_cycles)
        , sim_time_(0)
        , rng_(seed) {
        
        // Initialize Verilator
        Verilated::traceEverOn(true);
        Verilated::randSeed(seed);
        
        // Create DUT as shared pointer
        dut_ = std::make_shared<Vhello_world_top>();
        
        // Setup waveform tracing
        setup_tracing();
        
        // Initialize DUT
        reset_dut();
        
        // Create verification components
        setup_verification_components();
        
        // Generate test stimulus
        generate_test_stimulus();
        
        std::cout << "=== Modern Adder Testbench Initialized ===" << std::endl;
        std::cout << "Seed: " << seed_ << std::endl;
        std::cout << "Max Cycles: " << max_cycles_ << std::endl;
        std::cout << "Pipeline Depth: " << PIPELINE_DEPTH << std::endl;
        std::cout << "Transactions Generated: " << driver_->pending_count() << std::endl;
    }
    
    /**
     * @brief Destructor - Clean up resources
     */
    ~VerilatorSim() {
        if (trace_) {
            trace_->close();
        }
        
        // Print final reports
        print_final_reports();
    }
    
    /**
     * @brief Run the simulation
     */
    void run() {
        std::cout << "\n=== Starting Simulation ===" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        while (sim_time_ < (max_cycles_ * 2) && !Verilated::gotFinish()) {
            // Toggle clock
            dut_->clk_i = !dut_->clk_i;
            
            // Process positive clock edge
            if (dut_->clk_i == 1) {
                ctx_->increment_cycle();
                process_clock_cycle();
            }
            
            // Evaluate DUT
            dut_->eval();
            
            // Dump waveform
            if (trace_) {
                trace_->dump(sim_time_);
            }
            
            sim_time_++;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n=== Simulation Complete ===" << std::endl;
        std::cout << "Total Cycles: " << ctx_->current_cycle() << std::endl;
        std::cout << "Simulation Time: " << duration.count() << " ms" << std::endl;
    }
    
    /**
     * @brief Check if simulation passed
     */
    bool passed() const {
        // Check if checker reports pass
        bool checker_passed = checker_->is_pass_rate_acceptable();
        std::cout << "\nchecker_passed = " << checker_passed << std::endl;

        // Check if all transactions were processed
        bool all_transactions_driven = (driver_->pending_count() == 0);
        std::cout << "driver_->pending_count() = " << driver_->pending_count() << " , all_transactions_driven = " << all_transactions_driven << std::endl;

        // Check basic statistics
        const auto& checker_stats = checker_->get_adder_stats();
        bool has_sufficient_coverage = (checker_stats.corner_cases_checked > 0) && 
                                      (checker_stats.overflow_cases_checked > 0);
        std::cout << "has_sufficient_coverage = " << has_sufficient_coverage << std::endl;

        return checker_passed && all_transactions_driven && has_sufficient_coverage;
    }

private:
    // Simulation parameters
    const uint32_t seed_;
    const uint64_t max_cycles_;
    uint64_t sim_time_;
    std::mt19937 rng_;

    // Verilator components
    std::shared_ptr<Vhello_world_top> dut_;
    std::unique_ptr<VerilatedVcdC> trace_;

    // Verification components
    std::shared_ptr<AdderSimulationContext> ctx_;
    std::unique_ptr<AdderDriver> driver_;
    std::unique_ptr<AdderChecker<Vhello_world_top>> checker_;

    /**
     * @brief Setup waveform tracing
     */
    void setup_tracing() {
        trace_ = std::make_unique<VerilatedVcdC>();
        dut_->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_hello_world_top.vcd");
        std::cout << "Waveform tracing enabled: tb_hello_world_top.vcd" << std::endl;
    }
    
    /**
     * @brief Reset DUT to known state
     */
    void reset_dut() {
        dut_->clk_i = 0;
        dut_->a_i = 0;
        dut_->b_i = 0;
        dut_->eval();
    }

    /**
     * @brief Setup verification components (driver and checker)
     */
    void setup_verification_components() {
        // Configure simulation context
        ctx_ = std::make_shared<AdderSimulationContext>();

        // Configure driver
        AdderDriver::Config driver_config;
        driver_config.enable_input_validation = true;
        driver_config.enable_pipeline_tracking = true;
        driver_config.pipeline_depth = PIPELINE_DEPTH;
        driver_config.idle_value_a = 0;
        driver_config.idle_value_b = 0;

        // Create driver
        driver_ = std::make_unique<AdderDriver>("main_adder_driver", dut_, ctx_, driver_config);

        // Configure checker
        AdderCheckerConfig checker_config = create_strict_adder_config();
        checker_config.pipeline_depth = PIPELINE_DEPTH;
        checker_config.max_pending_transactions = static_cast<uint32_t>(max_cycles_);
        checker_config.min_pass_rate = 0.95; // 95% pass rate required
        checker_config.enable_value_logging = true;

        // Create checker
        checker_ = std::make_unique<AdderChecker<Vhello_world_top>>("main_adder_checker", dut_, ctx_, checker_config);

        std::cout << "Verification components initialized" << std::endl;
    }

    /**
     * @brief Generate comprehensive test stimulus
     */
    void generate_test_stimulus() {
        // Generate corner cases first (ensures good coverage)
        driver_->generate_corner_cases("corner");
        size_t corner_cases = driver_->pending_count();

        // Calculate remaining cycles for random tests
        size_t remaining_cycles = (max_cycles_ > corner_cases) ? (max_cycles_ - corner_cases) : 0;

        if (remaining_cycles > 0) {
            // Generate random transactions
            driver_->generate_random_transactions(remaining_cycles, rng_, "random");

            // // Add some directed cases for specific scenarios
            // add_directed_test_cases();
            //
            // // Add some idle cycles for pipeline flushing
            // if (remaining_cycles > 10) {
            //     driver_->drive_idle_cycles(5);
            // }
        }

        std::cout << "Test stimulus generated:" << std::endl;
        std::cout << "  Corner cases: " << corner_cases << std::endl;
        std::cout << "  Total transactions: " << driver_->pending_count() << std::endl;
    }

    /**
     * @brief Add specific directed test cases
     */
    void add_directed_test_cases() {
        // Test specific overflow scenarios
        driver_->add_directed_transaction(200, 100, "directed_overflow_1");
        driver_->add_directed_transaction(150, 150, "directed_overflow_2");
        driver_->add_directed_transaction(255, 255, "directed_max_overflow");

        // Test boundary conditions
        driver_->add_directed_transaction(128, 127, "directed_mid_boundary_1");
        driver_->add_directed_transaction(127, 128, "directed_mid_boundary_2");
  
        // Test power-of-2 values
        for (uint8_t i = 0; i < 8; ++i) {
            uint8_t val = 1 << i;
            driver_->add_directed_transaction(val, val, "directed_pow2_" + std::to_string(i));
        }
    }
    
    /**
     * @brief Process a single clock cycle
     */
    void process_clock_cycle() {
        // Drive inputs using the driver
        drive_inputs();

        // Check outputs using the checker (accounts for pipeline delay)
        check_outputs();

        // Log current state for debugging
        log_cycle_state();
    }

    /**
     * @brief Drive inputs for current cycle
     */
    void drive_inputs() {
        uint64_t current_cycle = ctx_->current_cycle();
        // Only start driving transaction after PIPELINE_DEPTH to ensure that all transactions driven are checked
        if (current_cycle >= PIPELINE_DEPTH){
            if (driver_->has_pending_transactions()) {
                // Get the next transaction
                AdderDriver::TransactionPtr next_transaction = driver_->get_next_transaction();
                // Add transaction to checker. expect_transaction accounts for the pipeline depth
                checker_->expect_transaction(next_transaction);
                // Drive the transaction
                driver_->drive_next();
            } else {
                // Drive idle values when no more transactions
                driver_->drive_idle_cycles(1);
            }
        }else{
            driver_->drive_idle_cycles(1);
        }
    }

    /**
     * @brief Check outputs for current cycle
     */
    void check_outputs() {
        uint64_t current_cycle = ctx_->current_cycle();
        // The checker handles pipeline delay internally
        // It will only check when appropriate transactions are available
        try {
            if (current_cycle >= PIPELINE_DEPTH) {
                checker_->check_cycle();
            }
        } catch (const std::exception& e) {
            std::cerr << "Checker error at cycle " << current_cycle << ": " << e.what() << std::endl;
        }
    }

    /**
     * @brief Log current cycle state for debugging
     */
    void log_cycle_state() {
        uint64_t current_cycle = ctx_->current_cycle();
        if (current_cycle <= max_cycles_) {
            std::cout << "Cycle " << std::setw(4) << current_cycle
                      << " | a_i: " << std::setw(3) << +dut_->a_i
                      << " | b_i: " << std::setw(3) << +dut_->b_i
                      << " | c_o: " << std::setw(3) << +dut_->c_o
                      << " | Pending: " << driver_->pending_count()
                      << std::endl;
        }
    }
    
    /**
     * @brief Print comprehensive final reports
     */
    void print_final_reports() const {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "FINAL SIMULATION REPORT" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        // Print driver report
        if (driver_) {
            driver_->print_report();
        }
        
        std::cout << std::endl;
        
        // Print checker report  
        if (checker_) {
            checker_->print_detailed_report();
            checker_->print_histogram();
        }
        
        // Print overall pass/fail status
        std::cout << "\n" << std::string(80, '-') << std::endl;
        std::cout << "OVERALL RESULT: " << (passed() ? "PASS" : "FAIL") << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // Print simulation summary
        std::cout << "\nSimulation Summary:" << std::endl;
        std::cout << "  Seed: " << seed_ << std::endl;
        std::cout << "  Cycles: " << ctx_->current_cycle() << std::endl;
        std::cout << "  Waveform: tb_hello_world_top.vcd" << std::endl;
        
        if (checker_) {
            std::cout << "  Pass Rate: " << std::fixed << std::setprecision(2) 
                      << (checker_->get_current_pass_rate() * 100.0) << "%" << std::endl;
        }
    }
};

/**
 * @brief Generate a cryptographically secure random seed
 */
uint32_t generate_secure_random_seed() {
    std::random_device rd;
    std::array<uint32_t, std::mt19937::state_size> seed_data{};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 seeded_rng{seq};
    return seeded_rng();
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Initialize Verilator
    Verilated::randReset(2);
    Verilated::commandArgs(argc, argv);
    
    // Parse command line arguments
    CommandLineParser cli_parser;
    cli_parser.add_argument("--seed", "Simulation seed (1 to 2^31-1)", false, true);
    cli_parser.add_argument("--cycles", "Maximum simulation cycles", false, true);
    cli_parser.set_default_value("--cycles", "100");
    
    try {
        cli_parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        cli_parser.print_help(argv[0]);
        return 1;
    }
    
    // Process seed argument
    uint32_t seed;
    if (auto seed_opt = cli_parser.get("--seed")) {
        try {
            seed = std::stoul(*seed_opt);
            if (seed == 0) {
                std::cerr << "Error: Seed cannot be 0" << std::endl;
                return 1;
            }
            std::cout << "Using user-provided seed: " << seed << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Invalid seed: " << *seed_opt << std::endl;
            return 1;
        }
    } else {
        seed = generate_secure_random_seed();
        std::cout << "Using generated random seed: " << seed << std::endl;
    }
    
    // Process cycles argument
    uint64_t max_cycles;
    try {
        auto cycles_opt = cli_parser.get("--cycles");
        max_cycles = cycles_opt ? std::stoull(*cycles_opt) : 100;
        if (max_cycles == 0) {
            std::cerr << "Error: Cycles must be greater than 0" << std::endl;
            return 1;
        }
        std::cout << "Maximum cycles: " << max_cycles << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Invalid cycles value" << std::endl;
        return 1;
    }
    
    // Run simulation
    try {
        VerilatorSim sim(seed, max_cycles);
        sim.run();
        return sim.passed() ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Simulation error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown simulation error occurred" << std::endl;
        return 1;
    }
}
