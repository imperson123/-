// NAPI-shaped C ABI wrapping lut_sa (HAP 辅轨). Wire to napi_define_properties in DevEco.
#include "lut_sa.h"

#include <cstdint>
#include <cstring>
#include <string>

extern "C" {

int tmac_napi_create_session(uint64_t* out_id) {
  return static_cast<int>(tmac_sa::CreateSession(out_id));
}

int tmac_napi_load_model(uint64_t id, const char* path) {
  if (!path) {
    return static_cast<int>(tmac_sa::Status::kInvalid);
  }
  return static_cast<int>(tmac_sa::LoadModel(id, std::string(path)));
}

int tmac_napi_infer(uint64_t id, const char* prompt, char* out, size_t cap) {
  return static_cast<int>(tmac_sa::InferTokenBatch(id, prompt, out, cap));
}

int tmac_napi_release(uint64_t id) {
  return static_cast<int>(tmac_sa::ReleaseSession(id));
}

int tmac_napi_module_register_stub(void) {
  // Placeholder for napi_module registration table.
  return 0;
}

}
