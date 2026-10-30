// LUT-SA HAP NAPI bridge — T-MAC LLM inference inside a HarmonyOS app.
//
// ArkTS side:  import tmac from 'libtmac_hap.so'
// Exposed (sync):
//   nativeVersion()                                        -> string
//   selfTest()                                             -> string   (LUT kernel: zero-in => zero-out)
//   bench(steps)                                           -> string   (LUT kernel micro-benchmark)
//   loadModel(path, filesDir, threads, nCtx, useMmap)       -> string
//   generate(prompt, nPredict, temp, topK)                  -> string   (text + tok/s timings)
//   release()                                              -> string
// Exposed (async, Promise — use these from the UI so the app stays responsive):
//   loadModelAsync(path, filesDir, threads, nCtx, useMmap)  -> Promise<string>
//   generateAsync(prompt, nPredict, temp, topK)             -> Promise<string>
//
// The static libs linked into this .so (per ABI, see CMakeLists.txt) already contain the
// T-MAC LUT kernels. The kcfg that matches those kernels is embedded below and written into
// the app sandbox on first use; TMAC_KCFG_FILE is then setenv'd — the T-MAC wrapper checks
// the environment variable first, so the build-machine path baked into the libs never matters.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "napi/native_api.h"
#include "llama.h"
#include "t-mac/kernels.h"

#if defined(__x86_64__)
// fp32 scalars (AVX2 build), matching staging-x64 kernels
#define TMAC_TFLOAT_BYTES 4
static const char *kKcfgIni = R"KCFG([qgemm_lut_t1_int8_m6400_k8640_n1_b2]
bm = 256
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 135
scales_size = 1
n_tile_num = 25

[qgemm_lut_t1_int8_m17280_k3200_n1_b2]
bm = 128
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 50
scales_size = 1
n_tile_num = 135

[qgemm_lut_t1_int8_m6400_k3200_n1_b2]
bm = 256
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 50
scales_size = 1
n_tile_num = 25
)KCFG";
#else
// fp16 scalars (NEON build), matching staging-arm64 kernels
#define TMAC_TFLOAT_BYTES 2
static const char *kKcfgIni = R"KCFG([qgemm_lut_t1_int8_m6400_k8640_n1_b2]
bm = 128
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 135
scales_size = 1
n_tile_num = 50

[qgemm_lut_t1_int8_m17280_k3200_n1_b2]
bm = 128
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 50
scales_size = 1
n_tile_num = 135

[qgemm_lut_t1_int8_m6400_k3200_n1_b2]
bm = 320
simd_n_in = 16
simd_n_out = 8
kfactor = 16
group_size = 128
lut_scales_size = 50
scales_size = 1
n_tile_num = 20
)KCFG";
#endif

namespace {

// ---------------------------------------------------------------- llama.cpp log capture
// llama.cpp/ggml print the full model description (hparams, special tokens, tensor buffers,
// graph splits, …) to stderr, which is invisible inside an ArkTS app. Take over the log
// callbacks so the app can show byte-for-byte the same output as the CLI.
std::mutex  g_log_mutex;
std::string g_log_buf;
constexpr size_t kLogCap = 256 * 1024;

void tmac_log_capture(enum ggml_log_level /*level*/, const char * text, void * /*user_data*/) {
    if (!text) return;
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (g_log_buf.size() < kLogCap) {
        g_log_buf.append(text);
    }
}

void tmac_log_clear() {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    g_log_buf.clear();
}

std::string tmac_log_take() {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    return g_log_buf;
}

void tmac_log_init() {
    static bool done = false;
    if (done) return;
    done = true;
    llama_log_set(tmac_log_capture, nullptr);
    ggml_log_set(tmac_log_capture, nullptr);
}

// ---------------------------------------------------------------- kernel self-test / bench
// Shape shared by both arch kernel sets: m_bits=128, k=3200, n=1, bits=2 -> 64 output rows.
constexpr int kMBits = 128;
constexpr int kK = 3200;
constexpr int kN = 1;
constexpr int kBits = 2;
constexpr int kOutRows = kMBits / kBits;   // 64

void *alloc64(size_t nbytes) {
    void *p = nullptr;
    if (posix_memalign(&p, 64, nbytes) != 0 || !p) {
        return nullptr;
    }
    std::memset(p, 0, nbytes);
    return p;
}

struct KernelBuffers {
    void *A = nullptr;
    void *qlut = nullptr;
    void *scales = nullptr;
    void *lut_scales = nullptr;
    void *lut_biases = nullptr;
    void *C = nullptr;

    size_t c_bytes() const { return (size_t) kOutRows * TMAC_TFLOAT_BYTES; }

    bool ok() const { return A && qlut && scales && lut_scales && lut_biases && C; }

    ~KernelBuffers() {
        std::free(A); std::free(qlut); std::free(scales);
        std::free(lut_scales); std::free(lut_biases); std::free(C);
    }
};

KernelBuffers make_kernel_buffers() {
    KernelBuffers b;
    b.A          = alloc64((size_t) kMBits * kK / 8);            // 51200 B
    b.qlut       = alloc64((size_t) kK / 4 * 16);                // 12800 B
    b.scales     = alloc64(256 * TMAC_TFLOAT_BYTES);             // kernels index up to 120+
    b.lut_scales = alloc64((size_t) kK / 64 * TMAC_TFLOAT_BYTES);
    b.lut_biases = alloc64((size_t) kK / 64 * TMAC_TFLOAT_BYTES);
    b.C          = alloc64((size_t) kOutRows * TMAC_TFLOAT_BYTES);
    return b;
}

bool all_zero(const void *p, size_t n) {
    const uint8_t *q = static_cast<const uint8_t *>(p);
    for (size_t i = 0; i < n; ++i) {
        if (q[i] != 0) return false;
    }
    return true;
}

int run_kernel_once(KernelBuffers &b) {
    std::memset(b.C, 0x5a, b.c_bytes());   // must be overwritten by the kernel
    return qgemm_lut_int8(kMBits, kK, kN, kBits, b.A, b.qlut, b.scales, b.lut_scales, b.lut_biases, b.C);
}

std::string run_self_test() {
    KernelBuffers b = make_kernel_buffers();
    if (!b.ok()) return "FAIL: buffer allocation";
    char local[320];
    std::snprintf(local, sizeof(local), "kernel m=%d k=%d n=%d b=%d (%s) | ", kMBits, kK, kN, kBits,
#if defined(__x86_64__)
                  "x86_64 AVX2"
#else
                  "arm64 NEON"
#endif
    );
    std::string out = local;
    const int ret = run_kernel_once(b);
    if (ret != 0) {
        std::snprintf(local, sizeof(local), "FAIL: qgemm_lut_int8 returned %d", ret);
        return out + local;
    }
    if (!all_zero(b.C, b.c_bytes())) return out + "FAIL: zero input produced non-zero output";
    return out + "PASS: LUT kernel ran, zero-in => zero-out";
}

std::string run_bench(int steps) {
    if (steps <= 0 || steps > 100000) return "FAIL: steps must be in [1, 100000]";
    KernelBuffers b = make_kernel_buffers();
    if (!b.ok()) return "FAIL: buffer allocation";
    if (run_kernel_once(b) != 0) return "FAIL: kernel error during warmup";
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < steps; ++i) {
        if (run_kernel_once(b) != 0) return "FAIL: kernel error during bench";
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    char local[320];
    std::snprintf(local, sizeof(local),
                  "kernel m=%d k=%d n=%d b=%d | steps=%d | total=%.2f ms | avg=%.4f ms",
                  kMBits, kK, kN, kBits, steps, total_ms, total_ms / steps);
    return local;
}

// ---------------------------------------------------------------- model / context state
struct LlmState {
    llama_model *model = nullptr;
    llama_context *ctx = nullptr;
    std::string kcfg_path;
    std::string model_path;
    int n_threads = 4;
    int n_ctx = 512;
    double load_ms = 0.0;
    // Chained-KV bookkeeping: the exact token chain currently living in the KV cache.
    // Next generate() finds the common prefix with this chain, keeps those KV cells and
    // only decodes the divergent suffix (multi-turn prefix reuse, zero recompute).
    std::vector<llama_token> kv_tokens;
};

LlmState g_llm;

bool ensure_kcfg(const std::string &files_dir) {
    if (!g_llm.kcfg_path.empty() && std::getenv("TMAC_KCFG_FILE")) return true;
    if (files_dir.empty()) return false;
    const std::string path = files_dir + "/kcfg.ini";
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(kKcfgIni, 1, std::strlen(kKcfgIni), f);
    std::fclose(f);
    setenv("TMAC_KCFG_FILE", path.c_str(), 1);
    g_llm.kcfg_path = path;
    return true;
}

void release_llm() {
    if (g_llm.ctx) { llama_free(g_llm.ctx); g_llm.ctx = nullptr; }
    if (g_llm.model) { llama_free_model(g_llm.model); g_llm.model = nullptr; }
    g_llm.model_path.clear();
    g_llm.load_ms = 0.0;
}

// top-k / top-p / temperature sampler + repetition penalty.
// (self-contained: this fork keeps its samplers in common/, which is not linked into the HAP)
struct SampleCfg {
    int   top_k = 40;
    float top_p = 0.95f;
    float temp = 0.8f;
    float repeat_penalty = 1.15f;   // > 1 suppresses tokens seen in the recent window
    int   repeat_last_n = 64;
};

llama_token sample_token(float *logits, int n_vocab, const SampleCfg &cfg,
                         const std::vector<llama_token> &recent, std::mt19937 &rng) {
    // 1) repetition penalty (llama.cpp semantics: divide positive logits, multiply negative)
    if (cfg.repeat_penalty > 1.0f && !recent.empty()) {
        const int n = std::min((int) recent.size(), cfg.repeat_last_n);
        for (int i = 0; i < n; ++i) {
            const llama_token t = recent[recent.size() - 1 - (size_t) i];
            if (t < 0 || t >= n_vocab) continue;
            if (logits[t] > 0) logits[t] /= cfg.repeat_penalty;
            else               logits[t] *= cfg.repeat_penalty;
        }
    }
    // 2) greedy fast path
    if (cfg.temp <= 0.0f) {
        return (llama_token) (std::max_element(logits, logits + n_vocab) - logits);
    }
    // 3) top-k candidates
    int top_k = cfg.top_k;
    if (top_k <= 0 || top_k > n_vocab) top_k = n_vocab;
    std::vector<int> idx(n_vocab);
    for (int i = 0; i < n_vocab; ++i) idx[i] = i;
    if (top_k < n_vocab) {
        std::nth_element(idx.begin(), idx.begin() + top_k, idx.end(),
                         [&](int a, int b) { return logits[a] > logits[b]; });
        idx.resize(top_k);
    }
    // 4) softmax over the candidates
    float maxl = -INFINITY;
    for (int i : idx) maxl = std::max(maxl, logits[i]);
    std::vector<double> p(idx.size());
    double sum = 0.0;
    for (size_t i = 0; i < idx.size(); ++i) {
        p[i] = std::exp((double) (logits[idx[i]] - maxl) / (double) cfg.temp);
        sum += p[i];
    }
    // 5) top-p (nucleus): keep the shortest prefix of the sorted candidates whose
    //    cumulative probability reaches top_p
    std::vector<int> order(idx.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int) i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return p[a] > p[b]; });
    const double top_p = (cfg.top_p > 0.0f && cfg.top_p < 1.0f) ? (double) cfg.top_p : 1.0;
    size_t keep = order.size();
    double acc_p = 0.0;
    for (size_t i = 0; i < order.size(); ++i) {
        acc_p += p[order[i]] / sum;
        if (acc_p >= top_p) { keep = i + 1; break; }
    }
    // 6) sample from the kept set
    double sumk = 0.0;
    for (size_t i = 0; i < keep; ++i) sumk += p[order[i]];
    std::uniform_real_distribution<double> dist(0.0, sumk);
    const double r = dist(rng);
    double acc = 0.0;
    for (size_t i = 0; i < keep; ++i) {
        acc += p[order[i]];
        if (r <= acc) return idx[order[i]];
    }
    return idx[order[keep - 1]];
}

napi_value make_string(napi_env env, const std::string &s) {
    napi_value v = nullptr;
    napi_create_string_utf8(env, s.c_str(), s.length(), &v);
    return v;
}

std::string get_string_arg(napi_env env, napi_value v) {
    size_t len = 0;
    napi_get_value_string_utf8(env, v, nullptr, 0, &len);
    std::string s(len, '\0');
    napi_get_value_string_utf8(env, v, &s[0], len + 1, &len);
    return s;
}

// ---------------------------------------------------------------- core work (runs on the JS thread for the
// sync entry points, on a NAPI worker thread for the Async ones)
std::string do_load(const std::string &model_path, const std::string &files_dir, int threads,
                    int n_ctx, bool use_mmap) {
    tmac_log_init();
    tmac_log_clear();   // capture this load's llama.cpp output verbatim
    release_llm();

    // Make the sandbox reachable for `hdc file send` (the app's own uid owns it, so chmod is
    // allowed; without this, /data/app/.../files stays 0770 and the shell user cannot push the
    // model in). Best effort — harmless if it fails.
    if (!files_dir.empty()) {
        chmod(files_dir.c_str(), 0777);
    }

    if (!ensure_kcfg(files_dir)) {
        return "FAIL: cannot write kcfg.ini into " + files_dir;
    }

    // Resolve the model: use the requested path, else fall back to <filesDir>/model.gguf.
    std::string resolved = model_path;
    std::string note;
    if (access(resolved.c_str(), R_OK) != 0) {
        const std::string alt = files_dir + "/model.gguf";
        if (access(alt.c_str(), R_OK) == 0) {
            resolved = alt;
            note = " [fell back to sandbox path]";
        }
    }
    if (access(resolved.c_str(), R_OK) != 0) {
        return "FAIL: model not readable: " + model_path + " (and " + files_dir +
               "/model.gguf). Push it with: hdc file send <local.gguf> " + files_dir +
               "/model.gguf";
    }

    const auto t0 = std::chrono::steady_clock::now();
    llama_model_params mparams = llama_model_default_params();
    mparams.use_mmap = use_mmap;
    mparams.n_gpu_layers = 0;
    llama_model *model = llama_load_model_from_file(resolved.c_str(), mparams);
    if (!model) {
        struct stat st;
        const long long sz = (stat(resolved.c_str(), &st) == 0) ? (long long) st.st_size : -1;
        char msg[512];
        std::snprintf(msg, sizeof(msg),
                      "FAIL: llama_load_model_from_file(%s)\n[file size: %lld bytes]%s",
                      resolved.c_str(), sz,
                      sz < 16 * 1024 * 1024
                          ? " — file is far too small for a model: the copy was incomplete "
                            "(out of storage space?) or the wrong file was picked"
                          : " — file size looks plausible; it may be a truncated/corrupt GGUF "
                            "or the wrong quantisation for this build");
        return msg;
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = (uint32_t) n_ctx;
    cparams.n_batch = 512;
    cparams.n_threads = threads;
    cparams.n_threads_batch = threads;
    // KV quantization (q8_0) is blocked by MODEL GEOMETRY, across llama.cpp versions — NOT by
    // flash-attn (earlier attribution was wrong; re-verified 2026-09-30 against source + crash sig):
    // BitNet-3B's head_dim is 100; every quantized KV type uses 32-element blocks (q8_0/q4_0/...),
    // and llama_new_context_with_model asserts n_embd_head_k % blck_size == 0 (src/llama.cpp:19307
    // here; the SAME unconditional assert exists in the 2026-04 new fork at llama-context.cpp:201)
    // -> GGML_ASSERT abort. The V/flash-attn gate (src/llama.cpp:19199) is LOG_ERROR + return
    // nullptr (graceful) and was NOT involved in our K-only experiment (type_v=f16, fa=false).
    // Unlocking requires padding the KV head dim to the block size (100 -> 128): a dedicated
    // work item, independent of any fork upgrade. Keep f16 KV here.
    cparams.type_k = GGML_TYPE_F16;
    cparams.type_v = GGML_TYPE_F16;
    cparams.flash_attn = false;
    // NOTE: no cparams.seed in this llama.cpp version — determinism comes from the sampler
    // below (std::mt19937 seeded with 42).
    llama_context *ctx = llama_new_context_with_model(model, cparams);
    if (!ctx) {
        llama_free_model(model);
        return "FAIL: llama_new_context_with_model";
    }
    const auto t1 = std::chrono::steady_clock::now();

    g_llm.model = model;
    g_llm.ctx = ctx;
    g_llm.model_path = resolved;
    g_llm.n_threads = threads;
    g_llm.n_ctx = n_ctx;
    g_llm.kv_tokens.clear();   // fresh context: empty KV chain
    g_llm.load_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // The complete llama.cpp verbose output (hparams, special tokens, tensor buffers, graph
    // splits, …) captured through the log callback — byte-for-byte the same as the CLI prints.
    const std::string log = tmac_log_take();
    char footer[640];
    std::snprintf(footer, sizeof(footer),
                  "\n--------\nload time = %.2f ms | threads = %d | ctx = %d | mmap = %s\n"
                  "model file: %s%s\nkcfg:       %s\n",
                  g_llm.load_ms, threads, n_ctx, use_mmap ? "on" : "off",
                  resolved.c_str(), note.c_str(), g_llm.kcfg_path.c_str());
    return log + footer;
}
// Streaming hook: when set (generateStreamAsync), every generated token piece is pushed to the
// JS thread through a napi_threadsafe_function. Plain C fn-pointer keeps this header-light;
// concurrency is safe because the app's busy flag allows one generate at a time.
static void (*g_stream_sink)(const char *piece, size_t n) = nullptr;
static napi_threadsafe_function g_active_tsfn = nullptr;

std::string do_generate(const std::string &prompt, int n_predict, double temp, int top_k) {
    if (!g_llm.model || !g_llm.ctx) return "FAIL: call loadModel() first";

    llama_model *model = g_llm.model;
    llama_context *ctx = g_llm.ctx;
    const int n_vocab = llama_n_vocab(model);

    std::vector<llama_token> tokens(prompt.size() + 8);
    int n_tok = llama_tokenize(model, prompt.c_str(), (int32_t) prompt.size(), tokens.data(),
                               (int32_t) tokens.size(), true, false);
    if (n_tok < 0) {
        tokens.resize(-n_tok);
        n_tok = llama_tokenize(model, prompt.c_str(), (int32_t) prompt.size(), tokens.data(),
                               (int32_t) tokens.size(), true, false);
    }
    if (n_tok <= 0) return "FAIL: tokenize";
    tokens.resize(n_tok);

    // ---- chained KV: reuse the longest common token prefix still in the cache ----
    // The cache holds a token chain from the previous turn; find where the new prompt diverges,
    // drop only that suffix (seq_rm) and decode just the delta — zero recompute of the prefix.
    int common = 0;
    const int cached = (int) g_llm.kv_tokens.size();
    const int max_cmp = std::min(n_tok, cached);
    while (common < max_cmp && tokens[(size_t) common] == g_llm.kv_tokens[(size_t) common]) {
        ++common;
    }
    if (common == 0) {
        llama_kv_cache_clear(ctx);                        // nothing reusable: fresh start
    } else if (common < cached) {
        llama_kv_cache_seq_rm(ctx, 0, common, -1);        // keep prefix chain, cut divergent tail
    }                                                     // common == cached: prompt fully cached

    const auto t_prompt0 = std::chrono::steady_clock::now();
    int n_past = common;
    if (n_tok > common) {
        llama_batch batch = llama_batch_get_one(tokens.data() + common, n_tok - common, common, 0);
        if (llama_decode(ctx, batch) != 0) return "FAIL: prompt decode";
        n_past = n_tok;
    }
    const auto t_prompt1 = std::chrono::steady_clock::now();

    std::mt19937 rng(42);
    const llama_token eos = llama_token_eos(model);
    std::string text;
    std::vector<llama_token> gen_ids;   // continuation tokens -> appended to the KV chain
    int generated = 0;

    const auto t_eval0 = std::chrono::steady_clock::now();
    double sampling_ms = 0.0;
    double ttft_ms = -1.0;   // eval-loop start -> first token produced (-1: none generated)
    SampleCfg scfg;
    scfg.top_k = top_k;
    scfg.temp  = (float) temp;
    std::vector<llama_token> recent = tokens;   // rep-penalty window grows with the reply
    for (int i = 0; i < n_predict && n_past < g_llm.n_ctx - 1; ++i) {
        float *logits = llama_get_logits(ctx);
        if (!logits) break;
        const auto ts0 = std::chrono::steady_clock::now();
        const llama_token id = sample_token(logits, n_vocab, scfg, recent, rng);
        const auto ts1 = std::chrono::steady_clock::now();
        sampling_ms += std::chrono::duration<double, std::milli>(ts1 - ts0).count();
        if (id == eos) break;
        char piece[256];
        const int np = llama_token_to_piece(model, id, piece, sizeof(piece), 0, false);
        if (np > 0) {
            text.append(piece, (size_t) np);
            if (ttft_ms < 0.0) {
                // true TTFT: request start (incl. prompt eval) -> first token
                ttft_ms = std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - t_prompt0).count();
            }
            if (g_stream_sink) g_stream_sink(piece, (size_t) np);
        }
        llama_batch b1 = llama_batch_get_one((llama_token *) &id, 1, n_past, 0);
        if (llama_decode(ctx, b1) != 0) break;
        gen_ids.push_back(id);
        recent.push_back(id);
        ++n_past;
        ++generated;
    }
    const auto t_eval1 = std::chrono::steady_clock::now();

    // build the KV chain for the next turn: prompt + continuation as decoded
    g_llm.kv_tokens = tokens;
    g_llm.kv_tokens.insert(g_llm.kv_tokens.end(), gen_ids.begin(), gen_ids.end());

    const double prompt_ms = std::chrono::duration<double, std::milli>(t_prompt1 - t_prompt0).count();
    const double eval_ms = std::chrono::duration<double, std::milli>(t_eval1 - t_eval0).count();
    const double total_ms = prompt_ms + eval_ms;
    // llama.cpp-style console report (matches the CLI output users expect)
    char head[512], perf[1400];
    std::snprintf(head, sizeof(head),
                  "sampler seed: 42\n"
                  "sampler params: top_k = %d, top_p = %.3f, temp = %.3f, repeat_penalty = %.2f (last_n = %d)\n"
                  "generate: n_ctx = %d, n_batch = 512, n_predict = %d\n"
                  "kv chain: reused %d of %d prompt tokens (prefix chain kept, delta decoded)\n\n%s\n",
                  top_k, (double) scfg.top_p, temp, (double) scfg.repeat_penalty, scfg.repeat_last_n,
                  g_llm.n_ctx, n_predict, common, n_tok, prompt.c_str());
    // peak RSS via /proc/self/status (VmHWM = high-water mark, kB)
    double peak_rss_mb = 0.0;
    if (FILE *st = fopen("/proc/self/status", "r")) {
        char line[256];
        while (fgets(line, sizeof(line), st)) {
            if (strncmp(line, "VmHWM:", 6) == 0) {
                peak_rss_mb = atof(line + 6) / 1024.0;
                break;
            }
        }
        fclose(st);
    }
    std::snprintf(perf, sizeof(perf),
                  "\nllama_perf_sampler_print:    sampling time = %8.2f ms / %5d runs ( %8.2f ms per token, %8.2f tokens per second)\n"
                  "llama_perf_context_print:        load time = %8.2f ms\n"
                  "llama_perf_context_print:  prompt eval time = %8.2f ms / %5d tokens ( %8.2f ms per token, %8.2f tokens per second)\n"
                  "llama_perf_context_print:        eval time = %8.2f ms / %5d runs ( %8.2f ms per token, %8.2f tokens per second)\n"
                  "llama_perf_context_print:       total time = %8.2f ms / %5d tokens\n"
                  "llama_perf_context_print:           ttft = %8.2f ms\n"
                  "llama_perf_context_print:       peak_rss = %8.1f MB\n",
                  sampling_ms, generated,
                  generated > 0 ? sampling_ms / generated : 0.0,
                  sampling_ms > 0 ? generated / (sampling_ms / 1000.0) : 0.0,
                  g_llm.load_ms,
                  prompt_ms, n_tok,
                  n_tok > 0 ? prompt_ms / n_tok : 0.0,
                  prompt_ms > 0 ? n_tok / (prompt_ms / 1000.0) : 0.0,
                  eval_ms, generated,
                  eval_ms > 0 && generated > 0 ? eval_ms / generated : 0.0,
                  eval_ms > 0 && generated > 0 ? generated / (eval_ms / 1000.0) : 0.0,
                  total_ms, n_tok + generated,
                  ttft_ms, peak_rss_mb);
    return std::string(head) + text + perf;
}

// ---------------------------------------------------------------- async plumbing (Promise + worker thread)
// Copy between two already-open file descriptors on a worker thread. The 1 GB model move MUST NOT
// run on the JS thread: a synchronous copy blocks the UI for >6 s and the system's watchdog kills
// the app (appfreeze / THREAD_BLOCK_6S — observed).
std::string do_copy_fds(int src_fd, int dst_fd) {
    constexpr size_t kChunk = 4u << 20;
    std::vector<char> buf(kChunk);
    long long total = 0;
    for (;;) {
        const ssize_t n = read(src_fd, buf.data(), buf.size());
        if (n == 0) break;
        if (n < 0) return "FAIL: read error during copy";
        ssize_t off = 0;
        while (off < n) {
            const ssize_t w = write(dst_fd, buf.data() + off, (size_t) (n - off));
            if (w <= 0) return "FAIL: write error during copy";
            off += w;
        }
        total += n;
    }
    char b[160];
    std::snprintf(b, sizeof(b), "OK: copied %.1f MB", (double) total / 1e6);
    return b;
}

struct AsyncJob {
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    int op = 0;             // 0 = load, 1 = generate, 2 = copy fds
    std::string s1, s2;     // load: model_path/files_dir, generate: prompt
    int fd_src = -1, fd_dst = -1;   // op 2
    int threads = 4, n_ctx = 512, n_predict = 32, top_k = 40;
    double temp = 0.8;
    bool use_mmap = false;
    bool stream = false;    // op 1 + stream: per-token tsfn callback
    napi_threadsafe_function tsfn = nullptr;
    std::string result;
};

void execute_job(napi_env, void *data) {
    AsyncJob *j = static_cast<AsyncJob *>(data);
    if (j->op == 0) {
        j->result = do_load(j->s1, j->s2, j->threads, j->n_ctx, j->use_mmap);
    } else if (j->op == 1) {
        j->result = do_generate(j->s2, j->n_predict, j->temp, j->top_k);
        g_stream_sink = nullptr;
        g_active_tsfn = nullptr;
    } else {
        j->result = do_copy_fds(j->fd_src, j->fd_dst);
    }
}

void complete_job(napi_env env, napi_status status, void *data) {
    AsyncJob *j = static_cast<AsyncJob *>(data);
    napi_value result = nullptr;
    napi_create_string_utf8(env, j->result.c_str(), j->result.size(), &result);
    if (status == napi_ok) {
        napi_resolve_deferred(env, j->deferred, result);
    } else {
        napi_value msg = nullptr;
        napi_create_string_utf8(env, "native async work failed", NAPI_AUTO_LENGTH, &msg);
        napi_value err = nullptr;
        napi_create_error(env, nullptr, msg, &err);
        napi_reject_deferred(env, j->deferred, err);
    }
    if (j->tsfn) {
        napi_release_threadsafe_function(j->tsfn, napi_tsfn_release);
        j->tsfn = nullptr;
    }
    napi_delete_async_work(env, j->work);
    delete j;
}

napi_value queue_job(napi_env env, AsyncJob *j, const char *name) {
    napi_value promise = nullptr;
    napi_create_promise(env, &j->deferred, &promise);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(env, nullptr, resource, execute_job, complete_job, j, &j->work);
    napi_queue_async_work(env, j->work);
    return promise;
}

// ---------------------------------------------------------------- NAPI entry points
napi_value NativeVersion(napi_env env, napi_callback_info) {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "LUT-SA native | %s | tfloat=%d B | kcfg embedded | static llama.cpp",
#if defined(__x86_64__)
                  "x86_64 (AVX2, no FMA)",
#else
                  "arm64 (NEON fp16)",
#endif
                  TMAC_TFLOAT_BYTES);
    return make_string(env, buf);
}

napi_value SelfTest(napi_env env, napi_callback_info) {
    return make_string(env, run_self_test());
}

napi_value Bench(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    int32_t steps = 100;
    if (argc >= 1) napi_get_value_int32(env, argv[0], &steps);
    return make_string(env, run_bench(steps));
}

napi_value LoadModel(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        return make_string(env, "FAIL: loadModel(modelPath, filesDir, [threads], [nCtx], [useMmap])");
    }
    const std::string model_path = get_string_arg(env, argv[0]);
    const std::string files_dir = get_string_arg(env, argv[1]);
    int32_t threads = 4, n_ctx = 512;
    bool use_mmap = false;
    if (argc >= 3) napi_get_value_int32(env, argv[2], &threads);
    if (argc >= 4) napi_get_value_int32(env, argv[3], &n_ctx);
    if (argc >= 5) napi_get_value_bool(env, argv[4], &use_mmap);
    return make_string(env, do_load(model_path, files_dir, threads, n_ctx, use_mmap));
}

napi_value LoadModelAsync(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    AsyncJob *j = new AsyncJob();
    j->op = 0;
    if (argc >= 1) j->s1 = get_string_arg(env, argv[0]);
    if (argc >= 2) j->s2 = get_string_arg(env, argv[1]);
    if (argc >= 3) napi_get_value_int32(env, argv[2], &j->threads);
    if (argc >= 4) napi_get_value_int32(env, argv[3], &j->n_ctx);
    if (argc >= 5) napi_get_value_bool(env, argv[4], &j->use_mmap);
    return queue_job(env, j, "tmac_load");
}

napi_value Generate(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4] = {nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) return make_string(env, "FAIL: generate(prompt, [nPredict], [temp], [topK])");
    const std::string prompt = get_string_arg(env, argv[0]);
    int32_t n_predict = 32, top_k = 40;
    double temp = 0.8;
    if (argc >= 2) napi_get_value_int32(env, argv[1], &n_predict);
    if (argc >= 3) napi_get_value_double(env, argv[2], &temp);
    if (argc >= 4) napi_get_value_int32(env, argv[3], &top_k);
    return make_string(env, do_generate(prompt, n_predict, temp, top_k));
}

// ---- streaming generate: every token piece is marshalled to the JS thread and passed to
// the app's onToken(piece) callback, then the Promise resolves with the full console report ----
static void tsfn_sink(const char *piece, size_t n) {
    if (g_active_tsfn == nullptr) return;
    std::string *p = new std::string(piece, n);
    napi_call_threadsafe_function(g_active_tsfn, p, napi_tsfn_blocking);
}

static void tsfn_call_js(napi_env env, napi_value js_cb, void * /*context*/, void *data) {
    std::string *p = static_cast<std::string *>(data);
    if (env != nullptr && js_cb != nullptr && p != nullptr) {
        napi_value undefined = nullptr, argv = nullptr, unused = nullptr;
        napi_get_undefined(env, &undefined);
        napi_create_string_utf8(env, p->c_str(), p->size(), &argv);
        napi_call_function(env, js_cb, undefined, 1, &argv, &unused);
    }
    delete p;
}

napi_value GenerateAsync(napi_env env, napi_callback_info info) {    size_t argc = 4;
    napi_value argv[4] = {nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    AsyncJob *j = new AsyncJob();
    j->op = 1;
    if (argc >= 1) j->s2 = get_string_arg(env, argv[0]);
    if (argc >= 2) napi_get_value_int32(env, argv[1], &j->n_predict);
    if (argc >= 3) napi_get_value_double(env, argv[2], &j->temp);
    if (argc >= 4) napi_get_value_int32(env, argv[3], &j->top_k);
    return queue_job(env, j, "tmac_generate");
}

napi_value GenerateStreamAsync(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 5) return make_string(env, "FAIL: generateStreamAsync(prompt, nPredict, temp, topK, onToken)");
    AsyncJob *j = new AsyncJob();
    j->op = 1;
    j->stream = true;
    j->s2 = get_string_arg(env, argv[0]);
    napi_get_value_int32(env, argv[1], &j->n_predict);
    napi_get_value_double(env, argv[2], &j->temp);
    napi_get_value_int32(env, argv[3], &j->top_k);
    napi_value name = nullptr;
    napi_create_string_utf8(env, "tmac_token", NAPI_AUTO_LENGTH, &name);
    if (napi_create_threadsafe_function(env, argv[4], nullptr, name, 0, 1,
                                        nullptr, nullptr, nullptr, tsfn_call_js,
                                        &j->tsfn) != napi_ok) {
        delete j;
        return make_string(env, "FAIL: create threadsafe function");
    }
    g_active_tsfn = j->tsfn;
    g_stream_sink = tsfn_sink;
    return queue_job(env, j, "tmac_generate_stream");
}

napi_value CopyFdAsync(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    AsyncJob *j = new AsyncJob();
    j->op = 2;
    int32_t src = -1, dst = -1;
    if (argc >= 1) napi_get_value_int32(env, argv[0], &src);
    if (argc >= 2) napi_get_value_int32(env, argv[1], &dst);
    j->fd_src = src;
    j->fd_dst = dst;
    return queue_job(env, j, "tmac_copy");
}

napi_value Release(napi_env env, napi_callback_info) {
    release_llm();
    return make_string(env, "OK: model released");
}

// chmod the sandbox so the host can `hdc file send` a model into it, and report the exact
// path the UI should use. Call this right after app start.
//
// Why this is needed on HarmonyOS 7: an app may only read files inside its own sandbox
// (/data/local/tmp is blocked by SELinux), but `hdc file send` as the shell user cannot create
// files in the sandbox either. What DOES work: the app creates a 0666 placeholder file, and the
// shell user can then overwrite it. So we chmod the dir AND create model.gguf as 0666.
napi_value PrepareSandbox(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) return make_string(env, "FAIL: prepareSandbox(filesDir)");
    const std::string files_dir = get_string_arg(env, argv[0]);
    if (files_dir.empty()) return make_string(env, "FAIL: empty filesDir");

    const int rc = chmod(files_dir.c_str(), 0777);

    const std::string model_path = files_dir + "/model.gguf";
    struct stat st;
    bool created = false;
    if (stat(model_path.c_str(), &st) != 0) {
        const int fd = open(model_path.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd >= 0) {
            close(fd);
            created = true;
        }
    } else {
        // keep an imported model host-writable for `hdc file send` (0666)
        chmod(model_path.c_str(), 0666);
    }
    const bool model_present = (stat(model_path.c_str(), &st) == 0) && st.st_size > 0;

    char buf[640];
    std::snprintf(buf, sizeof(buf),
                  "sandbox=%s | chmod=%s | placeholder=%s | model.gguf %s (%lld B)\n"
                  "push: hdc file send <local>.gguf %s/model.gguf",
                  files_dir.c_str(), rc == 0 ? "0777 ok" : "failed", created ? "created 0666" : "exists",
                  model_present ? "present" : "empty",
                  (long long) (model_present ? st.st_size : 0), files_dir.c_str());
    return make_string(env, buf);
}

}  // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"nativeVersion", nullptr, NativeVersion, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"selfTest", nullptr, SelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"bench", nullptr, Bench, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"loadModel", nullptr, LoadModel, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"loadModelAsync", nullptr, LoadModelAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generate", nullptr, Generate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateAsync", nullptr, GenerateAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateStreamAsync", nullptr, GenerateStreamAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"copyFdAsync", nullptr, CopyFdAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"prepareSandbox", nullptr, PrepareSandbox, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"release", nullptr, Release, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module tmacModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "tmac_hap",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterTmacModule(void) {
    napi_module_register(&tmacModule);
}
