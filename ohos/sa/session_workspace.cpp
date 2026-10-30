#include "session_workspace.h"

#include <cstdlib>
#include <cstring>

// qgemm_lut_int8 / preprocessor_int8 come via tmac_gemm_wrapper.h → kernels.h
// (kernels.h has no include guard — do not include it again here).

namespace tmac_sa {
namespace {

void* AlignAlloc(size_t nbytes) {
  void* p = nullptr;
  if (posix_memalign(&p, TMAC::kAllocAlignment, nbytes) != 0 || !p) {
    return nullptr;
  }
  std::memset(p, 0, nbytes);
  return p;
}

}  // namespace

SessionWorkspace::SessionWorkspace() = default;

SessionWorkspace::~SessionWorkspace() {
  FreeWorkspace();
}

void SessionWorkspace::FreeWorkspace() {
  if (ws_owned_) {
    free(qlut_);
    free(lut_scales_);
    free(lut_biases_);
  }
  qlut_ = lut_scales_ = lut_biases_ = nullptr;
  max_k_ = max_n_ = 0;
  ws_owned_ = false;
}

bool SessionWorkspace::Init(const std::string& kcfg_path, int n_threads, int act_group_size) {
  n_threads_ = n_threads > 0 ? n_threads : 1;
  act_group_ = act_group_size > 0 ? act_group_size : kDefaultActGroup;
  wrapper_ = std::make_unique<TMAC::TMACGeMMWrapper<float16_t, kG>>(
      n_threads_, act_group_, kcfg_path, "");
  ready_ = true;
  return true;
}

bool SessionWorkspace::EnsureWorkspace(int max_k, int max_n) {
  if (!ready_ || !wrapper_) {
    return false;
  }
  if (max_k <= max_k_ && max_n <= max_n_ && ws_owned_) {
    return true;
  }
  FreeWorkspace();
  const size_t qlut_bytes =
      static_cast<size_t>(max_n) * static_cast<size_t>(max_k / kG) * (1u << kG);
  const size_t scale_bytes =
      static_cast<size_t>(max_n) * static_cast<size_t>(max_k / act_group_) * sizeof(float16_t);
  qlut_ = AlignAlloc(qlut_bytes);
  lut_scales_ = AlignAlloc(scale_bytes);
  lut_biases_ = AlignAlloc(scale_bytes);
  if (!qlut_ || !lut_scales_ || !lut_biases_) {
    FreeWorkspace();
    return false;
  }
  max_k_ = max_k;
  max_n_ = max_n;
  ws_owned_ = true;
  wrapper_->set_workspace(max_k, max_n);
  return true;
}

bool SessionWorkspace::InitActivation(float16_t* B, int M_logical, int K, int N, int bits) {
  if (!ready_ || !B || !ws_owned_) {
    return false;
  }
  // Call dispatch directly — avoid wrapper DCHECK abort on unsupported shapes.
  const int m = M_logical * bits;
  int ret = preprocessor_int8(m, K, N, bits, B, lut_scales_, lut_biases_, qlut_);
  return ret == 0;
}

bool SessionWorkspace::ComputeTile(void* A, void* scales, void* C, int M_logical, int K, int N,
                                   int bits) {
  if (!ready_ || !A || !C || !ws_owned_) {
    return false;
  }
  const int m = M_logical * bits;
  int ret = qgemm_lut_int8(m, K, N, bits, A, qlut_, scales, lut_scales_, lut_biases_, C);
  return ret == 0;
}

}  // namespace tmac_sa
