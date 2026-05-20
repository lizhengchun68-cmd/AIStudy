#ifndef RAINFLOW_MODIFIED_FOUR_POINT_H
#define RAINFLOW_MODIFIED_FOUR_POINT_H

#include "rainflow_algorithm.h"

namespace AIstudy {
namespace kernel {
namespace rainflow {
namespace internal {    

class RainflowModifiedFourPoint final : public RainflowAlgorithm {
public:
    StatusOr<RainflowOutput> executeRainflowCounting(const RainflowInput& input) override;
};

} // namespace internal
} // namespace rainflow
} // namespace kernel
} // namespace AIstudy

#endif