#include "cross_coverpoint.h"

#include <sstream>

CrossCoverpoint::CrossCoverpoint(std::string name,
                                 std::vector<std::shared_ptr<Coverpoint>> constituents)
    : name_(std::move(name)), constituents_(std::move(constituents)) {
    // expected_bins = product of regular bin counts across all constituent coverpoints
    expected_bins_ = 1;
    for (const auto& cp : constituents_) {
        uint64_t n = cp->num_regular_bins();
        expected_bins_ = (n > 0) ? expected_bins_ * n : 0;
    }
}

std::string CrossCoverpoint::make_key(const std::vector<uint64_t>& values) {
    std::ostringstream oss;
    oss << "(";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    oss << ")";
    return oss.str();
}

void CrossCoverpoint::sample(const std::vector<uint64_t>& values) {
    ++total_samples_;
    ++bin_hits_[make_key(values)];
}

double CrossCoverpoint::coverage() const {
    if (expected_bins_ == 0) return 0.0;
    uint64_t seen = static_cast<uint64_t>(bin_hits_.size());
    // combinations_seen can't exceed expected_bins_, but cap defensively
    if (seen > expected_bins_) seen = expected_bins_;
    return static_cast<double>(seen) / static_cast<double>(expected_bins_);
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else                out += c;
    }
    return out;
}

std::string CrossCoverpoint::to_json_string() const {
    uint64_t seen = static_cast<uint64_t>(bin_hits_.size());
    double   cov  = coverage();

    std::ostringstream o;
    o << "{";
    o << "\"name\":\""           << json_escape(name_) << "\",";
    o << "\"constituents\":[";
    for (std::size_t i = 0; i < constituents_.size(); ++i) {
        if (i > 0) o << ",";
        o << "\"" << json_escape(constituents_[i]->name()) << "\"";
    }
    o << "],";
    o << "\"expected_bins\":"    << expected_bins_  << ",";
    o << "\"combinations_seen\":" << seen           << ",";
    o << "\"total_samples\":"    << total_samples_  << ",";
    o << "\"coverage\":"         << cov             << ",";
    o << "\"hits\":{";
    bool first = true;
    for (const auto& [key, count] : bin_hits_) {
        if (!first) o << ",";
        first = false;
        o << "\"" << json_escape(key) << "\":" << count;
    }
    o << "}}";
    return o.str();
}
