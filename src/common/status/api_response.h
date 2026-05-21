/**
 * @file api_response.h
 * @brief 统一 Skill / 调度层 JSON 响应：success + result | error{code,category,message}
 */

#ifndef AISTUDY_COMMON_STATUS_API_RESPONSE_H
#define AISTUDY_COMMON_STATUS_API_RESPONSE_H

#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_info.h"
#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <Poco/Dynamic/Var.h>
#include <string>

namespace AIstudy {

/** @brief 将 ErrorCodeWrapper 转为 JSON 对象 { code, category, message } */
Poco::JSON::Object::Ptr errorCodeToJsonObject(const ErrorCodeWrapper& error,
                                              const std::string& messageOverride = "");

/** @brief 在已有响应对象上设置 success=false 与 error 对象 */
void attachApiError(Poco::JSON::Object::Ptr response,
                    const ErrorCodeWrapper& error,
                    const std::string& messageOverride = "");

/** @brief 成功响应：{ "success": true, "result": ... } */
std::string makeApiSuccessResponse(const Poco::Dynamic::Var& result);

/** @brief 失败响应：{ "success": false, "error": { code, category, message } } */
std::string makeApiFailureResponse(const ErrorCodeWrapper& error,
                                   const std::string& messageOverride = "");
std::string makeApiFailureResponse(const ErrorInfo& info);
std::string makeApiFailureResponse(int code,
                                   const std::string& category,
                                   const std::string& message);

/** @brief Skill 业务结果（result 字段 JSON 对象）→ 完整 API 响应字符串 */
std::string makeApiResponse(const StatusOr<Poco::JSON::Object::Ptr>& skill_result);

} // namespace AIstudy

#endif // AISTUDY_COMMON_STATUS_API_RESPONSE_H
