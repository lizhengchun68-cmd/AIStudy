#include "rainflow.h"
#include "internal/rainflow_three_point.h"
#include "internal/rainflow_four_point.h"
#include "internal/rainflow_modified_four_point.h"
#include "common/common_status/exception/error_codes.h"

namespace AIstudy {
namespace kernel {
namespace rainflow {

namespace internal {
std::unique_ptr<RainflowAlgorithm> createRainflowAlgorithm(RainflowMethod m) {
    switch (m) {
        case RainflowMethod::ThreePoint: return std::make_unique<RainflowThreePoint>();
        case RainflowMethod::FourPoint: return std::make_unique<RainflowFourPoint>();
        case RainflowMethod::ModifiedFourPoint: return std::make_unique<RainflowModifiedFourPoint>();
        default: return nullptr;
    }
}
} // internal

StatusOr<RainflowOutput> rainflowCounting(const RainflowInput& input) {
   auto algo = internal::createRainflowAlgorithm(input.method);
    if (!algo) {
        return StatusOr<RainflowOutput>::Fail(
            ErrorCodeWrapper((int)SystemError::NOT_IMPLEMENTED, "system")
        );
    }
    return algo->executeRainflowCounting(input);
}

} // namespace rainflow
} // namespace kernel
} // namespace AIstudy