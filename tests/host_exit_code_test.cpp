#include <gtest/gtest.h>

#include "host/skill_host.h"
#include "scheduler/skill_registry.h"
#include "skill_contract_util.h"

using AIstudy::host::HostExitCode;
using AIstudy::host::SkillHost;

TEST(HostExitCode, SuccessResponseReturnsZero) {
    const std::string response = R"({"protocol":"1","ok":true,"result":{}})";
    EXPECT_EQ(SkillHost::exitCodeFromExecuteResponse(response), HostExitCode::Success);
}

TEST(HostExitCode, FailedResponseReturnsTwo) {
    const std::string response =
        R"({"protocol":"1","ok":false,"error":{"code":1001,"category":"validation","message":"x"}})";
    EXPECT_EQ(SkillHost::exitCodeFromExecuteResponse(response), HostExitCode::ExecuteFailed);
}

TEST(HostExitCode, InvalidJsonReturnsThree) {
    EXPECT_EQ(SkillHost::exitCodeFromExecuteResponse("not json"), HostExitCode::HostError);
}

TEST(HostExitCode, RainflowExecuteOkViaHost) {
    AIstudy::host::SkillHost host;
    const std::string request = R"({
        "protocol":"1",
        "skill_id":"rainflow",
        "payload":{"load_history":[1,2,3,2,1],"method":"ThreePoint"}
    })";
    const std::string response = host.execute(request);
    EXPECT_EQ(SkillHost::exitCodeFromExecuteResponse(response), HostExitCode::Success);
    auto obj = aistudy_test::parseJsonObject(response);
    EXPECT_TRUE(obj->getValue<bool>("ok"));
}
