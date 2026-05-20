#include "rainflow_adapter.h"
#include "kernel/rainflow/rainflow.h"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/Dynamic/Var.h>
#include <sstream>
#include <string>

using AIstudy::kernel::rainflow::RainflowInput;
using AIstudy::kernel::rainflow::RainflowMethod;
using AIstudy::kernel::rainflow::RainflowOutput;
using AIstudy::kernel::rainflow::rainflowCounting;

namespace AIstudy {
namespace adapter {
namespace rainflow {

// ==================== 辅助：Poco::JSON::Object -> RainflowInput ====================
static bool parse_input(Poco::JSON::Object::Ptr payload, RainflowInput& input) {
    // 1. 解析必需字段 "load_history" (Array)
    if (!payload->has("load_history") || !payload->isArray("load_history")) {
        return false;
    }
    Poco::JSON::Array::Ptr historyArray = payload->getArray("load_history");
    for (size_t i = 0; i < historyArray->size(); ++i) {
        input.load_history.push_back(historyArray->getElement<double>(i));
    }

    // 2. 解析必需字段 "method" (String -> Enum)
    if (!payload->has("method")) {
        return false;
    }
    const Poco::Dynamic::Var methodVar = payload->get("method");
    if (!methodVar.isString()) {
        return false;
    }
    const std::string method_str = methodVar.convert<std::string>();
    if (method_str == "ThreePoint") {
        input.method = RainflowMethod::ThreePoint;
    } else if (method_str == "FourPoint") {
        input.method = RainflowMethod::FourPoint;
    } else if (method_str == "ModifiedFourPoint") {
        input.method = RainflowMethod::ModifiedFourPoint;
    } else {
        return false; // 未知方法
    }

    // 3. 解析可选字段 "params" (Object)
    if (payload->has("params") && payload->isObject("params")) {
        Poco::JSON::Object::Ptr params = payload->getObject("params");
        input.params.threshold = params->optValue<double>("threshold", 0.0);
        input.params.grads = static_cast<size_t>(params->optValue<int>("grads", 100));
    } else {
        input.params.threshold = 0.0;
        input.params.grads = 100;
    }

    // 4. 固定字段
    input.task_name = "rainflow";
    input.module_id = "rainflow_module";
    return true;
}

// ==================== 辅助：RainflowOutput -> Poco::JSON::Object ====================
static Poco::JSON::Object::Ptr to_json(const RainflowOutput& output) {
    Poco::JSON::Object::Ptr result = new Poco::JSON::Object;

    Poco::JSON::Array::Ptr itemsArray = new Poco::JSON::Array;
    for (const auto& item : output.items) {
        Poco::JSON::Object::Ptr itemObj = new Poco::JSON::Object;
        itemObj->set("amplitude", item.amplitude);
        itemObj->set("mean", item.mean);
        itemObj->set("count", static_cast<int>(item.count));
        itemsArray->add(itemObj);
    }
    
    result->set("items", itemsArray);
    result->set("num_cycles", static_cast<int>(output.num_cycles));
    return result;
}

// ==================== 主适配器函数 ====================
std::string rainflow_adapter(const std::string& payload_json_str) {
    Poco::JSON::Object::Ptr response = new Poco::JSON::Object;
    try {
        // 1. 解析 payload JSON 字符串
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(payload_json_str);
        Poco::JSON::Object::Ptr payload = result.extract<Poco::JSON::Object::Ptr>();

        // 2. 构造 RainflowInput
        RainflowInput input;
        if (!parse_input(payload, input)) {
            response->set("success", false);
            response->set("error", "Invalid payload: missing or malformed fields (load_history, method)");
            std::ostringstream oss;
            Poco::JSON::Stringifier::condense(response, oss);
            return oss.str();
        }

        // 3. 调用核心算法，对接你自己的 StatusOr 系统
        auto algoResult = rainflowCounting(input);
        if (!algoResult.ok()) {
            response->set("success", false);
            // 假设 Status 有 message() 方法返回错误字符串
            response->set("error", algoResult.status().message());
            std::ostringstream oss;
            Poco::JSON::Stringifier::condense(response, oss);
            return oss.str();
        }

        // 4. 成功返回
        response->set("success", true);
        response->set("result", to_json(algoResult.value()));
        std::ostringstream oss;
        Poco::JSON::Stringifier::condense(response, oss);
        return oss.str();

    } catch (const Poco::JSON::JSONException& e) {
        Poco::JSON::Object::Ptr errResponse = new Poco::JSON::Object;
        errResponse->set("success", false);
        errResponse->set("error", std::string("JSON parse error: ") + e.what());
        std::ostringstream oss;
        Poco::JSON::Stringifier::condense(errResponse, oss);
        return oss.str();
    } catch (const std::exception& e) {
        Poco::JSON::Object::Ptr errResponse = new Poco::JSON::Object;
        errResponse->set("success", false);
        errResponse->set("error", std::string("Unexpected error: ") + e.what());
        std::ostringstream oss;
        Poco::JSON::Stringifier::condense(errResponse, oss);
        return oss.str();
    }
}

std::string rainflow_schema() {
    return R"({
        "title": "RainflowCounting",
        "description": "雨流计数法，计算载荷谱的应力幅值和均值分布",
        "input": {
            "type": "object",
            "properties": {
                "load_history": {
                    "type": "array",
                    "items": { "type": "number" },
                    "description": "载荷时间历程序列"
                },
                "method": {
                    "type": "string",
                    "enum": ["ThreePoint", "FourPoint", "ModifiedFourPoint"],
                    "description": "雨流计数算法"
                },
                "params": {
                    "type": "object",
                    "properties": {
                        "threshold": { "type": "number", "default": 0.0 },
                        "grads": { "type": "integer", "default": 100, "minimum": 1 }
                    }
                }
            },
            "required": ["load_history", "method"]
        },
        "output": {
            "type": "object",
            "properties": {
                "items": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "amplitude": { "type": "number" },
                            "mean": { "type": "number" },
                            "count": { "type": "integer" }
                        }
                    }
                },
                "num_cycles": { "type": "integer" }
            }
        }
    })";
}

} // namespace rainflow
} // namespace adapter
} // namespace AIstudy