#include "self_register.h"

#include <algorithm>
#include <string>
#include <utility>

namespace fp {

LoggerRegistry& LoggerRegistry::instance() {
    // 函数内静态局部变量：第一次被用到时才构造，
    // 因此不受翻译单元之间静态初始化顺序的影响。
    static LoggerRegistry registry;
    return registry;
}

bool LoggerRegistry::add(std::string_view kind, LoggerCreatorFn creator) {
    // C++17 的 unordered_map 还不支持异构查找（那是 C++20 的事），
    // 所以这里要为每次查找构造一个 std::string 键 —— 这是 string_view 没能
    // 渗透到全部容器接口的地方，值得记一笔。
    return creators_.emplace(std::string{kind}, std::move(creator)).second;
}

std::unique_ptr<Logger> LoggerRegistry::create(std::string_view kind) const {
    const auto it = creators_.find(std::string{kind});
    if (it == creators_.end()) {
        return nullptr;
    }
    return it->second();
}

std::vector<std::string> LoggerRegistry::kinds() const {
    std::vector<std::string> names;
    names.reserve(creators_.size());
    for (const auto& entry : creators_) {
        names.push_back(entry.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace fp
