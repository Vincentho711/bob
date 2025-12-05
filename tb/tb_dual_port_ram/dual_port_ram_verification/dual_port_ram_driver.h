#ifndef DUAL_PORT_RAM_DRIVER_H
#define DUAL_PORT_RAM_DRIVER_H
#include "driver.h"
#include "dual_port_ram_payload.h"
#include "dual_port_ram_sequencer.h"
#include "simulation_clock.h"
#include "simulation_task_symmetric_transfer.h"
#include <Vdual_port_ram.h>
#include <coroutine>
#include <memory>

class DualPortRamDriver
    : public BaseDriver<DualPortRamSequencer, Vdual_port_ram> {
public:
    using clock_t = simulation::Clock<Vdual_port_ram>;
    explicit DualPortRamDriver(std::shared_ptr<DualPortRamSequencer> sequencer,
                             std::shared_ptr<Vdual_port_ram> dut,
                             std::shared_ptr<clock_t> wr_clk,
                             const std::string &name = "DualPortRamDriver",
                             bool debug_enabled = true);

    simulation::Task run() override;
    simulation::Task wr_driver_run();
    simulation::Task rd_driver_run();

private:
    std::shared_ptr<clock_t> wr_clk;
};
#endif // DUAL_PORT_RAM_DRIVER_H
