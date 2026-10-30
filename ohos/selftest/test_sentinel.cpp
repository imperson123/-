// Sentinel test: find the EXACT write extent of the tbl kernel.
// Big guarded buffer (0x5A filler); C points into the middle; after the call
// we report every byte range the kernel touched.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "t-mac/kernels.h"

int32_t tbl_g4_int8_float_update_strue_k16_b2_ak16_fafalse_zfalse_ostrue(
    int32_t m, void* c, int8_t* lut, uint8_t* a, void* scales, void* lut_scales, void* lut_biases);
extern "C" int32_t tbl_float_reset(int32_t m, void* c);

int main() {
    const int m = 128;   // same as qgemm passes
    const int K = 3200;
    const int bits = 2;

    const size_t a_bytes = static_cast<size_t>(m) * K * bits / 8;  // 102400
    const size_t sc_floats = 64;
    uint8_t* A = static_cast<uint8_t*>(aligned_alloc(64, a_bytes + sc_floats * 4));
    memset(A, 0xF0, a_bytes);
    float* scales = reinterpret_cast<float*>(A + a_bytes);
    for (size_t i = 0; i < sc_floats; i++) scales[i] = 0.015625f;

    int8_t* qlut = static_cast<int8_t*>(aligned_alloc(64, static_cast<size_t>(K) / 64 * 256));
    memset(qlut, 1, static_cast<size_t>(K) / 64 * 256);

    // x86 kernel reads float lut_scales/lut_biases
    float* lut_scales = static_cast<float*>(aligned_alloc(64, 200 * 4));
    float* lut_biases = static_cast<float*>(aligned_alloc(64, 200 * 4));
    for (int i = 0; i < 200; i++) { lut_scales[i] = 0.01f; lut_biases[i] = 0.0f; }

    // Guarded C buffer: 8192 bytes of 0x5A; C := base + 4096
    const size_t guard = 8192;
    uint8_t* cbase = static_cast<uint8_t*>(aligned_alloc(64, guard));
    memset(cbase, 0x5A, guard);
    uint8_t* C = cbase + 4096;

    printf("=== sentinel write-extent test (m=%d) ===\n", m);
    printf("cbase=%p C=%p\n", (void*)cbase, (void*)C);
    fflush(stdout);

    int ret = tbl_float_reset(m, C);
    // reset(m) should only zero 2*m bytes = 256
    size_t lo_reset = guard, hi_reset = 0;
    for (size_t i = 0; i < guard; i++) {
        if (cbase[i] != 0x5A) { if (i < lo_reset) lo_reset = i; if (i > hi_reset) hi_reset = i; }
    }
    printf("after reset: touched [%zd, %zd]  = %zd bytes (expect 256)\n",
           (ptrdiff_t)lo_reset, (ptrdiff_t)hi_reset, hi_reset - lo_reset + 1);
    // refill, keep C intact
    for (size_t i = 0; i < guard; i++) if (cbase[i] != 0x5A) cbase[i] = 0x5A;

    ret += tbl_g4_int8_float_update_strue_k16_b2_ak16_fafalse_zfalse_ostrue(
        m, C, qlut, A, (void*)scales, lut_scales, lut_biases);

    size_t lo = guard, hi = 0;
    for (size_t i = 0; i < guard; i++) {
        if (cbase[i] != 0x5A) { if (i < lo) lo = i; if (i > hi) hi = i; }
    }
    printf("after tbl  : touched [%zd, %zd] = %zd bytes (C at offset 4096)\n",
           (ptrdiff_t)lo - 4096, (ptrdiff_t)hi - 4096, hi - lo + 1);
    printf("ret=%d\n", ret);
    printf("first 8 dwords at C: ");
    for (int i = 0; i < 8; i++) printf("%08x ", reinterpret_cast<uint32_t*>(C)[i]);
    printf("\nSENTINEL_DONE\n");
    fflush(stdout);
    return 0;
}
