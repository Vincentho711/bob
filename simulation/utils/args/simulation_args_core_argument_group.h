#ifndef SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H
#define SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include "simulation_logging_utils.h"
#include <string>

namespace simulation::args {

class CoreArgumentGroup final : public ArgumentGroup {
public:
    CoreArgumentGroup() : ArgumentGroup("") {} // empty_prefix = top-level namespace
    void register_args(GroupApp& app) override;
    void post_parse_resolve() override;
    [[nodiscard]] std::string_view description() const override;

    [[nodiscard]] uint64_t seed() const noexcept { return seed_; }
    [[nodiscard]] bool waves() const noexcept { return waves_; }
    [[nodiscard]] bool dry_run() const noexcept { return dry_run_; }
    [[nodiscard]] std::string_view verbosity_str() const noexcept { return verbosity_str_; }
    [[nodiscard]] std::string_view logfile_path_str() const noexcept { return logfile_path_str_; }

private:
    uint64_t seed_ = 1;
    std::string verbosity_str_;
    bool waves_ = false;
    bool dry_run_ = false;
    std::string logfile_path_str_;

};
}
#endif // SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H

