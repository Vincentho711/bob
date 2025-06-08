#ifndef ADDER_TRANSACTION_H
#define ADDER_TRANSACTION_H

#include "transaction.h"
#include <random>

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
    AdderTransaction(const std::string& name = "adder_txn");
    AdderTransaction(uint8_t a, uint8_t b, const std::string& name = "adder_txn");

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
    static AdderTransactionPtr create_actual(uint8_t a, uint8_t b, uint16_t result, const std::string& name = "actual_adder_txn");

    // Getters
    uint8_t get_a() const { return a_; }
    uint8_t get_b() const { return b_; }
    uint16_t get_result() const { return result_; }
    void set_result(uint16_t val) { result_ = val; }

    // Validation
    bool is_valid() const;

private:
    uint8_t a_;
    uint8_t b_;
    uint16_t result_; // Used for both expected and actual result

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
