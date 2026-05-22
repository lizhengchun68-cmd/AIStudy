// Copyright 2026.

#include "load/rainflow/load_rainflow_mean_range_grid.h"

#include "load/rainflow/load_rainflow_counting.h"

#include <QString>

#include <algorithm>
#include <cmath>

namespace crack {
namespace load {

namespace {

// Second axis is cycle amplitude 0.5*(S_max-S_min) (= item.amplitude), not peak-to-peak.
void MeanRangeAxesFromRainflowRecord(const RainflowCycleRecord& item,
                                     double* mean_axis,
                                     double* range_axis) {
  if (mean_axis == nullptr || range_axis == nullptr) {
    return;
  }
  const double s_max = item.mean + item.amplitude;
  const double s_min = item.mean - item.amplitude;
  *mean_axis = 0.5 * (s_max + s_min);
  *range_axis = 0.5 * (s_max - s_min);  // == item.amplitude
}

}  // namespace

RainflowCountingMethod RainflowMethodFromComboIndex(int i) {
  switch (i) {
    case 0:
      return RainflowCountingMethod::kThreePoint;
    case 1:
      return RainflowCountingMethod::kFourPoint;
    case 2:
      return RainflowCountingMethod::kModifiedFourPoint;
    default:
      return RainflowCountingMethod::kThreePoint;
  }
}

bool BuildMeanRangeGridFromHistory(const std::vector<double>& history,
                                   RainflowCountingMethod method,
                                   const RainflowCountingOptions& opt,
                                   MeanRangeGrid* out) {
  if (out == nullptr) {
    return false;
  }
  out->mean_labels.clear();
  out->range_labels.clear();
  out->grid.clear();

  std::vector<RainflowCycleRecord> cycles;
  if (!rainflow::RunRainflowCounting(history, method, opt, cycles) ||
      cycles.empty()) {
    return false;
  }

  QStringList range_labels;
  QStringList mean_labels;
  for (const auto& item : cycles) {
    double mean_axis = 0.0;
    double range_axis = 0.0;
    MeanRangeAxesFromRainflowRecord(item, &mean_axis, &range_axis);
    range_labels << QString::number(range_axis, 'f', 3);
    mean_labels << QString::number(mean_axis, 'f', 3);
  }
  range_labels.removeDuplicates();
  mean_labels.removeDuplicates();
  std::sort(range_labels.begin(), range_labels.end());
  std::sort(mean_labels.begin(), mean_labels.end());

  const int n_mean = mean_labels.size();
  const int n_range = range_labels.size();
  if (n_mean == 0 || n_range == 0) {
    return false;
  }

  std::vector<std::vector<double>> grid(
      static_cast<size_t>(n_mean),
      std::vector<double>(static_cast<size_t>(n_range), 0.0));

  for (const auto& item : cycles) {
    double mean_axis = 0.0;
    double range_axis = 0.0;
    MeanRangeAxesFromRainflowRecord(item, &mean_axis, &range_axis);
    const int r = mean_labels.indexOf(QString::number(mean_axis, 'f', 3));
    const int c = range_labels.indexOf(QString::number(range_axis, 'f', 3));
    if (r >= 0 && c >= 0) {
      grid[static_cast<size_t>(r)][static_cast<size_t>(c)] =
          static_cast<double>(item.count);
    }
  }

  out->mean_labels = std::move(mean_labels);
  out->range_labels = std::move(range_labels);
  out->grid = std::move(grid);
  return true;
}

bool BuildMeanRangeGridFromPersistedBlocks(
    const std::vector<RainflowCycleRecord>& rows,
    MeanRangeGrid* out) {
  if (out == nullptr || rows.empty()) {
    return false;
  }
  out->mean_labels.clear();
  out->range_labels.clear();
  out->grid.clear();

  const int N = static_cast<int>(rows.size());
  const int B = 2 * N;
  double min_m = rows[0].mean;
  double max_m = rows[0].mean;
  double min_a = rows[0].amplitude;
  double max_a = rows[0].amplitude;
  for (const auto& r : rows) {
    min_m = (std::min)(min_m, r.mean);
    max_m = (std::max)(max_m, r.mean);
    min_a = (std::min)(min_a, r.amplitude);
    max_a = (std::max)(max_a, r.amplitude);
  }
  constexpr double kEps = 1e-9;
  if (max_m - min_m < kEps) {
    min_m -= 0.5;
    max_m += 0.5;
  }
  if (max_a - min_a < kEps) {
    min_a -= 0.5;
    max_a += 0.5;
  }
  const double span_m = max_m - min_m;
  const double span_a = max_a - min_a;

  QStringList mean_labels;
  QStringList range_labels;
  mean_labels.reserve(B);
  range_labels.reserve(B);
  for (int i = 0; i < B; ++i) {
    const double mc =
        min_m + (static_cast<double>(i) + 0.5) * span_m / static_cast<double>(B);
    mean_labels << QString::number(mc, 'f', 3);
    const double ac =
        min_a + (static_cast<double>(i) + 0.5) * span_a / static_cast<double>(B);
    range_labels << QString::number(ac, 'f', 3);
  }

  std::vector<std::vector<double>> grid(
      static_cast<size_t>(B), std::vector<double>(static_cast<size_t>(B), 0.0));

  for (const auto& r : rows) {
    int im = static_cast<int>(
        std::floor((r.mean - min_m) / span_m * static_cast<double>(B)));
    int ia = static_cast<int>(
        std::floor((r.amplitude - min_a) / span_a * static_cast<double>(B)));
    if (im < 0) {
      im = 0;
    }
    if (ia < 0) {
      ia = 0;
    }
    if (im >= B) {
      im = B - 1;
    }
    if (ia >= B) {
      ia = B - 1;
    }
    grid[static_cast<size_t>(im)][static_cast<size_t>(ia)] += r.count;
  }

  out->mean_labels = std::move(mean_labels);
  out->range_labels = std::move(range_labels);
  out->grid = std::move(grid);
  return true;
}

}  // namespace load
}  // namespace crack
