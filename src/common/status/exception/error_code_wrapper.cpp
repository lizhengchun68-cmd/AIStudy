#include "common/common_status/exception/error_code_wrapper.h"
#include "common/common_status/exception/error_code_registry.h"
#include "common/common_status/exception/error_info.h"
#include "common/common_status/exception/exception.h"
#include <Poco/Error.h>
#include <sstream>
#include <cerrno>

namespace AIstudy {

ErrorCodeWrapper::ErrorCodeWrapper()
    : code_(0), category_("system")
{
}

ErrorCodeWrapper::ErrorCodeWrapper(int code)
    : code_(code), category_("system")
{
}

ErrorCodeWrapper::ErrorCodeWrapper(int code, const std::string& category)
    : code_(code), category_(category)
{
}

int ErrorCodeWrapper::code() const noexcept
{
    return code_;
}

const std::string& ErrorCodeWrapper::category() const noexcept
{
    return category_;
}

bool ErrorCodeWrapper::isSuccess() const noexcept
{
    return code_ == 0;
}

bool ErrorCodeWrapper::isFailure() const noexcept
{
    return code_ != 0;
}

ErrorCodeWrapper::operator bool() const noexcept
{
    return isFailure();
}

bool ErrorCodeWrapper::operator==(const ErrorCodeWrapper& other) const noexcept
{
    return code_ == other.code_ && category_ == other.category_;
}

bool ErrorCodeWrapper::operator!=(const ErrorCodeWrapper& other) const noexcept
{
    return !(*this == other);
}

bool ErrorCodeWrapper::operator<(const ErrorCodeWrapper& other) const noexcept
{
    if (category_ != other.category_) {
        return category_ < other.category_;
    }
    return code_ < other.code_;
}

ErrorCodeWrapper ErrorCodeWrapper::fromSystemError(int systemErrorCode)
{
    return ErrorCodeWrapper(systemErrorCode, ErrorCategory::SYSTEM);
}

int ErrorCodeWrapper::toSystemError() const
{
    if (category_ == ErrorCategory::SYSTEM) {
        return code_;
    }
    return -1; // Not a system error
}

std::string ErrorCodeWrapper::toString() const
{
    std::ostringstream oss;
    oss << category_ << ":" << code_;
    
    // Try to get description from registry
    auto& registry = ErrorCodeRegistry::getInstance();
    std::string desc = registry.getDescription(code_, category_);
    if (!desc.empty()) {
        oss << " - " << desc;
    } else {
        oss << " - " << message();
    }
    
    return oss.str();
}

std::string ErrorCodeWrapper::message() const
{
    if (code_ == 0) {
        return "Success";
    }
    
    // Try to get message from registry first
    auto& registry = ErrorCodeRegistry::getInstance();
    std::string regMsg = registry.getDescription(code_, category_);
    if (!regMsg.empty()) {
        return regMsg;
    }
    
    // Try to get message from Poco
    std::string pocoMsg = Poco::Error::getMessage(code_);
    if (!pocoMsg.empty() && pocoMsg != "Unknown error") {
        return pocoMsg;
    }
    
    // Fallback to generic message
    std::ostringstream oss;
    oss << "Error [" << category_ << ":" << code_ << "]";
    return oss.str();
}

ErrorInfo ErrorCodeWrapper::toErrorInfo() const {
    return getErrorInfo(*this);
}

namespace {

[[noreturn]] void throwByCategory(const ErrorCodeWrapper& ec, const std::string& msg,
                                  const ExceptionContext& ctx)
{
    const std::string& c = ec.category();
    std::string m = msg.empty() ? ec.message() : msg;
    if (c == ErrorCategory::SIMULATION) throw SimulationException(m, ec, ctx);
    if (c == ErrorCategory::CONFIG) throw ConfigurationException(m, ec, ctx);
    if (c == ErrorCategory::NETWORK) throw NetworkException(m, ec, ctx);
    if (c == ErrorCategory::FILESYSTEM) throw FileSystemException(m, ec, ctx);
    if (c == ErrorCategory::MEMORY) throw MemoryException(m, ec, ctx);
    if (c == ErrorCategory::CONVERGENCE) throw ConvergenceException(m, ec, ctx);
    if (c == ErrorCategory::SOLVER) throw SolverException(m, ec, ctx);
    if (c == ErrorCategory::MESH) throw MeshException(m, ec, ctx);
    if (c == ErrorCategory::PARAMETER) throw ParameterException(m, ec, ctx);
    if (c == ErrorCategory::VALIDATION) throw ValidationException(m, ec, ctx);
    throw SimUtilsException(m, ec, ctx);
}

} // namespace

void ErrorCodeWrapper::toException(const std::string& message) const {
    throwByCategory(*this, message, {});
}

void ErrorCodeWrapper::toException(const std::string& message,
                                   const char* file, int line,
                                   const char* function) const {
    ExceptionContext ctx;
    if (file) ctx.file = file;
    ctx.line = line;
    if (function) ctx.function = function;
    throwByCategory(*this, message, ctx);
}

} // namespace AIstudy
