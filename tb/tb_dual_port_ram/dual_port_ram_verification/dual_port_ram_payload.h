#ifndef DUAL_PORT_RAM_PAYLOAD_H
#define DUAL_PORT_RAM_PAYLOAD_H
#include <cstdint>

struct DualPortRamPayload {
    enum Type { Read, Write } type;
    uint32_t addr;
    uint32_t data;

    void reset() {
        type = Read;
        addr = 0U;
        data = 0U;
    }
};
#endif
