// Simulator-path selftest: x86_64 (AVX2) kernels from deploy/tuned.
// The DevEco emulator on Windows is x86_64, so this validates the kernel shapes
// it will call: qgemm_lut_t1_int8_m256_k4096_n1_b4 (M*bits=256 -> M=64, bits=4).
// Zero-input smoke, same contract as selftest/main.cpp (aarch64).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "t-mac/kernels.h"

namespace {

constexpr int kMBits = 256;  // M*bits with M=64, bits=4
constexpr int kK = 4096;
constexpr int kN = 1;
constexpr int kBits = 4;
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

// fp16 results are compared as raw 16-bit words: zero input must yield all-zero output.
bool all_zero(const void* buf, size_t nbytes) {
  const uint8_t* p = static_cast<const uint8_t*>(buf);
  for (size_t i = 0; i < nbytes; ++i) {
    if (p[i] != 0) return false;
  }
  return true;
}

}  // namespace

int main() {
  std::printf("sim selftest qgemm m=%d k=%d n=%d b=%d (x86_64/AVX2)\n", kMBits, kK, kN, kBits);

  void* A = aligned_alloc64(static_cast<size_t>(kMBits) * kK);
  void* scales = aligned_alloc64(524288);  // scales_size from deploy/tuned/kcfg.ini
  void* qlut = aligned_alloc64(static_cast<size_t>(kN) * (kK / kG) * (1 << kG));
  void* lut_scales = aligned_alloc64(static_cast<size_t>(kN) * (kK / kActGroup) * sizeof(uint16_t));
  void* lut_biases = aligned_alloc64(static_cast<size_t>(kN) * (kK / kActGroup) * sizeof(uint16_t));
  void* C1 = aligned_alloc64(static_cast<size_t>(kN) * kOutM * sizeof(uint16_t));
  void* C2 = aligned_alloc64(static_cast<size_t>(kN) * kOutM * sizeof(uint16_t));

  // Non-zero C beforehand — kernel must clear then write
  std::memset(C1, 0x5a, static_cast<size_t>(kN) * kOutM * sizeof(uint16_t));
  std::memset(C2, 0x5a, static_cast<size_t>(kN) * kOutM * sizeof(uint16_t));

  int ret1 = qgemm_lut_int8(kMBits, kK, kN, kBits, A, qlut, scales, lut_scales, lut_biases, C1);
  int ret2 = qgemm_lut_int8(kMBits, kK, kN, kBits, A, qlut, scales, lut_scales, lut_biases, C2);
  if (ret1 != 0 || ret2 != 0) {
    std::fprintf(stderr, "qgemm_lut_int8 failed ret1=%d ret2=%d\n", ret1, ret2);
    return 4;
  }

  const size_t c_bytes = static_cast<size_t>(kN) * kOutM * sizeof(uint16_t);
  const bool z1 = all_zero(C1, c_bytes);
  const bool z2 = all_zero(C2, c_bytes);
  std::printf("C1 all-zero: %s\n", z1 ? "yes" : "no");
  std::printf("C2 all-zero: %s\n", z2 ? "yes" : "no");

  if (!z1 || !z2) {
    std::fprintf(stderr, "zero-input check failed\n");
    return 6;
  }
  std::printf("sim selftest PASS\n");
  return 0;
}
