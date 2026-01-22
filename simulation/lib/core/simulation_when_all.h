// #ifndef SIMULATION_WHEN_ALL_H
// #define SIMULATION_WHEN_ALL_H
// #include <tuple>
// #include "detail/simulation_get_awaiter.h"
// #include "detail/simulation_when_all_counter.h"
// #include "detail/simulation_when_all_task.h"
// #include "simulation_task_symmetric_transfer.h"
//
// namespace simulation {
//     // Vector specialisation
//     template<typename T>
//     class WhenAllAwaitable {
//         using Container = std::vector<detail::WhenAllTask<T>>;
//     public:
//         explicit WhenAllAwaitable(Container&& tasks)
//             : counter_(detail::get_container_size(tasks)), tasks_(std::move(tasks)) {}
//
//         bool await_ready() const noexcept {
//             return detail::is_container_empty(tasks_) || counter_.is_ready();
//         }
//
//         void await_suspend(std::coroutine_handle<> continuation) noexcept {
//             // Start the children tasks
//             for (auto& t : tasks_) t.start(counter_);
//             // Only when all the loop is done, the parent calls start_awaiting().
//             // This provides the handle needed to wake the parent up and performs the final fetch_sub(1).
//             counter_.start_awaiting(continuation);
//         }
//
//         // Overload with non-void types
//         template<
//             typename U = T,
//             std::enable_if_t<!std::is_void_v<U>, int> = 0>
//         std::vector<U> await_resume() {
//             std::vector<U> results;
//             results.reserve(tasks_.size());
//             for (auto &t : tasks_) {
//                 results.push_back(std::move(t).result());
//             }
//         }
//         // Overload with void types
//         template<
//             typename U = T,
//             std::enable_if_t<std::is_void_v<U>, int> = 0>
//         void await_resume() {
//             for (auto &t : tasks_) {
//                 t.result();
//             }
//         }
//     protected:
//         detail::WhenAllCounter counter_;
//         Container tasks_;
//     };
//
//     // Tuple specialisation
//     template<typename... Awaitables>
//     class WhenAllTupleAwaitable {
//         using tasks = std::make_tuple(detail::make_when_all_task(std::forwad<Awaitables>(awaitables))...);
//     public:
//         explicit WhenAllTupleAwaitable(Awaitables&&... awaitables)
//             : counter_(sizeof...(Awaitables)), tasks_(std::move(tasks)) {}
//
//         bool await_ready() const noexcept { return counter_.is_ready(); }
//
//         void await_suspend(std::coroutine_handle<> continuation) noexcept {
//             std::apply([this](auto&... t) { (t.start(this->counter_), ...); }, tasks_);
//             counter_.start_awaiting(continuation);
//         }
//
//         template<typename... T = Awaitables, std::enable_if_t<detail::all_tasks_void<T...>::value, int> = 0>
//         void await_resume() {
//             std::apply([](auto&... ts) { (ts.result(), ...); }, tasks_);
//         }
//
//         // VERSION B: If tasks return values, return a tuple of those values
//         template<typename... T = Tasks, std::enable_if_t<!detail::all_tasks_void<T...>::value, int> = 0>
//         auto await_resume() {
//             return std::apply([](auto&... ts) {
//                 return std::make_tuple(std::move(ts).result()...);
//             }, tasks_);
//         }
//         void await_resume() {
//             return std::apply([](auto&... ts) {
//                 // If every task result is void, return void
//                 if constexpr ((std::is_void_v<decltype(ts.result())> && ...)) {
//                     (ts.result(), ...);
//                 } else {
//                     // Returns a tuple of non-void results
//                     return std::make_tuple(std::move(ts).result()...);
//                 }
//             }, tasks_);
//         }
//     private:
//         detail::WhenAllCounter counter_;
//         Container tasks_;
//     };
//
//     template<typename T>
//     [[nodiscard]] auto when_all(std::vector<Task<T>>&& awaitables) {
//         std::vector<detail::WhenAllTask<T>> internal_tasks;
//         internal_tasks.reserve(awaitables.size());
//         for (auto& a : awaitables) {
//             internal_tasks.push_back(detail::make_when_all_task(std::move(a)));
//         }
//         return WhenAllAwaitable<T>(std::move(internal_tasks));
//     }
//
//     template<typename... Awaitables>
//     [[nodiscard]] auto when_all(Awaitables&&... awaitables) {
//         auto tasks = std::make_tuple(detail::make_when_all_task(std::forward<Awaitables>(awaitables))...);
//         return WhenAllTupleAwaitable<decltype(detail::make_when_all_task(std::forward<Awaitables>(awaitables)))...>(std::move(tasks));
//     }
// }
// #endif // SIMULATION_WHEN_ALL_H
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

    // --- 1. The Base Logic (RAII for tasks and the counter) ---
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

    // --- 2. Tuple Specialization ---
    template<typename Container, bool AllVoid>
    class WhenAllTupleAwaitable : public WhenAllAwaitableBase<Container> {
    public:
        using WhenAllAwaitableBase<Container>::WhenAllAwaitableBase;

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            std::cout << "Tuple await suspend() entered" << std::endl;
            // Start children first (count is N+1, so it won't hit 0)
            std::apply([this](auto&... t) { 
                (t.start(this->counter_), ...); 
            }, this->tasks_);

            // This sets the handle AND does the final decrement.
            // If children are done, it resumes immediately.
            this->counter_.start_awaiting(continuation);
        }

        // Overload: If all tasks in the tuple are void
        template<bool V = AllVoid, std::enable_if_t<V, int> = 0>
        void await_resume() {
            std::cout << "done with void return" << std::endl;
            std::apply([](auto&... ts) { (ts.result(), ...); }, this->tasks_);
        }

        // Overload: If tasks return values (Returns a tuple of results)
        template<bool V = AllVoid, std::enable_if_t<!V, int> = 0>
        auto await_resume() {
            std::cout << "done with non-void tuple making." << std::endl;
            return std::apply([](auto&... ts) {
                return std::make_tuple(ts.result()...);
            }, this->tasks_);
        }
    };

    // --- 3. Vector Specialization ---
    template<typename T>
    class WhenAllVectorAwaitable : public WhenAllAwaitableBase<std::vector<detail::WhenAllTask<T>>> {
        using Container = std::vector<detail::WhenAllTask<T>>;
    public:
        using WhenAllAwaitableBase<Container>::WhenAllAwaitableBase;

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            for (auto& t : this->tasks_) t.start(this->counter_);
            this->counter_.start_awaiting(continuation);
        }

        template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
        std::vector<U> await_resume() {
            std::vector<U> results;
            results.reserve(this->tasks_.size());
            for (auto& t : this->tasks_) results.push_back(std::move(t).result());
            return results;
        }

        template<typename U = T, std::enable_if_t<std::is_void_v<U>, int> = 0>
        void await_resume() {
            for (auto& t : this->tasks_) t.result();
        }
    };

    // --- 4. Public API (The Factory Functions) ---

    // Variadic version using the decltype pattern
    template<typename... Awaitables>
    [[nodiscard]] auto when_all(Awaitables&&... awaitables) {
        auto tasks = std::make_tuple(
            detail::make_when_all_task(
                std::forward<Awaitables>(awaitables)
            )...
        );
        using Container = decltype(tasks);
        
        // Use your helper to check if we should return void or a tuple
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
