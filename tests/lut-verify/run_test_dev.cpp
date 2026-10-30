// Kernel-vs-reference comparison test: same math as T-MAC's tests/test_e2e.py,
// but running OUR compiled kernels.cc (GCC / OHOS clang / Android NDK clang)
// instead of the TVM runtime. Diagnoses the classic T-MAC SILENT failure:
// kernels return 0, print nothing, and emit all-zero / garbage C.
//
// Data: produced by gen_testdata_ffn.py (A.bin/B.bin/S.bin/Cref.bin).
// NOTE: qgemm is called per-chunk (chunk_m_bits = 128 rows * bits = 256),
// exactly like ggml.c does (chunk_size0 = ne0/n_tile_num = 128 outputs per call).
//
// Usage: ./run_test_dev <datadir> [chunk_rows]
//
// Build (host x86):
//   g++ -O2 -march=native -I deploy/tuned/<artifact> run_test_dev.cpp \
//       deploy/tuned/<artifact>/kernels.cc -o run_test_dev
// Build (Android arm64, run via adb -- /data/local/tmp exec IS allowed on Android,
// unlike retail HarmonyOS):
//   $NDK_HOME/toolchains/llvm/prebuilt/<host>/bin/aarch64-linux-android28-clang++ \
//       -O2 -march=armv8.2a+fp16 -I deploy/tuned/aarch64-hf-bitnet-3b \
//       run_test_dev.cpp deploy/tuned/aarch64-hf-bitnet-3b/kernels.cc \
//       -static-libstdc++ -o run_test_dev_android
//   adb push run_test_dev_android /data/local/tmp/
//   adb push vdata/* /data/local/tmp/vdata/
//   adb shell "/data/local/tmp/run_test_dev_android /data/local/tmp/vdata"
//
// Success criteria: qlut non-zero, lut_scales/biases non-zero, C within fp16
// rounding of Cref (see README). If qlut is all zero, your activation dtype
// contract is wrong (aarch64 preprocessor reads half*, x86 reads float*).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "kernels.h"   // flat include: -I deploy/tuned/<artifact>  (or -I <staging>/t-mac/include + "t-mac/kernels.h")

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : ".";
    const int K = 3200, Mbits = 17280, N = 1, bits = 2;
    const int Mrows = Mbits / bits;                          // 8640
    const size_t a_bytes = (size_t)Mrows * K * bits / 8;     // 6912000
    const size_t qlut_bytes = (size_t)K / 4 * (1 << 4);      // 12800
    const int lut_n = K / 64;                                // 50
    const size_t c_n = Mrows;

    auto p = [&](const char* f) { return dir + "/" + f; };

    uint8_t* A = (uint8_t*)aligned_alloc(64, a_bytes);
    float* B = (float*)aligned_alloc(64, K * sizeof(float));
    float* S = (float*)aligned_alloc(64, 256 * sizeof(float));   // broadcast buffer (like the runtime)
    FILE* f;
    if ((f = fopen(p("A.bin").c_str(), "rb"))) { fread(A, 1, a_bytes, f); fclose(f); }
    else { printf("ERR open A\n"); return 1; }
    if ((f = fopen(p("B.bin").c_str(), "rb"))) { fread(B, 4, K, f); fclose(f); }
    {
        float s0 = 0;
        if ((f = fopen(p("S.bin").c_str(), "rb"))) { fread(&s0, 4, 1, f); fclose(f); }
        for (int q = 0; q < 256; q++) S[q] = s0;   // broadcast per-tensor scale (kernel indexes up to [120])
        printf("scale broadcast: %.8f x256\n", s0);
    }

    int8_t* qlut = (int8_t*)aligned_alloc(64, qlut_bytes);
    float* lut_s = (float*)aligned_alloc(64, lut_n * sizeof(float));
    float* lut_b = (float*)aligned_alloc(64, lut_n * sizeof(float));
    float* C = (float*)aligned_alloc(64, c_n * sizeof(float));
    memset(C, 0, c_n * sizeof(float));

    // 1) preprocessor over the whole tensor (activation -> LUT)
    int r1 = preprocessor_int8(Mbits, K, N, bits, B, lut_s, lut_b, qlut);

    // diagnostics: how much of the activation-derived LUT is non-zero?
    {
        int nz_q = 0;
        for (size_t q = 0; q < qlut_bytes; q++) if (qlut[q] != 0) nz_q++;
        printf("qlut nonzero: %d/%zu | qlut[0..3]=%d %d %d %d\n",
               nz_q, qlut_bytes, qlut[0], qlut[1], qlut[2], qlut[3]);
        int nz_ls = 0, nz_lb = 0;
        for (int q = 0; q < lut_n; q++) { if (lut_s[q] != 0) nz_ls++; if (lut_b[q] != 0) nz_lb++; }
        printf("lut_scales nonzero: %d/%d | lut_biases nonzero: %d/%d\n", nz_ls, lut_n, nz_lb, lut_n);
    }

    // 2) qgemm per chunk: chunk_rows output rows per call (m_bits = chunk_rows*bits)
    //    chunk_rows=128 -> m256 kernel; 64 -> m128 kernel (both used end-to-end)
    const int chunk_rows = (argc > 2) ? atoi(argv[2]) : 128;
    const int n_chunks = Mrows / chunk_rows;
    const size_t a_chunk = (size_t)chunk_rows * K * bits / 8;
    printf("chunk_rows=%d n_chunks=%d\n", chunk_rows, n_chunks);
    int r2 = 0;
    for (int c = 0; c < n_chunks; c++) {
        int rr = qgemm_lut_int8(chunk_rows * bits, K, N, bits,
                                A + c * a_chunk, qlut, S, lut_s, lut_b, C + c * chunk_rows);
        if (rr != 0) { printf("chunk %d ret=%d\n", c, rr); r2 = rr; }
    }

    // 3) compare against the NumPy reference baked by gen_testdata_ffn.py
    float* Cref = (float*)aligned_alloc(64, c_n * sizeof(float));
    bool have_ref = false;
    if ((f = fopen(p("Cref.bin").c_str(), "rb"))) { fread(Cref, 4, c_n, f); fclose(f); have_ref = true; }
    printf("preproc ret=%d qgemm ret=%d (chunks=%d)\n", r1, r2, n_chunks);
    printf("C_impl[0..3]: %.6f %.6f %.6f %.6f\n", C[0], C[1], C[2], C[3]);
    if (have_ref) {
        double num = 0, den = 0;
        for (size_t i = 0; i < c_n; i++) { double d = C[i] - Cref[i]; num += d * d; den += (double)Cref[i] * Cref[i]; }
        printf("Cref[0..3]:  %.6f %.6f %.6f %.6f\n", Cref[0], Cref[1], Cref[2], Cref[3]);
        printf("NMSE = %.3e  (%s)\n", num / den, num / den < 1e-3 ? "PASS" : "FAIL");
    }
    if ((f = fopen(p("C_impl.bin").c_str(), "wb"))) { fwrite(C, 4, c_n, f); fclose(f); }
    printf("IMPL_DONE\n");
    return 0;
}
