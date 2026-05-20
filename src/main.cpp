#include <iostream>
#include <string>

#include "adapter/rainflow_adapter.h"
#include "scheduler/Dispatcher.h"

int main() {
    using namespace AIstudy;

    scheduler::Dispatcher dispatcher;
    dispatcher.registerAdapter("rainflow", adapter::rainflow::rainflow_adapter);

    const std::string payload = R"({
        "load_history": [1.0, 2.0, 3.0, 2.0, 1.0],
        "method": "ThreePoint",
        "params": {"threshold": 0.2, "grads": 50}
    })";

    // 直接走适配器
    std::cout << adapter::rainflow::rainflow_adapter(payload) << std::endl;

    // 走调度器信封：task_type + payload
    const std::string envelope = R"({
        "task_type": "rainflow",
        "payload": )" + payload + R"(
    })";
    std::cout << dispatcher.execute(envelope) << std::endl;

    return 0;
}