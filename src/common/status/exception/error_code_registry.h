/**
 * @file error_code_registry.h
 * @brief Error code registry for custom error code registration and lookup
 */

#ifndef AISTUDY_ERROR_CODE_REGISTRY_H
#define AISTUDY_ERROR_CODE_REGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <mutex>
#include <memory>

namespace AIstudy {

/** @brief (code, description) pair for query results */
using ErrorCodeEntry = std::pair<int, std::string>;

/** @brief Statistics over registered error codes (total and per-category counts) */
struct ErrorCodeStatistics {
    size_t total = 0;
    std::unordered_map<std::string, size_t> byCategory;
};

/**
 * @brief Thread-safe error code registry
 *
 * Allows registration and lookup of custom error codes with descriptions.
 */
class ErrorCodeRegistry {
public:
    static ErrorCodeRegistry& getInstance();

    bool registerErrorCode(int code, const std::string& category, const std::string& description);
    std::string getDescription(int code, const std::string& category) const;
    bool isRegistered(int code, const std::string& category) const;
    void clear();

    /** @brief Get all (code, description) entries for a category. */
    std::vector<ErrorCodeEntry> getErrorCodesByCategory(const std::string& category) const;

    /** @brief Total number of registered error codes. */
    size_t getErrorCodeCount() const;

    /** @brief Number of registered error codes in the given category. */
    size_t getErrorCodeCount(const std::string& category) const;

    /** @brief Find (code, category): returns (true, description) if found, (false, "") otherwise. */
    std::pair<bool, std::string> findErrorCode(int code, const std::string& category) const;

    /** @brief Compute statistics (total and per-category counts) from current registry. */
    ErrorCodeStatistics getErrorCodeStatistics() const;

private:
    ErrorCodeRegistry() = default;
    ~ErrorCodeRegistry() = default;
    ErrorCodeRegistry(const ErrorCodeRegistry&) = delete;
    ErrorCodeRegistry& operator=(const ErrorCodeRegistry&) = delete;

    // Key format: "category:code"
    using KeyType = std::string;
    std::unordered_map<KeyType, std::string> descriptions_;
    mutable std::mutex mutex_;

    static KeyType makeKey(int code, const std::string& category);
};

/** @brief True if (code, category) is registered. */
inline bool isValidErrorCode(int code, const std::string& category) {
    return ErrorCodeRegistry::getInstance().isRegistered(code, category);
}

/** @brief Validate (code, category): true if registered. Range/format checks optional. */
inline bool validateErrorCode(int code, const std::string& category) {
    return isValidErrorCode(code, category);
}

} // namespace AIstudy

#endif // AISTUDY_ERROR_CODE_REGISTRY_H
