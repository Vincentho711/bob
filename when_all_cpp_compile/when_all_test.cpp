#include "simulation_when_all_ready.h"
#include "simulation_when_all.h"
#include "simulation_task_symmetric_transfer.h"
#include "detail/simulation_when_all_task.h"
#include <cstdint>
#include <coroutine>
#include <exception>
#include <vector>

simulation::Task<> vo_0() {
    std::cout << "void_0" << std::endl;
    co_return;
}

simulation::Task<> vo_1() {
    std::cout << "void_1" << std::endl;
    co_return;
}
simulation::Task<uint32_t> return_val_0(uint32_t value) {
   co_return value + 1U;
}

simulation::Task<uint32_t> return_val_1(uint32_t value) {
   co_return value + 2U;
}

simulation::Task<> when_all_ready_non_void_tuple_top() {
    auto [val_0, val_1] = co_await simulation::when_all_ready(
        return_val_0(10U),
        return_val_1(20U));
    std::cout << val_0.result() << std::endl;
    std::cout << val_1.result() << std::endl;
}

simulation::Task<> when_all_ready_non_void_vector_top() {
    std::vector<simulation::Task<uint32_t>> tasks;
    tasks.emplace_back(return_val_0(30U));
    tasks.emplace_back(return_val_1(40U));
    std::vector<simulation::detail::WhenAllTask<uint32_t>> resultTasks =
      co_await simulation::when_all_ready(std::move(tasks));
    // auto resultTasks = co_await simulation::when_all_ready(std::move(tasks));
    for (size_t i = 0; i < resultTasks.size(); ++i) {
        try {
            uint32_t& value = resultTasks[i].result();
            std::cout << i << " = " << value << std::endl;
        } catch (const std::exception& ex) {
            std::cout << i << " : " << ex.what() << std::endl;
        }
    }
}

simulation::Task<> when_all_ready_void_tuple_top() {
    auto [val_0, val_1] = co_await simulation::when_all_ready(
        vo_0(),
        vo_1());
    val_0.result();
    val_1.result();
}

simulation::Task<> when_all_ready_void_vector_top() {
    std::vector<simulation::Task<>> tasks;
    tasks.emplace_back(vo_0());
    tasks.emplace_back(vo_1());
    std::vector<simulation::detail::WhenAllTask<void>> resultTasks =
      co_await simulation::when_all_ready(std::move(tasks));
    // auto resultTasks = co_await simulation::when_all_ready(std::move(tasks));
    for (size_t i = 0; i < resultTasks.size(); ++i) {
        try {
            resultTasks[i].result();
        } catch (const std::exception& ex) {
            std::cout << i << " : " << ex.what() << std::endl;
        }
    }
}

simulation::Task<> when_all_non_void_tuple_top() {
    auto [val_0, val_1] = co_await simulation::when_all(return_val_0(50U), return_val_1(60U));
    std::cout << "val_0 = " << val_0 << std::endl;
    std::cout << "val_1 = " << val_1 << std::endl;
}

simulation::Task<> when_all_non_void_vector_top() {
    std::vector<simulation::Task<uint32_t>> tasks;
    tasks.emplace_back(return_val_0(30U));
    tasks.emplace_back(return_val_1(40U));
    std::vector<uint32_t> results =
      co_await simulation::when_all(std::move(tasks));

    for (size_t i = 0 ; i < results.size(); ++i) {
        try {
            uint32_t& result = results[i];
            std::cout << "i = " << i << ", result = " << result << std::endl;
        } catch (const std::exception& ex) {
            std::cout << i << " : " << ex.what() << std::endl;
        }
    }
}

simulation::Task<> when_all_void_tuple_top() {
    co_await simulation::when_all(vo_0(), vo_1());
}

simulation::Task<> when_all_void_vector_top() {
    std::vector<simulation::Task<>> tasks;
    tasks.emplace_back(vo_0());
    tasks.emplace_back(vo_1());
    co_await simulation::when_all(std::move(tasks));
}


int main() {
    // Test when_all_ready()
    simulation::Task<> when_all_ready_non_void_tuple_task = when_all_ready_non_void_tuple_top();
    when_all_ready_non_void_tuple_task.start();
    simulation::Task<> when_all_ready_non_void_vector_task = when_all_ready_non_void_vector_top();
    when_all_ready_non_void_vector_task.start();
    simulation::Task<> when_all_ready_void_tuple_task = when_all_ready_void_tuple_top();
    when_all_ready_void_tuple_task.start();
    simulation::Task<> when_all_ready_void_vector_task = when_all_ready_void_vector_top();
    when_all_ready_void_vector_task.start();
    // Test when_all()
    simulation::Task<> when_all_non_void_tuple_task = when_all_non_void_tuple_top();
    when_all_non_void_tuple_task.start();
    simulation::Task<> when_all_non_void_vector_task = when_all_non_void_vector_top();
    when_all_non_void_vector_task.start();
    simulation::Task<> when_all_void_tuple_task = when_all_void_tuple_top();
    when_all_void_tuple_task.start();
    simulation::Task<> when_all_void_vector_task = when_all_void_vector_top();
    when_all_void_vector_task.start();
    return 0;
}
