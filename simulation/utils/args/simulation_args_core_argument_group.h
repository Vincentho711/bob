#ifndef SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H
#define SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include "simulation_logging_utils.h"
#include <string>

namespace simulation::args {

struct CoreArgumentDefaults {
    uint64_t    max_time_ps = 0;
    uint64_t    seed        = 1;
    std::string verbosity   = "info";
};

class CoreArgumentGroup final : public ArgumentGroup {
public:
    explicit CoreArgumentGroup(std::string binary_name, CoreArgumentDefaults defaults = {})
        : ArgumentGroup(""), // empty_prefix = top-level namespace
          binary_name_  (std::move(binary_name)),
          seed_         (defaults.seed),
          verbosity_str_(defaults.verbosity),
          max_time_ps_  (defaults.max_time_ps) {}

    void register_args(GroupApp& app) override;
    void post_parse_resolve() override;
    [[nodiscard]] std::string_view description() const override;

    [[nodiscard]] uint64_t seed() const noexcept { return seed_; }
    [[nodiscard]] bool waves() const noexcept { return waves_; }
    [[nodiscard]] bool dry_run() const noexcept { return dry_run_; }
    [[nodiscard]] std::string_view verbosity_str() const noexcept { return verbosity_str_; }
    [[nodiscard]] std::string_view output_dir() const noexcept { return output_dir_str_; }
    [[nodiscard]] std::string_view binary_name() const noexcept { return binary_name_; }
    [[nodiscard]] uint64_t max_time_ps() const noexcept { return max_time_ps_; }

private:
    std::string binary_name_;
    uint64_t seed_ = 1;
    std::string verbosity_str_;
    bool waves_ = false;
    bool dry_run_ = false;
    std::string output_dir_str_;
    uint64_t max_time_ps_;

};
}
#endif // SIMULATION_ARGS_CORE_ARGUMENT_GROUP_H

