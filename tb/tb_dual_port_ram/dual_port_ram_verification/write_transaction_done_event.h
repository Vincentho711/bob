#ifndef WRITE_TRANSACTION_DONE_EVENT_H
#define WRITE_TRANSACTION_DONE_EVENT_H
#include <coroutine>
#include <vector>

class WriteTransactionDoneEvent {
public:
    using handle_t = std::coroutine_handle<>;
    WriteTransactionDoneEvent() {}

    struct Awaiter {
        WriteTransactionDoneEvent &event;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) noexcept;
        constexpr void await_resume() const noexcept {}
    };

    void set_done() noexcept;

private:
    std::vector<handle_t> suspended_coroutines;

};
#endif // WRITE_TRANSACTION_DONE_EVENT_H
