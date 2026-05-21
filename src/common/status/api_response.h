#ifndef AISTUDY_COMMON_STATUS_API_RESPONSE_H
#define AISTUDY_COMMON_STATUS_API_RESPONSE_H

#include "common/status/exception/error_code_wrapper.h"
#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <Poco/Dynamic/Var.h>
#include <string>

namespace AIstudy {
/** @brief 协议 v1 响应 meta（skill_id、skill_version、duration_ms�?*/

struct ApiResponseMeta {
    std::string skill_id;
    std::string skill_version;
    int duration_ms = 0;
};

/** @brief Skill 执行结果 �?协议 v1 响应字符�?*/
std::string makeSkillApiResponse(const StatusOr<Poco::JSON::Object::Ptr>& skill_result,
                                 const std::string& request_id,
                                 const ApiResponseMeta& meta);
/** @brief 协议 v1 失败响应（信�?校验/路由错误等） */
std::string makeSkillApiFailureResponse(const ErrorCodeWrapper& error,
                                        const std::string& request_id,
                                        const ApiResponseMeta& meta,
                                        const std::string& messageOverride = "");
} // namespace AIstudy

#endif // AISTUDY_COMMON_STATUS_API_RESPONSE_H
