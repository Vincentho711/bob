#ifndef SIMULATION_ARGS_ARGUMENT_PARSER_H
#define SIMULATION_ARGS_ARGUMENT_PARSER_H

#include "simulation_args_argument_group.h"
#include "simulation_args_argument_registry.h"
#include "simulation_args_group_app.h"
#include "simulation_args_tokeniser.h"
#include "simulation_args_help_formatter.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace simulation::args {

// ─────────────────────────────────────────────────────────────────────────────
// SimulationArgumentParser — Top-level argument parsing orchestrator.
//
// Lifecycle:
//   1. add_group<T>(...)   — register one or more ArgumentGroups before parse.
//                            Each group owns a named prefix ("" = core, "uart0",
//                            "dram", etc.). Arguments are scoped accordingly:
//                              --seed          (core, prefix "")
//                              --uart0.baud-rate
//   2. parse(argc, argv)   — tokenise CLI; apply env vars then CLI tokens.
//                            Exits if --help/-h is seen.
//                            Throws std::invalid_argument on unknown flags.
//   3. resolve()           — post_parse_resolve → post_parse_validate →
//                            cross_validate on every registered group, in
//                            registration order. Exits if --dry-run was set.
//
// Typed group access (usable after parse()):
//   const auto& core  = parser.get<CoreArgumentGroup>();
//   const auto& uart0 = parser.get<Uart0ArgumentGroup>();
//
// ─────────────────────────────────────────────────────────────────────────────
class SimulationArgumentParser {
public:
    explicit SimulationArgumentParser(std::string  program_name,
                                      std::string  program_description = {});

    SimulationArgumentParser(const SimulationArgumentParser&)            = delete;
    SimulationArgumentParser& operator=(const SimulationArgumentParser&) = delete;
    SimulationArgumentParser(SimulationArgumentParser&&)                 = delete;
    SimulationArgumentParser& operator=(SimulationArgumentParser&&)      = delete;

    ~SimulationArgumentParser() = default;

    // ── Registration ─────────────────────────────────────────────────────────
    // Adds a group, calls group.register_args(), and indexes it by type for
    // get<T>(). Returns *this so calls can be chained.
    // Throws std::logic_error if the same concrete type is registered twice.
    template<std::derived_from<ArgumentGroup> T>
    SimulationArgumentParser& add_group(std::unique_ptr<T> group) {
        const std::type_index key{typeid(T)};
        if (group_by_type_.contains(key)) {
            throw std::logic_error(
                std::string("SimulationArgumentParser: group '")
                + typeid(T).name() + "' is already registered.");
        }
        GroupApp app{group->prefix(), registry_};
        group->register_args(app);
        group_by_type_.emplace(key, group.get());
        groups_.push_back(std::move(group));
        return *this;
    }

    // ── Parsing ───────────────────────────────────────────────────────────────
    // Tokenises argv, applies env-var overrides (SIM_<PREFIX>_<NAME>), then
    // applies CLI tokens. Precedence: default < env var < command line.
    // Exits(0) on --help/-h. Throws std::invalid_argument on errors.
    void parse(int argc, char** argv);

    // ── Resolution ───────────────────────────────────────────────────────────
    // Runs the full post-parse lifecycle on every registered group, in order:
    //   post_parse_resolve()  — derive values (e.g. randomise seed)
    //   post_parse_validate() — per-group validation
    //   cross_validate()      — inter-group validation
    // Exits(0) after printing resolved values if --dry-run was supplied.
    void resolve();

    // ── Typed group accessor ──────────────────────────────────────────────────
    // Returns a const reference to the registered group of concrete type T.
    // Throws std::logic_error if T was never added via add_group().
    template<std::derived_from<ArgumentGroup> T>
    [[nodiscard]] const T& get() const {
        const auto it = group_by_type_.find(std::type_index{typeid(T)});
        if (it == group_by_type_.end()) {
            throw std::logic_error(
                std::string("SimulationArgumentParser: group '")
                + typeid(T).name() + "' was not registered.");
        }
        return static_cast<const T&>(*it->second);
    }

    // Read-only access to the full registry (e.g. for cross_validate() impls).
    [[nodiscard]] const ArgumentRegistry& registry() const noexcept { return registry_; }

private:
    void apply_env_vars();
    void apply_tokens(const ArgumentTokeniser::Result& result);

    std::string      program_name_;
    std::string      program_description_;
    ArgumentRegistry registry_;

    // Groups in registration order — drives resolve() call order.
    std::vector<std::unique_ptr<ArgumentGroup>>        groups_;
    // Fast typed lookup: typeid(T) → raw pointer into groups_.
    std::unordered_map<std::type_index, ArgumentGroup*> group_by_type_;
};

}

#endif // SIMULATION_ARGS_ARGUMENT_PARSER_H
