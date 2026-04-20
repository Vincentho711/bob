#include "simulation_progress_reporter.h"

#include "simulation_context.h"
#include "simulation_logging_utils.h"

#include <cstdio>
#include <system_error>
#include <unistd.h>

namespace simulation {

ProgressReporter& ProgressReporter::instance() {
    static ProgressReporter inst;
    return inst;
}

ProgressReporter::~ProgressReporter() {
    if (out_.is_open()) {
        out_.flush();
        out_.close();
    }
}

void ProgressReporter::configure(std::filesystem::path output_dir,
                                 std::string test_name,
                                 uint64_t seed,
                                 uint64_t max_time_ps,
                                 uint32_t heartbeat_ms) {
    if (output_dir.empty()) {
        enabled_ = false;
        return;
    }

    test_name_ = std::move(test_name);
    seed_ = seed;
    max_time_ps_ = max_time_ps;
    heartbeat_interval_ = std::chrono::milliseconds(heartbeat_ms);

    pid_ = static_cast<uint64_t>(getpid());
    {
        char buf[256] = {0};
        hostname_ = (gethostname(buf, sizeof(buf) - 1) == 0) ? buf : "unknown";
    }

    {
        const auto now = std::chrono::system_clock::now();
        ts_start_utc_us_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count());
    }

    const std::filesystem::path jsonl_path = output_dir / "progress.jsonl";
    out_.open(jsonl_path, std::ios::app);
    if (!out_.is_open()) {
        simulation::log_warning("ProgressReporter",
            "Failed to open progress file '" + jsonl_path.string() +
            "'. Progress tracking disabled.");
        enabled_ = false;
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    start_wall_ = now;
    last_heartbeat_ = now;

    run_started_ = false;
    run_ended_ = false;
    domain_parent_stack_.clear();
    seq_start_times_.clear();
    seq_count_ = 0;
    enabled_ = true;
}

uint64_t ProgressReporter::wall_us_now_() const {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - start_wall_).count());
}

void ProgressReporter::json_escape_append_(std::string& out, std::string_view in) {
    for (const unsigned char c : in) {
        switch (c) {
            case '"':  out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\b': out.append("\\b");  break;
            case '\f': out.append("\\f");  break;
            case '\n': out.append("\\n");  break;
            case '\r': out.append("\\r");  break;
            case '\t': out.append("\\t");  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out.append(buf);
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
}

void ProgressReporter::json_field_append_(std::string& out, const JsonField& f) {
    out.push_back('"');
    json_escape_append_(out, f.key);
    out.append("\":");
    switch (f.kind) {
        case JsonKind::Str:
            out.push_back('"');
            json_escape_append_(out, f.s);
            out.push_back('"');
            break;
        case JsonKind::U64:
            out.append(std::to_string(f.u));
            break;
        case JsonKind::I64:
            out.append(std::to_string(f.i));
            break;
        case JsonKind::Bool:
            out.append(f.b ? "true" : "false");
            break;
        case JsonKind::Null:
            out.append("null");
            break;
    }
}

void ProgressReporter::write_event_(std::string_view type,
                                    std::string_view domain,
                                    std::initializer_list<JsonField> extras) {
    if (!enabled_) return;

    std::string line;
    line.reserve(256);
    line.push_back('{');

    // Envelope
    line.append("\"schema_version\":");
    line.append(std::to_string(PROGRESS_SCHEMA_VERSION));

    line.append(",\"t\":\"");
    json_escape_append_(line, type);
    line.push_back('"');

    line.append(",\"ts_wall_us\":");
    line.append(std::to_string(wall_us_now_()));

    line.append(",\"ts_sim_ps\":");
    line.append(std::to_string(simulation::current_time_ps));

    line.append(",\"domain\":\"");
    json_escape_append_(line, domain);
    line.push_back('"');

    for (const auto& f : extras) {
        line.push_back(',');
        json_field_append_(line, f);
    }
    line.append("}\n");

    out_ << line;
    out_.flush();
}

void ProgressReporter::run_start() {
    if (!enabled_ || run_started_) return;
    run_started_ = true;
    write_event_("run_start", "default", {
        JsonField{"test_name",       JsonKind::Str, 0, 0, false, test_name_},
        JsonField{"seed",            JsonKind::U64, seed_},
        JsonField{"max_time_ps",     JsonKind::U64, max_time_ps_},
        JsonField{"hostname",        JsonKind::Str, 0, 0, false, hostname_},
        JsonField{"pid",             JsonKind::U64, pid_},
        JsonField{"ts_start_utc_us", JsonKind::U64, ts_start_utc_us_},
    });
}

void ProgressReporter::seq_start(uint64_t seq_id,
                                 std::string_view name,
                                 uint64_t seed,
                                 std::string_view domain) {
    if (!enabled_) return;

    seq_start_times_[seq_id] = SeqStartInfo{wall_us_now_(), simulation::current_time_ps};
    ++seq_count_;

    auto& stack = domain_parent_stack_[std::string(domain)];
    const bool has_parent = !stack.empty();
    const uint64_t parent_id = has_parent ? stack.back() : 0;
    stack.push_back(seq_id);

    JsonField parent_field = has_parent
        ? JsonField{"parent_id", JsonKind::U64, parent_id}
        : JsonField{"parent_id", JsonKind::Null};

    write_event_("seq_start", domain, {
        JsonField{"seq_id", JsonKind::U64, seq_id},
        parent_field,
        JsonField{"name", JsonKind::Str, 0, 0, false, name},
        JsonField{"seed", JsonKind::U64, seed},
    });
}

void ProgressReporter::seq_end(uint64_t seq_id,
                               std::string_view status,
                               std::string_view domain) {
    if (!enabled_) return;

    auto it = domain_parent_stack_.find(std::string(domain));
    if (it != domain_parent_stack_.end() && !it->second.empty()) {
        auto& stack = it->second;
        if (stack.back() == seq_id) {
            stack.pop_back();
        } else {
            // Out-of-order pop: scan-and-erase so we never crash. Nested
            // sequences should terminate in strict LIFO order, so this branch
            // is a safety net for misuse.
            for (auto rit = stack.rbegin(); rit != stack.rend(); ++rit) {
                if (*rit == seq_id) {
                    stack.erase(std::next(rit).base());
                    break;
                }
            }
        }
    }

    uint64_t duration_wall_us = 0;
    uint64_t duration_sim_ps  = 0;
    auto it2 = seq_start_times_.find(seq_id);
    if (it2 != seq_start_times_.end()) {
        duration_wall_us = wall_us_now_() - it2->second.wall_us;
        duration_sim_ps  = simulation::current_time_ps - it2->second.sim_ps;
        seq_start_times_.erase(it2);
    }

    write_event_("seq_end", domain, {
        JsonField{"seq_id",           JsonKind::U64, seq_id},
        JsonField{"status",           JsonKind::Str, 0, 0, false, status},
        JsonField{"duration_wall_us", JsonKind::U64, duration_wall_us},
        JsonField{"duration_sim_ps",  JsonKind::U64, duration_sim_ps},
    });
}

void ProgressReporter::heartbeat(std::string_view domain) {
    if (!enabled_) return;
    const auto now = std::chrono::steady_clock::now();
    if (now - last_heartbeat_ < heartbeat_interval_) return;
    last_heartbeat_ = now;
    write_event_("heartbeat", domain, {
        JsonField{"seq_count", JsonKind::U64, seq_count_},
    });
}

void ProgressReporter::run_end(std::string_view status, std::string_view msg) {
    if (!enabled_ || run_ended_) return;
    run_ended_ = true;
    // Only emit a run_end if we actually emitted run_start. Suppresses orphan
    // events when the TB fails during bootstrap (after configure(), before run()).
    if (run_started_) {
        if (msg.empty()) {
            write_event_("run_end", "default", {
                JsonField{"status",    JsonKind::Str, 0, 0, false, status},
                JsonField{"seq_count", JsonKind::U64, seq_count_},
            });
        } else {
            write_event_("run_end", "default", {
                JsonField{"status",    JsonKind::Str, 0, 0, false, status},
                JsonField{"msg",       JsonKind::Str, 0, 0, false, msg},
                JsonField{"seq_count", JsonKind::U64, seq_count_},
            });
        }
    }
    out_.flush();
    out_.close();
    enabled_ = false;
}

}  // namespace simulation
