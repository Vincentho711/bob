#include "dual_port_ram_tb_argument_group.h"

void DualPortRamArgumentGroup::register_args(simulation::args::GroupApp& app) {
    app.add_enum_argument("test", test_name_, "Test to run", available_test_, "default");
}

std::string_view DualPortRamArgumentGroup::description() const {
    return "Dual-port RAM testbench specific arguments";
}
