// Copyright 2026. Build Mean-Amplitude-Count grid from load_history (shared by table + 3D).

#ifndef CRACK_LOAD_TREE_LOAD_RAINFLOW_MEAN_RANGE_GRID_H_
#define CRACK_LOAD_TREE_LOAD_RAINFLOW_MEAN_RANGE_GRID_H_

#include "load/load_common_define.h"

#include <QStringList>
#include <vector>

namespace crack {
namespace load {

struct MeanRangeGrid {
  QStringList mean_labels;
  QStringList range_labels;
  std::vector<std::vector<double>> grid;
};

RainflowCountingMethod RainflowMethodFromComboIndex(int index);

// Runs rainflow then bins to the same Mean/Amplitude grid (struct field: range_labels).
// Returns false if the engine fails, cycles are empty, or no grid dimensions (clears *out).
bool BuildMeanRangeGridFromHistory(const std::vector<double>& history,
                                   RainflowCountingMethod method,
                                   const RainflowCountingOptions& opt,
                                   MeanRangeGrid* out);

// Blocks load: bin persisted (mean, amplitude, count) rows into a B×B grid with
// B = 2*N, N = number of rows; counts accumulate in cells.
bool BuildMeanRangeGridFromPersistedBlocks(
    const std::vector<RainflowCycleRecord>& rows,
    MeanRangeGrid* out);

}  // namespace load
}  // namespace crack

#endif  // CRACK_LOAD_TREE_LOAD_RAINFLOW_MEAN_RANGE_GRID_H_
