#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

TEST(DispatcherContextError, ParseInvalidInboundHandleReturnsFieldPath) {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string request = R"({
        "protocol": "1",
        "skill_id": "host_echo",
        "context": {
            "context_id": "ctx-dispatch-err",
            "handles": {
                "bad_handle": { "kind": "file", "uri": "file://x" }
            }
        },
        "payload": { "message": "hi" }
    })";

    const std::string response = dispatcher.execute(request);
    auto obj = aistudy_test::parseJsonObject(response);
    ASSERT_TRUE(obj->has("ok"));
    EXPECT_FALSE(obj->getValue<bool>("ok"));
    ASSERT_TRUE(obj->has("error") && obj->isObject("error"));
    const std::string msg = obj->getObject("error")->getValue<std::string>("message");
    EXPECT_NE(msg.find("context.handles.bad_handle"), std::string::npos);
}

TEST(DispatcherContextError, MeshImportWithoutContextReturnsDetail) {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string request = R"({
        "protocol": "1",
        "skill_id": "mesh_import",
        "payload": { "source_path": "a.inp" }
    })";

    const std::string response = dispatcher.execute(request);
    auto obj = aistudy_test::parseJsonObject(response);
    ASSERT_FALSE(obj->getValue<bool>("ok"));
    const std::string msg = obj->getObject("error")->getValue<std::string>("message");
    EXPECT_NE(msg.find("context"), std::string::npos);
    EXPECT_NE(msg.find("no active context"), std::string::npos);
}
