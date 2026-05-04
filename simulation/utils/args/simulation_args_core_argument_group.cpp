#include "simulation_args_core_argument_group.h"
#include "simulation_logging_utils.h"
#include "simulation_progress_reporter.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <random>
#include <unistd.h>

void simulation::args::CoreArgumentGroup::register_args(GroupApp& app){
    app.add_argument<uint64_t>("seed", seed_, "RNG seed. 0 = randomise and log chosen value.", seed_);
    app.add_enum_argument("verbosity", verbosity_str_, "Log verbosity level.", {"debug", "info", "warning", "error", "fatal"}, verbosity_str_);
    app.add_argument<std::string>("output-dir", output_dir_str_,
        "Directory for all run artifacts (log, waveform, progress.jsonl). "
        "Created automatically if absent. Omit for stdout-only runs with no artifact capture.",
        output_dir_str_);
    app.add_flag("waves", waves_, "Enable waveform dump.");
    app.add_flag("dry-run", dry_run_, "Resolve and print all args, then exit without simulating.");
    app.add_argument<uint64_t>("max-time", max_time_ps_, "Max simulation time in ps. 0 = unlimited.", max_time_ps_);
}

void simulation::args::CoreArgumentGroup::post_parse_resolve() {
    // Verbosity -> LoggerConfig
    static const std::unordered_map<std::string, simulation::LogLevel> kLevelMap = {
        {"debug", simulation::LogLevel::DEBUG},
        {"info", simulation::LogLevel::INFO},
        {"warning", simulation::LogLevel::WARNING},
        {"error", simulation::LogLevel::ERROR},
        {"fatal", simulation::LogLevel::FATAL},
    };
    if (auto it = kLevelMap.find(verbosity_str_); it != kLevelMap.end()) {
        simulation::LoggerConfig::instance().set_min_log_level(it->second);
    }
    // Output directory: create it and wire up the logfile
    if (!output_dir_str_.empty()) {
        std::filesystem::create_directories(output_dir_str_);
        const std::string log_path = (std::filesystem::path(output_dir_str_) / (binary_name_ + ".log")).string();
        simulation::LoggerConfig::instance().set_log_file(log_path, simulation::OutputMode::FILE_ONLY);

        // Preserve the original terminal before redirecting stdout, so we can
        // write live status updates there even while logs go to the file.
        const bool stdout_is_tty = (isatty(STDOUT_FILENO) != 0);
        const int saved_stdout_fd = stdout_is_tty ? dup(STDOUT_FILENO) : -1;

        // Redirect both stdout and stderr to the log file so that Verilator
        // $fatal / SVA failure messages (printed via printf to stdout) and any
        // stderr output (ASan, C++ exceptions) are captured alongside the
        // structured log regardless of how the binary is launched
        // (campaign runner, reproduce.sh, or direct invocation).
        // The logger is already FILE_ONLY so no logger output goes to stdout;
        // redirecting stdout here only affects Verilator's own printf calls.
        std::freopen(log_path.c_str(), "a", stdout);
        std::freopen(log_path.c_str(), "a", stderr);

        if (saved_stdout_fd >= 0) {
            FILE* tty = fdopen(saved_stdout_fd, "w");
            if (tty) {
                std::fprintf(tty, "Simulation running — log: %s%s%s\n",
                             simulation::colours::DIM, log_path.c_str(), simulation::colours::RESET);
                std::fflush(tty);
                simulation::ProgressReporter::instance().set_console_output(tty);
            } else {
                close(saved_stdout_fd);
            }
        }
    }
    // Seed resolution
    if (seed_ == 0) {
        seed_ = std::random_device{}();
        simulation::log_info("CoreArgs", "Seed 0 specified - randomised seed: " + std::to_string(seed_));
    }
    // Modify max-time to ensure max-time=0 means running until all tasks complete.
    if (max_time_ps_ == 0) {
        max_time_ps_ = std::numeric_limits<uint64_t>::max();
        simulation::log_info("CoreArgs", "max-time 0 specified - running until all tasks complete.");
    }
}

[[nodiscard]] std::string_view simulation::args::CoreArgumentGroup::description() const {
    return "Core simulation arguments (always present in every testbench)";
}
