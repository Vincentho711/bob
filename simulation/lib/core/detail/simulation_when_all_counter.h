#ifndef SIMULATION_WHEN_ALL_COUNTER_H
#define SIMULATION_WHEN_ALL_COUNTER_H

#include <atomic>
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
    }
}
#endif // SIMULATION_WHEN_ALL_COUNTER_H
