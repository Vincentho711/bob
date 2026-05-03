#ifndef COVERGROUP_H
#define COVERGROUP_H

#include "coverpoint.h"
#include "cross_coverpoint.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Covergroup {
public:
    // Registers with CoverageRegistry on construction.
    // Throws std::invalid_argument if name is already registered.
    explicit Covergroup(std::string name);
    // Deregisters from CoverageRegistry on destruction.
    ~Covergroup();

    // Creates a new Coverpoint and returns a reference for chaining bin definitions.
    // Throws std::invalid_argument if cp_name already exists.
    Coverpoint& add_coverpoint(std::string cp_name);

    // Returns an existing Coverpoint by name.
    // Throws std::invalid_argument if not found.
    Coverpoint& coverpoint(const std::string& cp_name);

    // Adds a cross over coverpoints that must already exist.
    // Throws std::invalid_argument if any name is unknown or cross name is duplicate.
    void add_cross(std::string cross_name, std::vector<std::string> coverpoint_names);

    // Samples a cross coverpoint by name.
    // Throws std::invalid_argument if cross_name not found.
    void sample_cross(const std::string& cross_name, const std::vector<uint64_t>& values);

    // Weighted mean: (total regular bins hit + combinations seen)
    //              / (total regular bins defined + expected cross bins). 0.0 if none defined.
    double coverage() const;

    // Writes a single-covergroup coverage.json (useful for testing).
    void write_json(const std::filesystem::path& path) const;

    // JSON fragment for this covergroup (used by CoverageRegistry).
    std::string to_json_string() const;

    const std::string& name() const { return name_; }

    // Aggregates used by CoverageRegistry for weighted-mean total_coverage.
    uint64_t total_bins_defined() const;
    uint64_t total_bins_hit()     const;

private:
    std::string name_;

    // Insertion-ordered coverpoints: vector for order, map for O(1) lookup.
    std::vector<std::string>                                      cp_order_;
    std::unordered_map<std::string, std::shared_ptr<Coverpoint>>  coverpoints_;

    // Insertion-ordered cross coverpoints
    std::vector<std::string>                                         cx_order_;
    std::unordered_map<std::string, std::shared_ptr<CrossCoverpoint>> crosses_;
};

#endif // COVERGROUP_H
