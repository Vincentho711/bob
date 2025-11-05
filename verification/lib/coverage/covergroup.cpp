#include "covergroup.h"
#include <iostream>

void Covergroup::add_coverpoint(const std::string &name, uint32_t num_bins) {
    coverpoints_[name] = std::make_shared<Coverpoint>(name, num_bins);
}

void Covergroup::sample(const std::string &name, uint32_t value) {
    auto it = coverpoints_.find(name);
    if (it != coverpoints_.end()) {
        it->second->sample(value);
    } else {
        std::cerr << "[WARN] Attempted to sample unknown coverpoint: "
                  << name << std::endl;
    }
}

void Covergroup::add_cross(const std::string& name, const std::vector<std::string>& coverpoint_names) {
    cross_coverpoints_[name] = std::make_shared<CrossCoverpoint>(name);
}

void Covergroup::sample_cross(const std::string &name, const std::vector<uint32_t> &values) {
    auto it = cross_coverpoints_.find(name);
    if (it != cross_coverpoints_.end()) {
        it->second->sample(values);
    } else {
        std::cerr << "[WARN] Attempted to sample unknown cross: " << name << std::endl;
    }
}

void Covergroup::report() const {
    std::cout << "=== Coverage Report ===" << std::endl;
    for (const auto& [name, cp] : coverpoints_) {
        cp->report();
    }
    for (const auto& [name, cross] : cross_coverpoints_) {
        cross->report();
    }
}

std::shared_ptr<Coverpoint> Covergroup::get_coverpoint(const std::string &name) {
    auto it = coverpoints_.find(name);
    return (it != coverpoints_.end()) ? it->second : nullptr;
}

std::shared_ptr<CrossCoverpoint> Covergroup::get_cross_coverpoint(const std::string &name) {
    auto it = cross_coverpoints_.find(name);
    return (it != cross_coverpoints_.end()) ? it->second : nullptr;
}
