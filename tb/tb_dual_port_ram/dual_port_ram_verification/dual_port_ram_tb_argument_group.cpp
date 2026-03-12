#include "dual_port_ram_tb_argument_group.h"
#include "simulation_logging_utils.h"
#include "simulation_args_core_argument_group.h"

void DualPortRamArgumentGroup::register_args(simulation::args::GroupApp& app) {
    app.add_argument<std::string>("trace-file", trace_file_, "VCD waveform output filename.", std::string{"tb_dual_port_ram.vcd"});
}

std::string_view DualPortRamArgumentGroup::description() const {
    return "Dual-port RAM testbench specific arguments";
}

void DualPortRamArgumentGroup::cross_validate(const simulation::args::SimulationArgumentParser& parser) {
    const auto* desc = parser.registry().find("tb.trace-file");
    const bool trace_explicitly_set = desc && desc->source != simulation::args::ArgumentSource::Default;
    const bool waves_enabled = parser.get<simulation::args::CoreArgumentGroup>().waves();

    if (trace_explicitly_set && !waves_enabled) {
        waves_implied_ = true;
        simulation::log_info("DualPortRamArgs", "--tb.trace-file specified without --waves; waveform capture implicitly enabled.");
    }
}
