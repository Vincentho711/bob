#include <memory>
#include <coroutine>
#include <iostream>
#include <format>
#include <functional>

#include "simulation_kernal.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_when_all_ready.h"
#include "simulation_when_all.h"
#include "simulation_exceptions.h"
#include "simulation_logging_utils.h"

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
    SimulationEnvironment(uint64_t seed, uint64_t max_time)
        : seed_(seed),
          max_time_(max_time),
          logger_("SimEnv") {
        // Initialise Verilator
        Verilated::traceEverOn(true);
        Verilated::randSeed(seed);

        // Create DUT as shared pointer
        dut_ = std::make_shared<Vhello_world_top>();

        // Set up waveform tracing
        trace_= std::make_shared<VerilatedVcdC>();
        dut_->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_coroutine_sim_test.vcd");

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
        sim_kernal_ = std::make_unique<simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC>>(dut_, trace_);

        // Register clocking componenets with simulation kernal
        sim_kernal_->register_clock(wr_clk_);
        sim_kernal_->register_clock(rd_clk_);

        // Set up verification components
        checker_ = std::make_shared<BaseChecker>(wr_clk_, rd_clk_);

        // Set up task components
        coro_tasks.push_back(primary_checker_wr_task());
        coro_tasks.push_back(primary_checker_rd_task());
        // when_all() and when_all_ready() testcases
        coro_tasks.push_back(checker_->when_all_ready_non_void_tuple_top());
        coro_tasks.push_back(checker_->when_all_ready_non_void_vector_top());
        coro_tasks.push_back(checker_->when_all_ready_void_tuple_top());
        coro_tasks.push_back(checker_->when_all_ready_void_vector_top());
        coro_tasks.push_back(checker_->when_all_non_void_tuple_top());
        coro_tasks.push_back(checker_->when_all_non_void_vector_top());
        coro_tasks.push_back(checker_->when_all_void_tuple_top());
        coro_tasks.push_back(checker_->when_all_void_vector_top());

    }

    ~SimulationEnvironment() {
        if (trace_) {
            trace_->close();
        }
    }

    void start_sim_kernal() {
        auto run_ctx = logger_.scoped_context("SimulationRun");
        logger_.info("Starting simulation kernel...");
        // Pass a pointer of coro_tasks to the simulation kernal for error handling
        sim_kernal_->root_tasks = &coro_tasks;
        try {
            {
                auto startup_ctx = logger_.scoped_context("TaskStartup");
                for (simulation::Task<> &task: coro_tasks) {
                    task.start();
                }
            }
            {
                auto exec_ctx = logger_.scoped_context("Execution");
                sim_kernal_->run(max_time_);
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
    std::unique_ptr<simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC>> sim_kernal_;

    // Clocking components
    std::shared_ptr<simulation::Clock<Vhello_world_top>> wr_clk_;
    std::shared_ptr<simulation::Clock<Vhello_world_top>> rd_clk_;

    // Verification components
    std::shared_ptr<BaseChecker> checker_;

    // Task componenets
    std::vector<simulation::Task<>> coro_tasks;

    // Set up root tasks that might trigger other tasks,testing wrapping of task within a task
    simulation::Task<> primary_checker_wr_task() {
        co_await checker_->print_at_wr_clk_edges();
    }

    simulation::Task<> primary_checker_rd_task() {
        co_await checker_->print_at_rd_clk_edges();
    }
};

int main() {
    auto& global_log_config = simulation::LoggerConfig::instance();
    global_log_config.set_stdout_min_level(simulation::LogLevel::INFO);
    simulation::Logger main_logger("Main");
    try {
        SimulationEnvironment sim_env(123U, 10000000U);
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
