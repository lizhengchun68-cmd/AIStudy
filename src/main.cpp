#include <iostream>
#include <string>

#include "adapter/rainflow_adapter.h"
#include "common/status/api_response.h"
#include "scheduler/Dispatcher.h"

void printHelp() {
    std::cout << "Usage: dispatcher [options]\n"
              << "  --list                List all available modules\n"
              << "  --describe <module>   Show input/output schema for a module\n"
              << "  (no arguments)        Run in task execution mode (reads JSON from stdin)\n";
}

int main() {
    using namespace AIstudy;

    scheduler::Dispatcher dispatcher;
    dispatcher.registerAdapter("rainflow", adapter::rainflow::rainflow_execute, adapter::rainflow::rainflow_schema());

    const std::string payload = R"({
        "load_history": [1.0, 2.0, 3.0, 2.0, 1.0],
        "method": "ThreePoint",
        "params": {"threshold": 0.2, "grads": 50}
    })";

    // 直接走适配器（result 对象由 makeApiResponse 封装，与调度层一致）
    std::cout << makeApiResponse(adapter::rainflow::rainflow_execute(payload)) << std::endl;

    // 走调度器信封：task_type + payload（内部同样 makeApiResponse）
    const std::string envelope = R"({
        "task_type": "rainflow",
        "payload": )" + payload + R"(
    })";
    std::cout << dispatcher.execute(envelope) << std::endl;

    return 0;
}