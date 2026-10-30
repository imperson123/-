#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <arm_neon.h>

#include "t-mac/tmac_gemm_wrapper.h"

namespace tmac_sa {

// SA-owned buffers for direct-mode T-MAC (feasibility P3 § buffer lifecycle).
class SessionWorkspace {
 public:
  static constexpr int kG = 4;
  static constexpr int kDefaultActGroup = 32;

  SessionWorkspace();
  ~SessionWorkspace();

  SessionWorkspace(const SessionWorkspace&) = delete;
  SessionWorkspace& operator=(const SessionWorkspace&) = delete;

  // Prepare kcfg + thread wrapper. kcfg_path empty → env / TMAC_KCFG_FILE.
  bool Init(const std::string& kcfg_path, int n_threads, int act_group_size = kDefaultActGroup);

  // Allocate qlut / lut_scales / lut_biases for maxK, maxN (main thread only).
  bool EnsureWorkspace(int max_k, int max_n);

  // Activation preprocess once per decode step (main thread).
  // B: float16[N*K], logical M rows of weights will be computed later.
  bool InitActivation(float16_t* B, int M_logical, int K, int N, int bits);

  // One thread's GEMV tile: A/scales/C point at this thread's slice.
  bool ComputeTile(void* A, void* scales, void* C, int M_logical, int K, int N, int bits);

  TMAC::TMACGeMMWrapper<float16_t, kG>* wrapper() { return wrapper_.get(); }
  void* qlut() { return qlut_; }
  void* lut_scales() { return lut_scales_; }
  void* lut_biases() { return lut_biases_; }
  int n_threads() const { return n_threads_; }
  bool ready() const { return ready_; }

 private:
  void FreeWorkspace();

  std::unique_ptr<TMAC::TMACGeMMWrapper<float16_t, kG>> wrapper_;
  void* qlut_ = nullptr;
  void* lut_scales_ = nullptr;
  void* lut_biases_ = nullptr;
  int max_k_ = 0;
  int max_n_ = 0;
  int n_threads_ = 1;
  int act_group_ = kDefaultActGroup;
  bool ready_ = false;
  bool ws_owned_ = false;
};

}  // namespace tmac_sa
