#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AIstudy {
namespace scheduler {

    using AdapterFunc = std::string(*)(const std::string&);

    class Dispatcher {
public:
    // 注册一个适配器
    void registerAdapter(const std::string& task_type, AdapterFunc func);

    // 执行任务信封（输入整个信封 JSON 字符串，返回结果 JSON 字符串）
    std::string execute(const std::string& envelope_json);

    // 列出所有已注册的 task_type（供 --describe 使用，下一步会用到）
    std::vector<std::string> listTaskTypes() const;

private:
    std::unordered_map<std::string, AdapterFunc> registry_;
    std::string makeErrorResponse(const std::string& msg) const;
};

} // namespace scheduler
} // namespace AIstudy

#endif // DISPATCHER_H