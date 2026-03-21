#ifndef DUAL_PORT_RAM_TEST_REGISTRY_H
#define DUAL_PORT_RAM_TEST_REGISTRY_H
#include "test_registry.h"
#include "dual_port_ram_sequence.h"

using DualPortRamTestRegistry = TestRegistry<DualPortRamBaseSequence>;

// Registers all available tests into the registry.
// Called before argument parsing so test_registry.test_names() can feed --tb.test.
void register_dual_port_ram_tests(DualPortRamTestRegistry& registry);
#endif // DUAL_PORT_RAM_TEST_REGISTRY_H
