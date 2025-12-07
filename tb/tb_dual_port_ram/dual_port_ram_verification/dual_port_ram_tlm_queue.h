#ifndef DUAL_PORT_RAM_TLM_QUEUE_H
#define DUAL_PORT_RAM_TLM_QUEUE_H

#include "simulation_tlm_queue.h"
#include "dual_port_ram_transaction.h"
#include <string>

class DualPortRamTLMWrQueue : public simulation::TLMQueue<DualPortRamTransaction> {
public:
    explicit DualPortRamTLMWrQueue(const std::string &name = "DualPortRamTLMWrQueue");
};

class DualPortRamTLMRdQueue : public simulation::TLMQueue<DualPortRamTransaction> {
public:
    explicit DualPortRamTLMRdQueue(const std::string &name = "DualPortRamTLMRdQueue");
};
#endif // DUAL_PORT_RAM_TLM_QUEUE_H
