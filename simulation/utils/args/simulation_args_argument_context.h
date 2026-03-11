#ifndef SIMULATION_ARGS_ARGUMENT_CONTEXT_H
#define SIMULATION_ARGS_ARGUMENT_CONTEXT_H
#include "simulation_args_argument_parser.h"

namespace simulation::args {
class SimulationArgumentContext {
public:
    static void set(const SimulationArgumentParser& parser) noexcept {
        instance_ = &parser;
    }

    static const SimulationArgumentParser& current() {
        if (!instance_) {
            throw std::runtime_error("SimulationArgumentContext not initialised. " "Call SimulationArgumentContext::set(parser) before constructing components.");
        }
        return *instance_;
    }

    template<std::derived_from<ArgumentGroup> T>
    [[nodiscard]] static const T& get() {
        return current().get<T>();
    }
private:
    static inline const SimulationArgumentParser* instance_ = nullptr;
};
}
#endif // SIMULATION_ARGS_ARGUMENT_CONTEXT_H
