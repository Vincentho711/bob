#ifndef COVERGROUP_H
#define COVERGROUP_H
#include "coverpoint.h"
#include "cross_coverpoint.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class Covergroup {
public:
    Covergroup() = default;

    // Single coverpoint
    void add_coverpoint(const std::string &name, uint32_t num_bins);
    void sample(const std::string &name, uint32_t value);

    // Cross coverage
    void add_cross(const std::string &name, const std::vector<std::string> &coverpoint_names);
    void sample_cross(const std::string &name, const std::vector<uint32_t> &values);

    void report() const;

    std::shared_ptr<Coverpoint> get_coverpoint(const std::string &name);
    std::shared_ptr<CrossCoverpoint> get_cross_coverpoint(const std::string &name);

private:
    std::unordered_map<std::string, std::shared_ptr<Coverpoint>> coverpoints_;
    std::unordered_map<std::string, std::shared_ptr<CrossCoverpoint>> cross_coverpoints_;
};
#endif // COVERGROUP_H
