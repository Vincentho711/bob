#ifndef DUAL_PORT_RAM_TRANSACTION_H
#define DUAL_PORT_RAM_TRANSACTION_H
#include "transaction.h"
#include "dual_port_ram_payload.h"

class DualPortRamTransaction : public BaseTransaction<DualPortRamPayload> {
    explicit DualPortRamTransaction() {}

    virtual ~DualPortRamTransaction() = default;

};
#endif // DUAL_PORT_RAM_TRANSACTION_H
