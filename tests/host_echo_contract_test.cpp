#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

#include <string>

namespace {

std::string hostEchoTestsDir() {
#ifdef AISTUDY_PROJECT_ROOT
    return std::string(AISTUDY_PROJECT_ROOT) + "/skills/host_echo/tests";
#else
    return "skills/host_echo/tests";
#endif
}

} // namespace

TEST(HostEchoContract, ExecuteMatchesGoldenOk) {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    const std::string request =
        aistudy_test::readTextFile(hostEchoTestsDir() + "/request_ok.json");
    const std::string expected =
        aistudy_test::readTextFile(hostEchoTestsDir() + "/expected_ok.json");

    const std::string actual = dispatcher.execute(request);
    auto actualObj = aistudy_test::parseJsonObject(actual);
    auto expectedObj = aistudy_test::parseJsonObject(expected);
    aistudy_test::stripVolatileResponseFields(actualObj);
    aistudy_test::stripVolatileResponseFields(expectedObj);

    EXPECT_TRUE(aistudy_test::jsonEqual(actualObj, expectedObj))
        << "actual=" << actual;
}

TEST(SkillRegistry, ListIncludesAtLeastTwoSkills) {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    auto root = aistudy_test::parseJsonObject(dispatcher.listSkillsJson());
    auto skills = root->getArray("skills");
    ASSERT_GE(skills->size(), 2u);
    EXPECT_TRUE(dispatcher.loadFailures().empty());
}
