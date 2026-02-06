#ifndef SIMULATION_EVENT_SCHEDULAR_H
#define SIMULATION_EVENT_SCHEDULAR_H
#include <queue>
#include <vector>
#include <functional>
#include <cstdint>
#include <limits>
#include "simulation_clock.h"

namespace simulation {
template <typename DutType>
class Clock;

template <typename DutType>
struct ClockEvent {
    uint64_t time_ps;      // When this even occurs
    Clock<DutType>* clock; //When clock trigger this event
    ClockStep step;        // Which phase of the clock

    // Compare 2 Clock Events to see which one should be processed first
    // Priority queue uses reverse ordering (min-heap)
    bool operator<(const ClockEvent& other) const {
        // Primary : earlier time has higher priority
        if (time_ps != other.time_ps) {
            return time_ps > other.time_ps;
        }
        // Secondary : deterministic ordering for simultaneous events
        // Process clocks in pointer order
        if (clock != other.clock) {
            return clock > other.clock;
        }

        // Tertiary : step order
        return step > other.step;
    }
};

struct AsyncEvent {
    uint64_t time_ps;
    std::function<void()> callback;
    uint64_t priority; // Lower number = higher priority

    bool operator<(const AsyncEvent& other) const {
        if (time_ps != other.time_ps) return time_ps > other.time_ps;
        return priority > other.priority;
    }
};

template <typename DutType>
class EventScheduler {
public:
    EventScheduler() = default;

    void schedule_clock_event(uint64_t time_ps, Clock<DutType>* clock, ClockStep step) {
        clock_events_.emplace(time_ps, clock, step);
    }

    // Schedule async callback at specific time
    void schedule_async_event(uint64_t time_ps, std::function<void()> callback, uint32_t priority = 0) {
        async_events_.emplace(time_ps, callback, priority);
    }

    // Schedule callback after delay from current time
    void schedule_async_delay(uint64_t delay_ps, std::function<void()> callback, uint32_t priority = 0) {
        uint64_t target_time = current_time_ps_ + delay_ps;
        async_events_.emplace(target_time, callback, priority);
    }

    // Execute async callback immediately
    void execute_async_immediate(std::function<void()> callback) {
        immediate_events_.push_back(callback);
    }

    bool has_events() const {
        return !clock_events_.empty() || !async_events_.empty();
    }

    uint64_t peek_next_time() const {
        uint64_t clock_time = clock_events_.empty() ? std::numeric_limits<uint64_t>::max() : clock_events_.top().time_ps;
        uint64_t async_time = async_events_.empty() ? std::numeric_limits<uint64_t>::max() : async_events_.top().time_ps;
        return std::min(clock_time, async_time);
    }

    //
    struct EventBatch {
        uint64_t time_ps;
        std::vector<ClockEvent<DutType>> clock_events;
        std::vector<AsyncEvent> async_events;

        bool has_clock_events() const { return !clock_events.empty(); }
        bool has_async_events() const { return !async_events.empty(); }
    };

    // Get all events at the current time
    // Handles multiple clocks with events at the same time as well as async events
    EventBatch get_next_batch() {
        EventBatch batch;
        if (!has_events()) {
            return batch;
        }

        uint64_t next_time = peek_next_time();
        batch.time_ps = next_time;

        // Collect all clock events at this time
        while (!clock_events_.empty() && clock_events_.top().time_ps == next_time) {
            batch.clock_events.push_back(clock_events_.top());
            clock_events_.pop();
        }

        // Collect all async events at this time
        while (!async_events_.empty() && async_events_.top().time_ps == next_time) {
            batch.async_events.push_back(async_events_.top());
            async_events_.pop();
        }

        return batch;
    }

    void process_async_immediate_events() {
        auto events_copy = std::move(immediate_events_);
        immediate_events_.clear();

        for(auto& callback : events_copy) {
            callback();
        }
    }

    void set_current_time(uint64_t time_ps) {
        current_time_ps_ = time_ps;
    }

    void clear() {
        while (!clock_events_.empty()) {
            clock_events_.pop();
        }
        while (!async_events_.empty()) {
            async_events_.pop();
        }
        immediate_events_.clear();
    }

private:
    std::priority_queue<ClockEvent<DutType>> clock_events_;
    std::priority_queue<AsyncEvent> async_events_;
    std::vector<std::function<void()>> immediate_events_;

    uint64_t current_time_ps_ = 0;
};
}
#endif // SIMULATION_EVENT_SCHEDULER_H
