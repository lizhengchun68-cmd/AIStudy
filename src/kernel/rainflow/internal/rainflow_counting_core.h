#ifndef RAINFLOW_COUNTING_CORE_H
#define RAINFLOW_COUNTING_CORE_H

#include "kernel/rainflow/rainflow.h"

#include <vector>

namespace AIstudy {
namespace kernel {
namespace rainflow {
namespace internal {

struct CycleRecord {
    double amplitude = 0.0;
    double mean = 0.0;
    double count = 0.0;
};

bool isValidRainflowParams(const RainflowParams& params);

std::vector<CycleRecord> runThreePointPipeline(const std::vector<double>& load_history,
                                               const RainflowParams& params);

std::vector<CycleRecord> runFourPointPipeline(const std::vector<double>& load_history,
                                              const RainflowParams& params);

std::vector<CycleRecord> runModifiedFourPointPipeline(const std::vector<double>& load_history,
                                                      const RainflowParams& params);

RainflowOutput buildRainflowOutput(const std::vector<CycleRecord>& records);

} // namespace internal
} // namespace rainflow
} // namespace kernel
} // namespace AIstudy

#endif
