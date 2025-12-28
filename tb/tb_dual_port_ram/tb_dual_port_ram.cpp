#include <memory>
#include <coroutine>
#include <iostream>
#include <functional>

#include "simulation_kernal.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"
#include "simulation_task_symmetric_transfer.h"
#include "simualation_exceptions.h"

#include "dual_port_ram_driver.h"
#include "dual_port_ram_top_sequence.h"
#include "dual_port_ram_sequencer.h"
#include "dual_port_ram_tlm_queue.h"
#include "dual_port_ram_monitor.h"
#include "testcases/directed/dual_port_ram_directed_testcases.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vdual_port_ram.h>
#include "Vdual_port_ram_dual_port_ram.h"

class BaseChecker {
    public:
        using clock_t = simulation::Clock<Vdual_port_ram>;

        BaseChecker(std::shared_ptr<clock_t> wr_clk)
            : wr_clk(wr_clk) {}

        simulation::Task print_at_wr_clk_edges() {
            while (true) {
                co_await wr_clk->rising_edge(simulation::Phase::Drive);
                std::cout << "Resuming after wr_clk rising_edge is seen." << std::endl;
                co_await wr_clk->falling_edge(simulation::Phase::Drive);
                std::cout << "Resuming after wr_clk falling_edge is seen." << std::endl;
            }
        }

    private:
        std::shared_ptr<simulation::Clock<Vdual_port_ram>> wr_clk;

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
        dut_ = std::make_shared<Vdual_port_ram>();

        // Set up waveform tracing
        trace_= std::make_shared<VerilatedVcdC>();
        dut_->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_dual_port_ram.vcd");

        // Initialise clocking components
        std::function<void(bool)> wr_clk_drive_fn = [this](bool level) {
            dut_->wr_clk_i = level;  // Set the clk_i input of the DUT based on the clock level
        };
        wr_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("wr_clk", 5000U, dut_, wr_clk_drive_fn);
        // rd clk doesn't have to drive DUT clock signal, used for verification only.
        rd_clk_ = std::make_shared<simulation::Clock<Vdual_port_ram>>("rd_clk", 5000U, dut_, std::function<void(bool)>{});

        // Set up simulation components
        sim_kernal_ = std::make_unique<simulation::SimulationKernal<Vdual_port_ram, VerilatedVcdC>>(dut_, trace_);

        // Register clocking componenets with simulation kernal
        sim_kernal_->register_clock(wr_clk_);
        sim_kernal_->register_clock(rd_clk_);

        // Set up verification components
        tlm_wr_queue_ = std::make_shared<DualPortRamTLMWrQueue>();
        tlm_rd_queue_ = std::make_shared<DualPortRamTLMRdQueue>();
        sequencer_ = std::make_shared<DualPortRamSequencer>(wr_clk_, rd_clk_);
        // top_sequence_ = std::make_unique<DualPortRamTopSequence>();
        driver_ = std::make_shared<DualPortRamDriver>(sequencer_, dut_, wr_clk_, rd_clk_);
        monitor_ = std::make_shared<DualPortRamMonitor>(dut_, wr_clk_, rd_clk_, tlm_wr_queue_, tlm_rd_queue_);
        checker_ = std::make_shared<BaseChecker>(wr_clk_);

        // Set up top sequence to execute
        const auto dual_port_ram_module_addr_width = Vdual_port_ram_dual_port_ram::ADDR_WIDTH;
        const auto dual_port_ram_module_data_width = Vdual_port_ram_dual_port_ram::DATA_WIDTH;
        const uint32_t addr_width_arg = static_cast<uint32_t>(dual_port_ram_module_addr_width);
        const uint32_t data_width_arg = static_cast<uint32_t>(dual_port_ram_module_data_width);
        top_seq_ = std::make_unique<DualPortRamTopSequence>(addr_width_arg, data_width_arg);

        // Set up task components
        coro_tasks.emplace_back(checker_->print_at_wr_clk_edges());
        coro_tasks.emplace_back(sequencer_->start_sequence(std::move(top_seq_)));
        coro_tasks.emplace_back(driver_->wr_driver_run());
        coro_tasks.emplace_back(driver_->rd_driver_run());

    }

    ~SimulationEnvironment() {
        if (trace_) {
            trace_->close();
        }
    }

    void start_sim_kernal() {
        // Resume all root level coroutines as they are not started upon creation
        try {
            for (simulation::Task &task : coro_tasks) {
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

    // Sequence components
    std::unique_ptr<DualPortRamTopSequence> top_seq_;

    // Task componenets
    std::vector<simulation::Task> coro_tasks;
};

int main() {
    try {
        SimulationEnvironment sim_env(123U, 500000U);
        sim_env.start_sim_kernal();
        return 0;
    } catch (const simulation::VerificationError &e) {
        std::cerr << "Simulation Failed: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Simulation Error: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "Unknown simultion error occurred" << std::endl;
        return 1;
    }
}
