#ifndef SIMULATION_TASK_WHEN_ALL_READY_H
#define SIMULATION_TASK_WHEN_ALL_READY_H

#include <exception>
#include <tuple>
#include <vector>
#include <coroutine>
#include <utility>
#include <cassert>
#include <type_traits>

#include "detail/simulation_when_all_task.h"
#include "detail/simulation_when_all_counter.h"
#include "simulation_task_symmetric_transfer.h"

namespace simulation {
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
