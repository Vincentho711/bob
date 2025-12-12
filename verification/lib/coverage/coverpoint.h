#ifndef COVERPOINT_H
#define COVERPOINT_H
#include <string>
#include <unordered_map>
#include <cstdint>
#include <iostream>

class Coverpoint {
public:
    explicit Coverpoint(std::string name = "", uint32_t num_bins = 0);

    void sample(uint32_t value);
    double coverage() const;
    void report() const;

    void set_num_bins(uint32_t num_bins);
    const std::string &name() const { return name_; }

private:
    std::string name_;
    uint32_t num_bins_;
    uint64_t total_samples_;
    std::unordered_map<uint32_t, uint64_t> bin_hits_;
};
#endif // COVERPOINT_H
