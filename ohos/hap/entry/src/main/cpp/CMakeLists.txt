# LUT-SA HAP native library — T-MAC LLM inference inside a HarmonyOS app.
#
# Links prebuilt static llama.cpp + T-MAC LUT kernels (per ABI):
#   x86_64    -> prebuilt/x86_64/lib{llama,ggml}.a     (AVX2 kernels, staging-x64 package)
#   arm64-v8a -> prebuilt/arm64-v8a/lib{llama,ggml}.a  (NEON kernels, staging-arm64 package)
#
# The kernels live INSIDE libggml.a (GGML_TMAC=ON build) — do not compile kernels.cc here again.
cmake_minimum_required(VERSION 3.16)
project(tmac_hap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ohos/hap/entry/src/main/cpp -> up 6 levels to the repo root
get_filename_component(T_MAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../" ABSOLUTE)
set(LLAMA_SRC "${T_MAC_ROOT}/3rdparty/llama.cpp")
set(PREBUILT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../prebuilt")

if(OHOS_ARCH STREQUAL "x86_64")
  set(ARCH_DIR "x86_64")
  # NOTE: no -mfma — the DevEco emulator's virtual CPU has AVX2 but NOT FMA (cpuid probe).
  set(TMAC_ARCH_FLAGS -mavx2 -mf16c)
  set(TMAC_INC "${T_MAC_ROOT}/ohos/staging-x64/t-mac/include")
else()
  set(ARCH_DIR "arm64-v8a")
  set(TMAC_ARCH_FLAGS -march=armv8.2a+fp16)
  set(TMAC_INC "${T_MAC_ROOT}/ohos/staging-arm64/t-mac/include")
endif()

message(STATUS "tmac_hap: ${OHOS_ARCH} (${ARCH_DIR}), kernels from ${TMAC_INC}")

if(NOT EXISTS "${PREBUILT}/${ARCH_DIR}/libllama.a")
  message(FATAL_ERROR "prebuilt static libs missing: ${PREBUILT}/${ARCH_DIR}/libllama.a — see ohos/hap/README.md")
endif()

add_library(tmac_hap SHARED napi_init.cpp)

target_include_directories(tmac_hap PRIVATE
  "${LLAMA_SRC}/include"        # llama.h
  "${LLAMA_SRC}/ggml/include"   # ggml.h
  "${TMAC_INC}"                 # t-mac/kernels.h (arch-specific dispatchers)
  "${T_MAC_ROOT}/ohos/shim"     # dmlc/logging.h shim
)

target_compile_options(tmac_hap PRIVATE ${TMAC_ARCH_FLAGS} -O2)
target_compile_definitions(tmac_hap PRIVATE NDEBUG)

target_link_libraries(tmac_hap PRIVATE
  "${PREBUILT}/${ARCH_DIR}/libllama.a"
  "${PREBUILT}/${ARCH_DIR}/libggml.a"
  libace_napi.z.so
  pthread
  m
  ${CMAKE_DL_LIBS}
)
