#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tmac_sa {

enum class Status : int32_t {
  kOk = 0,
  kInvalid = 1,
  kNoSession = 2,
  kNotLoaded = 3,
  kKernelFail = 4,
  kNoWorkspace = 5,
};

struct SessionConfig {
  int n_threads = 4;
  int act_group_size = 32;
  std::string kcfg_path;  // empty → TMAC_KCFG_FILE
};

// IPC-shaped API (feasibility P3). samgr binding is separate; these are the SA entrypoints.
Status CreateSession(uint64_t* out_session_id);
Status CreateSessionWithConfig(uint64_t* out_session_id, const SessionConfig& cfg);
Status LoadModel(uint64_t session_id, const std::string& model_path);
Status PrepareWorkspace(uint64_t session_id, int max_k, int max_n);
// Warm decode GEMV path: zero weights, shape m=128,k=3200,n=1,b=2 (bitnet tile).
Status WarmKernel(uint64_t session_id, int n_threads_override = -1);
Status InferTokenBatch(uint64_t session_id, const char* prompt, char* out_buf, size_t out_cap);
Status ReleaseSession(uint64_t session_id);

}  // namespace tmac_sa
