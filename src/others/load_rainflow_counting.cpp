
#include "load/rainflow/load_rainflow_counting.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace crack {
namespace load {
namespace rainflow {
namespace {

constexpr double kMergeAbsTol = 0.001;
constexpr size_t kMinGradsCount = 10;
constexpr double kBoundaryMatchEps = 1e-9;

struct PartCycle {
  double max_value{0.0};
  double min_value{0.0};
  double count_frac{1.0};
};

static PartCycle MakePartCycleFromTwoLevels(double level_a, double level_b, double count_frac) {
  PartCycle part;
  part.max_value = std::max(level_a, level_b);
  part.min_value = std::min(level_a, level_b);
  part.count_frac = count_frac;
  return part;
}

static double PartCycleStressAmplitude(const PartCycle& part) {
  return (part.max_value - part.min_value) * 0.5;
}

static std::vector<double> ThreePointReorderByMaxAbs(const std::vector<double>& load_history) {
  size_t pivot_index = 0;
  double max_abs_value = std::abs(load_history[0]);
  for (size_t i = 1; i < load_history.size(); ++i) {
    const double abs_value = std::abs(load_history[i]);
    if (abs_value > max_abs_value) {
      max_abs_value = abs_value;
      pivot_index = i;
    }
  }
  std::vector<double> rotated_closed;
  rotated_closed.reserve(load_history.size() + 1);
  for (size_t i = pivot_index; i < load_history.size(); ++i) {
    rotated_closed.push_back(load_history[i]);
  }
  for (size_t i = 0; i < pivot_index; ++i) {
    rotated_closed.push_back(load_history[i]);
  }
  rotated_closed.push_back(load_history[pivot_index]);
  return rotated_closed;
}

static bool IsBinBoundaryValue(double sample, double min_v, double span, size_t bins) {
  for (size_t grid_i = 0; grid_i <= bins; ++grid_i) {
    const double boundary = min_v + (static_cast<double>(grid_i) / static_cast<double>(bins)) * span;
    const double scale = 1.0 + std::max(std::abs(sample), std::abs(boundary));
    if (std::abs(sample - boundary) <= kBoundaryMatchEps * scale) {
      return true;
    }
  }
  return false;
}

struct ThreePointGradeResult {
  std::vector<double> values;
  bool aborted_constant{false};
};

static ThreePointGradeResult ThreePointGradeBoundaryPreserving(const std::vector<double>& load_series,
                                                               size_t grads_count) {
  ThreePointGradeResult result;
  if (grads_count < 2) {
    result.values = load_series;
    return result;
  }
  const double min_v = *std::min_element(load_series.begin(), load_series.end());
  const double max_v = *std::max_element(load_series.begin(), load_series.end());
  const double span_check = std::abs(max_v - min_v);
  const double magnitude = std::max(1.0, std::max(std::abs(min_v), std::abs(max_v)));
  if (span_check <= 1e-6 * magnitude) {
    result.aborted_constant = true;
    return result;
  }
  const double span = max_v - min_v;
  const double bin_width = span / static_cast<double>(grads_count);
  result.values.reserve(load_series.size());
  for (double sample : load_series) {
    if (IsBinBoundaryValue(sample, min_v, span, grads_count)) {
      result.values.push_back(sample);
    } else {
      double normalized_position = (sample - min_v) / bin_width;
      int bin_index = static_cast<int>(std::floor(normalized_position));
      if (bin_index < 0) {
        bin_index = 0;
      }
      if (bin_index >= static_cast<int>(grads_count)) {
        bin_index = static_cast<int>(grads_count) - 1;
      }
      const double bin_mid = min_v + (static_cast<double>(bin_index) + 0.5) * bin_width;
      result.values.push_back(bin_mid);
    }
  }
  return result;
}

static bool IsStrictPeakOrValley(double left, double mid, double right) {
  return (mid > left && mid > right) || (mid < left && mid < right);
}

static std::vector<double> RemoveNonPeakValleyPoints(std::vector<double> series) {
  if (series.size() < 3) {
    return series;
  }
  while (true) {
    bool removed_one = false;
    for (size_t i = 1; i + 1 < series.size(); ++i) {
      const double left = series[i - 1];
      const double mid = series[i];
      const double right = series[i + 1];
      if (!IsStrictPeakOrValley(left, mid, right)) {
        series.erase(series.begin() + static_cast<std::ptrdiff_t>(i));
        removed_one = true;
        break;
      }
    }
    if (!removed_one) {
      break;
    }
  }
  return series;
}

static std::vector<PartCycle> ModifiedFourPointOnExtrema(std::vector<double> extrema_sequence) {
  std::vector<PartCycle> part_cycles;
  part_cycles.reserve(extrema_sequence.size());
  while (extrema_sequence.size() >= 4) {
    const double p1 = extrema_sequence[0];
    const double p2 = extrema_sequence[1];
    const double p3 = extrema_sequence[2];
    const double p4 = extrema_sequence[3];
    const double range_mid = std::abs(p2 - p3);
    const double range_left = std::abs(p1 - p2);
    const double range_right = std::abs(p3 - p4);
    if (range_mid <= range_left && range_mid <= range_right) {
      part_cycles.push_back(MakePartCycleFromTwoLevels(p2, p3, 1.0));
      extrema_sequence.erase(extrema_sequence.begin() + 1, extrema_sequence.begin() + 3);
    } else {
      extrema_sequence.erase(extrema_sequence.begin());
    }
  }
  for (size_t i = 0; i + 1 < extrema_sequence.size(); ++i) {
    part_cycles.push_back(
        MakePartCycleFromTwoLevels(extrema_sequence[i], extrema_sequence[i + 1], 0.5));
  }
  return part_cycles;
}

static std::vector<PartCycle> ThreePointStackOnExtrema(const std::vector<double>& extrema) {
  std::vector<PartCycle> cycles;
  if (extrema.size() < 2) {
    return cycles;
  }
  cycles.reserve(extrema.size());
  std::vector<double> range_stack;
  range_stack.reserve(extrema.size());
  for (double load_level : extrema) {
    range_stack.push_back(load_level);
    while (range_stack.size() >= 3) {
      const size_t stack_size = range_stack.size();
      const double x0 = range_stack[stack_size - 3];
      const double x1 = range_stack[stack_size - 2];
      const double x2 = range_stack[stack_size - 1];
      const double range_prev = std::abs(x1 - x0);
      const double range_new = std::abs(x2 - x1);
      if (range_new <= range_prev) {
        cycles.push_back(MakePartCycleFromTwoLevels(x1, x2, 1.0));
        range_stack.erase(range_stack.end() - 2, range_stack.end());
      } else {
        break;
      }
    }
  }
  for (size_t i = 0; i + 1 < range_stack.size(); ++i) {
    cycles.push_back(MakePartCycleFromTwoLevels(range_stack[i], range_stack[i + 1], 0.5));
  }
  return cycles;
}

static std::vector<PartCycle> ApplyRelativeThresholdPartCycles(const std::vector<PartCycle>& part_cycles,
                                                               double threshold) {
  if (part_cycles.empty()) {
    return {};
  }
  double max_amplitude = 0.0;
  for (const PartCycle& part : part_cycles) {
    max_amplitude = std::max(max_amplitude, PartCycleStressAmplitude(part));
  }
  if (max_amplitude <= 0.0) {
    return part_cycles;
  }
  const double amplitude_floor = max_amplitude * threshold;
  std::vector<PartCycle> kept;
  kept.reserve(part_cycles.size());
  for (const PartCycle& part : part_cycles) {
    if (PartCycleStressAmplitude(part) >= amplitude_floor) {
      kept.push_back(part);
    }
  }
  return kept;
}

static std::vector<RainflowCycleRecord> MergePartCyclesWithAbsTolerance(const std::vector<PartCycle>& part_cycles,
                                                                        double tol) {
  if (part_cycles.empty()) {
    return {};
  }
  const double inv_tol = 1.0 / tol;
  struct Agg {
    double weighted_max{0.0};
    double weighted_min{0.0};
    double weight_sum{0.0};
  };
  std::map<std::pair<long long, long long>, Agg> bucket;
  for (const PartCycle& part : part_cycles) {
    const auto key = std::make_pair(static_cast<long long>(std::llround(part.max_value * inv_tol)),
                                    static_cast<long long>(std::llround(part.min_value * inv_tol)));
    Agg& agg = bucket[key];
    agg.weighted_max += part.max_value * part.count_frac;
    agg.weighted_min += part.min_value * part.count_frac;
    agg.weight_sum += part.count_frac;
  }
  std::vector<RainflowCycleRecord> records;
  records.reserve(bucket.size());
  for (const auto& entry : bucket) {
    const Agg& agg = entry.second;
    if (agg.weight_sum <= 0.0) {
      continue;
    }
    const double rep_max = agg.weighted_max / agg.weight_sum;
    const double rep_min = agg.weighted_min / agg.weight_sum;
    RainflowCycleRecord record;
    record.amplitude = (rep_max - rep_min) * 0.5;
    record.mean = (rep_max + rep_min) * 0.5;
    record.count = agg.weight_sum;
    records.push_back(record);
  }
  return records;
}

static std::vector<RainflowCycleRecord> FinishWithThresholdAndMerge(std::vector<PartCycle> part_cycles,
                                                                    const RainflowCountingOptions& options) {
  part_cycles = ApplyRelativeThresholdPartCycles(part_cycles, options.threshold);
  return MergePartCyclesWithAbsTolerance(part_cycles, kMergeAbsTol);
}

// Full rainflow pipeline steps on raw load_history.
static std::vector<RainflowCycleRecord> RunThreePointPipeline(const std::vector<double>& load_history,
                                                                 const RainflowCountingOptions& options) {
  const std::vector<double>& rotated_closed_load = ThreePointReorderByMaxAbs(load_history);
  ThreePointGradeResult graded = ThreePointGradeBoundaryPreserving(rotated_closed_load, options.grads_count);
  if (graded.aborted_constant) {
    return {};
  }
  const std::vector<double>& extrema = RemoveNonPeakValleyPoints(graded.values);
  return FinishWithThresholdAndMerge(ThreePointStackOnExtrema(extrema), options);
}

static std::vector<PartCycle> FourPointStackOnExtrema(const std::vector<double>& extrema) {
  std::vector<PartCycle> cycles;
  if (extrema.size() < 2) {
    return cycles;
  }
  cycles.reserve(extrema.size());
  std::vector<double> range_stack;
  range_stack.reserve(extrema.size());
  for (double load_level : extrema) {
    range_stack.push_back(load_level);
    while (range_stack.size() >= 4) {
      const size_t stack_size = range_stack.size();
      const double x0 = range_stack[stack_size - 4];
      const double x1 = range_stack[stack_size - 3];
      const double x2 = range_stack[stack_size - 2];
      const double x3 = range_stack[stack_size - 1];
      const double range_mid = std::abs(x1 - x2);
      const double range_left = std::abs(x0 - x1);
      const double range_right = std::abs(x2 - x3);
      if (range_mid <= range_left && range_mid <= range_right) {
        cycles.push_back(MakePartCycleFromTwoLevels(x1, x2, 1.0));
        range_stack.erase(range_stack.end() - 3, range_stack.end() - 1);
      } else {
        break;
      }
    }
  }
  const size_t residual_count = range_stack.size();
  if (residual_count == 3) {
    const double endpoint_repeat = range_stack[0];
    const double interior_extremum = range_stack[1];
    cycles.push_back(MakePartCycleFromTwoLevels(endpoint_repeat, interior_extremum, 1.0));
  }
  return cycles;
}

static std::vector<RainflowCycleRecord> RunFourPointPipeline(const std::vector<double>& load_history,
                                                                const RainflowCountingOptions& options) {
  const std::vector<double>& rotated_closed_load = ThreePointReorderByMaxAbs(load_history);
  ThreePointGradeResult graded = ThreePointGradeBoundaryPreserving(rotated_closed_load, options.grads_count);
  if (graded.aborted_constant) {
    return {};
  }
  const std::vector<double>& extrema = RemoveNonPeakValleyPoints(graded.values);
  return FinishWithThresholdAndMerge(FourPointStackOnExtrema(extrema), options);
}

static std::vector<RainflowCycleRecord> RunModifiedFourPointPipeline(const std::vector<double>& load_history,
                                                                      const RainflowCountingOptions& options) {
  ThreePointGradeResult graded = ThreePointGradeBoundaryPreserving(load_history, options.grads_count);
  if (graded.aborted_constant) {
    return {};
  }
  std::vector<double> extrema = RemoveNonPeakValleyPoints(graded.values);
  return FinishWithThresholdAndMerge(ModifiedFourPointOnExtrema(std::move(extrema)), options);
}

}  // namespace

bool RunRainflowCounting(const std::vector<double>& load_history,
                         RainflowCountingMethod method,
                         const RainflowCountingOptions& options,
                         std::vector<RainflowCycleRecord>& out_cycles) {
  out_cycles.clear();
  if (load_history.size() < 2) {
    return false;
  }
  if (!(options.threshold > 0.0 && options.threshold < 1.0)) {
    return false;
  }
  if (options.grads_count < kMinGradsCount) {
    return false;
  }
  switch (method) {
    case RainflowCountingMethod::kThreePoint:
      out_cycles = RunThreePointPipeline(load_history, options);
      return true;
    case RainflowCountingMethod::kFourPoint:
      out_cycles = RunFourPointPipeline(load_history, options);
      return true;
    case RainflowCountingMethod::kModifiedFourPoint:
      out_cycles = RunModifiedFourPointPipeline(load_history, options);
      return true;
    default:
      return false;
  }
}
}  // namespace rainflow
}  // namespace load
}  // namespace crack
