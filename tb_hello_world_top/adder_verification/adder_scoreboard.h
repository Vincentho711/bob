#include "scoreboard.h"
#include "adder_transaction.h"
#include "adder_checker.h"
#include "adder_simulation_context.h"

struct AdderScoreboardConfig : public ScoreboardConfig {
    uint32_t pipeline_depth = 2;
};

template<DUT_TYPE, AdderTransaction> 
class AdderScoreboard : public BaseScoreboard<DUT_TYPE, AdderTransaction> {
public:
    using DutPtr = std::shared_ptr<DUT_TYPE>;
    using Base = BaseChecker<DUT_TYPE, AdderTransaction>;
    using TransactionPtr = std::shared_ptr<AdderTransaction>;
    using CheckerPtr = std::shared_ptr<AdderChecker>;
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
    AdderScoreboard(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const AdderScoreboardConfig& config, CheckerPtr checker);

    /**
      * @brief Destructor - prints final report
      */
    virtual ~AdderScoreboard();

    void push_expected(TransactionPtr txn);
    void push_actual(TransactionPtr txn);
    bool validate_scoreboard_config(const AdderScoreboardConfig& config, std::string& error_msg);
    void print_report() const;
    void print_summary() const;

protected:
    AdderScoreboardConfig adder_config_;
};
