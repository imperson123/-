#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "tile_compute.h"

namespace tmac_sa {

// Split M_logical across n_threads and run ComputeFn per tile (feasibility D5).
struct TilePlan {
  int m_logical_total = 0;
  int k = 0;
  int n = 1;
  int bits = 2;
  int n_threads = 1;
};

// Returns per-thread (m_begin, m_rows) covering [0, m_logical_total).
std::vector<std::pair<int, int>> SplitMTiles(int m_logical_total, int n_threads);

// Parallel dispatch: fn(thread_id, m_begin, m_rows). Returns false if any fn returns false.
bool RunParallelTiles(int n_threads, const std::function<bool(int, int, int)>& fn);

// Build TileJob list for contiguous A/C packing (row-major logical M).
// A_base layout assumed: each logical row block is caller-managed; here we only
// slice C and m_logical — A_tile left to caller via offsets in fn.
int RunTiledQGeMMParallel(const TileJob* jobs, size_t n_jobs, int n_threads);

}  // namespace tmac_sa
