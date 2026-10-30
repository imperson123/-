// SA smoke without samgr: Create → Load → Prepare → Infer → Release
#include "lut_sa.h"
#include "qos_policy.h"

#include <cstdio>
#include <cstring>

int main() {
  using tmac_sa::Status;

  tmac_sched::ApplyPerceptionSignal("foreground");

  uint64_t sid = 0;
  tmac_sa::SessionConfig cfg;
  cfg.n_threads = 4;
  Status st = tmac_sa::CreateSessionWithConfig(&sid, cfg);
  if (st != Status::kOk) {
    std::fprintf(stderr, "CreateSession failed %d\n", static_cast<int>(st));
    return 1;
  }

  st = tmac_sa::LoadModel(sid, "hf-bitnet-3b.gguf");
  if (st != Status::kOk) {
    std::fprintf(stderr, "LoadModel failed %d\n", static_cast<int>(st));
    return 2;
  }

  st = tmac_sa::PrepareWorkspace(sid, 3200, 1);
  if (st != Status::kOk) {
    std::fprintf(stderr, "PrepareWorkspace failed %d\n", static_cast<int>(st));
    return 3;
  }

  char out[512];
  st = tmac_sa::InferTokenBatch(sid, "hello lut-sa", out, sizeof(out));
  if (st != Status::kOk) {
    std::fprintf(stderr, "InferTokenBatch failed %d\n", static_cast<int>(st));
    return 4;
  }
  std::printf("%s\n", out);

  tmac_sched::ApplyPerceptionSignal("background");
  st = tmac_sa::WarmKernel(sid, 2);
  if (st != Status::kOk) {
    std::fprintf(stderr, "WarmKernel failed %d\n", static_cast<int>(st));
    return 5;
  }

  st = tmac_sa::ReleaseSession(sid);
  if (st != Status::kOk) {
    std::fprintf(stderr, "ReleaseSession failed %d\n", static_cast<int>(st));
    return 6;
  }

  std::printf("sa_smoke PASS\n");
  return 0;
}
