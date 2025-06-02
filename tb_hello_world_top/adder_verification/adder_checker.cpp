#include "adder_checker.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// This file contains explicit template instantiations and non-template utility functions
// The main template implementation is in adder_checker_impl.h

// Example explicit instantiation (uncomment when you have the actual DUT type):
// #include "Vhello_world_top.h"
// template class AdderChecker<Vhello_world_top>;

/**
 * @brief Utility function to print AdderCheckerConfig
 */
void print_adder_checker_config(const AdderCheckerConfig& config, std::ostream& os) {
    os << "AdderChecker Configuration:" << std::endl;
    os << "  Overflow Checking: " << (config.enable_overflow_checking ? "Enabled" : "Disabled") << std::endl;
    os << "  Timing Checks: " << (config.enable_timing_checks ? "Enabled" : "Disabled") << std::endl;
    os << "  Value Logging: " << (config.enable_value_logging ? "Enabled" : "Disabled") << std::endl;
    os << "  Strict Mode: " << (config.strict_mode ? "Enabled" : "Disabled") << std::endl;
    os << "  Pipeline Depth: " << config.pipeline_depth << std::endl;
    os << "  Max Pending Transactions: " << config.max_pending_transactions << std::endl;
    os << "  Max Response Cycles: " << config.max_response_cycles << std::endl;
    os << "  Min Pass Rate: " << std::fixed << std::setprecision(2) 
       << (config.min_pass_rate * 100.0) << "%" << std::endl;
    os << "  Min Transactions for Stats: " << config.min_transactions_for_stats << std::endl;
}

/**
 * @brief Utility function to print AdderCheckerStats
 */
void print_adder_checker_stats(const AdderCheckerStats& stats, std::ostream& os) {
    os << "AdderChecker Statistics:" << std::endl;
    os << "  Overflow Cases Checked: " << stats.overflow_cases_checked << std::endl;
    os << "  Corner Cases Checked: " << stats.corner_cases_checked << std::endl;
    os << "  Timing Violations: " << stats.timing_violations << std::endl;
    os << "  Value Mismatches: " << stats.value_mismatches << std::endl;
    os << "  Pipeline Errors: " << stats.pipeline_errors << std::endl;
    
    if (stats.result_histogram.size() > 0) {
        os << "  Result Range: " << stats.min_result_seen 
           << " to " << stats.max_result_seen << std::endl;
        os << "  Unique Results: " << stats.result_histogram.size() << std::endl;
    }
    
    if (stats.avg_check_latency_ns > 0) {
        os << "  Avg Check Latency: " << std::fixed << std::setprecision(3) 
           << stats.avg_check_latency_ns << " ns" << std::endl;
    }
}

/**
 * @brief Calculate statistics from result histogram
 */
void calculate_histogram_stats(const std::unordered_map<uint16_t, uint64_t>& histogram,
                              double& mean, double& std_dev, uint64_t& total_samples) {
    if (histogram.empty()) {
        mean = std_dev = 0.0;
        total_samples = 0;
        return;
    }
    
    // Calculate mean
    uint64_t sum = 0;
    total_samples = 0;
    for (const auto& pair : histogram) {
        sum += static_cast<uint64_t>(pair.first) * pair.second;
        total_samples += pair.second;
    }
    mean = static_cast<double>(sum) / total_samples;
    
    // Calculate standard deviation
    double variance_sum = 0.0;
    for (const auto& pair : histogram) {
        double diff = pair.first - mean;
        variance_sum += diff * diff * pair.second;
    }
    std_dev = std::sqrt(variance_sum / total_samples);
}

/**
 * @brief Print histogram in a compact format
 */
void print_result_histogram(const std::unordered_map<uint16_t, uint64_t>& histogram, 
                           std::ostream& os, size_t max_entries) {
    if (histogram.empty()) {
        os << "No histogram data available" << std::endl;
        return;
    }
    
    // Convert to sorted vector for display
    std::vector<std::pair<uint16_t, uint64_t>> sorted_histogram(histogram.begin(), histogram.end());
    std::sort(sorted_histogram.begin(), sorted_histogram.end());
    
    double mean, std_dev;
    uint64_t total_samples;
    calculate_histogram_stats(histogram, mean, std_dev, total_samples);
    
    os << "Result Histogram (Total: " << total_samples << " samples):" << std::endl;
    os << "  Mean: " << std::fixed << std::setprecision(2) << mean << std::endl;
    os << "  Std Dev: " << std::fixed << std::setprecision(2) << std_dev << std::endl;
    
    if (sorted_histogram.size() <= max_entries) {
        // Show all entries
        for (const auto& entry : sorted_histogram) {
            double percentage = (static_cast<double>(entry.second) / total_samples) * 100.0;
            os << "  " << std::setw(5) << entry.first << ": " 
               << std::setw(8) << entry.second << " (" 
               << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
        }
    } else {
        // Show top entries and summary
        std::partial_sort(sorted_histogram.begin(), 
                         sorted_histogram.begin() + max_entries,
                         sorted_histogram.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
        
        os << "  Top " << max_entries << " results:" << std::endl;
        for (size_t i = 0; i < max_entries; ++i) {
            const auto& entry = sorted_histogram[i];
            double percentage = (static_cast<double>(entry.second) / total_samples) * 100.0;
            os << "    " << std::setw(5) << entry.first << ": " 
               << std::setw(8) << entry.second << " (" 
               << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
        }
        os << "  ... and " << (sorted_histogram.size() - max_entries) << " more" << std::endl;
    }
}

/**
 * @brief Validate AdderCheckerConfig parameters
 */
bool validate_adder_checker_config(const AdderCheckerConfig& config, std::string& error_msg) {
    if (config.pipeline_depth == 0) {
        error_msg = "Pipeline depth must be greater than 0";
        return false;
    }
    
    if (config.max_pending_transactions == 0) {
        error_msg = "Max pending transactions must be greater than 0";
        return false;
    }
    
    if (config.max_response_cycles == 0) {
        error_msg = "Max response cycles must be greater than 0";
        return false;
    }
    
    if (config.min_pass_rate < 0.0 || config.min_pass_rate > 1.0) {
        error_msg = "Min pass rate must be between 0.0 and 1.0";
        return false;
    }
    
    if (config.min_transactions_for_stats == 0) {
        error_msg = "Min transactions for stats must be greater than 0";
        return false;
    }
    
    return true;
}

/**
 * @brief Create default configuration for different verification scenarios
 */
AdderCheckerConfig create_basic_adder_config() {
    AdderCheckerConfig config;
    config.enable_overflow_checking = true;
    config.enable_timing_checks = true;
    config.enable_value_logging = false;
    config.strict_mode = false;
    config.pipeline_depth = 2;  // Based on hello_world_top.sv
    return config;
}

AdderCheckerConfig create_strict_adder_config() {
    AdderCheckerConfig config = create_basic_adder_config();
    config.strict_mode = true;
    config.enable_value_logging = true;
    config.min_pass_rate = 1.0;  // 100% pass rate required
    return config;
}

AdderCheckerConfig create_performance_adder_config() {
    AdderCheckerConfig config = create_basic_adder_config();
    config.enable_timing_checks = true;
    config.max_response_cycles = 5;  // Tighter timing requirements
    config.max_pending_transactions = 10000;  // Allow more pending for throughput testing
    return config;
}

/**
 * @brief Utility to format timing duration
 */
std::string format_duration(const std::chrono::high_resolution_clock::duration& duration) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    if (ns < 1000) {
        return std::to_string(ns) + " ns";
    } else if (ns < 1000000) {
        return std::to_string(ns / 1000) + " μs";
    } else if (ns < 1000000000) {
        return std::to_string(ns / 1000000) + " ms";
    } else {
        return std::to_string(ns / 1000000000) + " s";
    }
}
