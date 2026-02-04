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

template <typename DutType>
class EventSchedular {
public:
    EventSchedular() = default;

    void schedule_event(uint64_t time_ps, Clock<DutType>* clock, ClockStep step) {
        event_queue_.emplace(time_ps, clock, step);
    }

    bool has_events() const {
        return !event_queue_.empty();
    }

    uint64_t peek_next_time() const {
        if (event_queue_.empty()) {
            return std::numeric_limits<uint64_t>::max();
        }

        return event_queue_.top().time_ps;
    }

    // Get all events at the current time
    // Handles multiple clocks with events at the same time
    std::vector<ClockEvent<DutType>> get_next_batch() {
        std::vector<ClockEvent<DutType>> batch;

        if (event_queue_.empty()) {
            return batch;
        }

        uint64_t current_time = event_queue_.top().time_ps;

        // Collect all events at this time
        while (!event_queue_.empty() && event_queue_.top().time_ps == current_time) {
            batch.push_back(event_queue_.top());
            event_queue_.pop();
        }
        return batch;
    }

    void clear() {
        while (!event_queue_.empty()) {
            event_queue_.pop();
        }
    }
private:
    std::priority_queue<ClockEvent<DutType>> event_queue_;
};
}
#endif // SIMULATION_EVENT_SCHEDULAR_H
