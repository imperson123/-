// arm64_kernel_test.cpp — aarch64 T-MAC kernel validation, meant to run under qemu-aarch64 (user mode).
// Same math as T-MAC's tests/test_e2e.py and the same data layout as the x86 test: the ffn_gate entry
// has an identical kcfg on both archs (bm=128, n_tile_num=135), so A.bin/B.bin/S.bin/Cref.bin are reusable.
// ARM specifics: Scales / LUT_Scales / LUT_Biases / C are fp16 (tmac_float_type = _Float16 on NEON).
#include "t-mac/kernels.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

typedef _Float16 half_t;

int main(int argc, char** argv) {
    const char* dir = (argc > 1) ? argv[1] : "/mnt/d/ohos-models/tmactest";
    const int chunk_rows = (argc > 2) ? atoi(argv[2]) : 64;

    const int K = 3200, Mbits = 17280, N = 1, bits = 2;
    const int Mrows = Mbits / bits;                        // 8640
    const size_t a_bytes = (size_t) Mrows * K * bits / 8;  // 6912000
    const size_t qlut_bytes = (size_t) K / 4 * 16;         // 12800
    const int lut_n = K / 64;                              // 50

    uint8_t* A     = (uint8_t*) aligned_alloc(64, a_bytes);
    float*   B     = (float*)   aligned_alloc(64, K * sizeof(float));
    half_t*  S     = (half_t*)  aligned_alloc(64, 256 * sizeof(half_t));
    int8_t*  qlut  = (int8_t*)  aligned_alloc(64, qlut_bytes);
    half_t*  lut_s = (half_t*)  aligned_alloc(64, lut_n * sizeof(half_t));
    half_t*  lut_b = (half_t*)  aligned_alloc(64, lut_n * sizeof(half_t));
    half_t*  C     = (half_t*)  aligned_alloc(64, Mrows * sizeof(half_t));
    memset(C, 0, Mrows * sizeof(half_t));

    char path[512];
    FILE* f;
    snprintf(path, sizeof(path), "%s/A.bin", dir);
    if (!(f = fopen(path, "rb"))) { printf("ERR open A\n"); return 1; }
    fread(A, 1, a_bytes, f); fclose(f);
    snprintf(path, sizeof(path), "%s/B.bin", dir);
    if (!(f = fopen(path, "rb"))) { printf("ERR open B\n"); return 1; }
    fread(B, 4, K, f); fclose(f);

    float s0 = 0;
    snprintf(path, sizeof(path), "%s/S.bin", dir);
    if ((f = fopen(path, "rb"))) { fread(&s0, 4, 1, f); fclose(f); }
    for (int i = 0; i < 256; i++) S[i] = (half_t) s0;   // fp16 broadcast, as the integration does
    printf("scale %.8f -> fp16 %.8f x256\n", s0, (float) S[0]);

    // The NEON preprocessor reads the activation as fp16 (half*), unlike the x86 fp32 build.
    half_t* B16 = (half_t*) aligned_alloc(64, K * sizeof(half_t));
    for (int i = 0; i < K; i++) B16[i] = (half_t) B[i];

    int r1 = preprocessor_int8(Mbits, K, N, bits, B16, lut_s, lut_b, qlut);
    printf("preproc ret=%d | qlut[0..3]=%d %d %d %d | ls[0]=%.6f lb[0]=%.6f\n",
           r1, qlut[0], qlut[1], qlut[2], qlut[3], (float) lut_s[0], (float) lut_b[0]);

    const int n_chunks = Mrows / chunk_rows;
    const size_t a_chunk = (size_t) chunk_rows * K * bits / 8;
    int r2 = 0;
    for (int c = 0; c < n_chunks; c++)
        r2 |= qgemm_lut_int8(chunk_rows * bits, K, N, bits,
                             A + c * a_chunk, qlut, S, lut_s, lut_b, C + c * chunk_rows);
    printf("chunk_rows=%d n_chunks=%d qgemm ret=%d\n", chunk_rows, n_chunks, r2);

    float* C32 = (float*) malloc(Mrows * sizeof(float));
    for (int i = 0; i < Mrows; i++) C32[i] = (float) C[i];

    snprintf(path, sizeof(path), "%s/C_arm64.bin", dir);
    if ((f = fopen(path, "wb"))) { fwrite(C32, 4, Mrows, f); fclose(f); }
    printf("C_arm64[0..3]: %.6f %.6f %.6f %.6f\n", C32[0], C32[1], C32[2], C32[3]);
    printf("C_arm64[128..131]: %.6f %.6f %.6f %.6f\n", C32[128], C32[129], C32[130], C32[131]);
    printf("ARM64_DONE\n");
    return 0;
}
