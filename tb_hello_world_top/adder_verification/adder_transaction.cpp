#include "adder_transaction.h"
#include <sstream>
#include <stdexcept>

// ============================================================================
// AdderTransaction Implementation
// ============================================================================

AdderTransaction::AdderTransaction(const std::string& name) 
    : Transaction(name), a_(0), b_(0), result_(0) {
    calculate_expected();
}

AdderTransaction::AdderTransaction(uint8_t a, uint8_t b, const std::string& name) 
    : Transaction(name), a_(a), b_(b), result_(0) {
    calculate_expected();
}

std::shared_ptr<Transaction> AdderTransaction::clone() const {
    return std::make_shared<AdderTransaction>(*this);
}

void AdderTransaction::copy(const Transaction& other) {
    Transaction::copy(other);

    const AdderTransaction* adder_txn = dynamic_cast<const AdderTransaction*>(&other);
    if (adder_txn != nullptr) {
        a_ = adder_txn->a_;
        b_ = adder_txn->b_;
        result_ = adder_txn->result_;
    }
}

bool AdderTransaction::compare(const Transaction& other) const {
    if (!Transaction::compare(other)) {
        return false;
    }
    
    const AdderTransaction* adder_txn = dynamic_cast<const AdderTransaction*>(&other);
    if (adder_txn == nullptr) {
        return false;
    }
    
    return (a_ == adder_txn->a_ && 
            b_ == adder_txn->b_ && 
            result_ == adder_txn->result_);
}

std::string AdderTransaction::convert2string() const {
    std::ostringstream oss;
    oss << get_type_name() << " [" << get_name() << "] (ID: " << get_transaction_id() << ")"
        << " - a=" << +a_ << ", b=" << +b_ << ", result=" << result_;
    return oss.str();
}

std::string AdderTransaction::get_type_name() const {
    return "AdderTransaction";
}

void AdderTransaction::randomize(std::mt19937& rng) {
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    a_ = dist(rng);
    b_ = dist(rng);
    calculate_expected();
}

void AdderTransaction::set_inputs(uint8_t a, uint8_t b) {
    a_ = a;
    b_ = b;
    calculate_expected();
}

void AdderTransaction::set_corner_case(CornerCase case_type) {
    set_corner_case_values(case_type);
    calculate_expected();
}

AdderTransaction::AdderTransactionPtr AdderTransaction::create_actual(uint8_t a, uint8_t b, uint16_t result, const std::string& name) {
    auto txn = std::make_shared<AdderTransaction>(name);
    txn->set_inputs(a, b);
    txn->set_result(result);
    return txn;
}

bool AdderTransaction::is_valid() const {
    // For adder, all 8-bit input combinations are valid
    return a_ <= 255 && b_ <= 255;
}

void AdderTransaction::calculate_expected() {
    result_ = static_cast<uint16_t>(a_) + static_cast<uint16_t>(b_);
}

void AdderTransaction::set_corner_case_values(CornerCase case_type) {
    switch (case_type) {
        case CornerCase::MIN_MIN:
            a_ = 0; b_ = 0;
            break;
        case CornerCase::MAX_MAX:
            a_ = 255; b_ = 255;
            break;
        case CornerCase::MIN_MAX:
            a_ = 0; b_ = 255;
            break;
        case CornerCase::MAX_MIN:
            a_ = 255; b_ = 0;
            break;
        case CornerCase::MID_MID:
            a_ = 128; b_ = 128;
            break;
        case CornerCase::SMALL_SMALL:
            a_ = 1; b_ = 1;
            break;
        case CornerCase::NEAR_OVERFLOW:
            a_ = 254; b_ = 1;
            break;
        case CornerCase::OVERFLOW_BOUNDARY:
            a_ = 255; b_ = 1;
            break;
        case CornerCase::AROUND_MID_1:
            a_ = 127; b_ = 128;
            break;
        case CornerCase::AROUND_MID_2:
            a_ = 128; b_ = 127;
            break;
        case CornerCase::NEAR_MAX:
            a_ = 254; b_ = 254;
            break;
        default:
            throw std::invalid_argument("Unknown corner case type");
    }
}

// ============================================================================
// AdderTransactionFactory Implementation
// ============================================================================

std::shared_ptr<AdderTransaction> AdderTransactionFactory::create_random(std::mt19937& rng, const std::string& name) {
    auto txn = std::make_shared<AdderTransaction>(name);
    txn->randomize(rng);
    return txn;
}

std::shared_ptr<AdderTransaction> AdderTransactionFactory::create_corner_case(AdderTransaction::CornerCase case_type, const std::string& name) {
    auto txn = std::make_shared<AdderTransaction>(name);
    txn->set_corner_case(case_type);
    return txn;
}

std::shared_ptr<AdderTransaction> AdderTransactionFactory::create_directed(uint8_t a, uint8_t b, const std::string& name) {
    return std::make_shared<AdderTransaction>(a, b, name);
}

std::vector<AdderTransaction::CornerCase> AdderTransactionFactory::get_all_corner_cases() {
    return {
        AdderTransaction::CornerCase::MIN_MIN,
        AdderTransaction::CornerCase::MAX_MAX,
        AdderTransaction::CornerCase::MIN_MAX,
        AdderTransaction::CornerCase::MAX_MIN,
        AdderTransaction::CornerCase::MID_MID,
        AdderTransaction::CornerCase::SMALL_SMALL,
        AdderTransaction::CornerCase::NEAR_OVERFLOW,
        AdderTransaction::CornerCase::OVERFLOW_BOUNDARY,
        AdderTransaction::CornerCase::AROUND_MID_1,
        AdderTransaction::CornerCase::AROUND_MID_2,
        AdderTransaction::CornerCase::NEAR_MAX
    };
}
