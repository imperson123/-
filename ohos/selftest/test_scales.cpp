// Same as test_qgemm_x64.cpp but with a PROPERLY SIZED scales buffer.
// The kernel indexes scales[i] for i up to ~(m/2/4/Bits)*8 + 8, i.e. needs
// hundreds of bytes, not the 2 bytes used by the earlier smoke test.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "t-mac/kernels.h"

int main(int argc, char** argv) {
    const int m_bits = 128;
    const int K = 3200;
    const int bits = 2;
    const int mode = (argc > 1) ? atoi(argv[1]) : 0;
    const uint8_t fill = (mode == 1) ? 0x00 : (mode == 2) ? 0xFF : (mode == 3) ? 0x55 : 0xF0;

    const size_t a_bytes = static_cast<size_t>(m_bits) * K * bits / 8;  // 102400
    uint8_t* A = static_cast<uint8_t*>(aligned_alloc(64, a_bytes + 64));
    memset(A, fill, a_bytes + 64);

    // scales: dedicated buffer, fp16 0x2400 (=0.015625), large enough
    const size_t sc_elems = 4096;
    uint16_t* scales = static_cast<uint16_t*>(aligned_alloc(64, sc_elems * 2));
    for (size_t i = 0; i < sc_elems; i++) scales[i] = 0x2400;

    const size_t qlut_bytes = static_cast<size_t>(K) / 64 * 256;  // 12800
    int8_t* qlut = static_cast<int8_t*>(aligned_alloc(64, qlut_bytes));
    memset(qlut, 1, qlut_bytes);

    const size_t ls_bytes = static_cast<size_t>(K) / 64 * 2;  // 100
    uint16_t* lut_scales = static_cast<uint16_t*>(aligned_alloc(64, ls_bytes));
    uint16_t* lut_biases = static_cast<uint16_t*>(aligned_alloc(64, ls_bytes));
    for (int i = 0; i < K / 64; i++) {
        lut_scales[i] = 0x2118;
        lut_biases[i] = 0x0000;
    }

    const size_t c_bytes = static_cast<size_t>(m_bits) / bits * 2;  // 128
    uint16_t* C = static_cast<uint16_t*>(aligned_alloc(64, c_bytes));
    memset(C, 0x5A, c_bytes);

    printf("=== proper-scales test: fill=0x%02x, scales[0..3]=%04x %04x %04x %04x\n",
           fill, scales[0], scales[1], scales[2], scales[3]);
    fflush(stdout);

    const int ret = qgemm_lut_int8(m_bits, K, 1, bits, A, qlut, (void*)scales, lut_scales, lut_biases, C);

    printf("ret=%d\n", ret);
    printf("C[0..7] = ");
    for (int i = 0; i < 8; i++) printf("0x%04x ", C[i]);
    printf("\n");
    fflush(stdout);
    return 0;
}
