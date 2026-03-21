#include "dual_port_ram_test_registry.h"
#include "tests/dual_port_ram_test_default_top_sequence.h"
#include "tests/dual_port_ram_test_directed_top_sequence.h"

// To add a new test:
//   1. Create tests/dual_port_ram_test_<name>.h/.cpp inheriting DualPortRamBaseSequence
//   2. #include it here
//   3. Add one register_test() call below

void register_dual_port_ram_tests(DualPortRamTestRegistry& registry) {
    registry.register_test("default",
        []() {
            return std::make_unique<DualPortRamTestDefaultTopSequence>();
        }, /*is_default=*/true);

    registry.register_test("directed",
        []() {
            return std::make_unique<DualPortRamTestDirectedTopSequence>();
        });
}
