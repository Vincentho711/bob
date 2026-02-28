#ifndef SIMULATION_ARGS_ARGUMENT_REGISTRY_H
#define SIMULATION_ARGS_ARGUMENT_REGISTRY_H
#include "simulation_args_argument_descriptor.h"
#include <unordered_map>
#include <string>
#include <unordered_map>
#include <span>

namespace simulation::args {
// ─────────────────────────────────────────────────────────────────────────────
// ArgumentRegistry — Ordered store of all registered ArgumentDescriptors.
//
// Owned by SimArgParser. Written only during group registration (before parse).
// Read-only after parse() is called.
// ─────────────────────────────────────────────────────────────────────────────
class ArgumentRegistry {
public:
    void register_argument(simulation::args::ArgumentDescriptor arg_desc);
    [[nodiscard]] simulation::args::ArgumentDescriptor* find(std::string_view full_name) noexcept;
    [[nodiscard]] const simulation::args::ArgumentDescriptor* find(std::string_view full_name) const noexcept;

    // Ordered iteration - preserves registration order
    [[nodiscard]] std::span<simulation::args::ArgumentDescriptor>   all() noexcept { return descriptors_; }
    [[nodiscard]] std::span<const simulation::args::ArgumentDescriptor>   all() const noexcept { return descriptors_; }

    [[nodiscard]] bool empty() const noexcept { return descriptors_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return descriptors_.size(); }

private:
    std::vector<simulation::args::ArgumentDescriptor> descriptors_;
    std::unordered_map<std::string, std::size_t> by_name_;
};
}
#endif
