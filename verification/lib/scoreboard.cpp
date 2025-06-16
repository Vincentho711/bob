#include "scoreboard.h"

void print_scoreboard_config(const ScoreboardConfig& config, std::ostream& os) {
    os << "Scoreboard Configuration:" << std::endl;
    os << "  Max Latency Cycles: " << config.max_latency_cycles << std::endl;
    os << "  Enable Out-of-Order Matching: " << config.enable_out_of_order_matching << std::endl;
    os << "  Stop on First Error: " << (config.stop_on_first_error ? "Enabled" : "Disabled") << std::endl;
    os << "  Detailed Logging: " << (config.enable_detailed_logging ? "Enabled" : "Disabled") << std::endl;
    os << "  Timeout Checking: " << (config.enable_timeout_checking ? "Enabled" : "Disabled") << std::endl;
    os << "  Use Checker: " << (config.use_checker ? "Enabled" : "Disabled") << std::endl;
    os << "  Log Level: " << static_cast<std::size_t>(config.log_level) << std::endl;
    os << "  Max Pending Transactions: " << config.max_pending_transactions << std::endl;
    os << "  Timeout Cycles: " << config.timeout_cycles << std::endl;
    os << "  Min Match Rate: " << std::fixed << std::setprecision(2) << (config.min_match_rate * 100.0) << "%" << std::endl;
}

void print_scoreboard_stats(const ScoreboardStats& stats, std::ostream& os) {
    os << "Scoreboard Statistics:" << std::endl;
    os << "  Expected Transactions Received: " << stats.total_expected << std::endl;
    os << "  Actual Transactions Received: " << stats.total_actual << std::endl;
    os << "  Expected Transactions Dropped: " << stats.expected_transaction_dropped << std::endl;
    os << "  Actual Transactions Dropped: " << stats.actual_transaction_dropped << std::endl;
    os << "  Transactions Matched: " << stats.matched << std::endl;
    os << "  Mismatches: " << stats.mismatch << std::endl;
    os << "  Timed-out: " << stats.timed_out << std::endl;
    os << "  Checks Performed: " << stats.checks_performed << std::endl;
    os << "  Checks Passed: " << stats.checks_passed << std::endl;
    os << "  Checks Failed: " << stats.checks_failed << std::endl;

    if ((stats.total_actual + stats.total_expected) > 0) {
        double match_rate = static_cast<double>(stats.matched / (stats.matched + stats.mismatch));
        os << "  Match Rate: " << std::fixed << std::setprecision(2) << (match_rate * 100.0) << "%" << std::endl;
    }

    // Calculate total simulation time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        stats.last_transaction_time - stats.first_transaction_time);

    if (duration.count() > 0) {
        os << "  Total Runtime: " << duration.count() << " ms" << std::endl;

        if ((stats.matched + stats.mismatch) > 0) {
            double throughput = static_cast<double>(stats.matched / (stats.matched + stats.mismatch)) /
                               (duration.count() / 1000.0);  // transactions per second
            os << "  Throughput: " << std::fixed << std::setprecision(1) << throughput << " txn/s" << std::endl;
        }
    }
}
