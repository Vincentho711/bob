#ifndef ADDER_TRANSACTION_H
#define ADDER_TRANSACTION_H

#include "transaction.h"
#include <random>
#include <optional>

/**
 * Adder-specific transaction class for RTL adder verification
 * Inherits from the UVM-style Transaction base class
 */
class AdderTransaction : public Transaction {
public:
    using AdderTransactionPtr = std::shared_ptr<AdderTransaction>;

    // Corner case enumeration for directed testing (moved before methods that use it)
    enum class CornerCase {
        MIN_MIN,        // 0 + 0
        MAX_MAX,        // 255 + 255
        MIN_MAX,        // 0 + 255
        MAX_MIN,        // 255 + 0
        MID_MID,        // 128 + 128
        SMALL_SMALL,    // 1 + 1
        NEAR_OVERFLOW,  // 254 + 1
        OVERFLOW_BOUNDARY, // 255 + 1
        AROUND_MID_1,   // 127 + 128
        AROUND_MID_2,   // 128 + 127
        NEAR_MAX       // 254 + 254
    };

    // Constructors
    AdderTransaction(Kind kind, const std::string& name = "adder_txn");
    AdderTransaction(uint8_t a, uint8_t b, const std::string& name = "adder_expected_txn");
    AdderTransaction(uint8_t a, uint8_t b, uint64_t cycle, const std::string& name = "adder_expected_txn");
    AdderTransaction(uint16_t result, uint64_t cycle, const std::string& name = "adder_actual_txn");

    // Virtual destructor
    virtual ~AdderTransaction() = default;

    // UVM-style methods implementation
    std::shared_ptr<Transaction> clone() const override;
    void copy(const Transaction& other) override;
    bool compare(const Transaction& other) const override;
    std::string convert2string() const override;
    std::string get_type_name() const override;

    // Adder-specific methods
    void randomize(std::mt19937& rng);
    void set_inputs(uint8_t a, uint8_t b);
    void set_corner_case(CornerCase case_type);
    void set_result(uint16_t result);
    void set_cycle(uint64_t cycle);
    static AdderTransactionPtr create_expected(uint8_t a, uint8_t b, uint64_t driven_cycle, const std::string& name = "expected_adder_txn");
    static AdderTransactionPtr create_actual(uint16_t c, uint64_t captured_cycle, const std::string& name = "actual_adder_txn");

    // Getters
    uint8_t get_a() const { if (!a_.has_value()) throw std::logic_error("a_ is not set."); return a_.value(); }
    uint8_t get_b() const { if (!b_.has_value()) throw std::logic_error("b_ is not set."); return b_.value(); }
    uint16_t get_result() const { if (!result_.has_value()) throw std::logic_error("result_ is not set."); return result_.value(); }
    uint64_t get_cycle() const { return cycle_; }

    // Validation
    bool is_valid() const;

private:
    std::optional<uint8_t> a_;
    std::optional<uint8_t> b_;
    std::optional<uint16_t> result_;
    uint64_t cycle_; // For expected transaction, store the cycle when transaction is driven into DUT. For actual, store the cycle when it comes out of the DUT.

    void calculate_expected();
    void set_corner_case_values(CornerCase case_type);
};

/**
 * Factory class for creating AdderTransaction objects
 * Provides convenient methods for different transaction types
 */
class AdderTransactionFactory {
public:
    static std::shared_ptr<AdderTransaction> create_random(std::mt19937& rng, const std::string& name = "random_adder_txn");
    static std::shared_ptr<AdderTransaction> create_corner_case(AdderTransaction::CornerCase case_type, const std::string& name = "corner_case_adder_txn");
    static std::shared_ptr<AdderTransaction> create_directed(uint8_t a, uint8_t b, const std::string& name = "directed_adder_txn");

    // Get all corner cases for comprehensive testing
    static std::vector<AdderTransaction::CornerCase> get_all_corner_cases();
};

#endif
