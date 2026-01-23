#ifndef SIMULATION_TASK_WHEN_ALL_H
#define SIMULATION_TASK_WHEN_ALL_H

#include <tuple>
#include <vector>
#include <coroutine>
#include <type_traits>

#include "detail/simulation_when_all_task.h"
#include "detail/simulation_when_all_counter.h"
#include "detail/simulation_get_awaiter.h"

namespace simulation {

    // Base WhenAllAwaitable
    template<typename Container>
    class WhenAllAwaitableBase {
    public:
        explicit WhenAllAwaitableBase(Container&& tasks)
            : counter_(detail::get_container_size(tasks)), tasks_(std::move(tasks)) {}

        bool await_ready() const noexcept {
            return detail::is_container_empty(tasks_) || counter_.is_ready(); 
        }

    protected:
        detail::WhenAllCounter counter_;
        Container tasks_;
    };

    // Tuple Specialisation
    template<typename Container, bool AllVoid>
    class WhenAllTupleAwaitable : public WhenAllAwaitableBase<Container> {
    public:
        using WhenAllAwaitableBase<Container>::WhenAllAwaitableBase;

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            std::cout << "Tuple await suspend() entered" << std::endl;
            // Start children first
            std::apply([this](auto&... t) { 
                (t.start(this->counter_), ...); 
            }, this->tasks_);

            // This sets the handle and does the final decrement.
            // If children are done, it resumes immediately.
            this->counter_.start_awaiting(continuation);
        }

        // Overload: If all tasks in the tuple are void
        template<bool V = AllVoid, std::enable_if_t<V, int> = 0>
        void await_resume() {
            std::apply([](auto&... ts) { (ts.result(), ...); }, this->tasks_);
        }

        // Overload: If tasks return values (Returns a tuple of results)
        template<bool V = AllVoid, std::enable_if_t<!V, int> = 0>
        auto await_resume() {
            return std::apply([](auto&... ts) {
                return std::make_tuple(ts.result()...);
            }, this->tasks_);
        }
    };

    // Vector Specialisation
    template<typename T>
    class WhenAllVectorAwaitable : public WhenAllAwaitableBase<std::vector<detail::WhenAllTask<T>>> {
        using Container = std::vector<detail::WhenAllTask<T>>;
    public:
        using WhenAllAwaitableBase<Container>::WhenAllAwaitableBase;

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            for (auto& t : this->tasks_) t.start(this->counter_);
            this->counter_.start_awaiting(continuation);
        }

        // If tasks return void
        template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
        std::vector<U> await_resume() {
            std::vector<U> results;
            results.reserve(this->tasks_.size());
            for (auto& t : this->tasks_) results.push_back(std::move(t).result());
            return results;
        }

        // If tasks return values
        template<typename U = T, std::enable_if_t<std::is_void_v<U>, int> = 0>
        void await_resume() {
            for (auto& t : this->tasks_) t.result();
        }
    };

    // Public API
    // Variadic version using the decltype pattern
    template<typename... Awaitables>
    [[nodiscard]] auto when_all(Awaitables&&... awaitables) {
        auto tasks = std::make_tuple(
            detail::make_when_all_task(
                std::forward<Awaitables>(awaitables)
            )...
        );
        using Container = decltype(tasks);

        // Use helper to check if we should return void or a tuple
        constexpr bool is_void = (std::is_void_v<detail::await_result_t<Awaitables>> && ...);

        return WhenAllTupleAwaitable<Container, is_void>(std::move(tasks));
    }

    // Vector version
    template<typename T>
    [[nodiscard]] auto when_all(std::vector<Task<T>>&& awaitables) {
        std::vector<detail::WhenAllTask<T>> internal_tasks;
        internal_tasks.reserve(awaitables.size());
        for (auto& a : awaitables) {
            internal_tasks.push_back(detail::make_when_all_task(std::move(a)));
        }
        return WhenAllVectorAwaitable<T>(std::move(internal_tasks));
    }
}

#endif
