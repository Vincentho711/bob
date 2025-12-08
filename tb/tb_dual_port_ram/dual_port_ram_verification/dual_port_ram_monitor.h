#ifndef DUAL_PORT_RAM_MONITOR_H
#define DUAL_PORT_RAM_MONITOR_H
#include "Vdual_port_ram.h"
#include "dual_port_ram_tlm_queue.h"
#include "dual_port_ram_transaction.h"
#include "simulation_clock.h"
#include "monitor.h"
#include <memory>

class DualPortRamMonitor
    : public BaseMonitor<DualPortRamTransaction, Vdual_port_ram> {
public:
  using ClockT = simulation::Clock<Vdual_port_ram>;
  using TxnPtr = std::shared_ptr<DualPortRamTransaction>;
  explicit DualPortRamMonitor(
      std::shared_ptr<Vdual_port_ram> dut,
      std::shared_ptr<ClockT> wr_clk,
      std::shared_ptr<ClockT> rd_clk,
      std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue,
      std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue,
      const std::string &name = "DualPortRamMonitor",
      bool debug_enabled = true
  );

  simulation::Task run() override;

  simulation::Task wr_port_run();

  simulation::Task rd_port_run();

  simulation::Task put_wr_transaction(TxnPtr wr_txn);

  simulation::Task put_rd_transaction(TxnPtr rd_txn);

private:
    std::string name_;
    std::shared_ptr<Vdual_port_ram> dut_;
    std::shared_ptr<ClockT> wr_clk_;
    std::shared_ptr<ClockT> rd_clk_;
    std::shared_ptr<DualPortRamTLMWrQueue> tlm_wr_queue_;
    std::shared_ptr<DualPortRamTLMRdQueue> tlm_rd_queue_;
};

#endif // DUAL_PORT_RAM_MONITOR_H
