
simulation::Task DualPortRamSequencer::ensure_wr_transactions_done() {
    for (auto &wr_txn_done_event: write_txn_done_events_) {
        co_await wr_txn_done_event;
    }
}
