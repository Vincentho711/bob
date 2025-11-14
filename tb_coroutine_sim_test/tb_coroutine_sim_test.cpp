#include <memory>
#include <coroutine>
#include <iostream>

#include "simulation_kernal.h"
#include "simulation_clock.h"
#include "simulation_phase_event.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <Vhello_world_top.h>

struct ExampleCoroutine {
    std::shared_ptr<simulation::Clock<Vhello_world_top, VerilatedVcdC>> wr_clk;

    ExampleCoroutine(std::shared_ptr<simulation::Clock<Vhello_world_top, VerilatedVcdC>> clk)
        : wr_clk(clk) {}

    struct task {
        struct promise_type {
            task get_return_object() {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };
        // Constructor
        // Default constructor
        task() noexcept = default;
        // Constructor with handle
        task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
        // No destructor to prevent it from being destroy when the task object is out of scope

        // Attribute
        std::coroutine_handle<promise_type> handle_;
    };

    task run() {
        while (true) {
            co_await wr_clk->rising_edge(simulation::Phase::Drive);
            std::cout << "Resuming after rising_edge is seen." << std::endl;
            co_await wr_clk->falling_edge(simulation::Phase::Drive);
            std::cout << "Resuming after falling_edge is seen." << std::endl;
        }
    }

};

// struct Driver {
//     simulation::Clock &clk;
//
//     Driver(simulation::Clock &c) : clk(c) {}
//
//
// }

struct SimulationEnvironment {
    simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC> &sim_kernal;
    std::shared_ptr<Vhello_world_top> dut = std::make_shared<Vhello_world_top>();
    std::shared_ptr<VerilatedVcdC> trace_ =  std::make_shared<VerilatedVcdC>();
    std::shared_ptr<simulation::Clock<Vhello_world_top, VerilatedVcdC>> wr_clk = std::make_shared<simulation::Clock<Vhello_world_top, VerilatedVcdC>>("wr_clk", 5000U, dut, trace_);
    std::shared_ptr<simulation::Clock<Vhello_world_top, VerilatedVcdC>> rd_clk = std::make_shared<simulation::Clock<Vhello_world_top, VerilatedVcdC>>("rd_clk", 4000U, dut, trace_);
    const int TRACE_DEPTH = 5;
    ExampleCoroutine example_coro = ExampleCoroutine(wr_clk);
    ExampleCoroutine::task running_task;

    SimulationEnvironment(simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC> &sk)
        : sim_kernal(sk) {
        sim_kernal.register_clock(wr_clk);
        sim_kernal.register_clock(rd_clk);
        // Initialise Verilator
        Verilated::traceEverOn(true);
        dut->trace(trace_.get(), TRACE_DEPTH);
        trace_->open("tb_coroutine_sim_test");
    }
    ~SimulationEnvironment() {
        if (running_task.handle_) {
            running_task.handle_.destroy();
        }
        if (trace_) {
            trace_->close();
        }
    }

    void start() {
        // Get task object
        running_task = example_coro.run();
        // Feed in the coroutine handle to start
        start_coroutine(running_task.handle_);
    }

    void start_coroutine(std::coroutine_handle<> handle) {
        handle.resume();
    }

};

int main() {
    simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC> sim_kernal = simulation::SimulationKernal<Vhello_world_top, VerilatedVcdC>();
    SimulationEnvironment sim_env = SimulationEnvironment(sim_kernal);
    // Schedule coroutine to start
    sim_env.start();
    // Start the kernal
    sim_kernal.run(100000U);
    return 0;
}
