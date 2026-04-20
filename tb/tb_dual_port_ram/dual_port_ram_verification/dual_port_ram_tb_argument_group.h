#ifndef DUAL_PORT_RAM_TB_ARGUMENT_GROUP_H
#define DUAL_PORT_RAM_TB_ARGUMENT_GROUP_H
#include <string_view>
#include <vector>
#include <string>
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include "simulation_args_argument_parser.h"

class DualPortRamArgumentGroup final : public simulation::args::ArgumentGroup {
public:
    explicit DualPortRamArgumentGroup(std::vector<std::string> available_tests)
        : simulation::args::ArgumentGroup("tb"), available_test_(std::move(available_tests)) {}
    void register_args(simulation::args::GroupApp& app) override;
    [[nodiscard]] std::string_view description() const override;
    [[nodiscard]] std::string test_name() const noexcept { return test_name_; }
private:
    std::string test_name_ = "default";
    std::vector<std::string> available_test_;
};
#endif
