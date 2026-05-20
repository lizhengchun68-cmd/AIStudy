#ifndef RAINFLOW_ALGORITHM_H
#define RAINFLOW_ALGORITHM_H

#include "kernel/rainflow/rainflow.h"
#include "common/status/status_or.h"
#include "common/status/exception/exception.h"

namespace AIstudy {
namespace kernel {
namespace rainflow {
namespace internal {

class RainflowAlgorithm {
public:
    virtual ~RainflowAlgorithm() = default;
    virtual StatusOr<RainflowOutput> executeRainflowCounting(const RainflowInput& input) = 0;
};

} // internal
} // rainflow
} // kernel
} // AIstudy

#endif