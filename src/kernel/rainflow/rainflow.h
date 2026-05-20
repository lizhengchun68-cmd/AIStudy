#ifndef RAINFLOW_H
#define RAINFLOW_H

#include <vector>
#include <string>
#include "common/status/status_or.h"

namespace AIstudy {
namespace kernel {
namespace rainflow {

// 雨流计数算法类型
enum class RainflowMethod {
    ThreePoint,
    FourPoint,
    ModifiedFourPoint
};

// 计数参数
struct RainflowParams {
    double threshold = 0.0;
    size_t grads=100;
};

// 单条结果
struct RainflowResultItem {
    double amplitude = 0.0;
    double mean = 0.0;
    size_t count = 0;
};

// ==============================
// Agent 标准输入
// ==============================
struct RainflowInput {
    std::string task_name;
    std::string module_id;
    std::vector<double> load_history;
    RainflowMethod method;
    RainflowParams params;
};

// ==============================
// Agent 标准输出
// ==============================
struct RainflowOutput {
    std::vector<RainflowResultItem> items{};
    size_t num_cycles = 0;
};

// ==============================
// 唯一对外接口（Agent 只调用这个）
// ==============================
StatusOr<RainflowOutput> rainflowCounting(const RainflowInput& input);

} // namespace rainflow
} // namespace kernel
} // namespace AIstudy

#endif