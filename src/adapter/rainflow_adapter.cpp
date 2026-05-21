#include "rainflow_adapter.h"
#include "kernel/rainflow/rainflow.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/exception.h"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/Dynamic/Var.h>
#include <string>

namespace AIstudy {
namespace adapter {
namespace rainflow {

namespace {

using kernel::rainflow::RainflowInput;
using kernel::rainflow::RainflowMethod;
using kernel::rainflow::RainflowOutput;
using kernel::rainflow::rainflowCounting;

ErrorCodeWrapper validationError() {
    return ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                          ErrorCategory::VALIDATION);
}

ErrorCodeWrapper validationErrorDataType() {
    return ErrorCodeWrapper(static_cast<int>(ValidationError::DATA_TYPE_MISMATCH),
                          ErrorCategory::VALIDATION);
}

StatusOr<Poco::JSON::Object::Ptr> parsePayloadJson(const std::string& payload_json_str) {
    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var parsed = parser.parse(payload_json_str);
        Poco::JSON::Object::Ptr payload = parsed.extract<Poco::JSON::Object::Ptr>();
        if (!payload) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(validationError());
        }
        return StatusOr<Poco::JSON::Object::Ptr>::Ok(payload);
    } catch (const Poco::JSON::JSONException&) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    } catch (const SimUtilsException& e) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(e.errorCode());
    } catch (const std::exception&) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
    }
}

StatusOr<RainflowInput> parseRainflowInput(Poco::JSON::Object::Ptr payload) {
    RainflowInput input;

    if (!payload->has("load_history") || !payload->isArray("load_history")) {
        return StatusOr<RainflowInput>::Fail(validationError());
    }
    Poco::JSON::Array::Ptr historyArray = payload->getArray("load_history");
    if (historyArray->size() == 0) {
        return StatusOr<RainflowInput>::Fail(validationError());
    }
    for (size_t i = 0; i < historyArray->size(); ++i) {
        try {
            input.load_history.push_back(historyArray->getElement<double>(static_cast<unsigned>(i)));
        } catch (...) {
            return StatusOr<RainflowInput>::Fail(validationErrorDataType());
        }
    }

    if (!payload->has("method")) {
        return StatusOr<RainflowInput>::Fail(validationError());
    }
    const Poco::Dynamic::Var methodVar = payload->get("method");
    if (!methodVar.isString()) {
        return StatusOr<RainflowInput>::Fail(validationErrorDataType());
    }
    const std::string method_str = methodVar.convert<std::string>();
    if (method_str == "ThreePoint") {
        input.method = RainflowMethod::ThreePoint;
    } else if (method_str == "FourPoint") {
        input.method = RainflowMethod::FourPoint;
    } else if (method_str == "ModifiedFourPoint") {
        input.method = RainflowMethod::ModifiedFourPoint;
    } else {
        return StatusOr<RainflowInput>::Fail(validationError());
    }

    if (payload->has("params")) {
        if (!payload->isObject("params")) {
            return StatusOr<RainflowInput>::Fail(validationErrorDataType());
        }
        Poco::JSON::Object::Ptr params = payload->getObject("params");
        input.params.threshold = params->optValue<double>("threshold", 0.0);
        const int grads = params->optValue<int>("grads", 100);
        if (grads < 1) {
            return StatusOr<RainflowInput>::Fail(validationError());
        }
        input.params.grads = static_cast<size_t>(grads);
    } else {
        input.params.threshold = 0.0;
        input.params.grads = 100;
    }

    input.task_name = "rainflow";
    input.module_id = "rainflow_module";
    return StatusOr<RainflowInput>::Ok(std::move(input));
}

Poco::JSON::Object::Ptr rainflowOutputToJson(const RainflowOutput& output) {
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

} // namespace

StatusOr<Poco::JSON::Object::Ptr> rainflow_execute(const std::string& payload_json_str) {
    auto payloadRes = parsePayloadJson(payload_json_str);
    if (!payloadRes.ok()) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(payloadRes.status());
    }

    auto inputRes = parseRainflowInput(payloadRes.value());
    if (!inputRes.ok()) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(inputRes.status());
    }

    StatusOr<RainflowOutput> algoRes = rainflowCounting(inputRes.value());
    if (!algoRes.ok()) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(algoRes.status());
    }

    return StatusOr<Poco::JSON::Object::Ptr>::Ok(rainflowOutputToJson(algoRes.value()));
}

} // namespace rainflow
} // namespace adapter
} // namespace AIstudy
