/**
 * @file status_or.h
 * @brief StatusOr<T>: value-or-error type for APIs that may fail (ERROR_CODE §�?
 */

#ifndef AISTUDY_STATUS_STATUS_OR_H
#define AISTUDY_STATUS_STATUS_OR_H

#include <utility>
#include <type_traits>
#include <ostream>
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/exception.h"
#include "common/status/exception/error_info.h"

namespace AIstudy {

/** @brief Forbid T = void; use Status-only APIs instead. */
template <typename T>
struct status_or_traits {
    static_assert(!std::is_same<T, void>::value, "StatusOr<T> requires T != void");
};

/**
 * @brief Holds either a value T or an ErrorCodeWrapper; mutually exclusive.
 * @tparam T Value type when ok(). Must be copyable or movable.
 *
 * Conventions (ERROR_CODE §�?:
 * - ok() == true: holds value; value(), operator*, operator-> safe.
 * - ok() == false: holds status; value(), operator*, operator-> throw SimUtilsException.
 * - No default constructor.
 * - value_or_default(default) returns value or default when failed.
 */
template <typename T>
class StatusOr {
public:
    using value_type = T;

    /** @brief Success: construct from T (explicit). */
    explicit StatusOr(const T& val) : has_value_(true) {
        new (&storage_) T(val);
    }
    explicit StatusOr(T&& val) noexcept : has_value_(true) {
        new (&storage_) T(std::move(val));
    }

    /** @brief Failure: construct from ErrorCodeWrapper (explicit). */
    explicit StatusOr(const ErrorCodeWrapper& s) : has_value_(false) {
        new (&status_storage_) ErrorCodeWrapper(s);
    }
    explicit StatusOr(ErrorCodeWrapper&& s) noexcept : has_value_(false) {
        new (&status_storage_) ErrorCodeWrapper(std::move(s));
    }

    /** @brief Failure: construct from (code, category). */
    explicit StatusOr(int code, const std::string& category)
        : has_value_(false) {
        new (&status_storage_) ErrorCodeWrapper(code, category);
    }

    /** @brief No default constructor. */
    StatusOr() = delete;

    ~StatusOr() {
        destroy();
    }

    StatusOr(const StatusOr& o) : has_value_(o.has_value_) {
        if (has_value_)
            new (&storage_) T(o.value_unsafe());
        else
            new (&status_storage_) ErrorCodeWrapper(o.status_unsafe());
    }

    StatusOr(StatusOr&& o) noexcept : has_value_(o.has_value_) {
        if (has_value_) {
            new (&storage_) T(std::move(o.value_unsafe()));
            o.destroy_value_only();
            o.init_status_and_clear();
        } else {
            new (&status_storage_) ErrorCodeWrapper(std::move(o.status_unsafe()));
            o.destroy_status_only();
            o.init_status_and_clear();
        }
    }

    StatusOr& operator=(const StatusOr& o) {
        if (this == &o) return *this;
        destroy();
        has_value_ = o.has_value_;
        if (has_value_)
            new (&storage_) T(o.value_unsafe());
        else
            new (&status_storage_) ErrorCodeWrapper(o.status_unsafe());
        return *this;
    }

    StatusOr& operator=(StatusOr&& o) noexcept {
        if (this == &o) return *this;
        destroy();
        has_value_ = o.has_value_;
        if (has_value_) {
            new (&storage_) T(std::move(o.value_unsafe()));
            o.destroy_value_only();
            o.init_status_and_clear();
        } else {
            new (&status_storage_) ErrorCodeWrapper(std::move(o.status_unsafe()));
            o.destroy_status_only();
            o.init_status_and_clear();
        }
        return *this;
    }

    StatusOr& operator=(const T& t) {
        destroy();
        has_value_ = true;
        new (&storage_) T(t);
        return *this;
    }

    StatusOr& operator=(T&& t) noexcept {
        destroy();
        has_value_ = true;
        new (&storage_) T(std::move(t));
        return *this;
    }

    StatusOr& operator=(const ErrorCodeWrapper& s) {
        destroy();
        has_value_ = false;
        new (&status_storage_) ErrorCodeWrapper(s);
        return *this;
    }

    StatusOr& operator=(ErrorCodeWrapper&& s) noexcept {
        destroy();
        has_value_ = false;
        new (&status_storage_) ErrorCodeWrapper(std::move(s));
        return *this;
    }

    /** @brief true if holding value, false if holding error. */
    bool ok() const noexcept { return has_value_; }
    bool failed() const noexcept { return !has_value_; }

    /** @brief Assertion-style: return value or throw SimUtilsException. */
    T& value() {
        if (!has_value_)
            throw_from_status();
        return value_unsafe();
    }
    const T& value() const {
        if (!has_value_)
            throw_from_status();
        return value_unsafe();
    }

    /** @brief Check-style: return value or default when failed. */
    T value_or_default(const T& default_val) const {
        return has_value_ ? value_unsafe() : default_val;
    }

    /** @brief Always valid: success returns default ErrorCodeWrapper. */
    const ErrorCodeWrapper& status() const {
        if (has_value_)
            return success_sentinel();
        return status_unsafe();
    }
    ErrorCodeWrapper& status() {
        if (has_value_)
            return success_sentinel_mut();
        return status_unsafe();
    }

    int code() const { return status().code(); }
    std::string message() const { return status().message(); }

    explicit operator bool() const noexcept { return ok(); }

    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }

    /** @brief Implicit conversion to Status (for logging, etc.). */
    operator ErrorCodeWrapper() const { return status(); }

    /** @brief Factory: success. */
    static StatusOr Ok(T val) { return StatusOr(std::move(val)); }
    /** @brief Factory: failure. */
    static StatusOr Fail(const ErrorCodeWrapper& s) { return StatusOr(s); }
    static StatusOr Fail(ErrorCodeWrapper&& s) { return StatusOr(std::move(s)); }
    static StatusOr Fail(int code, const std::string& category = "error") {
        return StatusOr(code, category);
    }

private:
    static ErrorCodeWrapper& success_sentinel_mut() {
        static ErrorCodeWrapper s;
        return s;
    }
    static const ErrorCodeWrapper& success_sentinel() {
        return success_sentinel_mut();
    }

    void throw_from_status() const {
        const ErrorCodeWrapper& s = status_unsafe();
        throw SimUtilsException(s.message(), s, ExceptionContext{});
    }

    void init_status_and_clear() {
        new (&status_storage_) ErrorCodeWrapper();
        has_value_ = false;
    }

    T& value_unsafe() { return *reinterpret_cast<T*>(&storage_); }
    const T& value_unsafe() const { return *reinterpret_cast<const T*>(&storage_); }
    ErrorCodeWrapper& status_unsafe() { return *reinterpret_cast<ErrorCodeWrapper*>(&status_storage_); }
    const ErrorCodeWrapper& status_unsafe() const { return *reinterpret_cast<const ErrorCodeWrapper*>(&status_storage_); }

    void destroy_value_only() {
        value_unsafe().~T();
    }
    void destroy_status_only() {
        status_unsafe().~ErrorCodeWrapper();
    }
    void destroy() {
        if (has_value_)
            destroy_value_only();
        else
            destroy_status_only();
    }

    bool has_value_;
    union {
        alignas(T) unsigned char storage_[sizeof(T)];
        alignas(ErrorCodeWrapper) unsigned char status_storage_[sizeof(ErrorCodeWrapper)];
    };
};

/** @brief Stream StatusOr: [OK] or [FAIL] code message (ERROR_CODE §�?. */
template <typename T>
std::ostream& operator<<(std::ostream& os, const StatusOr<T>& r) {
    if (r.ok())
        os << "[OK]";
    else
        os << "[FAIL] " << r.code() << " " << r.message();
    return os;
}

} // namespace AIstudy

#endif // AISTUDY_STATUS_STATUS_OR_H
