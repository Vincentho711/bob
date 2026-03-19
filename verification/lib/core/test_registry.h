#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

template <typename BaseSeqT>
class TestRegistry {
public:
    using FactoryFn = std::function<std::unique_ptr<BaseSeqT>()>;

    void register_test(const std::string& name, FactoryFn factory, bool is_default = false);
    std::unique_ptr<BaseSeqT> creat(const std::string& name) const;
    std::vector<std::string> test_names() const;
    const std::string& default_test_name() const;
    bool has_test(const std::string& name) const;
private:
    struct Entry {
        FactoryFn factory;
    };
    std::unordered_map<std::string, Entry> entries_;
    std::string default_name_;
};
