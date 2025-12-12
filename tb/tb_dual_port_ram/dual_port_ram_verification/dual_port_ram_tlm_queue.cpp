#include "dual_port_ram_tlm_queue.h"

DualPortRamTLMWrQueue::DualPortRamTLMWrQueue(const std::string &name)
    : simulation::TLMQueue<DualPortRamTransaction>(name) {}

DualPortRamTLMRdQueue::DualPortRamTLMRdQueue(const std::string &name)
    : simulation::TLMQueue<DualPortRamTransaction>(name) {}
