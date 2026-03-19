#include "test_registry.h"
#include "dual_port_ram_sequence.h"

using DualPortRamTestRegistry = TestRegistry<DualPortRamBaseSequence>;

void register_dual_port_ram_tests(DualPortRamTestRegistry& registry, uint32_t addr_width, uint32_t data_width, uint64_t seed);
