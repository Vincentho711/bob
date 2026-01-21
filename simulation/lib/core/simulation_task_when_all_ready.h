#ifndef SIMULATION_TASK_WHEN_ALL_READY_H
#define SIMULATION_TASK_WHEN_ALL_READY_H

#include <atomic>
#include <exception>
#include <tuple>
#include <vector>
#include <coroutine>
#include <utility>
#include <cassert>
#include <type_traits>

namespace simulation {
    namespace detail {
        // Helpers for Tuple vs Vector compatibility
        template<typename T>
        size_t get_container_size(const T& obj) {
            if constexpr (requires { obj.size(); }) return obj.size();
            else return std::tuple_size_v<std::remove_cvref_t<T>>;
        }

        template<typename T>
        bool is_container_empty(const T& obj) {
            if constexpr (requires { obj.empty(); }) return obj.empty();
            else return std::tuple_size_v<std::remove_cvref_t<T>> == 0;
        }

        struct WhenAllCounter {
            std::atomic<size_t> count_;
            std::coroutine_handle<> continuation_;

            WhenAllCounter(size_t count) noexcept : count_(count + 1), continuation_(nullptr) {}
            bool is_ready() const noexcept { return count_.load(std::memory_order_acquire) == 0; }

            // The last child calls notify and returns the continuation_ which is the parent handle
            std::coroutine_handle<> notify() noexcept {
                if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) return continuation_; // I am the last child, resume the parent.
                return std::noop_coroutine(); // The finished child task is not the last one, don't return anything
            }

            void start_awaiting(std::coroutine_handle<> continuation) noexcept {
                continuation_ = continuation; // Store the handle so children can find it
                // Check for rare but possible case when all the children are already done before we even finished starting them.
                if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    continuation_.resume();
                }
            }
        };

        template<typename T>
        auto get_awaiter(T&& value) {
            if constexpr (requires { std::forward<T>(value).operator co_await(); }) 
                return std::forward<T>(value).operator co_await();
            else if constexpr (requires { operator co_await(std::forward<T>(value)); }) 
                return operator co_await(std::forward<T>(value));
            else return std::forward<T>(value);
        }

        template<typename T>
        using await_result_t = decltype(get_awaiter(std::declval<T>()).await_resume());

        template<typename T>
        struct WhenAllTaskPromise {
            using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<T>>;
            using value_type = std::remove_reference_t<T>;
            WhenAllCounter* counter_ = nullptr;
            std::exception_ptr exception_;
            std::optional<value_type> result_;

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
                result_.emplace(std::forward<T>(result));
                return std::suspend_always{};
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
                return std::move(*result_);
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

        private:
            handle_t handle_;
        };

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<!std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(Awaitable a) -> WhenAllTask<Result> {
            Result result = co_await std::forward<Awaitable>(a);
            co_yield result;
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(Awaitable a) -> WhenAllTask<void> {
            co_await std::forward<Awaitable>(a);
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<!std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(std::reference_wrapper<Awaitable> a) -> WhenAllTask<Result> {
            Result result = co_await a.get();
            co_yield result;
        }

        template<
            typename Awaitable,
            typename Result = await_result_t<Awaitable>,
            std::enable_if_t<std::is_void_v<Result>, int> = 0>
        auto make_when_all_task(std::reference_wrapper<Awaitable> a) -> WhenAllTask<void> {
            co_await a.get();
        }
    }

    template<typename Container>
    class WhenAllReadyAwaitable {
    public:
        explicit WhenAllReadyAwaitable(Container&& tasks) 
            : counter_(detail::get_container_size(tasks)), tasks_(std::move(tasks)) {}

        bool await_ready() const noexcept { 
            return detail::is_container_empty(tasks_) || counter_.is_ready(); 
        }

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            // Start the children tasks
            for (auto& t : tasks_) t.start(counter_);
            // Only when all the loop is done, the parent calls start_awaiting().
            // This provides the handle needed to wake the parent up and performs the final fetch_sub(1).
            counter_.start_awaiting(continuation);
        }

        Container await_resume() noexcept { return std::move(tasks_); }

    protected:
        detail::WhenAllCounter counter_;
        Container tasks_;
    };

    template<typename... Awaitables>
    auto when_all_ready(Awaitables&&... awaitables) {
        auto tasks = std::make_tuple(detail::make_when_all_task(std::forward<Awaitables>(awaitables))...);
        using Container = decltype(tasks);

        struct TupleAwaiter : WhenAllReadyAwaitable<Container> {
            using WhenAllReadyAwaitable<Container>::WhenAllReadyAwaitable;
            void await_suspend(std::coroutine_handle<> continuation) noexcept {
                std::apply([this](auto&... t) { (t.start(this->counter_), ...); }, this->tasks_);
                this->counter_.start_awaiting(continuation);
            }
        };
        return TupleAwaiter(std::move(tasks));
    }

    template<typename T>
    auto when_all_ready(std::vector<Task<T>>&& awaitables) {
        std::vector<detail::WhenAllTask<T>> internal_tasks;
        internal_tasks.reserve(awaitables.size());
        for (auto& a : awaitables) internal_tasks.push_back(detail::make_when_all_task(std::move(a)));
        return WhenAllReadyAwaitable<std::vector<detail::WhenAllTask<T>>>(std::move(internal_tasks));
    }
}

#endif
