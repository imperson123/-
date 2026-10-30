#pragma once

#include <cstddef>
#include <cstdint>

namespace tmac_sa {

// Tile M/N and call qgemm_lut_int8 per tile (feasibility D5: caller owns parallelism).
// bits=2, g=4; M_logical rows; K cols; N=1 decode GEMV typical.
struct TileJob {
  int m_logical;
  int k;
  int n;
  int bits;
  void* A_tile;
  void* scales_tile;
  void* qlut;
  void* lut_scales;
  void* lut_biases;
  void* C_tile;
};

// Returns 0 on success (all tiles dispatched).
int RunTiledQGeMM(const TileJob* jobs, size_t n_jobs);

}  // namespace tmac_sa
