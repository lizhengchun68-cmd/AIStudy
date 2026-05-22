#include <gtest/gtest.h>

#include "scheduler/context_handle_rules.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_artifact_paths.h"
#include "scheduler/skill_context_store.h"
#include <Poco/File.h>
#include <chrono>
#include <thread>

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

TEST(ContextStore, DropReportsWhetherSessionExisted) {
    auto& store = ContextStore::instance();
    ASSERT_TRUE(store.dropContext("ctx-drop-report").ok());
    const auto first = store.dropContext("ctx-drop-report");
    ASSERT_TRUE(first.ok());
    EXPECT_FALSE(first.value());

    ASSERT_TRUE(store.ensureContext("ctx-drop-report").ok());
    const auto second = store.dropContext("ctx-drop-report");
    ASSERT_TRUE(second.ok());
    EXPECT_TRUE(second.value());
}

TEST(ContextStore, CloseContextRemovesSession) {
    auto& store = ContextStore::instance();
    const std::string id = "ctx-close-m7a";
    ASSERT_TRUE(store.closeContext(id).ok());
    ASSERT_TRUE(store.ensureContext(id).ok());
    ASSERT_TRUE(store.hasContext(id));

    const auto closed = store.closeContext(id);
    ASSERT_TRUE(closed.ok());
    EXPECT_TRUE(closed.value());
    EXPECT_FALSE(store.hasContext(id));
}

TEST(ContextStore, PurgeExpiredRemovesIdleSession) {
    auto& store = ContextStore::instance();
    const int previous_ttl = ContextStore::defaultContextTtlSeconds();
    ContextStore::setDefaultContextTtlSeconds(1);
    const std::string id = "ctx-ttl-m7a";
    ASSERT_TRUE(store.closeContext(id).ok());
    ASSERT_TRUE(store.ensureContext(id).ok());
    ASSERT_TRUE(store.hasContext(id));

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_GE(store.purgeExpiredContexts(), 1);
    EXPECT_FALSE(store.hasContext(id));

    ContextStore::setDefaultContextTtlSeconds(previous_ttl);
}

TEST(ContextStore, CloseRemovesArtifactDirectory) {
    auto& store = ContextStore::instance();
    const std::string id = "ctx-artifact-rm";
    ASSERT_TRUE(store.closeContext(id).ok());
    ASSERT_TRUE(store.ensureContext(id).ok());
    const std::string dir = AIstudy::scheduler::artifactContextDir(id);
    Poco::File f(dir);
    ASSERT_TRUE(f.exists());

    ASSERT_TRUE(store.closeContext(id).ok());
    EXPECT_FALSE(Poco::File(dir).exists());
}
