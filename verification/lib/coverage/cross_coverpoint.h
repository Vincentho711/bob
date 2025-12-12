#ifndef CROSS_COVERGROUP_H
#define CROSS_COVERGROUP_H
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <sstream>

class CrossCoverpoint {
public:
    explicit CrossCoverpoint(std::string name)
        : name_(std::move(name)), total_samples_(0) {}

    // Sample one combination (tuple of integer values)
    void sample(const std::vector<uint32_t>& values);

    // Calculate coverage percentage
    double coverage() const;

    // Report coverage results
    void report() const;

private:
    std::string name_;
    uint64_t total_samples_;
    std::unordered_map<std::string, uint64_t> bin_hits_; // key = joined string of values

    static std::string make_key(const std::vector<uint32_t>& values);
};
#endif // CROSS_COVERGROUP_H
