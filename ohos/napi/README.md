# NAPI 辅轨说明

手机零售 HarmonyOS NEXT **不能**跑真 SystemAbility（feasibility D7）。

本目录 `napi_bridge_stub.cpp` 为 HAP 内 N-API 注册占位。DevEco 工程步骤：

1. 新建 Native C++ 模块，链接 `libtmac_kernels.a`
2. 导出 `createSession` / `inferTokenBatch` / `releaseSession` 与 `ohos/sa/lut_sa.h` 对齐
3. ArkTS 薄封装调用；视频中作为「应用内加速」辅演示

主轨仍以开发板 SA 为准。
