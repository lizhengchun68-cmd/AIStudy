#include <gtest/gtest.h>

#include "common/status/exception/error_codes.h"
#include "kernel/rainflow/rainflow.h"

#include <cmath>
#include <string>
#include <vector>

namespace {

using AIstudy::ValidationError;
using AIstudy::kernel::rainflow::RainflowInput;
using AIstudy::kernel::rainflow::RainflowMethod;
using AIstudy::kernel::rainflow::RainflowOutput;
using AIstudy::kernel::rainflow::RainflowParams;
using AIstudy::kernel::rainflow::rainflowCounting;

constexpr double kTol = 1e-9;

RainflowInput makeInput(RainflowMethod method,
                        std::vector<double> load_history,
                        double threshold = 0.2,
                        size_t grads = 50) {
    RainflowInput input;
    input.method = method;
    input.load_history = std::move(load_history);
    input.params.threshold = threshold;
    input.params.grads = grads;
    return input;
}

void expectSingleCycle(const RainflowOutput& output,
                       double amplitude,
                       double mean,
                       size_t count) {
    ASSERT_EQ(output.items.size(), 1u) << "expected one cycle bucket";
    EXPECT_NEAR(output.items[0].amplitude, amplitude, kTol);
    EXPECT_NEAR(output.items[0].mean, mean, kTol);
    EXPECT_EQ(output.items[0].count, count);
    EXPECT_EQ(output.num_cycles, count);
}

struct RainflowMethodParam {
    RainflowMethod method;
    const char* name;
};

std::string methodTestName(const ::testing::TestParamInfo<RainflowMethodParam>& info) {
    return info.param.name;
}

const RainflowMethodParam kAllRainflowMethods[] = {
    {RainflowMethod::ThreePoint, "ThreePoint"},
    {RainflowMethod::FourPoint, "FourPoint"},
    {RainflowMethod::ModifiedFourPoint, "ModifiedFourPoint"},
};

} // namespace

class RainflowCountingMethodTest : public ::testing::TestWithParam<RainflowMethodParam> {};

TEST_P(RainflowCountingMethodTest, CanonicalTriangleLoadHistory) {
    const auto res = rainflowCounting(
        makeInput(GetParam().method, {1.0, 2.0, 3.0, 2.0, 1.0}, 0.2, 50));
    ASSERT_TRUE(res.ok()) << GetParam().name;
    expectSingleCycle(res.value(), 1.0, 2.0, 1);
}

TEST_P(RainflowCountingMethodTest, TwoPointAlternatingLoadHistory) {
    const auto res =
        rainflowCounting(makeInput(GetParam().method, {0.0, 10.0}, 0.2, 50));
    ASSERT_TRUE(res.ok()) << GetParam().name;
    expectSingleCycle(res.value(), 5.0, 5.0, 1);
}

TEST_P(RainflowCountingMethodTest, ZeroThresholdKeepsCycles) {
    const auto res =
        rainflowCounting(makeInput(GetParam().method, {1.0, 2.0, 3.0, 2.0, 1.0}, 0.0, 50));
    ASSERT_TRUE(res.ok()) << GetParam().name;
    expectSingleCycle(res.value(), 1.0, 2.0, 1);
}

INSTANTIATE_TEST_SUITE_P(AllMethods,
                         RainflowCountingMethodTest,
                         ::testing::ValuesIn(kAllRainflowMethods),
                         methodTestName);

TEST(RainflowCountingTest, ThreePointExplicit) {
    const auto res = rainflowCounting(
        makeInput(RainflowMethod::ThreePoint, {1.0, 2.0, 3.0, 2.0, 1.0}));
    ASSERT_TRUE(res.ok());
    expectSingleCycle(res.value(), 1.0, 2.0, 1);
}

TEST(RainflowCountingTest, FourPointExplicit) {
    const auto res = rainflowCounting(
        makeInput(RainflowMethod::FourPoint, {1.0, 2.0, 3.0, 2.0, 1.0}));
    ASSERT_TRUE(res.ok());
    expectSingleCycle(res.value(), 1.0, 2.0, 1);
}

TEST(RainflowCountingTest, ModifiedFourPointExplicit) {
    const auto res = rainflowCounting(
        makeInput(RainflowMethod::ModifiedFourPoint, {1.0, 2.0, 3.0, 2.0, 1.0}));
    ASSERT_TRUE(res.ok());
    expectSingleCycle(res.value(), 1.0, 2.0, 1);
}

TEST(RainflowCountingTest, ConstantLoadReturnsEmpty) {
    for (RainflowMethod method :
         {RainflowMethod::ThreePoint, RainflowMethod::FourPoint,
          RainflowMethod::ModifiedFourPoint}) {
        const auto res =
            rainflowCounting(makeInput(method, {5.0, 5.0, 5.0, 5.0}, 0.2, 50));
        ASSERT_TRUE(res.ok());
        EXPECT_TRUE(res.value().items.empty());
        EXPECT_EQ(res.value().num_cycles, 0u);
    }
}

TEST(RainflowCountingTest, SinglePointHistoryReturnsEmpty) {
    const auto res =
        rainflowCounting(makeInput(RainflowMethod::ThreePoint, {3.0}, 0.2, 50));
    ASSERT_TRUE(res.ok());
    EXPECT_TRUE(res.value().items.empty());
    EXPECT_EQ(res.value().num_cycles, 0u);
}

TEST(RainflowCountingTest, InvalidThresholdAtOneFails) {
    RainflowInput input = makeInput(RainflowMethod::ThreePoint, {1.0, 2.0, 3.0});
    input.params.threshold = 1.0;
    const auto res = rainflowCounting(input);
    ASSERT_FALSE(res.ok());
    EXPECT_EQ(res.status().category(), "validation");
    EXPECT_EQ(res.status().code(), static_cast<int>(ValidationError::INVALID_INPUT));
}

TEST(RainflowCountingTest, InvalidNegativeThresholdFails) {
    RainflowInput input = makeInput(RainflowMethod::FourPoint, {1.0, 2.0, 3.0});
    input.params.threshold = -0.1;
    const auto res = rainflowCounting(input);
    ASSERT_FALSE(res.ok());
    EXPECT_EQ(res.status().code(), static_cast<int>(ValidationError::INVALID_INPUT));
}

TEST(RainflowCountingTest, InvalidZeroGradsFails) {
    RainflowInput input = makeInput(RainflowMethod::ModifiedFourPoint, {1.0, 2.0, 3.0});
    input.params.grads = 0;
    const auto res = rainflowCounting(input);
    ASSERT_FALSE(res.ok());
    EXPECT_EQ(res.status().code(), static_cast<int>(ValidationError::INVALID_INPUT));
}
