#include "covergroup.h"
#include "coverage_registry.h"

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

Covergroup::Covergroup(std::string name) : name_(std::move(name)) {
    CoverageRegistry::instance().register_group(this);
}

Covergroup::~Covergroup() {
    CoverageRegistry::instance().deregister_group(this);
}

Coverpoint& Covergroup::add_coverpoint(std::string cp_name) {
    if (coverpoints_.count(cp_name))
        throw std::invalid_argument(
            std::format("covergroup '{}': coverpoint '{}' already exists",
                        name_, cp_name));
    auto cp = std::make_shared<Coverpoint>(cp_name);
    cp_order_.push_back(cp_name);
    coverpoints_[cp_name] = cp;
    return *cp;
}

Coverpoint& Covergroup::coverpoint(const std::string& cp_name) {
    auto it = coverpoints_.find(cp_name);
    if (it == coverpoints_.end())
        throw std::invalid_argument(
            std::format("covergroup '{}': coverpoint '{}' not found",
                        name_, cp_name));
    return *it->second;
}

void Covergroup::add_cross(std::string cross_name,
                            std::vector<std::string> coverpoint_names) {
    if (crosses_.count(cross_name))
        throw std::invalid_argument(
            std::format("covergroup '{}': cross '{}' already exists",
                        name_, cross_name));
    std::vector<std::shared_ptr<Coverpoint>> constituents;
    constituents.reserve(coverpoint_names.size());
    for (const auto& cpn : coverpoint_names) {
        auto it = coverpoints_.find(cpn);
        if (it == coverpoints_.end())
            throw std::invalid_argument(
                std::format("covergroup '{}': cross '{}' references unknown coverpoint '{}'",
                            name_, cross_name, cpn));
        constituents.push_back(it->second);
    }
    auto cx = std::make_shared<CrossCoverpoint>(cross_name, std::move(constituents));
    cx_order_.push_back(cross_name);
    crosses_[cross_name] = cx;
}

void Covergroup::sample_cross(const std::string& cross_name,
                               const std::vector<uint64_t>& values) {
    auto it = crosses_.find(cross_name);
    if (it == crosses_.end())
        throw std::invalid_argument(
            std::format("covergroup '{}': cross '{}' not found", name_, cross_name));
    it->second->sample(values);
}

uint64_t Covergroup::total_bins_defined() const {
    uint64_t n = 0;
    for (const auto& cpn : cp_order_)
        n += coverpoints_.at(cpn)->num_regular_bins();
    for (const auto& cxn : cx_order_)
        n += crosses_.at(cxn)->expected_bins();
    return n;
}

uint64_t Covergroup::total_bins_hit() const {
    uint64_t n = 0;
    for (const auto& cpn : cp_order_)
        n += coverpoints_.at(cpn)->regular_bins_hit();
    for (const auto& cxn : cx_order_)
        n += crosses_.at(cxn)->combinations_seen();
    return n;
}

double Covergroup::coverage() const {
    uint64_t defined = total_bins_defined();
    if (defined == 0) return 0.0;
    return static_cast<double>(total_bins_hit()) / static_cast<double>(defined);
}

std::string Covergroup::to_json_string() const {
    static auto escape = [](const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"')       out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else                out += c;
        }
        return out;
    };

    std::ostringstream o;
    o << "{";
    o << "\"name\":\""   << escape(name_) << "\",";
    o << "\"coverage\":" << coverage()    << ",";

    o << "\"coverpoints\":[";
    for (std::size_t i = 0; i < cp_order_.size(); ++i) {
        if (i > 0) o << ",";
        o << coverpoints_.at(cp_order_[i])->to_json_string();
    }
    o << "],";

    o << "\"cross_coverpoints\":[";
    for (std::size_t i = 0; i < cx_order_.size(); ++i) {
        if (i > 0) o << ",";
        o << crosses_.at(cx_order_[i])->to_json_string();
    }
    o << "]}";
    return o.str();
}

void Covergroup::write_json(const std::filesystem::path& path) const {
    uint64_t defined = total_bins_defined();
    double   total   = (defined > 0)
                       ? static_cast<double>(total_bins_hit()) / static_cast<double>(defined)
                       : 0.0;
    std::string compact = "{\"schema_version\":1,\"total_coverage\":"
                        + std::to_string(total)
                        + ",\"covergroups\":["
                        + to_json_string()
                        + "]}";
    std::ofstream f(path);
    f << pretty_json(compact);
}
