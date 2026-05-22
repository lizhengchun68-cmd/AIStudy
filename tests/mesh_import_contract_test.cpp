#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_context_store.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

#include <string>

namespace {

std::string meshImportTestsDir() {
#ifdef AISTUDY_PROJECT_ROOT
    return std::string(AISTUDY_PROJECT_ROOT) + "/skills/mesh_import/tests";
#else
    return "skills/mesh_import/tests";
#endif
}

} // namespace

TEST(MeshImportContract, ExecuteRegistersMeshHandle) {
    AIstudy::scheduler::ContextStore::instance().dropContext("test-session-m5b");

    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string request =
        aistudy_test::readTextFile(meshImportTestsDir() + "/request_ok.json");
    const std::string expected =
        aistudy_test::readTextFile(meshImportTestsDir() + "/expected_ok.json");

    const std::string actual = dispatcher.execute(request);
    auto actualObj = aistudy_test::parseJsonObject(actual);
    auto expectedObj = aistudy_test::parseJsonObject(expected);
    aistudy_test::stripVolatileResponseFields(actualObj);
    aistudy_test::stripVolatileResponseFields(expectedObj);

    EXPECT_TRUE(aistudy_test::jsonEqual(actualObj, expectedObj)) << "actual=" << actual;

    const auto mesh = AIstudy::scheduler::ContextStore::instance().getArtifact("test-session-m5b",
                                                                                "mesh_main");
    ASSERT_TRUE(mesh.ok());
    EXPECT_EQ(mesh.value().uri, "artifact://test-session-m5b/mesh_main.h5");
}

TEST(ContextWorkflow, TwoStepShareContextId) {
    AIstudy::scheduler::ContextStore::instance().dropContext("wf-m5b");

    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string import_req = R"({
        "protocol": "1",
        "skill_id": "mesh_import",
        "context": { "context_id": "wf-m5b" },
        "payload": { "source_path": "a.inp", "mesh_handle": "mesh_step" }
    })";
    const std::string import_res = dispatcher.execute(import_req);
    auto import_obj = aistudy_test::parseJsonObject(import_res);
    ASSERT_TRUE(import_obj->getValue<bool>("ok"));

    const std::string echo_req = R"({
        "protocol": "1",
        "skill_id": "host_echo",
        "context": {
            "context_id": "wf-m5b",
            "handles": {
                "mesh_step": { "kind": "mesh", "uri": "artifact://wf-m5b/mesh_step.h5" }
            }
        },
        "payload": { "message": "mesh registered" }
    })";
    const std::string echo_res = dispatcher.execute(echo_req);
    auto echo_obj = aistudy_test::parseJsonObject(echo_res);
    ASSERT_TRUE(echo_obj->getValue<bool>("ok"));

    const auto got =
        AIstudy::scheduler::ContextStore::instance().getArtifact("wf-m5b", "mesh_step");
    ASSERT_TRUE(got.ok());
}
