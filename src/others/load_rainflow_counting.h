// Copyright 2026. Rainflow counting API

#ifndef CRACK_LOAD_RAINFLOW_LOAD_RAINFLOW_COUNTING_H_
#define CRACK_LOAD_RAINFLOW_LOAD_RAINFLOW_COUNTING_H_

#include "load/load_common_define.h"

#include <vector>

namespace crack {
namespace load {
namespace rainflow {

bool RunRainflowCounting(const std::vector<double>& load_history,
                         RainflowCountingMethod method,
                         const RainflowCountingOptions& options,
                         std::vector<RainflowCycleRecord>& out_cycles);

}  // namespace rainflow
}  // namespace load
}  // namespace crack

#endif  // CRACK_LOAD_RAINFLOW_LOAD_RAINFLOW_COUNTING_H_
