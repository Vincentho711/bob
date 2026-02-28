#include "simulation_args_argument_registry.h"
#include <stdexcept>

void simulation::args::ArgumentRegistry::register_argument(simulation::args::ArgumentDescriptor arg_desc) {
    if (by_name_.contains(arg_desc.full_name)) {
        throw std::invalid_argument("Argument \"--" + arg_desc.full_name + "\" is already registered.\n"
                                    " Check for duplicate add_argument calls in register_argument().");
    }
    const std::size_t idx = descriptors_.size();
    by_name_[arg_desc.full_name] = idx;
    descriptors_.push_back(std::move(arg_desc));
}


[[nodiscard]] simulation::args::ArgumentDescriptor* simulation::args::ArgumentRegistry::find(std::string_view full_name) noexcept {
    if (const auto it = by_name_.find(std::string(full_name)); it != by_name_.end()) {
        return &descriptors_[it->second];
    }
    return nullptr;
}
[[nodiscard]] const simulation::args::ArgumentDescriptor* simulation::args::ArgumentRegistry::find(std::string_view full_name) const noexcept {
    if (const auto it = by_name_.find(std::string(full_name)); it != by_name_.end()) {
        return &descriptors_[it->second];
    }
    return nullptr;
}
