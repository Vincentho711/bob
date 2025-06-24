#include "adder_scoreboard.h"

AdderScoreboard::AdderScoreboard(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderScoreboardConfig& config, AdderCheckerPtr checker)
    : Base(name, dut, checker, ctx, config), adder_config_(config), ctx_(ctx), checker_(checker)  {
    reset();
    log_info("AdderScoreboard initialised.");
}

void AdderScoreboard::reset() {
    Base::reset();
    log_info("AdderScoreboard reset to default state.");
}

void AdderScoreboard::print_report() const {
    Base::print_report();
}

void AdderScoreboard::print_summary() const {
    Base::print_summary();
}
