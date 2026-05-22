#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_context_store.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

TEST(ContextCloseContract, SkillEndsSession) {
    auto& store = AIstudy::scheduler::ContextStore::instance();
    const std::string id = "ctx-close-skill";
    ASSERT_TRUE(store.closeContext(id).ok());

    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string import_req = R"({
        "protocol": "1",
        "skill_id": "mesh_import",
        "context": { "context_id": "ctx-close-skill" },
        "payload": { "source_path": "a.inp", "mesh_handle": "mesh_done" }
    })";
    auto import_obj = aistudy_test::parseJsonObject(dispatcher.execute(import_req));
    ASSERT_TRUE(import_obj->getValue<bool>("ok"));
    ASSERT_TRUE(store.hasContext(id));

    const std::string close_req = R"({
        "protocol": "1",
        "skill_id": "context_close",
        "context": { "context_id": "ctx-close-skill" },
        "payload": {}
    })";
    auto close_obj = aistudy_test::parseJsonObject(dispatcher.execute(close_req));
    ASSERT_TRUE(close_obj->getValue<bool>("ok"));
    auto result = close_obj->getObject("result");
    ASSERT_TRUE(result->getValue<bool>("closed"));
    EXPECT_EQ(result->getValue<std::string>("context_id"), id);
    EXPECT_FALSE(store.hasContext(id));

    const auto got = store.getArtifact(id, "mesh_done");
    EXPECT_FALSE(got.ok());
}

TEST(ContextCloseContract, EnvelopeCloseFlagEndsSession) {
    auto& store = AIstudy::scheduler::ContextStore::instance();
    const std::string id = "ctx-close-flag";
    ASSERT_TRUE(store.closeContext(id).ok());

    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string req = R"({
        "protocol": "1",
        "skill_id": "host_echo",
        "context": { "context_id": "ctx-close-flag", "close": true },
        "payload": { "message": "bye" }
    })";
    auto obj = aistudy_test::parseJsonObject(dispatcher.execute(req));
    ASSERT_TRUE(obj->getValue<bool>("ok"));
    EXPECT_FALSE(store.hasContext(id));
}
