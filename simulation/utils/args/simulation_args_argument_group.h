#ifndef SIMULATION_ARGS_ARGUMENT_GROUP
#define SIMULATION_ARGS_ARGUMENT_GROUP
#include "simulation_args_group_app.h"

#include <string>
#include <string_view>
namespace simulation::args {
// Forward declaration
// ArgumentGroup references SimulationArguementParser for cross_validate.
class SimulationArgumentParser;

// ArgumentGroup - Abstract base class for every argument container
class ArgumentGroup {
public:
    explicit ArgumentGroup(std::string prefix) : prefix_(std::move(prefix)) {}

    virtual ~ArgumentGroup() = default;

    ArgumentGroup(const ArgumentGroup&)       = delete;
    ArgumentGroup& operator=(const ArgumentGroup&) = delete;
    ArgumentGroup(ArgumentGroup&&) = delete;
    ArgumentGroup& operator=(ArgumentGroup&&) = delete;

    // Pure virtual interface
    virtual void register_args(GroupApp& app) = 0;
    [[nodiscard]] virtual std::string_view description() const = 0;

    // Post parse hooks
    virtual void post_parse_resolve() {}
    virtual void post_parse_validate() {}
    virtual void cross_validate(const SimulationArgumentParser&) {}

    // Metadata
    [[nodiscard]] std::string_view prefix() const noexcept { return prefix_; }
private:
    std::string prefix_;
};
}
#endif // SIMULATION_ARGS_ARGUMENT_GROUP
