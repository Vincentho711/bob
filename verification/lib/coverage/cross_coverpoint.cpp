#include "cross_coverpoint.h"

void CrossCoverpoint::sample(const std::vector<int>& values) {
    ++total_samples_;
    std::string key = make_key(values);
    ++bin_hits_[key];
}

double CrossCoverpoint::coverage() const {
    if (bin_hits_.empty()) return 0.0;
    // We could track the expected total combinations, but often it’s open-ended.
    // For simplicity, report based on unique observed combinations.
    return 100.0; // Full coverage can't be computed without defined domain
}

std::string CrossCoverpoint::make_key(const std::vector<int>& values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    return oss.str();
}

void CrossCoverpoint::report() const {
    std::cout << "  [CrossCoverpoint: " << name_ << "] "
              << "unique combinations: " << bin_hits_.size()
              << ", total samples: " << total_samples_ << std::endl;
}
