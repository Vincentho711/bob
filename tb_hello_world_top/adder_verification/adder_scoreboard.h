#ifndef ADDER_SCOREBOARD_H
#define ADDER_SCOREBOARD_H

#include "scoreboard.h"
#include "adder_transaction.h"
#include "adder_simulation_context.h"
#include "covergroup.h"
#include "Vhello_world_top.h"
#include <memory>

/**
 * @class AdderScoreboardConfig
 * @brief Configuration for a AdderScoreboard object
 *
 */
struct AdderScoreboardConfig : public ScoreboardConfig {
    bool enable_overflow_check = true;

    AdderScoreboardConfig() {
        log_level = ScoreboardLogLevel::DEBUG; // Overrides base default
    }

    explicit AdderScoreboardConfig(const ScoreboardConfig &base)
        : ScoreboardConfig(base) {
        log_level = ScoreboardLogLevel::DEBUG;
    }
};

struct AdderScoreboardStats : public ScoreboardStats {
    Covergroup cg;

    AdderScoreboardStats() {
        cg.add_coverpoint("sum_range", 512);
        cg.add_coverpoint("operand_a_range", 256);
        cg.add_coverpoint("operand_b_range", 256);
        // Add cross coverage
        cg.add_cross("operand_a_x_operand_b", {"operand_a_range", "operand_b_range"});
    }

    void sample(uint8_t a, uint8_t b, uint32_t sum) {
        cg.sample("sum_range", sum);
        cg.sample("operand_a_range", static_cast<uint32_t>(a));
        cg.sample("operand_b_range", static_cast<uint32_t>(b));
        cg.sample_cross("operand_a_x_operand_b", {static_cast<uint32_t>(a), static_cast<uint32_t>(b)});
    }

    void report_coverage() const {
        cg.report();
    }
};

class AdderScoreboard : public BaseScoreboard<Vhello_world_top, AdderTransaction> {
public:
    using Base = BaseScoreboard<Vhello_world_top, AdderTransaction>;
    using DutPtr = Base::DutPtr;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;

    explicit AdderScoreboard(const std::string& name, DutPtr dut,
                             AdderScoreboardConfig config, AdderSimulationContextPtr ctx, AdderCheckerPtr checker);
};
#endif
