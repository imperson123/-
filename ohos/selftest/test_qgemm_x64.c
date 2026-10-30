// Standalone smoke test for the generated x86_64-ohos LUT kernel (ags64 branch).
// Reproduces the model call path: qgemm_lut_int8(m=128, k=3200, n=1, b=2).
// Data layout mirrors the runtime: A = packed weights [+ trailing fp16 scale].
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "t-mac/kernels.h"

int main(int argc, char** argv) {
    const int m_bits = 128;  // chunk m (64 rows * bits=2)
    const int K = 3200;
    const int bits = 2;
    int mode = (argc > 1) ? atoi(argv[1]) : 0;  // 0=0xF0 1=0x00 2=0xFF 3=0x55

    // A: packed weights (m_bits x K bits) + trailing fp16 scale (scales_size=1)
    size_t a_bytes = (size_t)m_bits * K * bits / 8;  // 102400
    size_t a_total = a_bytes + 2;
    uint8_t* A = (uint8_t*)aligned_alloc(64, a_total);
    uint8_t fill = (mode == 1) ? 0x00 : (mode == 2) ? 0xFF : (mode == 3) ? 0x55 : 0xF0;
    memset(A, fill, a_total);  // nonzero nibbles; trailing 2 bytes act as the scale
    // force a known fp16 scale in the trailing bytes: 0x2400 = 0.015625
    A[a_bytes] = 0x00;
    A[a_bytes + 1] = 0x24;
    printf("mode=%d fill=0x%02x scale_bytes=0x%02x%02x\n", mode, fill, A[a_bytes + 1], A[a_bytes]);

    // qlut: K/64 groups * 256 bytes
    size_t qlut_bytes = (size_t)K / 64 * 256;  // 12800
    int8_t* qlut = (int8_t*)aligned_alloc(64, qlut_bytes);
    memset(qlut, 1, qlut_bytes);  // all-ones LUT

    // lut_scales / lut_biases: K/64 fp16 each
    size_t ls_bytes = (size_t)K / 64 * 2;  // 100
    uint16_t* lut_scales = (uint16_t*)aligned_alloc(64, ls_bytes);
    uint16_t* lut_biases = (uint16_t*)aligned_alloc(64, ls_bytes);
    for (int i = 0; i < K / 64; i++) {
        lut_scales[i] = 0x2118;  // ~0.01 in fp16 (approx)
        lut_biases[i] = 0x0000;  // 0.0
    }

    // C: m_bits/bits fp16 outputs
    size_t c_bytes = (size_t)m_bits / bits * 2;  // 128
    uint16_t* C = (uint16_t*)aligned_alloc(64, c_bytes);
    memset(C, 0x5A, c_bytes);

    printf("call qgemm_lut_int8(m=%d, k=%d, n=1, b=%d)\n", m_bits, K, bits);
    fflush(stdout);

    int ret = qgemm_lut_int8(m_bits, K, 1, bits, A, qlut, (void*)(A + a_bytes), lut_scales, lut_biases, C);

    printf("ret=%d\n", ret);
    printf("C[0..7] = ");
    for (int i = 0; i < 8; i++) printf("0x%04x ", C[i]);
    printf("\n");

    // --- Layer-by-layer: call the tbl update intrinsic directly ---
    extern "C" int32_t tbl_g4_int8_float_update_strue_k16_b2_ak16_fafalse_zfalse_ostrue(
        int32_t m, void* c, int8_t* lut, uint8_t* a, void* scales, void* lut_scales, void* lut_biases);
    extern "C" int32_t tbl_float_reset(int32_t m, void* c);

    alignas(32) uint16_t CBits[256];
    tbl_float_reset(256, CBits);
    tbl_g4_int8_float_update_strue_k16_b2_ak16_fafalse_zfalse_ostrue(
        256, CBits, qlut, A, (void*)(A + a_bytes), lut_scales, lut_biases);
    printf("after 1 tbl call: CBits[0..7] = ");
    for (int i = 0; i < 8; i++) printf("0x%04x ", CBits[i]);
    printf("\nSMOKE_DONE\n");
    fflush(stdout);
    return 0;
}
