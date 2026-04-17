#include "simulation_args_progress_argument_group.h"

void simulation::args::ProgressArgumentGroup::register_args(GroupApp& app) {
    app.add_flag("enabled", enabled_,
        "Enable per-run progress tracking (writes JSON-lines to <dir>/<run_id>/progress.jsonl).",
        enabled_);
    app.add_argument<std::string>("dir", dir_,
        "Base directory for per-run progress artefacts. A subdirectory named "
        "<UTC_ts>_<test>_<seedhex> is created beneath it.",
        dir_);
    app.add_argument<uint32_t>("heartbeat-ms", heartbeat_ms_,
        "Minimum wall-clock interval (ms) between heartbeat events.",
        heartbeat_ms_);
    app.add_argument<std::string>("batch-id", batch_id_,
        "Optional identifier grouping multiple runs into one batch (e.g. a CI job ID). "
        "Passed verbatim into every run_start event.",
        batch_id_);
}

[[nodiscard]] std::string_view simulation::args::ProgressArgumentGroup::description() const {
    return "Progress tracking arguments (automatic per-test-case event stream)";
}
