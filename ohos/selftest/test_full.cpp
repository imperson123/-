// Correctly-sized smoke test for the x86_64 LUT kernel (m=128, k=3200, n=1, b=2).
// x86 float_type = float (32-bit), as the AVX2 path uses _mm256_*_ps.
//   A          : packed weights (m_bits*K*bits/8 = 102400 B) [+ scales region]
//   scales     : 64 fp32 (256 B), indexed ((i/4)/Bits)*8 .. +8
//   qlut       : K/64 * 256 = 12800 B
//   lut_scales : fp32 array (kernel reads _mm256_set1_ps)
//   lut_biases : fp32 array
//   C          : m_bits floats = 128 fp32 = 512 B  (kernel writes c + i*2 for i<m/2)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "t-mac/kernels.h"

int main() {
    const int m_bits = 128;
    const int K = 3200;
    const int bits = 2;

    const size_t a_bytes = static_cast<size_t>(m_bits) * K * bits / 8;  // 102400
    const size_t sc_floats = 64;                                         // scales elements needed
    const size_t a_total = a_bytes + sc_floats * sizeof(float);

    uint8_t* A = static_cast<uint8_t*>(aligned_alloc(64, a_total));
    memset(A, 0xF0, a_bytes);
    float* scales = reinterpret_cast<float*>(A + a_bytes);
    for (size_t i = 0; i < sc_floats; i++) scales[i] = 0.015625f;

    const size_t qlut_bytes = static_cast<size_t>(K) / 64 * 256;  // 12800
    int8_t* qlut = static_cast<int8_t*>(aligned_alloc(64, qlut_bytes));
    memset(qlut, 1, qlut_bytes);

    const size_t n_ls = 200;  // K/ActK
    float* lut_scales = static_cast<float*>(aligned_alloc(64, n_ls * sizeof(float)));
    float* lut_biases = static_cast<float*>(aligned_alloc(64, n_ls * sizeof(float)));
    for (size_t i = 0; i < n_ls; i++) {
        lut_scales[i] = 0.01f;
        lut_biases[i] = 0.0f;
    }

    const size_t c_floats = 128;  // m_bits floats (kernel writes m/2*2 per iteration)
    float* C = static_cast<float*>(aligned_alloc(64, c_floats * sizeof(float)));
    for (size_t i = 0; i < c_floats; i++) C[i] = 0.0f;

    printf("=== full-size test ===\n");
    printf("A=%p scales=%p qlut=%p lut_scales=%p C=%p\n",
           (void*)A, (void*)scales, (void*)qlut, (void*)lut_scales, (void*)C);
    fflush(stdout);

    const int ret = qgemm_lut_int8(m_bits, K, 1, bits, A, qlut, (void*)scales,
                                   lut_scales, lut_biases, C);
    printf("ret=%d\n", ret);
    printf("C[0..7] = ");
    for (int i = 0; i < 8; i++) printf("%.6f ", C[i]);
    printf("\nC[64..71] = ");
    for (int i = 64; i < 72; i++) printf("%.6f ", C[i]);
    printf("\nFULL_DONE\n");
    fflush(stdout);
    return 0;
}
