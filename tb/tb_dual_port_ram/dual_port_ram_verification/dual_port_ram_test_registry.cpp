#include "dual_port_ram_test_registry.h"
#include "tests/dual_port_ram_test_default_top_sequence.h"
#include <cstdint>

void register_dual_port_ram_tests(DualPortRamTestRegistry& registry, uint32_t addr_width, uint32_t data_width, uint64_t seed) {
    registry.register_test("default", [addr_width, data_width, seed]() {
        return std::make_unique<DualPortRamTestDefaultTopSequence>(addr_width, data_width, seed);
    }, /*is_default=*/ true);
}
