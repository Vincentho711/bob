#include "coverage_registry.h"
#include "covergroup.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

static std::string pretty_json(const std::string& compact, int width = 2) {
    std::string out;
    out.reserve(compact.size() * 2);
    int  depth     = 0;
    bool in_string = false;
    auto indent    = [&]() {
        out += '\n';
        for (int i = 0; i < depth * width; ++i) out += ' ';
    };
    for (std::size_t i = 0; i < compact.size(); ++i) {
        char c = compact[i];
        if (in_string) {
            out += c;
            if (c == '\\' && i + 1 < compact.size()) out += compact[++i];
            else if (c == '"') in_string = false;
        } else {
            switch (c) {
                case '"':  in_string = true; out += c; break;
                case '{': case '[': out += c; ++depth; indent(); break;
                case '}': case ']': --depth; indent(); out += c; break;
                case ',': out += c; indent(); break;
                case ':': out += ": "; break;
                default:  out += c;
            }
        }
    }
    return out;
}

CoverageRegistry& CoverageRegistry::instance() {
    static CoverageRegistry inst;
    return inst;
}

void CoverageRegistry::register_group(Covergroup* cg) {
    for (const auto* g : groups_) {
        if (g->name() == cg->name())
            throw std::invalid_argument(
                std::format("CoverageRegistry: duplicate covergroup name '{}'", cg->name()));
    }
    groups_.push_back(cg);
}

void CoverageRegistry::deregister_group(Covergroup* cg) {
    auto it = std::find(groups_.begin(), groups_.end(), cg);
    if (it != groups_.end()) groups_.erase(it);
}

double CoverageRegistry::total_coverage() const {
    uint64_t defined = 0, hit = 0;
    for (const auto* g : groups_) {
        defined += g->total_bins_defined();
        hit     += g->total_bins_hit();
    }
    return (defined > 0) ? static_cast<double>(hit) / static_cast<double>(defined) : 0.0;
}

void CoverageRegistry::write_json(const std::filesystem::path& path) const {
    double total = total_coverage();
    std::string compact;
    compact += "{\"schema_version\":1,\"total_coverage\":";
    compact += std::to_string(total);
    compact += ",\"covergroups\":[";
    for (std::size_t i = 0; i < groups_.size(); ++i) {
        if (i > 0) compact += ",";
        compact += groups_[i]->to_json_string();
    }
    compact += "]}";
    std::ofstream f(path);
    f << pretty_json(compact);
}
