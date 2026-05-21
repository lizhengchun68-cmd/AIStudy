#include "common/common_status/exception/exception.h"
#include "common/common_status/exception/error_info.h"
#include <stdexcept>

namespace AIstudy {

SimUtilsException::SimUtilsException(const std::string& message, const ErrorCodeWrapper& errorCode,
                                     const ExceptionContext& context)
    : std::runtime_error(message), errorCode_(errorCode), context_(context), cause_()
{
}

const ErrorCodeWrapper& SimUtilsException::errorCode() const noexcept
{
    return errorCode_;
}

ErrorInfo SimUtilsException::toErrorInfo() const
{
    return getErrorInfo(errorCode_, context_, what());
}

void SimUtilsException::setCause(std::exception_ptr p) noexcept
{
    cause_ = std::move(p);
}

std::exception_ptr SimUtilsException::getCause() const noexcept
{
    return cause_;
}

std::vector<std::string> SimUtilsException::getExceptionChain() const
{
    std::vector<std::string> out;
    out.push_back(what());
    std::exception_ptr p = cause_;
    while (p) {
        try {
            std::rethrow_exception(p);
        } catch (const std::exception& e) {
            out.push_back(e.what());
            const SimUtilsException* su = dynamic_cast<const SimUtilsException*>(&e);
            p = su ? su->getCause() : std::exception_ptr();
        } catch (...) {
            out.push_back("(unknown)");
            p = std::exception_ptr();
        }
    }
    return out;
}

} // namespace AIstudy
