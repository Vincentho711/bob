#ifndef SIMULATION_EVENT_H
#define SIMULATION_EVENT_H
#include <coroutine>
#include <vector>

namespace simulation{
    class Event {
    public:
        using handle_t = std::coroutine_handle<>;
        Event() {}

        struct Awaiter {
            Event &event;
            bool await_ready() const noexcept { return event.triggered_; }
            void await_suspend(handle_t h) noexcept;
            constexpr void await_resume() const noexcept {}
        };
        auto operator co_await() {return Awaiter{*this}; }
        void trigger() noexcept;
        void reset() noexcept;

    private:
        bool triggered_ = false;
        std::vector<handle_t> waiters_;
    };
}

#endif // SIMULATION_EVENT_H
