#include "tile_compute.h"

#include "t-mac/kernels.h"

namespace tmac_sa {

int RunTiledQGeMM(const TileJob* jobs, size_t n_jobs) {
  if (!jobs && n_jobs > 0) {
    return -1;
  }
  for (size_t i = 0; i < n_jobs; ++i) {
    const TileJob& j = jobs[i];
    const int m = j.m_logical * j.bits;
    int ret = qgemm_lut_int8(m, j.k, j.n, j.bits, j.A_tile, j.qlut, j.scales_tile,
                             j.lut_scales, j.lut_biases, j.C_tile);
    if (ret != 0) {
      return ret;
    }
  }
  return 0;
}

}  // namespace tmac_sa
