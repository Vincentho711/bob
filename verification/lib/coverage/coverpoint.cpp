#include "coverpoint.h"
#include "simulation_exceptions.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>

Coverpoint::Coverpoint(std::string name) : name_(std::move(name)) {}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool Coverpoint::ranges_overlap(const std::vector<Range>& a,
                                 const std::vector<Range>& b) const {
    for (const auto& ra : a)
        for (const auto& rb : b)
            if (ra.lo <= rb.hi && rb.lo <= ra.hi) return true;
    return false;
}

const Coverpoint::Bin* Coverpoint::find_bin(uint64_t value) const {
    // Illegal/ignore bins take priority — check them first.
    for (const auto& bin : bins_) {
        if (bin.type == BinType::REGULAR) continue;
        for (const auto& r : bin.ranges)
            if (value >= r.lo && value <= r.hi) return &bin;
    }
    for (const auto& bin : bins_) {
        if (bin.type != BinType::REGULAR) continue;
        for (const auto& r : bin.ranges)
            if (value >= r.lo && value <= r.hi) return &bin;
    }
    return nullptr;
}

Coverpoint& Coverpoint::add_bin_impl(std::string bin_name, BinType type,
                                      std::vector<Range> ranges) {
    // Duplicate name check
    for (const auto& b : bins_) {
        if (b.name == bin_name)
            throw std::invalid_argument(
                std::format("coverpoint '{}': duplicate bin name '{}'",
                            name_, bin_name));
    }
    // Overlap check against all existing bins
    for (const auto& b : bins_) {
        if (ranges_overlap(ranges, b.ranges))
            throw std::invalid_argument(
                std::format("coverpoint '{}': bin '{}' overlaps with existing bin '{}'",
                            name_, bin_name, b.name));
    }
    bins_.push_back({std::move(bin_name), type, std::move(ranges), 0});
    return *this;
}

// ---------------------------------------------------------------------------
// Public bin definition API
// ---------------------------------------------------------------------------

Coverpoint& Coverpoint::add_bin(std::string bin_name, std::vector<uint64_t> values) {
    std::vector<Range> ranges;
    ranges.reserve(values.size());
    for (uint64_t v : values) ranges.push_back({v, v});
    return add_bin_impl(std::move(bin_name), BinType::REGULAR, std::move(ranges));
}

Coverpoint& Coverpoint::add_bin(std::string bin_name, uint64_t lo, uint64_t hi) {
    if (lo > hi)
        throw std::invalid_argument(
            std::format("coverpoint '{}': bin '{}' has lo > hi", name_, bin_name));
    return add_bin_impl(std::move(bin_name), BinType::REGULAR, {{lo, hi}});
}

Coverpoint& Coverpoint::add_auto_bins(std::string prefix, uint64_t min, uint64_t max,
                                       uint32_t count) {
    if (min > max)
        throw std::invalid_argument(
            std::format("coverpoint '{}': add_auto_bins min > max", name_));
    if (count == 0) {
        uint64_t n = max - min + 1;
        if (n > 4096)
            throw std::invalid_argument(
                std::format("coverpoint '{}': add_auto_bins range {} too large for "
                            "per-value bins (max 4096); use count > 0 for range bins",
                            name_, n));
        for (uint64_t v = min; v <= max; ++v)
            add_bin(prefix + "_" + std::to_string(v - min), {v, v});
    } else {
        uint64_t total  = max - min + 1;
        uint64_t width  = total / count;
        uint64_t rem    = total % count;
        uint64_t lo     = min;
        for (uint32_t i = 0; i < count; ++i) {
            uint64_t hi = lo + width - 1 + (i < rem ? 1 : 0);
            add_bin(prefix + "_" + std::to_string(i), lo, hi);
            lo = hi + 1;
        }
    }
    return *this;
}

Coverpoint& Coverpoint::add_illegal_bin(std::string bin_name,
                                         std::vector<uint64_t> values) {
    std::vector<Range> ranges;
    ranges.reserve(values.size());
    for (uint64_t v : values) ranges.push_back({v, v});
    return add_bin_impl(std::move(bin_name), BinType::ILLEGAL, std::move(ranges));
}

Coverpoint& Coverpoint::add_illegal_bin(std::string bin_name, uint64_t lo, uint64_t hi) {
    if (lo > hi)
        throw std::invalid_argument(
            std::format("coverpoint '{}': illegal bin '{}' has lo > hi", name_, bin_name));
    return add_bin_impl(std::move(bin_name), BinType::ILLEGAL, {{lo, hi}});
}

Coverpoint& Coverpoint::add_ignore_bin(std::string bin_name,
                                        std::vector<uint64_t> values) {
    std::vector<Range> ranges;
    ranges.reserve(values.size());
    for (uint64_t v : values) ranges.push_back({v, v});
    return add_bin_impl(std::move(bin_name), BinType::IGNORE, std::move(ranges));
}

Coverpoint& Coverpoint::add_ignore_bin(std::string bin_name, uint64_t lo, uint64_t hi) {
    if (lo > hi)
        throw std::invalid_argument(
            std::format("coverpoint '{}': ignore bin '{}' has lo > hi", name_, bin_name));
    return add_bin_impl(std::move(bin_name), BinType::IGNORE, {{lo, hi}});
}

// ---------------------------------------------------------------------------
// Sampling
// ---------------------------------------------------------------------------

void Coverpoint::sample(uint64_t value) {
    ++total_samples_;
    // find_bin checks illegal/ignore before regular (priority order)
    Bin* hit = const_cast<Bin*>(find_bin(value));
    if (!hit) {
        std::cerr << "[WARN] coverpoint '" << name_ << "': unmapped value "
                  << value << "\n";
        return;
    }
    if (hit->type == BinType::ILLEGAL) {
        throw simulation::VerificationError(
            std::format("coverpoint '{}': illegal bin '{}' hit (value={})",
                        name_, hit->name, value));
    }
    ++hit->hits;
}

// ---------------------------------------------------------------------------
// Coverage query
// ---------------------------------------------------------------------------

uint64_t Coverpoint::num_regular_bins() const {
    uint64_t n = 0;
    for (const auto& b : bins_)
        if (b.type == BinType::REGULAR) ++n;
    return n;
}

uint64_t Coverpoint::regular_bins_hit() const {
    uint64_t n = 0;
    for (const auto& b : bins_)
        if (b.type == BinType::REGULAR && b.hits > 0) ++n;
    return n;
}

double Coverpoint::coverage() const {
    uint64_t total = num_regular_bins();
    if (total == 0) return 0.0;
    return static_cast<double>(regular_bins_hit()) / static_cast<double>(total);
}

// ---------------------------------------------------------------------------
// JSON serialization
// ---------------------------------------------------------------------------

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"')  { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else           { out += c; }
    }
    return out;
}

static const char* bin_type_str(BinType t) {
    switch (t) {
        case BinType::REGULAR: return "regular";
        case BinType::ILLEGAL: return "illegal";
        case BinType::IGNORE:  return "ignore";
    }
    return "unknown";
}

std::string Coverpoint::to_json_string() const {
    uint64_t num_reg  = num_regular_bins();
    uint64_t reg_hit  = regular_bins_hit();
    double   cov      = (num_reg > 0)
                        ? static_cast<double>(reg_hit) / static_cast<double>(num_reg)
                        : 0.0;

    std::ostringstream o;
    o << "{";
    o << "\"name\":\""         << json_escape(name_) << "\",";
    o << "\"num_bins\":"       << num_reg            << ",";
    o << "\"bins_hit\":"       << reg_hit            << ",";
    o << "\"total_samples\":"  << total_samples_     << ",";
    o << "\"coverage\":"       << cov                << ",";
    o << "\"bins\":[";
    for (std::size_t i = 0; i < bins_.size(); ++i) {
        const auto& b = bins_[i];
        if (i > 0) o << ",";
        o << "{";
        o << "\"name\":\""  << json_escape(b.name)    << "\",";
        o << "\"type\":\""  << bin_type_str(b.type)   << "\",";
        o << "\"hits\":"    << b.hits;
        o << "}";
    }
    o << "]}";
    return o.str();
}
