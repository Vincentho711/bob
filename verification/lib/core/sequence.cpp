#include "sequence.h"

std::atomic<uint64_t> BaseSequenceIdCounter::counter{0};
