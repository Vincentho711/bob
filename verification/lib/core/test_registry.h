#ifndef TEST_REGISTRY_H
#define TEST_REGISTRY_H
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

template <typename BaseSeqT>
class TestRegistry {
public:
    using FactoryFn = std::function<std::unique_ptr<BaseSeqT>()>;

    void register_test(const std::string& name, FactoryFn factory, bool is_default = false) {
        if (entries_.count(name)) {
            throw std::invalid_argument("Test '" + name + "' is already registered.");
        }
        // First registered test (or explicit is_default) becomes the default.
        if (is_default || default_name_.empty()) {
            default_name_ = name;
        }
        entries_.emplace(name, Entry{std::move(factory)});
    }

    std::unique_ptr<BaseSeqT> create(const std::string& name) const {
        auto it = entries_.find(name);
        if (it == entries_.end()) {
            std::string msg = "Unknown test '" + name + "'. Available:";
            for (const auto& [n, _] : entries_) msg += " " + n;
            throw std::invalid_argument(msg);
        }
        return it->second.factory();
    }

    std::vector<std::string> test_names() const {
        std::vector<std::string> names;
        names.reserve(entries_.size());
        for (const auto& [n, _] : entries_) names.push_back(n);
        return names;
    }

    const std::string& default_test_name() const { return default_name_; }

    bool has_test(const std::string& name) const { return entries_.count(name) > 0; }

private:
    struct Entry { FactoryFn factory; };
    std::unordered_map<std::string, Entry> entries_;
    std::string default_name_;
};
#endif // TEST_REGISTRY_H
