#ifndef SIMULATION_TASK_H
#define SIMULATION_TASK_H

#include <utility>
#include <coroutine>

namespace simulation {
    class Task {
        public:
            struct promise_type {
                Task get_return_object() {
                    return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
                }
                std::suspend_never initial_suspend() noexcept { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void return_void() noexcept {}
                void unhandled_exception() {}
            };
            Task() noexcept = default;

            Task(const Task&) = delete;
            Task& operator=(const Task&) = delete;

            Task(Task&& other)
                : handle_(std::exchange(other.handle_, nullptr)) {}

            Task& operator=(Task&& other) {
                if (handle_) {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
                return *this;
            }

            ~Task() {
                if (handle_) {
                    handle_.destroy();
                }
            }
        private:
            explicit Task(std::coroutine_handle<promise_type> handle)
                : handle_(handle) {}

            std::coroutine_handle<promise_type> handle_;
    };
}

#endif // SIMULATION_TASK_H
