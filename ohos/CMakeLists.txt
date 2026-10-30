cmake_minimum_required(VERSION 3.16)
project(tmac_ohos LANGUAGES CXX)

# Native LUT library for OpenHarmony — zero TVM (feasibility D1/D4).
# Never define TMAC_USE_TVM_THREADPOOL.

if(NOT DEFINED T_MAC_ROOT)
  get_filename_component(T_MAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
# Normalize Windows backslash paths passed via -DT_MAC_ROOT=... (e.g. "C:\Users" breaks CMake string parsing)
file(TO_CMAKE_PATH "${T_MAC_ROOT}" T_MAC_ROOT)

if(NOT DEFINED TMAC_KERNEL_DIR)
  set(TMAC_KERNEL_DIR "${T_MAC_ROOT}/deploy/tuned/aarch64-hf-bitnet-3b")
endif()
file(TO_CMAKE_PATH "${TMAC_KERNEL_DIR}" TMAC_KERNEL_DIR)

set(TMAC_KERNELS_SOURCE "${TMAC_KERNEL_DIR}/kernels.cc")
set(TMAC_KCFG_FILE_PATH "${TMAC_KERNEL_DIR}/kcfg.ini")

if(NOT EXISTS "${TMAC_KERNELS_SOURCE}")
  message(FATAL_ERROR "kernels.cc not found: ${TMAC_KERNELS_SOURCE}")
endif()

set(TMAC_GEN_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/generated_include")
file(MAKE_DIRECTORY "${TMAC_GEN_INCLUDE}/t-mac")
configure_file(
  "${TMAC_KERNEL_DIR}/kernels.h"
  "${TMAC_GEN_INCLUDE}/t-mac/kernels.h"
  COPYONLY
)

add_library(tmac_kernels STATIC "${TMAC_KERNELS_SOURCE}")
target_include_directories(tmac_kernels
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/shim"
    "${TMAC_GEN_INCLUDE}"
    "${T_MAC_ROOT}/include"
    "${TMAC_KERNEL_DIR}"
)
target_compile_features(tmac_kernels PUBLIC cxx_std_17)
target_compile_options(tmac_kernels PRIVATE -O3 -fPIC -march=armv8.2a+dotprod+fp16)
target_compile_definitions(tmac_kernels PUBLIC "TMAC_KCFG_FILE=\"${TMAC_KCFG_FILE_PATH}\"")

add_custom_command(TARGET tmac_kernels POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tmac_kernels>/kcfg"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${TMAC_KCFG_FILE_PATH}" "$<TARGET_FILE_DIR:tmac_kernels>/kcfg/kcfg.ini"
)

option(TMAC_OHOS_BUILD_SELFTEST "Build ohos LUT selftest binary" ON)
if(TMAC_OHOS_BUILD_SELFTEST)
  add_executable(tmac_selftest selftest/main.cpp)
  target_link_libraries(tmac_selftest PRIVATE tmac_kernels)
  target_compile_options(tmac_selftest PRIVATE -O2 -march=armv8.2a+dotprod+fp16)
endif()

option(TMAC_OHOS_BUILD_SA "Build LUT SystemAbility shared lib + smoke" ON)
if(TMAC_OHOS_BUILD_SA)
  add_library(tmac_sa SHARED
    sa/lut_sa.cpp
    sa/tile_compute.cpp
    sa/tile_pool.cpp
    sa/session_workspace.cpp
    sched/qos_policy.cpp
    napi/napi_bridge_stub.cpp
  )
  target_link_libraries(tmac_sa PRIVATE tmac_kernels)
  target_include_directories(tmac_sa PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sa"
    "${CMAKE_CURRENT_SOURCE_DIR}/sched"
  )
  target_compile_options(tmac_sa PRIVATE -O2 -fPIC -march=armv8.2a+dotprod+fp16)
  set_target_properties(tmac_sa PROPERTIES OUTPUT_NAME "tmac_sa")
  # pthread via std::thread on OHOS musl
  find_package(Threads REQUIRED)
  target_link_libraries(tmac_sa PRIVATE Threads::Threads)

  add_executable(sa_smoke sa/sa_smoke.cpp)
  target_link_libraries(sa_smoke PRIVATE tmac_sa)
  target_compile_options(sa_smoke PRIVATE -O2 -march=armv8.2a+dotprod+fp16)
endif()
