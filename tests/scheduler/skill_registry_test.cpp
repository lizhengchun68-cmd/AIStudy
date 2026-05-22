#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_manifest.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

#include <string>

namespace {

std::string projectRoot() {
#ifdef AISTUDY_PROJECT_ROOT
    return AISTUDY_PROJECT_ROOT;
#else
    return ".";
#endif
}

} // namespace

TEST(SkillManifestLoad, RejectsManifestWithoutId) {
    const auto res = AIstudy::scheduler::loadSkillManifest(
        projectRoot() + "/tests/fixtures/manifest_invalid.json");
    EXPECT_FALSE(res.ok());
    EXPECT_EQ(res.status().category(), "validation");
}

TEST(SkillRegistry, RecordsLoadFailureWhenManifestMissing) {
    AIstudy::scheduler::Dispatcher dispatcher;
    const std::string badRoot = projectRoot() + "/tests/fixtures/__no_skills_dir__";
    AIstudy::scheduler::registerBuiltinSkills(dispatcher, badRoot);

    ASSERT_FALSE(dispatcher.loadFailures().empty());
    const auto& failures = dispatcher.loadFailures();
    bool foundRainflow = false;
    for (const auto& f : failures) {
        if (f.binding_id == "rainflow") {
            foundRainflow = true;
            EXPECT_NE(f.manifest_path.find("skills/rainflow/manifest.json"), std::string::npos);
            EXPECT_FALSE(f.error.empty());
            break;
        }
    }
    EXPECT_TRUE(foundRainflow);

    auto listRoot = aistudy_test::parseJsonObject(dispatcher.listSkillsJson());
    ASSERT_TRUE(listRoot->has("load_errors"));
    auto errors = listRoot->getArray("load_errors");
    ASSERT_GE(errors->size(), 1u);
    bool listedRainflow = false;
    for (size_t i = 0; i < errors->size(); ++i) {
        auto item = errors->getObject(static_cast<unsigned>(i));
        if (item && item->getValue<std::string>("id") == "rainflow") {
            listedRainflow = true;
            EXPECT_FALSE(item->getValue<std::string>("load_error").empty());
            break;
        }
    }
    EXPECT_TRUE(listedRainflow);

    const std::string request = R"({
        "protocol": "1",
        "skill_id": "rainflow",
        "payload": { "load_history": [1], "method": "ThreePoint" }
    })";
    auto response = aistudy_test::parseJsonObject(dispatcher.execute(request));
    ASSERT_FALSE(response->getValue<bool>("ok"));
}
