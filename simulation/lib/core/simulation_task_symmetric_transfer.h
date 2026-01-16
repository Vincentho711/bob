#ifndef SIMULATION_TASK_SYMMETRIC_TRANSFER_H
#define SIMULATION_TASK_SYMMETRIC_TRANSFER_H

#include <utility>
#include <coroutine>
#include <exception>
#include <utility>
#include <variant>
#include "simulation_exceptions.h"

namespace simulation {
    // Forward declaration
    template<typename T = void>
    class [[nodiscard]] Task;

    namespace detail {
        struct TaskPromiseBase {
            std::coroutine_handle<> continuation_ = nullptr;

            // Refrain from starting until co_awaited.
            std::suspend_always initial_suspend() noexcept { return {}; }

            // When this Task finish, check if another coroutine Task is waiting for us and jump to it.
            struct FinalAwaiter {
                bool await_ready() const noexcept { return false; }

                // This returns the handle to resume (the parent)
                std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept {
                    auto& promise = reinterpret_cast<std::coroutine_handle<TaskPromiseBase>&>(h).promise();
                    if (promise.continuation_) {
                        return promise.continuation_;
                    }
                    return std::noop_coroutine();
                }
                void await_resume() const noexcept {}
            };

            FinalAwaiter final_suspend() noexcept { return {}; }
        };

        template<typename T>
        struct TaskPromise final : TaskPromiseBase {
            // result can either be a type of exception_ptr or the return type T
            std::variant<std::monostate, T, std::exception_ptr> result_;

            Task<T> get_return_object();

            // If the exception is not caught with a try-except block, call this unhandled_exception()
            void unhandled_exception() {
                result_.template emplace<std::exception_ptr>(std::current_exception());
            }

            void return_value(T&& value) {
                result_.template emplace<T>(std::forward<T>(value));
            }

            T& result() {
                if (std::holds_alternative<std::exception_ptr>(result_)) {
                    std::rethrow_exception(std::get<std::exception_ptr>(result_));
                }
                return std::get<T>(result_);
            }
        };

        template<>
        struct TaskPromise<void> final : TaskPromiseBase {
            std::exception_ptr exception_ = nullptr;

            Task<void> get_return_object();

            void unhandled_exception() {
                exception_ = std::current_exception();
            }
            void return_void() noexcept {}

            void result() {
                if (exception_) std::rethrow_exception(exception_);
            }
        };
    }

    template<typename T>
    class Task {
    public:
        using promise_type = detail::TaskPromise<T>;

        explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}
        ~Task() { if (handle_) handle_.destroy(); }
        Task(Task&& other) : handle_(std::exchange(other.handle_, nullptr)) {}
        Task& operator=(Task&& other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            return *this;
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

                decltype(auto) await_resume() {
                    return handle_.promise().result();
                }
            };
            return Awaiter{handle_};
        }

        void start() {
            if (handle_ && !handle_.done()) {
                handle_.resume();
                // Immediately check if the tick caused a crash
                check_exception();
            }
        }

        void check_exception() const {
            if (handle_ && handle_.done()) {
                handle_.promise().result();
            }
        }

    private:
        std::coroutine_handle<promise_type> handle_;
    };

    template<typename T>
    Task<T> detail::TaskPromise<T>::get_return_object() {
        return Task<T>{ std::coroutine_handle<TaskPromise<T>>::from_promise(*this) };
    }

    inline Task<void> detail::TaskPromise<void>::get_return_object() {
        return Task<void>{ std::coroutine_handle<TaskPromise<void>>::from_promise(*this) };
    }

}
#endif // SIMULATION_TASK_SYMMETRIC_TRANSFER_H
