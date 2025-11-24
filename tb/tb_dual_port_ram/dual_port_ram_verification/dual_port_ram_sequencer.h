#ifndef DUAL_PORT_RAM_SEQUENCER_H
#define DUAL_PORT_RAM_SEQUENCER_H
#include "sequencer.h"

class DualPortRamSequencer : public BaseSequencer<Vdual_port_ram> {
public:
    using DutPtr = std::shared_ptr<Vdual_port_ram>;
}

#endif // DUAL_PORT_RAM_SEQUENCER_H
