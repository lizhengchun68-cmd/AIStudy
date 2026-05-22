#include <gtest/gtest.h>

#include "common/status/exception/error_codes.h"
#include "scheduler/Dispatcher.h"
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

std::string rainflowTestsDir() {
    return projectRoot() + "/skills/rainflow/tests";
}

AIstudy::scheduler::Dispatcher makeDispatcher() {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);
    return dispatcher;
}

} // namespace

TEST(RainflowContract, ExecuteMatchesGoldenOk) {
    auto dispatcher = makeDispatcher();
    const std::string request =
        aistudy_test::readTextFile(rainflowTestsDir() + "/request_ok.json");
    const std::string expected =
        aistudy_test::readTextFile(rainflowTestsDir() + "/expected_ok.json");

    const std::string actual = dispatcher.execute(request);
    auto actualObj = aistudy_test::parseJsonObject(actual);
    auto expectedObj = aistudy_test::parseJsonObject(expected);
    aistudy_test::stripVolatileResponseFields(actualObj);
    aistudy_test::stripVolatileResponseFields(expectedObj);

    EXPECT_TRUE(aistudy_test::jsonEqual(actualObj, expectedObj))
        << "actual=" << actual;
}

TEST(RainflowContract, InvalidMethodFailsAtScheduler) {
    auto dispatcher = makeDispatcher();
    const std::string request =
        aistudy_test::readTextFile(rainflowTestsDir() + "/request_invalid_method.json");
    const std::string expected =
        aistudy_test::readTextFile(rainflowTestsDir() + "/expected_error.json");

    const std::string actual = dispatcher.execute(request);
    auto actualObj = aistudy_test::parseJsonObject(actual);
    auto expectedObj = aistudy_test::parseJsonObject(expected);
    aistudy_test::stripVolatileResponseFields(actualObj);
    aistudy_test::stripVolatileResponseFields(expectedObj);

    ASSERT_FALSE(actualObj->getValue<bool>("ok"));
    auto actualErr = actualObj->getObject("error");
    auto expectedErr = expectedObj->getObject("error");
    EXPECT_EQ(actualErr->getValue<std::string>("category"),
              expectedErr->getValue<std::string>("category"));
    EXPECT_EQ(actualErr->getValue<int>("code"),
              static_cast<int>(AIstudy::ValidationError::CONSTRAINT_VIOLATION));
    EXPECT_EQ(actualErr->getValue<std::string>("message"), "method: value not in enum");
    EXPECT_EQ(actualObj->getValue<std::string>("protocol"), "1");
}

TEST(SkillRegistry, ListIncludesRainflowWhenManifestPresent) {
    auto dispatcher = makeDispatcher();
    const std::string listJson = dispatcher.listSkillsJson();
    auto root = aistudy_test::parseJsonObject(listJson);
    ASSERT_TRUE(root->has("skills"));
    auto skills = root->getArray("skills");
    bool found = false;
    for (size_t i = 0; i < skills->size(); ++i) {
        auto item = skills->getObject(static_cast<unsigned>(i));
        if (item && item->getValue<std::string>("id") == "rainflow") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
    EXPECT_TRUE(dispatcher.loadFailures().empty());
}
