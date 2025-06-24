#include "scoreboard.h"
#include "adder_transaction.h"
#include "adder_checker.h"
#include "adder_simulation_context.h"
#include "Vhello_world_top.h"

struct AdderScoreboardConfig : public ScoreboardConfig {
    uint32_t pipeline_depth = 2;
};

class AdderScoreboard : public BaseScoreboard<Vhello_world_top, AdderTransaction> {
public:
    using DutPtr = std::shared_ptr<Vhello_world_top>;
    using Base = BaseScoreboard<Vhello_world_top, AdderTransaction>;
    using AdderTransactionPtr = std::shared_ptr<AdderTransaction>;
    using AdderCheckerPtr = std::shared_ptr<AdderChecker<Vhello_world_top>>;
    using AdderSimulationContextPtr = std::shared_ptr<AdderSimulationContext>;

    /**
     * @brief Construct AdderScoreboard
     * 
     * @param name Baseboard instance name
     * @param dut Shared pointer to DUT
     * @param ctx Adder-specific simulation context for tracking global state
     * @param config Adder-specific configuration
     * @param checker Shared pointer to checker object
     */
    AdderScoreboard(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderScoreboardConfig& config, AdderCheckerPtr checker);

    /**
      * @brief Destructor - prints final report
      */
    virtual ~AdderScoreboard();

    void push_expected(AdderTransactionPtr txn);
    void push_actual(AdderTransactionPtr txn);
    bool validate_scoreboard_config(const AdderScoreboardConfig& config, std::string& error_msg);
    void print_report() const override;
    void print_summary() const override;

protected:
    AdderScoreboardConfig adder_config_;
    void reset() override;

private:
    AdderSimulationContextPtr ctx_;
    AdderCheckerPtr checker_;
};
