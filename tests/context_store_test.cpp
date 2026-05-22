#include <gtest/gtest.h>

#include "scheduler/context_handle_rules.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_context_store.h"

using AIstudy::scheduler::ArtifactMeta;
using AIstudy::scheduler::ContextStore;
using AIstudy::scheduler::HandleKind;
using AIstudy::scheduler::lastContextStoreDetail;

TEST(ContextStore, PutAndGetInSameContext) {
    auto& store = ContextStore::instance();
    ASSERT_TRUE(store.dropContext("ctx-a").ok());

    ASSERT_TRUE(store.ensureContext("ctx-a").ok());

    ArtifactMeta meta;
    meta.handle_id = "mesh_unit";
    meta.kind = HandleKind::Mesh;
    meta.uri = "artifact://ctx-a/mesh_unit.h5";
    ASSERT_TRUE(store.putArtifact("ctx-a", meta).ok());

    const auto got = store.getArtifact("ctx-a", "mesh_unit");
    ASSERT_TRUE(got.ok());
    EXPECT_EQ(got.value().uri, meta.uri);

    ASSERT_TRUE(store.dropContext("ctx-a").ok());
}

TEST(ContextStore, MergeInboundOverwritesHandle) {
    auto& store = ContextStore::instance();
    ASSERT_TRUE(store.dropContext("ctx-b").ok());
    ASSERT_TRUE(store.ensureContext("ctx-b").ok());

    ArtifactMeta first;
    first.handle_id = "file_cfg";
    first.kind = HandleKind::File;
    first.uri = "file://old";
    ArtifactMeta second = first;
    second.uri = "file://new";

    ASSERT_TRUE(store.mergeInbound("ctx-b", {first}).ok());
    ASSERT_TRUE(store.mergeInbound("ctx-b", {second}).ok());

    const auto got = store.getArtifact("ctx-b", "file_cfg");
    ASSERT_TRUE(got.ok());
    EXPECT_EQ(got.value().uri, "file://new");

    ASSERT_TRUE(store.dropContext("ctx-b").ok());
}

TEST(ContextStore, InvalidContextIdSetsDetail) {
    auto& store = ContextStore::instance();
    const auto r = store.ensureContext("");
    ASSERT_FALSE(r.ok());
    EXPECT_NE(lastContextStoreDetail().find("context.context_id"), std::string::npos);
    EXPECT_NE(lastContextStoreDetail().find("must not be empty"), std::string::npos);
}

TEST(ContextStore, MissingHandleSetsDetailPath) {
    auto& store = ContextStore::instance();
    ASSERT_TRUE(store.dropContext("ctx-missing").ok());
    ASSERT_TRUE(store.ensureContext("ctx-missing").ok());

    const auto got = store.getArtifact("ctx-missing", "mesh_absent");
    ASSERT_FALSE(got.ok());
    EXPECT_NE(lastContextStoreDetail().find("context.handles.mesh_absent"), std::string::npos);
    EXPECT_NE(lastContextStoreDetail().find("not registered"), std::string::npos);

    ASSERT_TRUE(store.dropContext("ctx-missing").ok());
}
