#include <gtest/gtest.h>

#include "common/status/exception/error_codes.h"
#include "scheduler/skill_manifest.h"
#include "scheduler/skill_payload_validator.h"

#include <Poco/JSON/Parser.h>

using AIstudy::ValidationError;
using AIstudy::scheduler::loadSkillManifest;
using AIstudy::scheduler::validatePayloadAgainstManifest;

TEST(SkillPayloadValidator, RejectsInvalidMethodEnum) {
    const auto manifestRes =
        loadSkillManifest(AISTUDY_PROJECT_ROOT "/skills/rainflow/manifest.json");
    ASSERT_TRUE(manifestRes.ok());

    Poco::JSON::Parser parser;
    auto payload = parser.parse(R"({
        "load_history": [1, 2],
        "method": "BadEnum"
    })")
                       .extract<Poco::JSON::Object::Ptr>();

    const auto valid = validatePayloadAgainstManifest(payload, manifestRes.value());
    EXPECT_FALSE(valid.ok());
    EXPECT_EQ(valid.error().category(), "validation");
    EXPECT_EQ(valid.error().code(), static_cast<int>(ValidationError::CONSTRAINT_VIOLATION));
    EXPECT_EQ(valid.detail(), "method: value not in enum");
}

TEST(SkillPayloadValidator, RejectsUnknownPayloadField) {
    const auto manifestRes =
        loadSkillManifest(AISTUDY_PROJECT_ROOT "/skills/rainflow/manifest.json");
    ASSERT_TRUE(manifestRes.ok());

    Poco::JSON::Parser parser;
    auto payload = parser.parse(R"({
        "load_history": [1, 2, 3],
        "method": "ThreePoint",
        "typo_field": true
    })")
                       .extract<Poco::JSON::Object::Ptr>();

    const auto valid = validatePayloadAgainstManifest(payload, manifestRes.value());
    EXPECT_FALSE(valid.ok());
    EXPECT_EQ(valid.error().code(), static_cast<int>(ValidationError::CONSTRAINT_VIOLATION));
    EXPECT_EQ(valid.detail(), "typo_field: additional property not allowed");
}

TEST(SkillPayloadValidator, RejectsEmptyLoadHistory) {
    const auto manifestRes =
        loadSkillManifest(AISTUDY_PROJECT_ROOT "/skills/rainflow/manifest.json");
    ASSERT_TRUE(manifestRes.ok());

    Poco::JSON::Parser parser;
    auto payload = parser.parse(R"({
        "load_history": [],
        "method": "ThreePoint"
    })")
                       .extract<Poco::JSON::Object::Ptr>();

    const auto valid = validatePayloadAgainstManifest(payload, manifestRes.value());
    EXPECT_FALSE(valid.ok());
    EXPECT_EQ(valid.error().code(), static_cast<int>(ValidationError::CONSTRAINT_VIOLATION));
    EXPECT_EQ(valid.detail(), "load_history: array has too few items");
}
