#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "simulation_context.h"
#include "transaction.h"
#include "checker.h"
#include <iostream>

/**
 * @brief Enumeration for scoreboard log levels
 */
enum class ScoreboardLogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Configuration structure for checker behavior
 */
struct ScoreboardConfig {
    uint32_t max_latency_cycles = 10;
    bool enable_out_of_order_matching = false;
    bool stop_on_first_error = true;
    bool use_chceker = true;
    ScoreboardLogLevel log_level = ScoreboardLogLevel::INFO;

    ScoreboardConfig() = default;
};

/**
 * @brief Base statistics structure for scoreboard
 */
struct ScoreboardStats {
    uint64_t total_expected = 0;
    uint64_t total_actual = 0;
    uint64_t matched = 0;
    uint64_t mismatch = 0;
    uint64_t timed_out = 0;

    ScoreboardStats() = default;
};

template<typename DUT_TYPE, typename TRANSACTION_TYPE>
class BaseScoreboard {
public:
  using DutPtr = std::shared_ptr<DUT_TYPE>;
  using TransactionPtr = std::shared_ptr<TRANSACTION_TYPE>;
  using SimulationContextPtr = std::shared_ptr<SimulationContext>;
  using CheckerPtr = std::shared_ptr<BaseChecker<DUT_TYPE, TRANSACTION_TYPE>>;

  /**
    * @brief Structure to hold pending transactions with metadata
    */
  struct ExpectedTransaction {
      TransactionPtr transaction;
      uint64_t expected_cycle;
      uint64_t submitted_cycle;

      ExpectedTransaction(TransactionPtr txn, uint64_t exp_cycle, uint64_t sub_cycle)
          : transaction(txn), expected_cycle(exp_cycle), submitted_cycle(sub_cycle) {}
  };

protected:
    std::string name_;
    DutPtr dut_;
    ScoreboardConfig config_;
    ScoreboardStats stats_;
    SimulationContextPtr ctx_;
    std::queue<ExpectedTransaction> expected_transactions_;
};

