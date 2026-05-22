#include <gtest/gtest.h>

#include "scheduler/skill_manifest.h"
#include "scheduler/skill_payload_validator.h"

#include <Poco/JSON/Parser.h>

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
    EXPECT_EQ(valid.status().category(), "validation");
}
