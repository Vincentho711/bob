#ifndef SIMULATION_PROGRESS_REPORTER_H
#define SIMULATION_PROGRESS_REPORTER_H

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simulation {

inline constexpr unsigned PROGRESS_SCHEMA_VERSION = 2;

class ProgressReporter {
public:
    static ProgressReporter& instance();

    ProgressReporter(const ProgressReporter&) = delete;
    ProgressReporter& operator=(const ProgressReporter&) = delete;
    ProgressReporter(ProgressReporter&&) = delete;
    ProgressReporter& operator=(ProgressReporter&&) = delete;

    // Must be called before any run_start / seq_start / heartbeat. Writes
    // progress.jsonl directly into output_dir (caller is responsible for
    // creating the directory). An empty output_dir leaves the reporter disabled.
    // On any filesystem error, logs a single warning and leaves the reporter
    // disabled; all subsequent calls become no-ops.
    void configure(std::filesystem::path output_dir,
                   std::string test_name,
                   uint64_t seed,
                   uint64_t max_time_ps,
                   uint32_t heartbeat_ms);

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    void run_start();
    void seq_start(uint64_t seq_id,
                   std::string_view name,
                   uint64_t seed,
                   std::string_view domain = "default");
    void seq_end(uint64_t seq_id,
                 std::string_view status,
                 std::string_view domain = "default");
    void heartbeat(std::string_view domain = "default");
    void run_end(std::string_view status, std::string_view msg = "");  // idempotent

private:
    ProgressReporter() = default;
    ~ProgressReporter();

    enum class JsonKind : uint8_t { Str, U64, I64, Bool, Null };
    struct JsonField {
        std::string_view key;
        JsonKind kind;
        uint64_t u = 0;
        int64_t  i = 0;
        bool     b = false;
        std::string_view s{};
    };

    // Build and write one event line. Envelope keys (schema_version, run_id, t,
    // ts_wall_us, ts_sim_ps, domain) are emitted first; `extras` are appended
    // in the order provided. Flushes after each line.
    void write_event_(std::string_view type,
                      std::string_view domain,
                      std::initializer_list<JsonField> extras);

    static void json_escape_append_(std::string& out, std::string_view in);
    static void json_field_append_(std::string& out, const JsonField& f);

    [[nodiscard]] uint64_t wall_us_now_() const;

    bool enabled_ = false;
    std::ofstream out_;

    std::string test_name_;
    uint64_t seed_ = 0;
    uint64_t max_time_ps_ = 0;

    // run_start metadata captured at configure() time
    std::string hostname_;
    uint64_t    pid_ = 0;
    uint64_t    ts_start_utc_us_ = 0;

    std::chrono::steady_clock::time_point start_wall_{};
    std::chrono::steady_clock::time_point last_heartbeat_{};
    std::chrono::milliseconds heartbeat_interval_{500};

    // Per-domain stack of currently-running sequence IDs. Push on seq_start,
    // pop on seq_end. Lets us emit parent_id without any TB-author input.
    std::unordered_map<std::string, std::vector<uint64_t>> domain_parent_stack_;

    struct SeqStartInfo { uint64_t wall_us; uint64_t sim_ps; };
    std::unordered_map<uint64_t, SeqStartInfo> seq_start_times_;
    uint64_t seq_count_ = 0;

    bool run_started_ = false;
    bool run_ended_ = false;
};

}  // namespace simulation

#endif  // SIMULATION_PROGRESS_REPORTER_H
