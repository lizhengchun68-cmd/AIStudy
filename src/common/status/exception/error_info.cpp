#include "common/common_status/exception/error_info.h"
#include "common/common_status/exception/exception.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>

namespace AIstudy {

namespace {

std::string formatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm_buf, &t);
    std::tm* tm = &tm_buf;
#else
    std::tm* tm = std::gmtime(&t);
#endif
    if (!tm) return {};
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace

std::string getFormattedMessage(const ExceptionContext& ctx) {
    if (ctx.empty()) return {};
    std::ostringstream oss;
    oss << " at ";
    if (!ctx.file.empty()) oss << ctx.file;
    if (ctx.line > 0) oss << ":" << ctx.line;
    if (!ctx.function.empty()) oss << " in " << ctx.function;
    std::string s = oss.str();
    return (s == " at ") ? std::string() : s;
}

std::string ErrorInfo::toString() const {
    std::ostringstream oss;
    oss << "[" << category << ":" << errorCode.code() << "] " << message;
    if (!timestamp.empty()) oss << " (" << timestamp << ")";
    return oss.str();
}

std::string ErrorInfo::toFormattedString() const {
    std::ostringstream oss;
    oss << "[" << category << ":" << errorCode.code() << "] " << message;
    std::string ctxStr = getFormattedMessage(context);
    if (!ctxStr.empty()) oss << ctxStr;
    if (!timestamp.empty()) oss << " [" << timestamp << "]";
    return oss.str();
}

ErrorInfo getErrorInfo(const ErrorCodeWrapper& error) {
    ErrorInfo info;
    info.errorCode = error;
    info.message   = error.message();
    info.category  = error.category();
    info.timestamp = formatTimestamp();
    return info;
}

ErrorInfo getErrorInfo(const ErrorCodeWrapper& error, const ExceptionContext& ctx) {
    ErrorInfo info = getErrorInfo(error);
    info.context = ctx;
    return info;
}

ErrorInfo getErrorInfo(const ErrorCodeWrapper& error, const ExceptionContext& ctx,
                       const std::string& messageOverride) {
    ErrorInfo info = getErrorInfo(error, ctx);
    if (!messageOverride.empty()) info.message = messageOverride;
    return info;
}

ErrorCodeWrapper extractErrorCode(const SimUtilsException& e) {
    return e.errorCode();
}

ErrorCodeWrapper extractErrorCode(const std::exception& e) {
    const auto* su = dynamic_cast<const SimUtilsException*>(&e);
    if (su) return su->errorCode();
    return ErrorCodeWrapper(-1, "unknown");
}

ErrorCodeWrapper extractErrorCode(const std::exception_ptr& eptr) {
    if (!eptr) return ErrorCodeWrapper(-1, "unknown");
    try {
        std::rethrow_exception(eptr);
    } catch (const SimUtilsException& e) {
        return e.errorCode();
    } catch (const std::exception& e) {
        return extractErrorCode(e);
    } catch (...) {
        return ErrorCodeWrapper(-1, "unknown");
    }
}

ErrorInfo extractErrorInfo(const SimUtilsException& e) {
    return e.toErrorInfo();
}

ErrorInfo extractErrorInfo(const std::exception& e) {
    const auto* su = dynamic_cast<const SimUtilsException*>(&e);
    if (su) return su->toErrorInfo();
    ErrorInfo info;
    info.message = e.what();
    info.category = "unknown";
    info.timestamp = formatTimestamp();
    return info;
}

std::string ErrorInfo::toJson() const {
    Poco::JSON::Object::Ptr obj(new Poco::JSON::Object);
    obj->set("code", errorCode.code());
    obj->set("category", category.empty() ? errorCode.category() : category);
    obj->set("message", message);
    if (!context.empty()) {
        Poco::JSON::Object::Ptr ctx(new Poco::JSON::Object);
        ctx->set("file", context.file);
        ctx->set("line", context.line);
        ctx->set("function", context.function);
        obj->set("context", ctx);
    }
    if (!timestamp.empty())
        obj->set("timestamp", timestamp);
    std::ostringstream oss;
    Poco::JSON::Stringifier::stringify(obj, oss);
    return oss.str();
}

ErrorInfo ErrorInfo::fromJson(const std::string& json) {
    ErrorInfo info;
    if (json.empty()) return info;
    try {
        Poco::JSON::Parser p;
        auto var = p.parse(json);
        Poco::JSON::Object::Ptr obj = var.extract<Poco::JSON::Object::Ptr>();
        if (!obj) return info;
        int code = obj->getValue<int>("code");
        std::string cat = obj->getValue<std::string>("category");
        info.errorCode = ErrorCodeWrapper(code, cat);
        info.category = cat;
        info.message = obj->getValue<std::string>("message");
        if (obj->has("context")) {
            auto ctxVar = obj->get("context");
            Poco::JSON::Object::Ptr ctx = ctxVar.extract<Poco::JSON::Object::Ptr>();
            if (ctx) {
                info.context.file = ctx->getValue<std::string>("file");
                info.context.line = ctx->getValue<int>("line");
                info.context.function = ctx->getValue<std::string>("function");
            }
        }
        if (obj->has("timestamp"))
            info.timestamp = obj->getValue<std::string>("timestamp");
    } catch (...) { /* return default */ }
    return info;
}

} // namespace AIstudy
