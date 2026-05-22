#include "rainflow_four_point.h"
#include "rainflow_counting_core.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_category.h"

namespace AIstudy {
namespace kernel {
namespace rainflow {
namespace internal {

StatusOr<RainflowOutput> RainflowFourPoint::executeRainflowCounting(const RainflowInput& input) {
    if (!isValidRainflowParams(input.params)) {
        return StatusOr<RainflowOutput>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                             ErrorCategory::VALIDATION));
    }
    if (input.load_history.size() < 2) {
        return StatusOr<RainflowOutput>::Ok(RainflowOutput{});
    }
    const std::vector<CycleRecord> records =
        runFourPointPipeline(input.load_history, input.params);
    return StatusOr<RainflowOutput>::Ok(buildRainflowOutput(records));
}

} // namespace internal
} // namespace rainflow
} // namespace kernel
} // namespace AIstudy
