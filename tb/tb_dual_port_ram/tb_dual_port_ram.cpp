#include <memory>
#include <coroutine>
#include <iostream>
#include <functional>

#include "simulation_kernal.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_when_all.h"
#include "simulation_when_all_ready.h"
#include "simulation_exceptions.h"
#include "simulation_logging_utils.h"

#include "dual_port_ram_driver.h"
#include "dual_port_ram_top_sequence.h"
#include "dual_port_ram_sequencer.h"
#include "dual_port_ram_tlm_queue.h"
#include "dual_port_ram_monitor.h"
#include "dual_port_ram_scoreboard.h"
#include "testcases/directed/dual_port_ram_directed_testcases.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vdual_port_ram.h>
#include "Vdual_port_ram_dual_port_ram.h"

class BaseChecker {
    public:
        using clock_t = simulation::Clock<Vdual_port_ram>;

        BaseChecker(std::shared_ptr<clock_t> wr_clk)
            : wr_clk(wr_clk), logger_("BaseChecker") {}

        simulation::Task<> test_same_phase_event() {
            for (uint32_t i = 0 ; i < 3U ; ++i) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.debug("Test waiting for same phase event in a single task. 1.");
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.debug("Test waiting for same phase event in a single task. 2.");
            }
        }

        simulation::Task<> print_at_wr_clk_edges() {
            while (true) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.debug("Resuming after wr_clk rising_edge is seen.");
                co_await wr_clk->falling_edge(simulation::Phase::Drive);
                logger_.debug("Resuming after wr_clk falling_edge is seen.");
            }
        }

        simulation::Task<uint32_t> return_value_1(uint32_t value) {
            throw std::runtime_error("Testing the throwing of runtime error in void Task.");
            co_return value;
        }

        simulation::Task<uint32_t> return_value_2(uint32_t value) {
          co_return value + 1U;
        }

        simulation::Task<> when_all_ready_return_value_top_task() {
            auto [task1, task2] = co_await simulation::when_all_ready(
                return_value_1(20U),
                return_value_2(80U));

            logger_.info("task1 result=" + std::to_string(task1.result()));
            logger_.info("task2 result=" + std::to_string(task2.result()));
        }

        simulation::Task<> when_all_return_value_top_task() {
            auto [task1, task2] = co_await simulation::when_all(
                return_value_1(30U),
                return_value_2(40U)
            );

            logger_.info("task1 result=" + std::to_string(task1));
            logger_.info("task2 result=" + std::to_string(task2));
        }

        simulation::Task<> empty_task_1() {
            for (uint32_t i = 0 ; i < 5; ++i) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("empty_task_1's i = " + std::to_string(i));
             };
        }

        simulation::Task<> empty_task_2() {
            for (uint32_t i = 0 ; i < 6; ++i) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("empty_task_2's i = " + std::to_string(i));
                // if (i == 5) {
                //     logger_.fatal(
                //         std::string(simulation::colours::BOLD) +
                //         simulation::colours::BRIGHT_RED +
                //         "Testing the throwing of runtime error in void Task." +
                //         simulation::colours::RESET
                //     );
                // }
             };
        }

        simulation::Task<> empty_top_task() {
            std::vector<simulation::Task<>> tasks;
            tasks.emplace_back(empty_task_1());
            tasks.emplace_back(empty_task_2());
            // co_await returns the vector of internal WhenAllTask objects
            co_await simulation::when_all(std::move(tasks));
            // auto results = co_await simulation::when_all_ready(std::move(tasks));
            // for (auto & t : results) {
            //     // Check the result to make sure it didn't throw an exception
            //     t.result();
            // }
            logger_.info("empty_top_task done.");
        }

    private:
        std::shared_ptr<simulation::Clock<Vdual_port_ram>> wr_clk;
        simulation::Logger logger_;

};

class SimulationEnvironment {
public:
    SimulationEnvironment(uint32_t seed, uint64_t max_time)
        : seed_(seed),
          max_time_(max_time),
          logger_("SimEnv") {

        // ========================================================================
        // Configure Global Logging Settings
        // ========================================================================
        auto& log_config = simulation::LoggerConfig::instance();

        // Set up file logging if desired (can be controlled via command-line later)
        // log_config.set_log_file("sim.log", simulation::OutputMode::SEPARATE_LEVELS);
        // log_config.set_stdout_min_level(simulation::LogLevel::INFO);
        // log_config.set_file_min_level(simulation::LogLevel::DEBUG);

        // Configure timestamp display
        log_config.set_show_timestamp(true);
        // log_config.set_timestamp_precision(3);  // Show as 1000.000ps

        logger_.info("===========================================");
        logger_.info("Starting Dual Port RAM Simulation");
        logger_.info("===========================================");
        logger_.info("Seed: " + std::to_string(seed));
        logger_.info("Max Time: " + std::to_string(max_time) + "ps");

         // ========================================================================
        // Initialise Verilator
        // ========================================================================
        Verilated::traceEverOn(true);
        Verilated::randSeed(seed);

        // Create DUT as shared pointer
        dut_ = std::make_shared<Vdual_port_ram>();

        // Set up waveform tracing
        trace_= std::make_shared<VerilatedVcdC>();
        dut_->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_dual_port_ram.vcd");
        logger_.info("Waveform tracing enabled: tb_dual_port_ram.vcd");

        // ========================================================================
        // Initialise clocking components
        // ========================================================================
        std::function<void(bool)> wr_clk_drive_fn = [this](bool level) {
            dut_->wr_clk_i = level;  // Set the clk_i input of the DUT based on the clock level
        };
        wr_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("wr_clk", 5000U, dut_, wr_clk_drive_fn);
        // rd clk doesn't have to drive DUT clock signal, used for verification only.
        rd_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("rd_clk", 5000U, dut_, std::function<void(bool)>{});
        logger_.debug("Clock components initialised: ");
        logger_.debug("   wr_clk: period=5000ps");
        logger_.debug("   rd_clk: period=5000ps");

        // ========================================================================
        // Set up simulation components
        // ========================================================================
        sim_kernal_ = std::make_unique<simulation::SimulationKernal<Vdual_port_ram, VerilatedVcdC>>(dut_, trace_);

        // Register clocking componenets with simulation kernal
        sim_kernal_->register_clock(wr_clk_);
        sim_kernal_->register_clock(rd_clk_);
        logger_.debug("Clocks registered with simulation kernel");

        // ========================================================================
        // Set up verification components
        // ========================================================================
        tlm_wr_queue_ = std::make_shared<DualPortRamTLMWrQueue>();
        tlm_rd_queue_ = std::make_shared<DualPortRamTLMRdQueue>();
        sequencer_ = std::make_shared<DualPortRamSequencer>(wr_clk_, rd_clk_);
        // top_sequence_ = std::make_unique<DualPortRamTopSequence>();
        driver_ = std::make_shared<DualPortRamDriver>(sequencer_, dut_, wr_clk_, rd_clk_);
        monitor_ = std::make_shared<DualPortRamMonitor>(dut_, wr_clk_, rd_clk_, tlm_wr_queue_, tlm_rd_queue_);
        checker_ = std::make_shared<BaseChecker>(wr_clk_);
        scoreboard_ = std::make_shared<DualPortRamScoreboard>(tlm_wr_queue_, tlm_rd_queue_, wr_clk_, 1U);
        // explicit DualPortRamScoreboard(std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue, std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue, std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle, const std::string &name, bool debug_enabled);
        logger_.info("Verification components created");

        // ========================================================================
        // Set up top sequence to execute
        // ========================================================================
        const auto dual_port_ram_module_addr_width = Vdual_port_ram_dual_port_ram::ADDR_WIDTH;
        const auto dual_port_ram_module_data_width = Vdual_port_ram_dual_port_ram::DATA_WIDTH;
        const uint32_t addr_width_arg = static_cast<uint32_t>(dual_port_ram_module_addr_width);
        const uint32_t data_width_arg = static_cast<uint32_t>(dual_port_ram_module_data_width);
        top_seq_ = std::make_unique<DualPortRamTopSequence>(addr_width_arg, data_width_arg, seed_);
        logger_.info("Top sequence configured (ADDR_WIDTH=" + std::to_string(addr_width_arg) + 
                    ", DATA_WIDTH=" + std::to_string(data_width_arg) + ")");

        // ========================================================================
        // Set up task components
        // ========================================================================
        logger_.debug("Creating coroutine tasks...");
        // coro_tasks.emplace_back(checker_->test_same_phase_event());
        // coro_tasks.emplace_back(checker_->when_all_ready_return_value_top_task());
        // coro_tasks.emplace_back(checker_->when_all_return_value_top_task());
        coro_tasks.emplace_back(checker_->empty_top_task());
        // coro_tasks.emplace_back(checker_->print_at_wr_clk_edges());
        coro_tasks.emplace_back(sequencer_->start_sequence(std::move(top_seq_)));
        coro_tasks.emplace_back(driver_->wr_driver_run());
        coro_tasks.emplace_back(scoreboard_->update_ram_model());
        coro_tasks.emplace_back(driver_->rd_driver_run());
        coro_tasks.emplace_back(monitor_->wr_port_run());
        coro_tasks.emplace_back(monitor_->rd_port_run());
        coro_tasks.emplace_back(scoreboard_->run_read_capture());
        coro_tasks.emplace_back(scoreboard_->run_write_capture());

        logger_.info("Created " + std::to_string(coro_tasks.size()) + " coroutine tasks");
        logger_.info("===========================================");

    }

    ~SimulationEnvironment() {
        if (trace_) {
            // Capture the last signals
            dut_->eval();
            trace_->dump(sim_kernal_->time);
            trace_->close();
            logger_.info("Waveform trace closed");
        }
    }

    void start_sim_kernal() {
        logger_.info("Starting simulation kernel...");
        // Pass a pointer of coro_tasks to the simulation kernal for error handling
        sim_kernal_->root_tasks = &coro_tasks;
        // Resume all root level coroutines as they are not started upon creation
        try {
            for (simulation::Task<> &task : coro_tasks) {
                task.start();
            }
            sim_kernal_->run(max_time_);
        } catch (...) {
            throw;
        }
    }

private:
    // Simulation parameters
    const int32_t TRACE_DEPTH = 5;
    const uint64_t seed_;
    const uint64_t max_time_;

    // Logger for this class
    simulation::Logger logger_;

    // Verilator components
    std::shared_ptr<Vdual_port_ram> dut_;
    std::shared_ptr<VerilatedVcdC> trace_;

    // Simulation components
    std::unique_ptr<simulation::SimulationKernal<Vdual_port_ram, VerilatedVcdC>> sim_kernal_;

    // Clocking components
    std::shared_ptr<simulation::Clock<Vdual_port_ram>> wr_clk_;
    std::shared_ptr<simulation::Clock<Vdual_port_ram>> rd_clk_;

    // Verification components
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue_;
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue_;
    std::shared_ptr<BaseChecker> checker_;
    std::shared_ptr<DualPortRamSequencer> sequencer_;
    std::shared_ptr<DualPortRamDriver> driver_;
    std::shared_ptr<DualPortRamMonitor> monitor_;
    std::shared_ptr<DualPortRamScoreboard> scoreboard_;

    // Sequence components
    std::unique_ptr<DualPortRamTopSequence> top_seq_;

    // Task componenets
    std::vector<simulation::Task<>> coro_tasks;
};

int main() {
    // ============================================================================
    // Global Logger Configuration (affects all loggers in the simulation)
    // ============================================================================
    auto& global_log_config = simulation::LoggerConfig::instance();

    // Example configurations:
    // global_log_config.set_log_file("simulation.log", simulation::OutputMode::SEPARATE_LEVELS);
    // global_log_config.set_stdout_min_level(simulation::LogLevel::INFO);   // Console: less verbose
    // global_log_config.set_file_min_level(simulation::LogLevel::DEBUG);    // File: captur

    // ============================================================================
    // Run Simulation
    // ============================================================================
    simulation::Logger main_logger("Main");
    try {
        SimulationEnvironment sim_env(123U, 500000U);
        sim_env.start_sim_kernal();

        main_logger.test_passed("Simulation Passed");
        return 0;
    } catch (const simulation::VerificationError &e) {
        main_logger.test_failed(std::string("Verification Error: ") + e.what());
        return 1;
    } catch (const std::exception &e) {
        main_logger.test_failed(std::string("Runtime Error: ") + e.what());
        return 2;
    } catch (...) {
        main_logger.test_failed("Unknown simulation error occurred");
        return 1;
    }
}
