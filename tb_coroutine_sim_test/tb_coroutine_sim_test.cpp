#include <memory>
#include <coroutine>
#include <iostream>
#include <functional>

#include "simulation_kernal.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simulation_exceptions.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

class BaseChecker {
    public:
        using clock_t = simulation::Clock<Vhello_world_top>;

        BaseChecker(std::shared_ptr<clock_t> wr_clk, std::shared_ptr<clock_t> rd_clk)
            : wr_clk(wr_clk), rd_clk(rd_clk) {}

        simulation::Task print_at_wr_clk_edges() {
            while (true) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                std::cout << "Resuming after wr_clk rising_edge 1 is seen." << std::endl;
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                std::cout << "Resuming after wr_clk rising_edge 2 is seen." << std::endl;
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                std::cout << "Resuming after wr_clk rising_edge 3 is seen." << std::endl;
                // Test throwing VerificationError
                // simulation::report_fatal("Test fatal error!");
            }
        }

        simulation::Task print_at_rd_clk_edges() {
            while (true) {
                co_await rd_clk->rising_edge(simulation::Phase::Drive);
                std::cout << "Resuming after rd_clk rising_edge is seen." << std::endl;
                co_await rd_clk->falling_edge(simulation::Phase::Drive);
                std::cout << "Resuming after rd_clk falling_edge is seen." << std::endl;
            }
        }

    private:
        std::shared_ptr<simulation::Clock<Vhello_world_top>> wr_clk;
        std::shared_ptr<simulation::Clock<Vhello_world_top>> rd_clk;

};

class SimulationEnvironment {
public:
    SimulationEnvironment(uint32_t seed, uint64_t max_time)
        : seed_(seed),
          max_time_(max_time) {
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

    }

    ~SimulationEnvironment() {
        if (trace_) {
            trace_->close();
        }
    }

    void start_sim_kernal() {
        // Pass a pointer of coro_tasks to the simulation kernal for error handling
        sim_kernal_->root_tasks = &coro_tasks;
        try {
            for (simulation::Task &task: coro_tasks) {
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
    const uint32_t seed_;
    const uint64_t max_time_;

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
    std::vector<simulation::Task> coro_tasks;

    // Set up root tasks that might trigger other tasks,testing wrapping of task within a task
    simulation::Task primary_checker_wr_task() {
        co_await checker_->print_at_wr_clk_edges();
    }

    simulation::Task primary_checker_rd_task() {
        co_await checker_->print_at_rd_clk_edges();
    }
};

int main() {
    try {
        SimulationEnvironment sim_env(123U, 100000U);
        sim_env.start_sim_kernal();
        std::cout << "\033[1;32m" << "Simulation Passed." << "\033[0m" << std::endl;
        return 0;
    } catch (const simulation::VerificationError &e) {
        std::cerr << "\033[1;31m" <<  "Simulation Failed: " << e.what() << "\033[0m" <<std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "\033[1;31m" << "Simulation Error: " << e.what() << "\033[0m" <<std::endl;
        return 2;
    } catch (...) {
        std::cerr << "\033[1;31m" << "Unknown simultion error occurred." << "\033[0m" <<std::endl;
        return 1;
    }
}
