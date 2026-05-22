#include <gtest/gtest.h>

#include "scheduler/skill_protocol.h"

using AIstudy::scheduler::parseSkillEnvelope;

TEST(SkillProtocol, RejectsMissingProtocol) {
    const std::string json = R"({
        "skill_id": "rainflow",
        "payload": { "load_history": [1], "method": "ThreePoint" }
    })";
    const auto result = parseSkillEnvelope(json);
    EXPECT_FALSE(result.ok());
}

TEST(SkillProtocol, ParsesProtocolV1Envelope) {
    const std::string json = R"({
        "protocol": "1",
        "skill_id": "rainflow",
        "payload": { "load_history": [1, 2], "method": "ThreePoint" }
    })";
    const auto result = parseSkillEnvelope(json);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().skill_id, "rainflow");
    EXPECT_FALSE(result.value().request_id.empty());
    EXPECT_TRUE(result.value().payload != nullptr);
    EXPECT_TRUE(result.value().context_id.empty());
}

TEST(SkillProtocol, ParsesEnvelopeContext) {
    const std::string json = R"({
        "protocol": "1",
        "skill_id": "mesh_import",
        "context": {
            "context_id": "sess-1",
            "handles": {
                "file_in": { "kind": "file", "uri": "file://data.inp" }
            }
        },
        "payload": { "source_path": "data.inp" }
    })";
    const auto result = parseSkillEnvelope(json);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().context_id, "sess-1");
    ASSERT_EQ(result.value().inbound_handles.size(), 1u);
    EXPECT_EQ(result.value().inbound_handles[0].handle_id, "file_in");
}

TEST(SkillProtocol, ParsesOptionsTimeoutMs) {
    const std::string json = R"({
        "protocol": "1",
        "skill_id": "rainflow",
        "options": { "timeout_ms": 5000 },
        "payload": { "load_history": [1], "method": "ThreePoint" }
    })";
    const auto result = parseSkillEnvelope(json);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().timeout_ms, 5000);
}
