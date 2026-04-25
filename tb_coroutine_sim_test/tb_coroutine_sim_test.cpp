#include <memory>
#include <coroutine>
#include <iostream>
#include <format>
#include <functional>

#include "simulation_kernel.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_when_all_ready.h"
#include "simulation_when_all.h"
#include "simulation_exceptions.h"
#include "simulation_logging_utils.h"

#include "simulation_args_type_converter.h"
#include "simulation_args_argument_descriptor.h"
#include "simulation_args_tokeniser.h"
#include "simulation_args_group_app.h"
#include "simulation_args_argument_group.h"
#include "simulation_args_core_argument_group.h"
#include "simulation_args_progress_argument_group.h"
#include "simulation_args_argument_parser.h"
#include "simulation_args_argument_context.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

class BaseChecker {
    public:
        using clock_t = simulation::Clock<Vhello_world_top>;

        BaseChecker(std::shared_ptr<clock_t> wr_clk, std::shared_ptr<clock_t> rd_clk)
            : wr_clk(wr_clk), rd_clk(rd_clk), logger_("BaseChecker") {}

        simulation::Task<> print_at_wr_clk_edges() {
            while (true) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("Resuming after wr_clk rising_edge 1 is seen.");
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("Resuming after wr_clk rising_edge 2 is seen.");
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("Resuming after wr_clk rising_edge 3 is seen.");
                // Test throwing VerificationError
                // simulation::report_fatal("Test fatal error!");
            }
        }

        simulation::Task<> print_at_rd_clk_edges() {
            while (true) {
                co_await rd_clk->rising_edge(simulation::Phase::Drive);
                logger_.info("Resuming after rd_clk rising_edge is seen.");
                co_await rd_clk->falling_edge(simulation::Phase::Drive);
                logger_.info("Resuming after rd_clk falling_edge is seen.");
            }
        }

        // Dummy tasks to test when_all() and when_all_ready()
        simulation::Task<> when_all_void_task_0() {
            logger_.info("when_all_void_task_0");
            co_return;
        }

        simulation::Task<> when_all_void_task_1() {
            logger_.info("when_all_void_task_1");
            co_return;
        }

        simulation::Task<uint32_t> when_all_return_value_task_0(uint32_t value) {
            co_return value + 1U;
        }

        simulation::Task<uint32_t> when_all_return_value_task_1(uint32_t value) {
            co_return value + 2U;
        }

        // when_all_ready() tests, testing non_void type for both vector and tuple format
        simulation::Task<> when_all_ready_non_void_tuple_top() {
            logger_.info("=== when_all_ready_non_void_tuple_top() ===");
            auto [val_0, val_1] = co_await simulation::when_all_ready(
                when_all_return_value_task_0(10U),
                when_all_return_value_task_1(20U));
            logger_.info(std::format("Expected = 11, Actual = {}", val_0.result()));
            logger_.info(std::format("Expected = 22, Actual = {}", val_1.result()));
        }

        simulation::Task<> when_all_ready_non_void_vector_top() {
            logger_.info("=== when_all_ready_non_void_vector_top() ===");
            std::vector<simulation::Task<uint32_t>> tasks;
            tasks.emplace_back(when_all_return_value_task_0(30U));
            tasks.emplace_back(when_all_return_value_task_1(40U));
            std::vector<simulation::detail::WhenAllTask<uint32_t>> resultTasks =
              co_await simulation::when_all_ready(std::move(tasks));
            for (size_t i = 0; i < resultTasks.size(); ++i) {
                try {
                    uint32_t& value = resultTasks[i].result();
                    logger_.info(std::format("i={}, value={}", i , value));
                } catch (const std::exception& ex) {
                    logger_.error(std::format("i={}, {}", i , ex.what()));
                }
            }
        }

        // when_all_ready() tests, testing void type for both vector and tuple format
        simulation::Task<> when_all_ready_void_tuple_top() {
            logger_.info("=== when_all_ready_void_tuple_top() ===");
            auto [val_0, val_1] = co_await simulation::when_all_ready(
                when_all_void_task_0(),
                when_all_void_task_1());
            val_0.result();
            val_1.result();
        }

        simulation::Task<> when_all_ready_void_vector_top() {
            logger_.info("=== when_all_ready_void_vector_top() ===");
            std::vector<simulation::Task<>> tasks;
            tasks.emplace_back(when_all_void_task_0());
            tasks.emplace_back(when_all_void_task_1());
            std::vector<simulation::detail::WhenAllTask<void>> resultTasks =
              co_await simulation::when_all_ready(std::move(tasks));
            for (size_t i = 0; i < resultTasks.size(); ++i) {
                try {
                    resultTasks[i].result();
                } catch (const std::exception& ex) {
                    logger_.error(std::format("i={}, {}", i , ex.what()));
                }
            }
        }

        // when_all() tests, testing non-void type for both vector and tuple format
        simulation::Task<> when_all_non_void_tuple_top() {
            logger_.info("=== when_all_non_void_tuple_top() ===");
            auto [val_0, val_1] = co_await simulation::when_all(when_all_return_value_task_0(50U), when_all_return_value_task_1(60U));
            logger_.info(std::format("val_0 = {}", val_0));
            logger_.info(std::format("val_1 = {}", val_1));
        }

        simulation::Task<> when_all_non_void_vector_top() {
            logger_.info("=== when_all_non_void_vector_top() ===");
            std::vector<simulation::Task<uint32_t>> tasks;
            tasks.emplace_back(when_all_return_value_task_0(30U));
            tasks.emplace_back(when_all_return_value_task_1(40U));
            std::vector<uint32_t> results =
              co_await simulation::when_all(std::move(tasks));

            for (size_t i = 0 ; i < results.size(); ++i) {
                try {
                    uint32_t& result = results[i];
                    logger_.info(std::format("i={}, result={}", i , result));
                } catch (const std::exception& ex) {
                    logger_.error(std::format("i={}, {}", i , ex.what()));
                }
            }
        }

        // when_all() tests, testing void type for both vector and tuple format
        simulation::Task<> when_all_void_tuple_top() {
            logger_.info("=== when_all_void_tuple_top() ===");
            co_await simulation::when_all(when_all_void_task_0(), when_all_void_task_1());
        }

        simulation::Task<> when_all_void_vector_top() {
            logger_.info("=== when_all_void_vector_top() ===");
            std::vector<simulation::Task<>> tasks;
            tasks.emplace_back(when_all_void_task_0());
            tasks.emplace_back(when_all_void_task_1());
            co_await simulation::when_all(std::move(tasks));
        }

    private:
        std::shared_ptr<simulation::Clock<Vhello_world_top>> wr_clk;
        std::shared_ptr<simulation::Clock<Vhello_world_top>> rd_clk;
        simulation::Logger logger_;

};

class SimulationEnvironment {
public:
    SimulationEnvironment(uint64_t seed, uint64_t max_time, bool waves, const std::string& output_dir)
        : seed_(seed),
          max_time_(max_time),
          logger_("SimEnv") {
        Verilated::randSeed(seed);

        // Create DUT as shared pointer
        dut_ = std::make_shared<Vhello_world_top>();

        if (waves) {
            const std::string vcd_name = "tb_coroutine_sim_test.vcd";
            const std::string vcd_path = output_dir.empty() ? vcd_name : output_dir + "/" + vcd_name;
            Verilated::traceEverOn(true);
            trace_ = std::make_shared<VerilatedVcdC>();
            dut_->trace(trace_.get(), TRACE_DEPTH);
            trace_->open(vcd_path.c_str());
        }

        // Initialise clocking components
        std::function<void(bool)> wr_clk_drive_fn = [this](bool level) {
            dut_->clk_i = level;  // Set the clk_i input of the DUT based on the clock level
        };
        wr_clk_ = std::make_shared<simulation::Clock<Vhello_world_top>>("wr_clk", 5000U, dut_, wr_clk_drive_fn);

        std::function<void(bool)> rd_clk_drive_fn = [this](bool level) {
            dut_->a_i = level;  // Set the a_i input of the DUT based on the clock level
        };

        rd_clk_ = std::make_shared<simulation::Clock<Vhello_world_top>>("rd_clk", 4000U, dut_, rd_clk_drive_fn);

        // Set up simulation components
        sim_kernel_ = std::make_unique<simulation::SimulationKernel<Vhello_world_top, VerilatedVcdC>>(dut_, trace_);

        // Register clocking componenets with simulation kernal
        sim_kernel_->register_clock(wr_clk_);
        sim_kernel_->register_clock(rd_clk_);

        // Initialise - clocks self-schedule their first events
        sim_kernel_->initialise();

        // Set up verification components
        checker_ = std::make_shared<BaseChecker>(wr_clk_, rd_clk_);

        // Active tasks: finite when_all tests; gate simulation termination
        active_tasks_.push_back(checker_->when_all_ready_non_void_tuple_top());
        active_tasks_.push_back(checker_->when_all_ready_non_void_vector_top());
        active_tasks_.push_back(checker_->when_all_ready_void_tuple_top());
        active_tasks_.push_back(checker_->when_all_ready_void_vector_top());
        active_tasks_.push_back(checker_->when_all_non_void_tuple_top());
        active_tasks_.push_back(checker_->when_all_non_void_vector_top());
        active_tasks_.push_back(checker_->when_all_void_tuple_top());
        active_tasks_.push_back(checker_->when_all_void_vector_top());

        // Reactive tasks: infinite clock-edge loops; destroyed when active tasks complete
        reactive_tasks_.push_back(primary_checker_wr_task());
        reactive_tasks_.push_back(primary_checker_rd_task());

    }

    ~SimulationEnvironment() {
        if (trace_) {
            trace_->close();
        }
    }

    void start_sim_kernel() {
        auto run_ctx = logger_.scoped_context("SimulationRun");
        logger_.info("Starting simulation kernel...");
        // Wire active and reactive task vectors to the kernel
        sim_kernel_->active_tasks   = &active_tasks_;
        sim_kernel_->reactive_tasks = &reactive_tasks_;
        try {
            {
                auto startup_ctx = logger_.scoped_context("TaskStartup");
                for (simulation::Task<>& task : active_tasks_)   task.start();
                for (simulation::Task<>& task : reactive_tasks_) task.start();
            }
            {
                auto exec_ctx = logger_.scoped_context("Execution");
                sim_kernel_->run(max_time_);
            }
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
    std::shared_ptr<Vhello_world_top> dut_;
    std::shared_ptr<VerilatedVcdC> trace_;

    // Simulation components
    std::unique_ptr<simulation::SimulationKernel<Vhello_world_top, VerilatedVcdC>> sim_kernel_;

    // Clocking components
    std::shared_ptr<simulation::Clock<Vhello_world_top>> wr_clk_;
    std::shared_ptr<simulation::Clock<Vhello_world_top>> rd_clk_;

    // Verification components
    std::shared_ptr<BaseChecker> checker_;

    // Task components
    std::vector<simulation::Task<>> active_tasks_;    // finite tasks; gate termination
    std::vector<simulation::Task<>> reactive_tasks_;  // infinite tasks; destroyed on drain

    // Set up root tasks that might trigger other tasks,testing wrapping of task within a task
    simulation::Task<> primary_checker_wr_task() {
        co_await checker_->print_at_wr_clk_edges();
    }

    simulation::Task<> primary_checker_rd_task() {
        co_await checker_->print_at_rd_clk_edges();
    }
};

int main(int argc, char** argv) {
    auto& global_log_config = simulation::LoggerConfig::instance();
    global_log_config.set_stdout_min_level(simulation::LogLevel::INFO);
    simulation::Logger main_logger("Main");

    const std::string binary_name = std::filesystem::path(argv[0]).stem().string();
    simulation::args::SimulationArgumentParser arg_parser(binary_name);
    try {
        arg_parser.add_group(std::make_unique<simulation::args::CoreArgumentGroup>(
            binary_name,
            simulation::args::CoreArgumentDefaults{
                .max_time_ps = 100'000'000'000   // Override default max_time for this testbench
            }
        ));
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
    try {
        SimulationEnvironment sim_env(
            123U, 10000000U,
            core_args.waves(),
            std::string(core_args.output_dir())
        );
        sim_env.start_sim_kernel();
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
