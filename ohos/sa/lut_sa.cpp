#include "lut_sa.h"

#include "qos_policy.h"
#include "session_workspace.h"
#include "tile_compute.h"
#include "tile_pool.h"

#include <arm_neon.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tmac_sa {
namespace {

constexpr int kWarmMLogical = 64;  // → qgemm m=128
constexpr int kWarmK = 3200;
constexpr int kWarmN = 1;
constexpr int kWarmBits = 2;

struct Session {
  bool loaded = false;
  std::string model_path;
  SessionConfig cfg;
  std::unique_ptr<SessionWorkspace> ws;
};

std::mutex g_mu;
std::unordered_map<uint64_t, Session> g_sessions;
std::atomic<uint64_t> g_next{1};

Session* FindUnlocked(uint64_t id) {
  auto it = g_sessions.find(id);
  return it == g_sessions.end() ? nullptr : &it->second;
}

}  // namespace

Status CreateSession(uint64_t* out_session_id) {
  return CreateSessionWithConfig(out_session_id, SessionConfig{});
}

Status CreateSessionWithConfig(uint64_t* out_session_id, const SessionConfig& cfg) {
  if (!out_session_id) {
    return Status::kInvalid;
  }
  auto ws = std::make_unique<SessionWorkspace>();
  if (!ws->Init(cfg.kcfg_path, cfg.n_threads, cfg.act_group_size)) {
    return Status::kKernelFail;
  }
  std::lock_guard<std::mutex> lock(g_mu);
  uint64_t id = g_next.fetch_add(1);
  Session s;
  s.cfg = cfg;
  s.ws = std::move(ws);
  g_sessions.emplace(id, std::move(s));
  *out_session_id = id;
  return Status::kOk;
}

Status LoadModel(uint64_t session_id, const std::string& model_path) {
  std::lock_guard<std::mutex> lock(g_mu);
  Session* s = FindUnlocked(session_id);
  if (!s) {
    return Status::kNoSession;
  }
  s->model_path = model_path;
  s->loaded = true;
  // Full gguf load lands when llama is linked into SA; path is recorded for IPC.
  return Status::kOk;
}

Status PrepareWorkspace(uint64_t session_id, int max_k, int max_n) {
  std::lock_guard<std::mutex> lock(g_mu);
  Session* s = FindUnlocked(session_id);
  if (!s || !s->ws) {
    return Status::kNoSession;
  }
  if (!s->ws->EnsureWorkspace(max_k, max_n)) {
    return Status::kNoWorkspace;
  }
  return Status::kOk;
}

Status WarmKernel(uint64_t session_id, int n_threads_override) {
  SessionWorkspace* ws = nullptr;
  int n_threads = 1;
  {
    std::lock_guard<std::mutex> lock(g_mu);
    Session* s = FindUnlocked(session_id);
    if (!s || !s->ws) {
      return Status::kNoSession;
    }
    if (!s->loaded) {
      return Status::kNotLoaded;
    }
    if (!s->ws->EnsureWorkspace(kWarmK, kWarmN)) {
      return Status::kNoWorkspace;
    }
    ws = s->ws.get();
    n_threads = n_threads_override > 0 ? n_threads_override : s->cfg.n_threads;
  }

  tmac_sched::SetThreadQoS(tmac_sched::QoSLevel::kUserInteractive);
  tmac_sched::LogSchedDecision("warm_kernel", tmac_sched::GetThreadQoS());

  // Zero packed weights + shared qlut (already zero from EnsureWorkspace).
  const size_t a_bytes = static_cast<size_t>(kWarmMLogical) * kWarmBits * kWarmK;
  const size_t c_bytes = static_cast<size_t>(kWarmN) * kWarmMLogical * sizeof(float16_t);
  std::vector<uint8_t> A(a_bytes, 0);
  std::vector<uint8_t> scales(4096, 0);
  std::vector<uint8_t> C(c_bytes, 0);

  // Single-tile warm: full m=64 (multi-tile needs packed-weight row offsets from ggml).
  (void)n_threads;
  TileJob job{};
  job.m_logical = kWarmMLogical;
  job.k = kWarmK;
  job.n = kWarmN;
  job.bits = kWarmBits;
  job.A_tile = A.data();
  job.scales_tile = scales.data();
  job.qlut = ws->qlut();
  job.lut_scales = ws->lut_scales();
  job.lut_biases = ws->lut_biases();
  job.C_tile = C.data();

  int ret = RunTiledQGeMMParallel(&job, 1, 1);
  if (ret != 0) {
    // Fallback: SessionWorkspace compute path
    if (!ws->ComputeTile(A.data(), scales.data(), C.data(), kWarmMLogical, kWarmK, kWarmN,
                         kWarmBits)) {
      return Status::kKernelFail;
    }
  }
  return Status::kOk;
}

Status InferTokenBatch(uint64_t session_id, const char* prompt, char* out_buf, size_t out_cap) {
  if (!prompt || !out_buf || out_cap == 0) {
    return Status::kInvalid;
  }

  Status st = WarmKernel(session_id, -1);
  if (st != Status::kOk) {
    return st;
  }

  std::string model;
  {
    std::lock_guard<std::mutex> lock(g_mu);
    Session* s = FindUnlocked(session_id);
    if (!s) {
      return Status::kNoSession;
    }
    model = s->model_path;
  }

  // Until llama is in-process: return structured status (kernel warm OK).
  std::snprintf(out_buf, out_cap,
                "[tmac_sa] warm_ok model=%s prompt_len=%zu shape=m128_k3200_n1_b2 qos=%d",
                model.c_str(), std::strlen(prompt),
                static_cast<int>(tmac_sched::GetThreadQoS()));
  return Status::kOk;
}

Status ReleaseSession(uint64_t session_id) {
  std::lock_guard<std::mutex> lock(g_mu);
  auto it = g_sessions.find(session_id);
  if (it == g_sessions.end()) {
    return Status::kNoSession;
  }
  g_sessions.erase(it);
  return Status::kOk;
}

}  // namespace tmac_sa
