#include <gtest/gtest.h>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

TEST(SkillHostHealth, HealthReportsRainflowLoaded) {
    AIstudy::scheduler::Dispatcher dispatcher;
    AIstudy::scheduler::registerBuiltinSkills(dispatcher);

    auto health = aistudy_test::parseJsonObject(dispatcher.healthJson());
    EXPECT_EQ(health->getValue<std::string>("protocol"), "1");
    EXPECT_TRUE(health->getValue<bool>("ok"));
    EXPECT_GE(health->getValue<int>("skills_loaded"), 2);
    EXPECT_TRUE(health->has("checks"));
    auto checks = health->getObject("checks");
    ASSERT_TRUE(checks);
    EXPECT_TRUE(checks->getValue<bool>("poco"));
    EXPECT_TRUE(dispatcher.loadFailures().empty());
}
