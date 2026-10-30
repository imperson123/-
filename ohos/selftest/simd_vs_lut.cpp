// Same-shape SIMD-vs-LUT benchmark for the HarmonyOS port of T-MAC.
// Shape from BitNet-b1.58-3B ffn_gate: weight N=8640 x K=3200, activation M=1 (one token).
//
//   A) llama.cpp SIMD path: ggml_mul_mat over the same weights, Q4_0 / Q8_0.
//      (K-quants need K % 256 == 0 and 3200 is not, so Q4_K is impossible for this model.)
//      A PERSISTENT ggml threadpool is used -- ggml_graph_compute_with_ctx() creates and
//      destroys a pool per call, which on an emulator dominates the measurement.
//   B) T-MAC LUT path: 135 x qgemm_lut m128_k3200 (chunk_rows=64, exactly like ggml.c:
//      chunk_size0 = ne0 / n_tile_num = 8640 / 135 = 64).
//
// The emulator is noisy (the same config measured 0.49 ms and 1.07 ms minutes apart), so
// configurations are interleaved over several rounds and the MIN and MEDIAN per config are
// reported -- min approximates the interference-free value, median the typical one.
#ifndef LUT_ONLY_BUILD
#include "ggml.h"
#endif
#include "t-mac/kernels.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include <unistd.h>

#define KK 3200
#define NN 8640
#define NCHUNK 135
#define A_CHUNK (64 * KK * 2 / 8)   // 51200 bytes per chunk (64 rows x 3200 x 2bit)
#define C_CHUNK 64
#define MAXROUND 16

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec * 1e3 + (double) ts.tv_nsec / 1e6;
}

// ---------------- A) llama.cpp SIMD path (persistent threadpool) ----------------
#ifndef LUT_ONLY_BUILD
struct simd_setup {
    struct ggml_context *ctx;
    void *mem;
    struct ggml_cgraph *gf;
    struct ggml_tensor *wq, *ylast;
    size_t qb;
    int mats;
};

static struct simd_setup setup_simd(enum ggml_type qtype, int mats_per_graph, int nmul) {
    struct simd_setup s;
    const size_t ctx_size = (size_t) (150ull * 1024 * 1024 * nmul + 64ull * 1024 * 1024);
    s.mem = malloc(ctx_size);
    if (!s.mem) { printf("SIMD: malloc ctx fail (%zu B)\n", ctx_size); exit(1); }
    struct ggml_init_params ip = { ctx_size, s.mem, false };
    s.ctx = ggml_init(ip);
    s.mats = mats_per_graph;

    const int Ntot = NN * nmul;
    struct ggml_tensor *w = ggml_new_tensor_2d(s.ctx, GGML_TYPE_F32, KK, Ntot);
    float *wd = (float *) w->data;
    for (int i = 0; i < KK * Ntot; i++)
        wd[i] = (float) ((int) ((i * 2654435761u) % 1024) - 512) / 512.0f;

    s.wq = ggml_new_tensor_2d(s.ctx, qtype, KK, Ntot);
    s.qb = ggml_quantize_chunk(qtype, wd, s.wq->data, 0, Ntot, KK, NULL);

    struct ggml_tensor *x = ggml_new_tensor_2d(s.ctx, GGML_TYPE_F32, KK, 1);
    float *xd = (float *) x->data;
    for (int i = 0; i < KK; i++)
        xd[i] = (float) ((int) ((i * 40503u) % 256) - 128) / 128.0f;

    s.gf = ggml_new_graph(s.ctx);
    s.ylast = NULL;
    for (int i = 0; i < mats_per_graph; i++) {
        s.ylast = ggml_mul_mat(s.ctx, s.wq, x);
        ggml_build_forward_expand(s.gf, s.ylast);
    }
    printf("      [info] %s: N=%d (x%d), %zu B, %4.2f bpw, y=%.5f\n",
           ggml_type_name(qtype), Ntot, nmul, s.qb, (double) s.qb * 8.0 / ((double) KK * Ntot),
           ((float *) s.ylast->data)[0]);
    return s;
}

// returns ms per matmul
static double run_simd(struct simd_setup *s, int n_threads, int reps) {
    struct ggml_threadpool_params tpp = ggml_threadpool_params_default(n_threads);
    struct ggml_threadpool *pool = ggml_threadpool_new(&tpp);
    struct ggml_cplan plan = ggml_graph_plan(s->gf, n_threads, pool);
    uint8_t *work = plan.work_size ? (uint8_t *) malloc(plan.work_size) : NULL;
    plan.work_data = work;

    ggml_graph_compute(s->gf, &plan);   // warmup

    double t0 = now_ms();
    for (int r = 0; r < reps; r++)
        ggml_graph_compute(s->gf, &plan);
    double t1 = now_ms();

    ggml_threadpool_free(pool);
    if (work) free(work);
    return (t1 - t0) / ((double) reps * s->mats);
}
#endif  // LUT_ONLY_BUILD

// ---------------- B) T-MAC LUT path ----------------
static uint8_t *gA;
static int8_t  *g_qlut;
static float   *gS, *g_ls, *g_lb, *gC, *gB;
static pthread_barrier_t g_bar;
static int g_nth = 1, g_reps = 1, g_copies = 1;
static double g_t0[16], g_t1[16];

static void *lut_worker(void *arg) {
    long t = (long) arg;
    pthread_barrier_wait(&g_bar);           // single release barrier
    g_t0[t] = now_ms();
    const int total = g_copies * NCHUNK;
    for (int r = 0; r < g_reps; r++)
        for (int k = (int) t; k < total; k += g_nth) {
            const int copy = k / NCHUNK;
            const int c    = k % NCHUNK;
            qgemm_lut_int8(128, KK, 1, 2, gA + (size_t) (copy * NCHUNK + c) * A_CHUNK,
                           g_qlut, gS, g_ls, g_lb, gC + c * C_CHUNK);
        }
    g_t1[t] = now_ms();
    return NULL;
}

static void lut_init(const char *dir, int copies) {   // one-time data load
    char path[512];
    const size_t a_bytes = (size_t) NN * KK * 2 / 8;   // 6912000
    g_copies = copies;
    gA     = (uint8_t *) aligned_alloc(64, a_bytes * copies);
    gB     = (float *)   aligned_alloc(64, KK * sizeof(float));
    gS     = (float *)   aligned_alloc(64, 256 * sizeof(float));
    g_qlut = (int8_t *)  aligned_alloc(64, (size_t) KK / 4 * 16);
    g_ls   = (float *)   aligned_alloc(64, 50 * sizeof(float));
    g_lb   = (float *)   aligned_alloc(64, 50 * sizeof(float));
    gC     = (float *)   aligned_alloc(64, NN * sizeof(float));
    memset(gC, 0, NN * sizeof(float));

    FILE *f;
    snprintf(path, sizeof(path), "%s/A.bin", dir);
    if (!(f = fopen(path, "rb"))) { printf("LUT: cannot open %s\n", path); exit(1); }
    fread(gA, 1, a_bytes, f); fclose(f);
    for (int i = 1; i < copies; i++)                    // cold-stream copies
        memcpy(gA + (size_t) i * a_bytes, gA, a_bytes);
    snprintf(path, sizeof(path), "%s/B.bin", dir);
    if (!(f = fopen(path, "rb"))) { printf("LUT: cannot open %s\n", path); exit(1); }
    fread(gB, 4, KK, f); fclose(f);
    snprintf(path, sizeof(path), "%s/S.bin", dir);
    float s0 = 0;
    if ((f = fopen(path, "rb"))) { fread(&s0, 4, 1, f); fclose(f); }
    for (int q = 0; q < 256; q++) gS[q] = s0;

    preprocessor_int8(NN * 2, KK, 1, 2, gB, g_ls, g_lb, g_qlut);   // fill the shared LUT
    double p0 = now_ms();
    for (int i = 0; i < 500; i++)
        preprocessor_int8(NN * 2, KK, 1, 2, gB, g_ls, g_lb, g_qlut);
    double p1 = now_ms();
    printf("      [info] LUT preprocessor: %.1f us/call (one per tensor per token, not in qgemm numbers)\n",
           (p1 - p0) / 500 * 1000);
}

// returns ms per matmul for the given thread count
static double bench_lut(int n_threads, int reps) {
    for (int t = 0; t < n_threads; t++) g_t0[t] = g_t1[t] = 0;
    g_nth = n_threads;
    g_reps = reps;
    pthread_barrier_init(&g_bar, NULL, n_threads + 1);
    pthread_t th[16];
    for (long t = 0; t < n_threads; t++) pthread_create(&th[t], NULL, lut_worker, (void *) t);
    usleep(50 * 1000);
    pthread_barrier_wait(&g_bar);
    for (int t = 0; t < n_threads; t++) pthread_join(th[t], NULL);
    pthread_barrier_destroy(&g_bar);

    double lo = g_t0[0], hi = g_t1[0];
    for (int t = 0; t < n_threads; t++) {
        if (g_t0[t] < lo) lo = g_t0[t];
        if (g_t1[t] > hi) hi = g_t1[t];
    }
    return (hi - lo) / reps / g_copies;
}

// ---------------- driver ----------------
#define NCFG 8
static const char *cfg_name[NCFG] = { "SIMD Q4_0 1t", "SIMD Q4_0 4t", "SIMD Q8_0 1t", "SIMD Q8_0 4t",
                                      "LUT  1t", "LUT  2t", "LUT  3t", "LUT  4t" };
static double samples[NCFG][MAXROUND];
static int    nsamp[NCFG];

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *) a, y = *(const double *) b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *dir = (argc > 1) ? argv[1] : "/data/local/tmp/llm-tmac";
    const int rounds = (argc > 2) ? atoi(argv[2]) : 5;
    const int reps_lut = (argc > 3) ? atoi(argv[3]) : 6;
    const int reps_simd = (argc > 4) ? atoi(argv[4]) : 4;
    const int mats = (argc > 5) ? atoi(argv[5]) : 50;
    const int lut_only = (argc > 6) && !strcmp(argv[6], "lut");
    const int cold = (argc > 6) && !strcmp(argv[6], "cold");
    const int lut_copies = cold ? 32 : 1;      // 32 x 6.9 MB = 221 MB streamed, nothing cached
    const int nmul       = cold ? 3 : 1;       // SIMD weights x3 = 46 MB (Q4_0) / 88 MB (Q8_0)

    printf("=== same shape: N=%d x K=%d, M=1 (BitNet-3B ffn_gate)%s%s ===\n",
           NN, KK, lut_only ? " [LUT only -- native build without ggml]" : "",
           cold ? " [COLD STREAM mode]" : "");
    printf("=== %d interleaved rounds: LUT reps=%d (x%d copies), SIMD reps=%d x %d mats (x%d) ===\n",
           rounds, reps_lut, lut_copies, reps_simd, mats, nmul);

    for (int c = 0; c < NCFG; c++) for (int r = 0; r < MAXROUND; r++) samples[c][r] = -1;

    lut_init(dir, lut_copies);
#ifndef LUT_ONLY_BUILD
    struct simd_setup q4 = {0}, q8 = {0};
    if (!lut_only) {
        q4 = setup_simd(GGML_TYPE_Q4_0, mats, nmul);
        q8 = setup_simd(GGML_TYPE_Q8_0, mats, nmul);
    }
#endif

    for (int round = 0; round < rounds; round++) {
        printf("round %d: ", round);
#ifndef LUT_ONLY_BUILD
        if (!lut_only) {
            samples[0][nsamp[0]++] = run_simd(&q4, 1, reps_simd);
            samples[1][nsamp[1]++] = run_simd(&q4, 4, reps_simd);
            samples[2][nsamp[2]++] = run_simd(&q8, 1, reps_simd);
            samples[3][nsamp[3]++] = run_simd(&q8, 4, reps_simd);
        }
#endif
        samples[4][nsamp[4]++] = bench_lut(1, reps_lut);
        samples[5][nsamp[5]++] = bench_lut(2, reps_lut);
        samples[6][nsamp[6]++] = bench_lut(3, reps_lut);
        samples[7][nsamp[7]++] = bench_lut(4, reps_lut);
        for (int c = 0; c < NCFG; c++)
            if (nsamp[c] > 0) printf("%.3f ", samples[c][nsamp[c] - 1]);
        printf("\n");
    }

    printf("\n%-19s %10s %10s %12s\n", "config", "MIN ms", "MEDIAN ms", "G weights/s(min)");
    for (int c = 0; c < NCFG; c++) {
        char nm[80];
        snprintf(nm, sizeof(nm), "%s%s", cold ? "COLD " : "", cfg_name[c]);
        if (nsamp[c] == 0) { printf("%-19s %10s %10s %12s\n", nm, "-", "-", "-"); continue; }
        double tmp[MAXROUND];
        memcpy(tmp, samples[c], sizeof(double) * nsamp[c]);
        qsort(tmp, nsamp[c], sizeof(double), cmp_double);
        double mn = tmp[0], med = tmp[nsamp[c] / 2];
        double n_w = (double) NN * KK;
        printf("%-19s %10.3f %10.3f %12.1f\n", nm, mn, med, n_w / (mn * 1e-3) / 1e9);
    }
    printf("DONE\n");
    return 0;
}
