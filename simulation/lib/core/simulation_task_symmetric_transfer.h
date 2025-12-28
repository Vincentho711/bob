#ifndef SIMULATION_TASK_SYMMETRIC_TRANSFER_H
#define SIMULATION_TASK_SYMMETRIC_TRANSFER_H

#include <utility>
#include <coroutine>
#include <exception>
#include "simulation_exceptions.h"

namespace simulation {
    class Task {
    public:
        struct promise_type {
            // Store the handle of the coroutine waiting for us (the parent)
            std::coroutine_handle<> continuation_ = nullptr;
            // Store the exception if any occured within a coroutine
            std::exception_ptr exception_ = nullptr;

            Task get_return_object() {
                return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            // Refrain from starting until co_awaited.
            std::suspend_always initial_suspend() noexcept { return {}; }

            // When this Task finish, check if another coroutine Task is waiting for us and jump to it.
            struct FinalAwaiter {
                bool await_ready() const noexcept { return false; }

                // This returns the handle to resume (the parent)
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    auto& promise = h.promise();
                    if (promise.continuation_) {
                        return promise.continuation_;
                    }
                    return std::noop_coroutine();
                }
                void await_resume() const noexcept {}
            };

            FinalAwaiter final_suspend() noexcept { return {}; }

            void return_void() noexcept {}
            void unhandled_exception() {
                exception_ = std::current_exception();
            } // Simplified handling
        };

        // --- Standard RAII boilerplate ---
        Task() noexcept = default;
        ~Task() {
            if (handle_) handle_.destroy();
        }
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        Task(Task&& other) : handle_(std::exchange(other.handle_, nullptr)) {}
        Task& operator=(Task&& other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            return *this;
        }

        void start() {
            if (handle_ && !handle_.done()) {
                handle_.resume();
                // Check if coroutine exited due to an exception
                check_exception();
            }
        }

        // Rethrow the stored exception if one exists
        void check_exception() const {
            if (handle_ && handle_.promise().exception_) {
                std::rethrow_exception(handle_.promise().exception_);
            }
        }

        // Making task awaitable, co_await Task();
        auto operator co_await() const & noexcept {
            struct Awaiter {
                std::coroutine_handle<promise_type> handle_;

                bool await_ready() const noexcept { 
                    return !handle_ || handle_.done(); 
                }

                // Store the continuation in the task's promise so that the final_suspend() knows
                // which coroutine to resume when the Task finishes
                std::coroutine_handle<> await_suspend(std::coroutine_handle<> calling_coroutine) noexcept {
                    handle_.promise().continuation_ = calling_coroutine;
                    return handle_;
                }

                void await_resume() {
                    // When the sub-task finishes and returns to the parent,
                    // check if that sub-task threw an exception
                    if (handle_.promise().exception_) {
                        std::rethrow_exception(handle_.promise().exception_);
                    }
                }
            };
            return Awaiter{handle_};
        }

    private:
        explicit Task(std::coroutine_handle<promise_type> handle)
            : handle_(handle) {}

        std::coroutine_handle<promise_type> handle_;
    };
}
#endif // SIMULATION_TASK_SYMMETRIC_TRANSFER_H
