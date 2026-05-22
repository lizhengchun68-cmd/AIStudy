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
