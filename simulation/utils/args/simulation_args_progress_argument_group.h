#ifndef SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
#define SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include <cstdint>
#include <string>

namespace simulation::args {

struct ProgressArgumentDefaults {
    bool        enabled      = true;
    std::string dir          = "runs";
    uint32_t    heartbeat_ms = 500;
    std::string batch_id     = "";
};

class ProgressArgumentGroup final : public ArgumentGroup {
public:
    explicit ProgressArgumentGroup(ProgressArgumentDefaults defaults = {})
        : ArgumentGroup("progress"),
          enabled_(defaults.enabled),
          dir_(defaults.dir),
          heartbeat_ms_(defaults.heartbeat_ms),
          batch_id_(defaults.batch_id) {}

    void register_args(GroupApp& app) override;
    [[nodiscard]] std::string_view description() const override;

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    [[nodiscard]] std::string_view dir() const noexcept { return dir_; }
    [[nodiscard]] uint32_t heartbeat_ms() const noexcept { return heartbeat_ms_; }
    [[nodiscard]] std::string_view batch_id() const noexcept { return batch_id_; }

private:
    bool enabled_;
    std::string dir_;
    uint32_t heartbeat_ms_;
    std::string batch_id_;
};

}  // namespace simulation::args

#endif  // SIMULATION_ARGS_PROGRESS_ARGUMENT_GROUP_H
