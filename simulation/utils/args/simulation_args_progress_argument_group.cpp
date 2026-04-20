#include "simulation_args_progress_argument_group.h"

void simulation::args::ProgressArgumentGroup::register_args(GroupApp& app) {
    app.add_argument<uint32_t>("heartbeat-ms", heartbeat_ms_,
        "Minimum wall-clock interval (ms) between heartbeat events.",
        heartbeat_ms_);
}

[[nodiscard]] std::string_view simulation::args::ProgressArgumentGroup::description() const {
    return "Progress tracking arguments (automatic per-test-case event stream)";
}
