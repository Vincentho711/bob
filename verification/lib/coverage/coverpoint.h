#ifndef COVERPOINT_H
#define COVERPOINT_H

#include <cstdint>
#include <string>
#include <vector>

enum class BinType : uint8_t { REGULAR, ILLEGAL, IGNORE };

class Coverpoint {
public:
    struct Range { uint64_t lo; uint64_t hi; };

    struct Bin {
        std::string name;
        BinType     type;
        std::vector<Range> ranges;
        uint64_t    hits = 0;
    };

    explicit Coverpoint(std::string name);

    // All add_*() return *this for chaining.
    // Throws std::invalid_argument if bin_name is duplicate or values overlap an existing bin.
    Coverpoint& add_bin(std::string bin_name, std::vector<uint64_t> values);
    Coverpoint& add_bin(std::string bin_name, uint64_t lo, uint64_t hi);
    // count=0: one bin per value (max range 4096 values); count>0: count equal-width range bins.
    Coverpoint& add_auto_bins(std::string prefix, uint64_t min, uint64_t max,
                              uint32_t count = 0);
    Coverpoint& add_illegal_bin(std::string bin_name, std::vector<uint64_t> values);
    Coverpoint& add_illegal_bin(std::string bin_name, uint64_t lo, uint64_t hi);
    Coverpoint& add_ignore_bin(std::string bin_name, std::vector<uint64_t> values);
    Coverpoint& add_ignore_bin(std::string bin_name, uint64_t lo, uint64_t hi);

    // Illegal/ignore bins take priority over regular bins.
    // Illegal bin hit: throws simulation::VerificationError.
    // Ignore bin hit:  increments bin.hits only (excluded from coverage).
    // Unmapped value:  warns to std::cerr, increments total_samples_ only.
    void sample(uint64_t value);

    // Weighted fraction: regular bins hit / regular bins defined. 0.0 if none defined.
    double coverage() const;

    std::string to_json_string() const;
    const std::string& name() const { return name_; }

    // Number of regular bins (used by Covergroup for weighted-mean calculation).
    uint64_t num_regular_bins() const;
    uint64_t regular_bins_hit() const;

    const std::vector<Bin>& bins() const { return bins_; }

private:
    Coverpoint& add_bin_impl(std::string bin_name, BinType type,
                             std::vector<Range> ranges);
    // Returns nullptr if value is not covered by any range in any bin.
    const Bin* find_bin(uint64_t value) const;
    bool ranges_overlap(const std::vector<Range>& a, const std::vector<Range>& b) const;

    std::string       name_;
    std::vector<Bin>  bins_;       // insertion-ordered
    uint64_t          total_samples_ = 0;
};

#endif // COVERPOINT_H
