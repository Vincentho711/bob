#ifndef DUAL_PORT_RAM_DRIVER_H
#define DUAL_PORT_RAM_DRIVER_H
#include "dual_port_ram_sequencer.h"
#include "dual_port_ram_payload.h"
#include "simulation_clock.h"
#include "simulation_task.h"
#include "driver.h"
#include <coroutine>
#include <memory>
#include <Vdual_port_ram.h>

class DualPortRamDriver : public BaseDriver<DualPortRamPayload, Vdual_port_ram> {
public:
    using clock_t = simulation::Clock<Vdual_port_ram>;
    explicit DualPortRamDriver(std::shared_ptr<DualPortRamSequencer> sequencer, std::shared_ptr<Vdual_port_ram> dut, std::shared_ptr<clock_t> wr_clk);

    simulation::Task run() override;

    simulation::Task wr_driver_run();
    simulation::Task rd_driver_run();

private:
    std::shared_ptr<DualPortRamSequencer> p_sequencer;
    std::shared_ptr<clock_t> wr_clk;
};
#endif // DUAL_PORT_RAM_DRIVER_H
