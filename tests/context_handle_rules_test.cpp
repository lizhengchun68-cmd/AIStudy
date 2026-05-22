#include <gtest/gtest.h>

#include <Poco/JSON/Object.h>

#include "scheduler/context_handle_rules.h"

using AIstudy::scheduler::HandleKind;
using AIstudy::scheduler::handleKindFromPrefix;
using AIstudy::scheduler::isValidContextId;
using AIstudy::scheduler::isValidHandleId;
using AIstudy::scheduler::parseContextObject;
using AIstudy::scheduler::parseHandleEntry;

TEST(ContextHandleRules, AcceptsValidHandleIds) {
    EXPECT_TRUE(isValidHandleId("mesh_main"));
    EXPECT_TRUE(isValidHandleId("result_stress_01"));
    EXPECT_TRUE(isValidHandleId("file_model.inp"));
    EXPECT_EQ(handleKindFromPrefix("mesh_a"), HandleKind::Mesh);
}

TEST(ContextHandleRules, RejectsInvalidHandleIds) {
    EXPECT_FALSE(isValidHandleId(""));
    EXPECT_FALSE(isValidHandleId("data_001"));
    EXPECT_FALSE(isValidHandleId("mesh"));
}

TEST(ContextHandleRules, AcceptsValidContextId) {
    EXPECT_TRUE(isValidContextId("550e8400-e29b-41d4-a716-446655440000"));
}

TEST(ContextHandleRules, ParseContextObjectWithHandles) {
    Poco::JSON::Object::Ptr ctx(new Poco::JSON::Object);
    ctx->set("context_id", "session-1");
    Poco::JSON::Object::Ptr handles(new Poco::JSON::Object);
    Poco::JSON::Object::Ptr mesh(new Poco::JSON::Object);
    mesh->set("kind", "mesh");
    mesh->set("uri", "artifact://session/mesh_main.h5");
    handles->set("mesh_main", mesh);
    ctx->set("handles", handles);

    const auto parsed = parseContextObject(ctx);
    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.value().first, "session-1");
    ASSERT_EQ(parsed.value().second.size(), 1u);
    EXPECT_EQ(parsed.value().second[0].handle_id, "mesh_main");
}

TEST(ContextHandleRules, RejectsKindMismatch) {
    Poco::JSON::Object::Ptr entry(new Poco::JSON::Object);
    entry->set("kind", "file");
    const auto r = parseHandleEntry("mesh_x", entry, "context.handles.mesh_x");
    EXPECT_FALSE(r.ok());
}

TEST(ContextHandleRules, RejectsContextWithoutId) {
    Poco::JSON::Object::Ptr ctx(new Poco::JSON::Object);
    ctx->set("handles", Poco::JSON::Object::Ptr(new Poco::JSON::Object));
    EXPECT_FALSE(parseContextObject(ctx).ok());
}
