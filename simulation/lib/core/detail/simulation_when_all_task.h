#ifndef SIMULATION_WHEN_ALL_TASK
#define SIMULATION_WHEN_ALL_TASK

#include <coroutine>
#include <exception>
#include <utility>
#include <optional>
#include "simulation_when_all_counter.h"
#include "simulation_get_awaiter.h"

namespace simulation {
    namespace detail {
        template<typename T>
        struct WhenAllTaskPromise {
            using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<T>>;
            using value_type = std::remove_reference_t<T>;
            WhenAllCounter* counter_ = nullptr;
            std::exception_ptr exception_;
            std::add_pointer_t<T> result_;

            auto get_return_object() noexcept { return coroutine_handle_t::from_promise(*this); }
            std::suspend_always initial_suspend() noexcept { return {}; }

            auto final_suspend() noexcept {
                struct CompletionNotifier {
                    bool await_ready() const noexcept { return false; }
                    std::coroutine_handle<> await_suspend(coroutine_handle_t h) noexcept {
                        return h.promise().counter_->notify();
                    }
                    void await_resume() const noexcept {}
                };
                return CompletionNotifier{};
            }
            void unhandled_exception() noexcept { exception_ = std::current_exception(); }

            auto yield_value(T&& result) noexcept {
                result_ = std::addressof(result);
                return final_suspend();
            }

            T& result() & {
                if (exception_) {
                    std::rethrow_exception(exception_);
                }
                return *result_;
            }
            T&& result() && {
                if (exception_) {
                    std::rethrow_exception(exception_);
                }
                return std::forward<T>(*result_);
            }


            void return_void() noexcept {
                // Should have either suspened at co yield pointer or an exception was thrown before running off
                // the end of the coroutine.
                assert(false);
            }
        };

        template<>
        struct WhenAllTaskPromise<void> {
            using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<void>>;
            WhenAllCounter* counter_ = nullptr;
            std::exception_ptr exception_;

            auto get_return_object() noexcept { return coroutine_handle_t::from_promise(*this); }
            std::suspend_always initial_suspend() noexcept { return {}; }

            auto final_suspend() noexcept {
                struct CompletionNotifier {
                    bool await_ready() const noexcept { return false; }
                    std::coroutine_handle<> await_suspend(coroutine_handle_t h) noexcept {
                        return h.promise().counter_->notify();
                    }
                    void await_resume() const noexcept {}
                };
                return CompletionNotifier{};
            }

            void unhandled_exception() noexcept { exception_ = std::current_exception(); }

            void return_void() noexcept {}

            void result() {
                if (exception_) {
                    std::rethrow_exception(exception_);
                }
            }
        };

        template<typename T>
        class [[nodiscard]] WhenAllTask {
        public:
            using promise_type = WhenAllTaskPromise<T>;
            using handle_t = std::coroutine_handle<promise_type>;

            WhenAllTask(handle_t h) : handle_(h) {}
            WhenAllTask(const WhenAllTask&) = delete;
            WhenAllTask& operator=(const WhenAllTask&) = delete;
            WhenAllTask(WhenAllTask&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
            WhenAllTask& operator=(WhenAllTask&& other) noexcept {
                if (this != &other) {
                    if (handle_) handle_.destroy();
                    handle_ = std::exchange(other.handle_, nullptr);
                }
                return *this;
            }
            ~WhenAllTask() { if (handle_) handle_.destroy(); }

            void start(WhenAllCounter& counter) noexcept {
                handle_.promise().counter_ = &counter;
                handle_.resume();
            }

            decltype(auto) result() & {
                return handle_.promise().result();
            }

            decltype(auto) result() && {
                return std::move(handle_.promise()).result();
            }

            decltype(auto) non_void_result() &
                requires (!std::is_void_v<T>) {
                return this->result();
            }

            void non_void_result() &
                requires std::is_void_v<T> {
                this->result();
            }

            decltype(auto) non_void_result() &&
                requires (!std::is_void_v<T>) {
                return std::move(*this).result();
            }

            void non_void_result() &&
                requires std::is_void_v<T> {
                std::move(*this).result();
            }

        private:
            handle_t handle_;
        };

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<!std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(Awaitable a) -> WhenAllTask<Result> {
            co_yield co_await static_cast<Awaitable&&>(a);
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(Awaitable a) -> WhenAllTask<void> {
            co_await static_cast<Awaitable&&>(a);
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<!std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(std::reference_wrapper<Awaitable> a) -> WhenAllTask<Result> {
            co_yield co_await a.get();
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(std::reference_wrapper<Awaitable> a) -> WhenAllTask<void> {
            co_await a.get();
        }
    }
}
#endif // SIMULATION_WHEN_ALL_TASK
