#ifndef SIMULATION_GET_AWAITER_H
#define SIMULATION_GET_AWAITER_H
#include <utility>

namespace simulation {
    namespace detail {
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
    }
}
#endif // SIMULATION_GET_AWAITER_H
