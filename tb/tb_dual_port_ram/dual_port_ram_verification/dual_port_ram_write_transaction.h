#ifndef DUAL_PORT_RAM_WRITE_TRANSACTION_H
#define DUAL_PORT_RAM_WRITE_TRANSACTION_H

#include <cstdint>
#include <optional>
#include <string>
#include "transaction.h"
#include "write_transaction_done_event.h"

class DualPortRamWriteTransaction : public Transaction {
public:
    using DualPortRamWrTxnPtr = std::shared_ptr<DualPortRamWriteTransaction>;

    // Constructors
    DualPortRamWriteTransaction(bool wr_en, uint32_t wr_addr, uint32_t wr_data, const std::string &name = "wr_txn");
    DualPortRamWriteTransaction(bool wr_en, uint32_t wr_addr, uint32_t wr_data, WriteTransactionDoneEvent wr_txn_done_event, const std::string &name = "wr_txn");

private:
    bool wr_en_;
    uint32_t wr_addr_;
    uint32_t wr_data_;
    std::optional<WriteTransactionDoneEvent> wr_txn_done_event_;
    std::string name_;
};

#endif
