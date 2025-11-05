#include "coverpoint.h"

Coverpoint::Coverpoint(std::string name, uint32_t num_bins)
    : name_(std::move(name)),
      num_bins_(num_bins),
      total_samples_(0U) {}

void Coverpoint::set_num_bins(uint32_t num_bins) {
    num_bins_ = num_bins;
}

void Coverpoint::sample(uint32_t value) {
    ++total_samples_;
    ++bin_hits_[value];
}

double Coverpoint::coverage() const {
    if (num_bins_ == 0U) {
        return 0.0;
    }
    return static_cast<double>(bin_hits_.size()) / num_bins_ * 100.0;
}

void Coverpoint::report() const {
    std::cout << "  [Coverpoint: " << name_ << "] coverage: "
              << coverage() << "% (" << bin_hits_.size() << "/" << num_bins_
              << " bins hit, " << total_samples_ << " samples)" << std::endl;
}
