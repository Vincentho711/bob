#include "dual_port_ram_write_transaction.h"

DualPortRamWriteTransaction::DualPortRamWriteTransaction(const std::string &name, bool wr_en, uint32_t wr_addr, uint32_t wr_data)
    : name_(name), wr_en_(wr_en), wr_addr_(wr_addr), wr_data_(wr_data), wr_txn_done_event_(std::nullopt){}

DualPortRamWriteTransaction::DualPortRamWriteTransaction(const std::string &name, bool wr_en, uint32_t wr_addr, uint32_t wr_data, WriteTransactionDoneEvent wr_txn_done_event)
    : name_(name), wr_en_(wr_en), wr_addr_(wr_addr), wr_data_(wr_data), wr_txn_done_event_(wr_txn_done_event){}
