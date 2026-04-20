#ifndef SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
#define SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include <cstdint>

namespace simulation::args {

struct ProgressArgumentDefaults {
    uint32_t heartbeat_ms = 500;
};

class ProgressArgumentGroup final : public ArgumentGroup {
public:
    explicit ProgressArgumentGroup(ProgressArgumentDefaults defaults = {})
        : ArgumentGroup("progress"),
          heartbeat_ms_(defaults.heartbeat_ms) {}

    void register_args(GroupApp& app) override;
    [[nodiscard]] std::string_view description() const override;

    [[nodiscard]] uint32_t heartbeat_ms() const noexcept { return heartbeat_ms_; }

private:
    uint32_t heartbeat_ms_;
};

}  // namespace simulation::args

#endif  // SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
