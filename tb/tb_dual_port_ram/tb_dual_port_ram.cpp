#include <memory>
#include <coroutine>
#include <iostream>
#include <functional>

#include "simulation_context.h"
#include "simulation_kernel.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_when_all.h"
#include "simulation_when_all_ready.h"
#include "simulation_exceptions.h"
#include "simulation_logging_utils.h"
#include "simulation_delay.h"

#include "simulation_args_type_converter.h"
#include "simulation_args_argument_descriptor.h"
#include "simulation_args_tokeniser.h"
#include "simulation_args_group_app.h"
#include "simulation_args_argument_group.h"
#include "simulation_args_core_argument_group.h"
#include "simulation_args_progress_argument_group.h"
#include "simulation_args_argument_parser.h"
#include "simulation_args_argument_context.h"
#include "simulation_progress_reporter.h"

#include "dual_port_ram_driver.h"
#include "dual_port_ram_sequencer.h"
#include "dual_port_ram_tlm_queue.h"
#include "dual_port_ram_monitor.h"
#include "dual_port_ram_scoreboard.h"
#include "dual_port_ram_tb_argument_group.h"
#include "dual_port_ram_test_registry.h"

#include <stdexcept>
#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vdual_port_ram.h>
#include "Vdual_port_ram_dual_port_ram.h"

class BaseChecker : public simulation::SimulationComponent<Vdual_port_ram>{
    public:
        using clock_t = simulation::Clock<Vdual_port_ram>;

        BaseChecker(const std::string &name, std::shared_ptr<clock_t> wr_clk)
            : simulation::SimulationComponent<Vdual_port_ram>(name), wr_clk(wr_clk) {}

        simulation::Task<> test_same_phase_event() {
            auto test_ctx = this->logger_.scoped_context("PhaseEventTest");

            for (uint32_t i = 0 ; i < 3U ; ++i) {
                auto iteration_ctx = this->logger_.scoped_context("Iteration" + std::to_string(i));
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                this->logger_.debug("Test waiting for same phase event in a single task. 1.");
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                this->logger_.debug("Test waiting for same phase event in a single task. 2.");
            }
        }

        simulation::Task<> print_at_wr_clk_edges() {
            auto edge_ctx = logger_.scoped_context("ClockEdgeMonitor");
            while (true) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                this->logger_.debug("Resuming after wr_clk rising_edge is seen.");
                co_await wr_clk->falling_edge(simulation::Phase::Drive);
                this->logger_.debug("Resuming after wr_clk falling_edge is seen.");
            }
        }

        simulation::Task<uint32_t> return_value_1(uint32_t value) {
            // throw std::runtime_error("Testing the throwing of runtime error in void Task.");
            co_return value;
        }

        simulation::Task<uint32_t> return_value_2(uint32_t value) {
          co_return value + 1U;
        }

        simulation::Task<> when_all_ready_return_value_top_task() {
            auto [task1, task2] = co_await simulation::when_all_ready(
                return_value_1(20U),
                return_value_2(80U));

            this->logger_.info("task1 result=" + std::to_string(task1.result()));
            this->logger_.info("task2 result=" + std::to_string(task2.result()));
        }

        simulation::Task<> when_all_return_value_top_task() {
            auto [task1, task2] = co_await simulation::when_all(
                return_value_1(30U),
                return_value_2(40U)
            );

            this->logger_.info("task1 result=" + std::to_string(task1));
            this->logger_.info("task2 result=" + std::to_string(task2));
        }

        simulation::Task<> empty_task_1() {
            for (uint32_t i = 0 ; i < 5; ++i) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                this->logger_.info("empty_task_1's i = " + std::to_string(i));
            };
            schedule_one_off_async_event();
        }

        simulation::Task<> empty_task_2() {
            for (uint32_t i = 0 ; i < 6; ++i) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                this->logger_.info("empty_task_2's i = " + std::to_string(i));
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
            this->logger_.info("empty_top_task done.");
        }

        void schedule_one_off_async_event() {
            auto one_off_ctx = this->logger_.scoped_context("One Off Async Event");
            scheduler().schedule_async_event(246780, [this]() {
                this->logger_.info(std::format("schedule_one_off_async_event() fired at {}ps", this->current_time()));
            });
        }

        simulation::Task<> async_test_task() {
            auto async_test_task_ctx = this->logger_.scoped_context("async test task");
            co_await simulation::delay_us<Vdual_port_ram>(1);
            this->logger_.info("Waited 1 us.");
            co_await simulation::delay_us<Vdual_port_ram>(2);
            this->logger_.info("Waited 2 us.");
        }

    private:
        std::shared_ptr<simulation::Clock<Vdual_port_ram>> wr_clk;

};

class SimulationEnvironment {
public:
    SimulationEnvironment(uint64_t seed, bool waves, uint64_t max_time,
                          const DualPortRamTestRegistry& test_registry)
        : seed_(seed),
          waves_(waves),
          max_time_(max_time),
          logger_("SimEnv"),
          sim_context_(nullptr) {

        auto init_ctx = logger_.scoped_context("Initialisation");

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

        // ========================================================================
        // Configure automatic progress tracking. Must happen before any sequence
        // construction so that seq_start events see a ready reporter.
        // ========================================================================
        {
            const auto& prog_args = simulation::args::SimulationArgumentContext::get<simulation::args::ProgressArgumentGroup>();
            const auto& tb_args   = simulation::args::SimulationArgumentContext::get<DualPortRamArgumentGroup>();
            simulation::ProgressReporter::instance().configure(
                std::filesystem::path(prog_args.dir()),
                std::string(tb_args.test_name()),
                seed,
                max_time,
                prog_args.heartbeat_ms(),
                prog_args.enabled(),
                std::string(prog_args.batch_id()));
        }

        logger_.info("===========================================");
        logger_.info("Starting Dual Port RAM Simulation");
        logger_.info("===========================================");
        logger_.info("Seed: " + std::to_string(seed));
        logger_.info("Max Time: " + std::to_string(max_time) + "ps");

         // ========================================================================
        // Initialise Verilator
        // ========================================================================
        auto verilator_ctx = logger_.scoped_context("Verilator");
        // Create DUT as shared pointer
        dut_ = std::make_shared<Vdual_port_ram>();
        Verilated::randSeed(seed);

        if (waves) {
            const auto& tb_args = simulation::args::SimulationArgumentContext::get<DualPortRamArgumentGroup>();
            Verilated::traceEverOn(true);
            // Set up waveform tracing
            trace_= std::make_shared<VerilatedVcdC>();
            dut_->trace(trace_.get(), TRACE_DEPTH);
            trace_->open(std::string(tb_args.trace_file()).c_str());
            logger_.info("Waveform tracing enabled: " + std::string(tb_args.trace_file()));
        }


        // ========================================================================
        // Initialise clocking components
        // ========================================================================
        {
            auto clock_ctx = logger_.scoped_context("ClockSetup");
            std::function<void(bool)> wr_clk_drive_fn = [this](bool level) {
                dut_->wr_clk_i = level;  // Set the clk_i input of the DUT based on the clock level
            };
            wr_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("wr_clk", 5000U, dut_, wr_clk_drive_fn);
            // rd clk doesn't have to drive DUT clock signal, used for verification only.
            rd_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("rd_clk", 5000U, dut_, std::function<void(bool)>{}, 1000U);
            logger_.debug("Clock components initialised: ");
            logger_.debug("   wr_clk: period=5000ps");
            logger_.debug("   rd_clk: period=5000ps");
        }

        // ========================================================================
        // Set up simulation components
        // ========================================================================
        {
            auto kernel_ctx = logger_.scoped_context("KernelSetup");
            sim_kernel_ = std::make_unique<simulation::SimulationKernel<Vdual_port_ram, VerilatedVcdC>>(dut_, trace_);

            // Create and set context
            sim_context_ = std::make_unique<simulation::SimulationContext<Vdual_port_ram>>(sim_kernel_->get_scheduler(), dut_);
            simulation::SimulationContext<Vdual_port_ram>::set_current(sim_context_.get());

            // Register clocking componenets with simulation kernel
            sim_kernel_->register_clock(wr_clk_);
            sim_kernel_->register_clock(rd_clk_);
            logger_.debug("Clocks registered with simulation kernel");

            // Initialise - clocks self-schedule their first events
            sim_kernel_->initialise();
            logger_.debug("Clocks initialised and self-scheduled");
        }

        // ========================================================================
        // Read DUT parameters from generated model
        // ========================================================================
        const uint32_t addr_width_arg = static_cast<uint32_t>(Vdual_port_ram_dual_port_ram::ADDR_WIDTH);
        const uint32_t data_width_arg = static_cast<uint32_t>(Vdual_port_ram_dual_port_ram::DATA_WIDTH);

        // ========================================================================
        // Set up verification components
        // ========================================================================
        {
            auto vip_ctx = logger_.scoped_context("VIPSetup");
            tlm_wr_queue_ = std::make_shared<DualPortRamTLMWrQueue>();
            tlm_rd_queue_ = std::make_shared<DualPortRamTLMRdQueue>();
            sequencer_ = std::make_shared<DualPortRamSequencer>(wr_clk_, rd_clk_);
            // top_sequence_ = std::make_unique<DualPortRamTopSequence>();
            driver_ = std::make_shared<DualPortRamDriver>(sequencer_, wr_clk_, rd_clk_);
            monitor_ = std::make_shared<DualPortRamMonitor>(wr_clk_, rd_clk_, tlm_wr_queue_, tlm_rd_queue_);
            checker_ = std::make_shared<BaseChecker>("BaseChecker", wr_clk_);
            scoreboard_ = std::make_shared<DualPortRamScoreboard>(tlm_wr_queue_, tlm_rd_queue_, wr_clk_, 1U);
            // explicit DualPortRamScoreboard(std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue, std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue, std::shared_ptr<ClockT> wr_clk, uint32_t wr_delay_cycle, const std::string &name, bool debug_enabled);
            logger_.info("Verification components created");
        }

        // ========================================================================
        // Set up top sequence to execute, selected by command line argument "test"
        // ========================================================================
        {
            auto seq_ctx = logger_.scoped_context("SequenceSetup");
            const auto& tb_args = simulation::args::SimulationArgumentContext::get<DualPortRamArgumentGroup>();
            const std::string test_name = tb_args.test_name();
            logger_.info("Selected test: " + test_name);
            top_seq_ = test_registry.create(test_name);

            logger_.info("Top sequence configured (ADDR_WIDTH=" + std::to_string(addr_width_arg) +
                        ", DATA_WIDTH=" + std::to_string(data_width_arg) + ")");
        }

        // ========================================================================
        // Set up task components
        // ========================================================================
        {
            auto task_ctx = logger_.scoped_context("TaskSetup");
            logger_.debug("Creating coroutine tasks...");
            // Active tasks: finite, gate simulation termination
            active_tasks_.emplace_back(checker_->test_same_phase_event());
            // active_tasks_.emplace_back(checker_->when_all_ready_return_value_top_task());
            // active_tasks_.emplace_back(checker_->when_all_return_value_top_task());
            active_tasks_.emplace_back(checker_->empty_top_task());
            active_tasks_.emplace_back(checker_->async_test_task());
            // active_tasks_.emplace_back(checker_->print_at_wr_clk_edges());
            active_tasks_.emplace_back(sequencer_->start_sequence(std::move(top_seq_)));

            // Reactive tasks: infinite while(true) loops, destroyed when active tasks complete
            reactive_tasks_.emplace_back(driver_->wr_driver_run());
            reactive_tasks_.emplace_back(driver_->rd_driver_run());
            reactive_tasks_.emplace_back(monitor_->wr_port_run());
            reactive_tasks_.emplace_back(monitor_->rd_port_run());
            reactive_tasks_.emplace_back(scoreboard_->run_write_capture());
            reactive_tasks_.emplace_back(scoreboard_->run_read_capture());
            reactive_tasks_.emplace_back(scoreboard_->update_ram_model());

            logger_.info("Created " + std::to_string(active_tasks_.size()) + " active tasks, " +
                         std::to_string(reactive_tasks_.size()) + " reactive tasks");
        }

        logger_.info("===========================================");

    }

    ~SimulationEnvironment() {
        if (trace_) {
            // Capture the last signals
            dut_->eval();
            trace_->dump(sim_kernel_->time);
            trace_->close();
            logger_.info("Waveform trace closed");
        }
    }

  simulation::RunResult start_sim_kernel() {
        auto run_ctx = logger_.scoped_context("SimulationRun");
        logger_.info("Starting simulation kernel...");
        // Wire active and reactive task vectors to the kernel
        sim_kernel_->active_tasks   = &active_tasks_;
        sim_kernel_->reactive_tasks = &reactive_tasks_;
        // Resume all root level coroutines as they are not started upon creation
        simulation::RunResult result = simulation::RunResult::Completed;
        try {
            {
                auto startup_ctx = logger_.scoped_context("TaskStartup");
                for (simulation::Task<>& task : active_tasks_)   task.start();
                for (simulation::Task<>& task : reactive_tasks_) task.start();
            }
            {
                auto exec_ctx = logger_.scoped_context("Execution");
                result = sim_kernel_->run(max_time_);
            }
        } catch (...) {
            throw;
        }
        return result;
    }

private:
    // Simulation parameters
    const int32_t TRACE_DEPTH = 5;
    const uint64_t seed_;
    const bool waves_;
    const uint64_t max_time_;

    // Logger for this class
    simulation::Logger logger_;

    // Verilator components
    std::shared_ptr<Vdual_port_ram> dut_;
    std::shared_ptr<VerilatedVcdC> trace_;

    // Simulation components
    std::unique_ptr<simulation::SimulationKernel<Vdual_port_ram, VerilatedVcdC>> sim_kernel_;
    std::unique_ptr<simulation::SimulationContext<Vdual_port_ram>> sim_context_;

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
    std::unique_ptr<DualPortRamBaseSequence> top_seq_;

    // Task components
    std::vector<simulation::Task<>> active_tasks_;    // finite tasks; gate termination
    std::vector<simulation::Task<>> reactive_tasks_;  // infinite tasks; destroyed on drain
};

int main(int argc, char** argv) {
    // ============================================================================
    // Configure SimulationArgumentParser first
    // ============================================================================
    // Populate the test registry before argument parsing so test_names() can
    // feed --tb.test valid_values (for --help and parse-time validation).
    DualPortRamTestRegistry test_registry;
    register_dual_port_ram_tests(test_registry);

    simulation::args::SimulationArgumentParser arg_parser("tb_dual_port_ram");
    try {
        arg_parser.add_group(std::make_unique<simulation::args::CoreArgumentGroup>(
            simulation::args::CoreArgumentDefaults{
                .max_time_ps = 100'000'000   // Override default max_time for this testbench
            }
        ));
        arg_parser.add_group(std::make_unique<simulation::args::ProgressArgumentGroup>());
        arg_parser.add_group(std::make_unique<DualPortRamArgumentGroup>(test_registry.test_names()));
        arg_parser.parse(argc, argv);
        arg_parser.resolve();

        simulation::args::SimulationArgumentContext::set(arg_parser);


    } catch (const std::invalid_argument& e) {
        std::cerr << std::format(
            "Argument error while parsing CLI arguments:\n  {}\n",
            e.what()
        );
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        std::cerr << std::format(
            "Runtime error during initialisation:\n  {}\n",
            e.what()
        );
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << std::format(
            "Unhandled exception:\n  {}\n",
            e.what()
        );
        return EXIT_FAILURE;
    }
    const auto& core_args = arg_parser.get<simulation::args::CoreArgumentGroup>();
    const auto& tb_args = arg_parser.get<DualPortRamArgumentGroup>();

    const bool waves = core_args.waves() || tb_args.waves_implied();
    const uint64_t max_time_ps = core_args.max_time_ps();

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
        SimulationEnvironment sim_env(core_args.seed(), waves, max_time_ps, test_registry);
        simulation::RunResult run_result = sim_env.start_sim_kernel();

        if (run_result == simulation::RunResult::MaxTimeReached) {
            main_logger.test_max_time_reached(core_args.max_time_ps());
        }

        main_logger.test_passed("Simulation Passed");
        return 0;
    } catch (const simulation::VerificationError &e) {
        simulation::ProgressReporter::instance().run_end("error", e.what());
        main_logger.test_failed(std::format("Verification error: {}", e.what()));
        main_logger.error(std::format("Component: {}", e.get_component_name()));
        main_logger.error(std::format("Timestamp: {}ps", e.get_timestamp()));
        const std::source_location location = e.get_location();
        main_logger.error(std::format("{}:{}: in function {}", location.file_name(), location.line(), location.function_name()));
        return 1;
    } catch (const std::runtime_error &e) {
        simulation::ProgressReporter::instance().run_end("error", e.what());
        main_logger.test_failed(std::string("Runtime Error: ") + e.what());
        return 1;
    } catch (const std::exception &e) {
        simulation::ProgressReporter::instance().run_end("error", e.what());
        main_logger.test_failed(std::string("Runtime Error: ") + e.what());
        return 2;
    } catch (...) {
        simulation::ProgressReporter::instance().run_end("error", "unknown exception");
        main_logger.test_failed("Unknown simulation error occurred");
        return 1;
    }
}
