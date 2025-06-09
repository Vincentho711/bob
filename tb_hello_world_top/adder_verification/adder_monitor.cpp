#include "adder_monitor.h"
#include "adder_transaction.h"

// Constructor with default configuration
AdderMonitor::AdderMonitor(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx)
    : Base(name, dut, ctx), ctx_(ctx) {
    reset();
    log_info("AdderMonitor initialised.");
}

AdderMonitor::AdderTransactionPtr AdderMonitor::sample_input() {
    uint64_t current_cycle = ctx_->current_cycle();
    auto dut = get_dut();
    uint8_t a = dut->a_i;
    uint8_t b = dut->b_i;

    auto txn = AdderTransaction::create_expected(a, b, current_cycle, "monitored_adder_txn");
    log_debug("Sampled DUT input -> a:" + std::to_string(a) + ", b:" + std::to_string(b));
    return txn;
}

AdderMonitor::AdderTransactionPtr AdderMonitor::sample_output() {
    uint64_t current_cycle = ctx_->current_cycle();
    auto dut = get_dut();
    uint16_t c = dut->c_o;

    auto txn = AdderTransaction::create_actual(c, current_cycle, "monitored_adder_txn");
    log_debug("Sampled DUT output -> c:" + std::to_string(c));
    return txn;
}

void AdderMonitor::reset() {
    Base::reset();
    log_info("AdderMonitor reset to default state.");
}
