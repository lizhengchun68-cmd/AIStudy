#include "common/common_status/exception/error_code_registry.h"
#include <sstream>

namespace AIstudy {

ErrorCodeRegistry& ErrorCodeRegistry::getInstance()
{
    static ErrorCodeRegistry instance;
    return instance;
}

ErrorCodeRegistry::KeyType ErrorCodeRegistry::makeKey(int code, const std::string& category)
{
    std::ostringstream oss;
    oss << category << ":" << code;
    return oss.str();
}

bool ErrorCodeRegistry::registerErrorCode(int code, const std::string& category, const std::string& description)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    KeyType key = makeKey(code, category);
    auto it = descriptions_.find(key);
    if (it != descriptions_.end()) {
        return false; // Already registered
    }
    
    descriptions_[key] = description;
    return true;
}

std::string ErrorCodeRegistry::getDescription(int code, const std::string& category) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    KeyType key = makeKey(code, category);
    auto it = descriptions_.find(key);
    if (it != descriptions_.end()) {
        return it->second;
    }
    
    return "";
}

bool ErrorCodeRegistry::isRegistered(int code, const std::string& category) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    KeyType key = makeKey(code, category);
    return descriptions_.find(key) != descriptions_.end();
}

void ErrorCodeRegistry::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    descriptions_.clear();
}

std::vector<ErrorCodeEntry> ErrorCodeRegistry::getErrorCodesByCategory(const std::string& category) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ErrorCodeEntry> out;
    std::string prefix = category + ":";
    for (const auto& kv : descriptions_) {
        if (kv.first.size() > prefix.size() && kv.first.compare(0, prefix.size(), prefix) == 0) {
            int code = 0;
            try {
                code = std::stoi(kv.first.substr(prefix.size()));
            } catch (...) { continue; }
            out.emplace_back(code, kv.second);
        }
    }
    return out;
}

size_t ErrorCodeRegistry::getErrorCodeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return descriptions_.size();
}

size_t ErrorCodeRegistry::getErrorCodeCount(const std::string& category) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    std::string prefix = category + ":";
    for (const auto& kv : descriptions_) {
        if (kv.first.size() > prefix.size() && kv.first.compare(0, prefix.size(), prefix) == 0)
            ++n;
    }
    return n;
}

std::pair<bool, std::string> ErrorCodeRegistry::findErrorCode(int code, const std::string& category) const
{
    std::string desc = getDescription(code, category);
    return { !desc.empty(), desc };
}

ErrorCodeStatistics ErrorCodeRegistry::getErrorCodeStatistics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    ErrorCodeStatistics s;
    s.total = descriptions_.size();
    for (const auto& kv : descriptions_) {
        std::size_t pos = kv.first.find(':');
        if (pos != std::string::npos) {
            std::string cat = kv.first.substr(0, pos);
            s.byCategory[cat]++;
        }
    }
    return s;
}

} // namespace AIstudy
