// P1 selftest: qgemm_lut_int8(m=128,k=3200,n=1,b=2) smoke + NMSE vs zero (A=0).

#include <arm_neon.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "t-mac/kernels.h"

namespace {

constexpr int kMBits = 128;  // M*bits with M=64, bits=2
constexpr int kK = 3200;
constexpr int kN = 1;
constexpr int kBits = 2;
constexpr int kG = 4;
constexpr int kActGroup = 32;
constexpr int kOutM = 64;  // logical output rows

void* aligned_alloc64(size_t nbytes) {
  void* p = nullptr;
  if (posix_memalign(&p, 64, nbytes) != 0 || !p) {
    std::fprintf(stderr, "posix_memalign failed (%zu)\n", nbytes);
    std::exit(2);
  }
  std::memset(p, 0, nbytes);
  return p;
}

double nmse(const float16_t* a, const float16_t* b, size_t n) {
  double num = 0.0, den = 0.0;
  for (size_t i = 0; i < n; ++i) {
    double av = static_cast<double>(a[i]);
    double bv = static_cast<double>(b[i]);
    double d = av - bv;
    num += d * d;
    den += bv * bv;
  }
  if (den < 1e-12) {
    return num;
  }
  return num / den;
}

}  // namespace

int main() {
  std::printf("P1 selftest qgemm m=%d k=%d n=%d b=%d\n", kMBits, kK, kN, kBits);

  const size_t a_bytes = static_cast<size_t>(kMBits) * kK;
  void* A = aligned_alloc64(a_bytes);
  void* scales = aligned_alloc64(4096);
  void* qlut = aligned_alloc64(static_cast<size_t>(kN) * (kK / kG) * (1 << kG));
  void* lut_scales = aligned_alloc64(static_cast<size_t>(kN) * (kK / kActGroup) * sizeof(float16_t));
  void* lut_biases = aligned_alloc64(static_cast<size_t>(kN) * (kK / kActGroup) * sizeof(float16_t));
  void* C1 = aligned_alloc64(static_cast<size_t>(kN) * kOutM * sizeof(float16_t));
  void* C2 = aligned_alloc64(static_cast<size_t>(kN) * kOutM * sizeof(float16_t));

  // Non-zero C beforehand — kernel must clear then write
  std::memset(C1, 0x5a, static_cast<size_t>(kN) * kOutM * sizeof(float16_t));
  std::memset(C2, 0x5a, static_cast<size_t>(kN) * kOutM * sizeof(float16_t));

  int ret1 = qgemm_lut_int8(kMBits, kK, kN, kBits, A, qlut, scales, lut_scales, lut_biases, C1);
  int ret2 = qgemm_lut_int8(kMBits, kK, kN, kBits, A, qlut, scales, lut_scales, lut_biases, C2);
  if (ret1 != 0 || ret2 != 0) {
    std::fprintf(stderr, "qgemm_lut_int8 failed ret1=%d ret2=%d\n", ret1, ret2);
    return 4;
  }

  auto* c1 = reinterpret_cast<float16_t*>(C1);
  auto* c2 = reinterpret_cast<float16_t*>(C2);
  double err = nmse(c1, c2, static_cast<size_t>(kN) * kOutM);
  std::printf("determinism NMSE(C1,C2)=%.6e\n", err);

  std::vector<float16_t> zeros(static_cast<size_t>(kN) * kOutM, static_cast<float16_t>(0.f));
  double err0 = nmse(c1, zeros.data(), zeros.size());
  std::printf("NMSE(C, zero_ref)=%.6e\n", err0);

  if (err > 1e-6) {
    std::fprintf(stderr, "determinism check failed\n");
    return 5;
  }
  if (err0 > 1e-3) {
    std::fprintf(stderr, "NMSE vs zero > 1e-3 (got %.6e)\n", err0);
    return 6;
  }

  std::printf("P1 selftest PASS\n");
  free(A);
  free(scales);
  free(qlut);
  free(lut_scales);
  free(lut_biases);
  free(C1);
  free(C2);
  return 0;
}
