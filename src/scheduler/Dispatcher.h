#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief Skill 执行结果：成功时为写入 API `result` 字段的 JSON 对象 */
using SkillResultJson = Poco::JSON::Object::Ptr;

/**
 * @brief Skill 适配器：payload JSON 字符串 → StatusOr<result 对象>
 *
 * 不在此层生成 success/error 信封；由 Dispatcher::execute 统一调用 makeApiResponse。
 */
using SkillExecuteFunc = StatusOr<SkillResultJson> (*)(const std::string& payload_json);

class Dispatcher {
public:
    void registerAdapter(const std::string& task_type, SkillExecuteFunc func, const std::string& schema);

    /** @brief 解析调度信封，调用 Skill，并序列化为统一 API JSON 响应 */
    std::string execute(const std::string& envelope_json);

    std::vector<std::string> listTaskTypes() const;

    std::string getSchema(const std::string& task_type) const;

private:
    struct AdapterInfo {
        SkillExecuteFunc func;
        std::string schema;
    };
    std::unordered_map<std::string, AdapterInfo> registry_;
    std::string makeErrorResponse(const std::string& msg) const;
};

} // namespace scheduler
} // namespace AIstudy

#endif // DISPATCHER_H
