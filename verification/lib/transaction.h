#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <memory>
#include <sstream>
#include <typeinfo>
#include <iostream>

/**
 * Base transaction class following UVM-style patterns
 * Provides core functionality for all transaction types
 */
class Transaction {
public:
    // Enum to indicate whether a transaction is an expected or actual transaction
    enum class Kind { Expected, Actual };

    // Constructor
    Transaction(Kind kind, const std::string& name = "transaction");

    // Virtual destructor for proper inheritance
    virtual ~Transaction() = default;

    // Copy constructor and assignment operator
    Transaction(const Transaction& other);
    Transaction& operator=(const Transaction& other);

    // Move constructor and assignment operator
    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;

    // Core UVM-style methods
    virtual std::shared_ptr<Transaction> clone() const = 0;
    virtual void copy(const Transaction& other);
    virtual bool compare(const Transaction& other) const;
    virtual std::string convert2string() const;
    virtual void print(std::ostream& os = std::cout) const;

    // Getters and setters
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    std::uint64_t get_transaction_id() const { return transaction_id_; }

    Kind kind() const { return kind_; }

    // Utility methods
    virtual std::string get_type_name() const;
    bool is_expected() const { return kind_ == Kind::Expected; }
    bool is_actual() const { return kind_ == Kind::Actual; }

    // Stream operator
    friend std::ostream& operator<<(std::ostream& os, const Transaction& txn);

protected:
    std::string name_;
    Kind kind_;
    std::uint64_t transaction_id_;

private:
    static std::uint64_t next_transaction_id_;

    void assign_transaction_id();
};

#endif
