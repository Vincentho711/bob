#ifndef DUAL_PORT_RAM_TB_ARGUMENT_GROUP_H
#define DUAL_PORT_RAM_TB_ARGUMENT_GROUP_H
#include <string_view>
#include <string>
#include "simulation_args_argument_group.h"
#include "simulation_args_group_app.h"
#include "simulation_args_argument_parser.h"

class DualPortRamArgumentGroup final : public simulation::args::ArgumentGroup {
public:
    DualPortRamArgumentGroup() : simulation::args::ArgumentGroup("tb") {}
    void register_args(simulation::args::GroupApp& app) override;
    void cross_validate(const simulation::args::SimulationArgumentParser& parser);
    [[nodiscard]] std::string_view description() const override;
    [[nodiscard]] std::string_view trace_file() const noexcept { return trace_file_; }
    [[nodiscard]] bool waves_implied() const noexcept { return waves_implied_; }
private:
    std::string trace_file_ = "tb_dual_port_ram.vcd";
    bool waves_implied_ = false;
};
#endif
