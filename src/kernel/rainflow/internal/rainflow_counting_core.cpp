#include "rainflow_counting_core.h"


#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

namespace AIstudy {
namespace kernel {
namespace rainflow {
namespace internal {

namespace {

constexpr double kMergeAbsTol = 0.001;
constexpr double kBoundaryMatchEps = 1e-9;

struct PartCycle {
    double max_value{0.0};
    double min_value{0.0};
    double count_frac{1.0};
};

PartCycle makePartCycleFromTwoLevels(double level_a, double level_b, double count_frac) {
    PartCycle part;
    part.max_value = std::max(level_a, level_b);
    part.min_value = std::min(level_a, level_b);
    part.count_frac = count_frac;
    return part;
}

double partCycleStressAmplitude(const PartCycle& part) {
    return (part.max_value - part.min_value) * 0.5;
}

std::vector<double> threePointReorderByMaxAbs(const std::vector<double>& load_history) {
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

bool isBinBoundaryValue(double sample, double min_v, double span, size_t bins) {
    for (size_t grid_i = 0; grid_i <= bins; ++grid_i) {
        const double boundary =
            min_v + (static_cast<double>(grid_i) / static_cast<double>(bins)) * span;
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

ThreePointGradeResult threePointGradeBoundaryPreserving(const std::vector<double>& load_series,
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
        if (isBinBoundaryValue(sample, min_v, span, grads_count)) {
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

bool isStrictPeakOrValley(double left, double mid, double right) {
    return (mid > left && mid > right) || (mid < left && mid < right);
}

std::vector<double> removeNonPeakValleyPoints(std::vector<double> series) {
    if (series.size() < 3) {
        return series;
    }
    while (true) {
        bool removed_one = false;
        for (size_t i = 1; i + 1 < series.size(); ++i) {
            const double left = series[i - 1];
            const double mid = series[i];
            const double right = series[i + 1];
            if (!isStrictPeakOrValley(left, mid, right)) {
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

std::vector<PartCycle> modifiedFourPointOnExtrema(std::vector<double> extrema_sequence) {
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
            part_cycles.push_back(makePartCycleFromTwoLevels(p2, p3, 1.0));
            extrema_sequence.erase(extrema_sequence.begin() + 1, extrema_sequence.begin() + 3);
        } else {
            extrema_sequence.erase(extrema_sequence.begin());
        }
    }
    for (size_t i = 0; i + 1 < extrema_sequence.size(); ++i) {
        part_cycles.push_back(
            makePartCycleFromTwoLevels(extrema_sequence[i], extrema_sequence[i + 1], 0.5));
    }
    return part_cycles;
}

std::vector<PartCycle> threePointStackOnExtrema(const std::vector<double>& extrema) {
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
                cycles.push_back(makePartCycleFromTwoLevels(x1, x2, 1.0));
                range_stack.erase(range_stack.end() - 2, range_stack.end());
            } else {
                break;
            }
        }
    }
    for (size_t i = 0; i + 1 < range_stack.size(); ++i) {
        cycles.push_back(makePartCycleFromTwoLevels(range_stack[i], range_stack[i + 1], 0.5));
    }
    return cycles;
}

std::vector<PartCycle> applyRelativeThresholdPartCycles(const std::vector<PartCycle>& part_cycles,
                                                        double threshold) {
    if (part_cycles.empty()) {
        return {};
    }
    double max_amplitude = 0.0;
    for (const PartCycle& part : part_cycles) {
        max_amplitude = std::max(max_amplitude, partCycleStressAmplitude(part));
    }
    if (max_amplitude <= 0.0) {
        return part_cycles;
    }
    const double amplitude_floor = max_amplitude * threshold;
    std::vector<PartCycle> kept;
    kept.reserve(part_cycles.size());
    for (const PartCycle& part : part_cycles) {
        if (partCycleStressAmplitude(part) >= amplitude_floor) {
            kept.push_back(part);
        }
    }
    return kept;
}

std::vector<CycleRecord> mergePartCyclesWithAbsTolerance(const std::vector<PartCycle>& part_cycles,
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
    std::vector<CycleRecord> records;
    records.reserve(bucket.size());
    for (const auto& entry : bucket) {
        const Agg& agg = entry.second;
        if (agg.weight_sum <= 0.0) {
            continue;
        }
        const double rep_max = agg.weighted_max / agg.weight_sum;
        const double rep_min = agg.weighted_min / agg.weight_sum;
        CycleRecord record;
        record.amplitude = (rep_max - rep_min) * 0.5;
        record.mean = (rep_max + rep_min) * 0.5;
        record.count = agg.weight_sum;
        records.push_back(record);
    }
    return records;
}

std::vector<CycleRecord> finishWithThresholdAndMerge(std::vector<PartCycle> part_cycles,
                                                      const RainflowParams& params) {
    if (params.threshold > 0.0) {
        part_cycles = applyRelativeThresholdPartCycles(part_cycles, params.threshold);
    }
    return mergePartCyclesWithAbsTolerance(part_cycles, kMergeAbsTol);
}

size_t effectiveGradsCount(size_t grads) {
    return std::max(grads, size_t{2});
}

std::vector<CycleRecord> runThreePointPipelineImpl(const std::vector<double>& load_history,
                                                   const RainflowParams& params) {
    const std::vector<double> rotated_closed_load = threePointReorderByMaxAbs(load_history);
    ThreePointGradeResult graded = threePointGradeBoundaryPreserving(
        rotated_closed_load, effectiveGradsCount(params.grads));
    if (graded.aborted_constant) {
        return {};
    }
    const std::vector<double> extrema = removeNonPeakValleyPoints(graded.values);
    return finishWithThresholdAndMerge(threePointStackOnExtrema(extrema), params);
}

std::vector<PartCycle> fourPointStackOnExtrema(const std::vector<double>& extrema) {
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
                cycles.push_back(makePartCycleFromTwoLevels(x1, x2, 1.0));
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
        cycles.push_back(makePartCycleFromTwoLevels(endpoint_repeat, interior_extremum, 1.0));
    }
    return cycles;
}

std::vector<CycleRecord> runFourPointPipelineImpl(const std::vector<double>& load_history,
                                                  const RainflowParams& params) {
    const std::vector<double> rotated_closed_load = threePointReorderByMaxAbs(load_history);
    ThreePointGradeResult graded = threePointGradeBoundaryPreserving(
        rotated_closed_load, effectiveGradsCount(params.grads));
    if (graded.aborted_constant) {
        return {};
    }
    const std::vector<double> extrema = removeNonPeakValleyPoints(graded.values);
    return finishWithThresholdAndMerge(fourPointStackOnExtrema(extrema), params);
}

std::vector<CycleRecord> runModifiedFourPointPipelineImpl(const std::vector<double>& load_history,
                                                        const RainflowParams& params) {
    ThreePointGradeResult graded = threePointGradeBoundaryPreserving(
        load_history, effectiveGradsCount(params.grads));
    if (graded.aborted_constant) {
        return {};
    }
    std::vector<double> extrema = removeNonPeakValleyPoints(graded.values);
    return finishWithThresholdAndMerge(modifiedFourPointOnExtrema(std::move(extrema)), params);
}

} // namespace

bool isValidRainflowParams(const RainflowParams& params) {
    return params.threshold >= 0.0 && params.threshold < 1.0 && params.grads >= 1;
}

std::vector<CycleRecord> runThreePointPipeline(const std::vector<double>& load_history,
                                               const RainflowParams& params) {
    return runThreePointPipelineImpl(load_history, params);
}

std::vector<CycleRecord> runFourPointPipeline(const std::vector<double>& load_history,
                                              const RainflowParams& params) {
    return runFourPointPipelineImpl(load_history, params);
}

std::vector<CycleRecord> runModifiedFourPointPipeline(const std::vector<double>& load_history,
                                                      const RainflowParams& params) {
    return runModifiedFourPointPipelineImpl(load_history, params);
}

RainflowOutput buildRainflowOutput(const std::vector<CycleRecord>& records) {
    RainflowOutput out;
    double total_cycles = 0.0;
    out.items.reserve(records.size());
    for (const CycleRecord& record : records) {
        RainflowResultItem item;
        item.amplitude = record.amplitude;
        item.mean = record.mean;
        item.count = static_cast<size_t>(std::llround(record.count));
        out.items.push_back(item);
        total_cycles += record.count;
    }
    out.num_cycles = static_cast<size_t>(std::llround(total_cycles));
    return out;
}

} // namespace internal
} // namespace rainflow
} // namespace kernel
} // namespace AIstudy
