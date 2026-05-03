#ifndef COVERAGE_REGISTRY_H
#define COVERAGE_REGISTRY_H

#include <filesystem>
#include <vector>

class Covergroup;

// Singleton registry analogous to simulation::ProgressReporter.
// Covergroup constructor/destructor call register/deregister automatically.
// Not thread-safe — single-threaded simulation only.
class CoverageRegistry {
public:
    static CoverageRegistry& instance();

    // Called by Covergroup constructor. Throws std::invalid_argument on duplicate name.
    void register_group(Covergroup* cg);
    // Called by Covergroup destructor.
    void deregister_group(Covergroup* cg);

    // Weighted mean: sum(bins_hit) / sum(bins_defined) across all registered covergroups.
    // Returns 0.0 if no bins are defined.
    double total_coverage() const;

    // Writes schema_version 1 coverage.json containing all registered covergroups.
    void write_json(const std::filesystem::path& path) const;

private:
    CoverageRegistry() = default;
    std::vector<Covergroup*> groups_;  // insertion-ordered, non-owning
};

#endif // COVERAGE_REGISTRY_H
