#include "transaction.h"
#include <iostream>
#include <iomanip>

// Static member initialization
std::uint64_t Transaction::next_transaction_id_ = 1;

Transaction::Transaction(const std::string& name) 
    : name_(name) {
    assign_transaction_id();
}

Transaction::Transaction(const Transaction& other) 
    : name_(other.name_) {
    assign_transaction_id();
}

Transaction& Transaction::operator=(const Transaction& other) {
    if (this != &other) {
        name_ = other.name_;
        // Note: transaction_id_ remains unchanged during assignment
    }
    return *this;
}

Transaction::Transaction(Transaction&& other) noexcept 
    : name_(std::move(other.name_)), 
      transaction_id_(other.transaction_id_) {
    other.transaction_id_ = 0;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        transaction_id_ = other.transaction_id_;
        other.transaction_id_ = 0;
    }
    return *this;
}

void Transaction::copy(const Transaction& other) {
    if (this != &other) {
        name_ = other.name_;
        // transaction_id_ remains unchanged
    }
}

bool Transaction::compare(const Transaction& other) const {
    return (typeid(*this) == typeid(other)) && (name_ == other.name_);
}

std::string Transaction::convert2string() const {
    std::ostringstream oss;
    oss << get_type_name() << " [" << name_ << "] (ID: " << transaction_id_ << ")";
    return oss.str();
}

void Transaction::print(std::ostream& os) const {
    os << convert2string();
}

std::string Transaction::get_type_name() const {
    std::string type_name = typeid(*this).name();
    // Simple demangling attempt (compiler-specific)
    #ifdef __GNUG__
        // For GCC, you might want to use abi::__cxa_demangle here
        // For simplicity, we'll just return the mangled name
    #endif
    return type_name;
}

void Transaction::assign_transaction_id() {
    transaction_id_ = next_transaction_id_++;
}

std::ostream& operator<<(std::ostream& os, const Transaction& txn) {
    txn.print(os);
    return os;
}
