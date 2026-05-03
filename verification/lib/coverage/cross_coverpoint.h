#ifndef CROSS_COVERPOINT_H
#define CROSS_COVERPOINT_H

#include "coverpoint.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CrossCoverpoint {
public:
    // constituents: the Coverpoint objects being crossed (must outlive this object).
    // expected_bins = product of each constituent's regular bin count.
    CrossCoverpoint(std::string name,
                    std::vector<std::shared_ptr<Coverpoint>> constituents);

    // values[i] corresponds to constituents[i].sample value.
    void sample(const std::vector<uint64_t>& values);

    // combinations_seen / expected_bins. 0.0 if expected_bins == 0.
    double coverage() const;

    uint64_t expected_bins()    const { return expected_bins_; }
    uint64_t combinations_seen() const { return static_cast<uint64_t>(bin_hits_.size()); }

    std::string to_json_string() const;
    const std::string& name() const { return name_; }

private:
    static std::string make_key(const std::vector<uint64_t>& values);

    std::string name_;
    std::vector<std::shared_ptr<Coverpoint>> constituents_;
    uint64_t    expected_bins_  = 0;
    uint64_t    total_samples_  = 0;
    std::unordered_map<std::string, uint64_t> bin_hits_;
};

#endif // CROSS_COVERPOINT_H
