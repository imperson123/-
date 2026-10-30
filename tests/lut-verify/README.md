# LUT 内核对照验证工具

诊断 T-MAC 经典静默失败（内核返回 0、无报错、输出全 0/乱码）的 A/B 工具：
真实 HF 权重 → 已知良好输入 + NumPy 参考答案 → 设备上跑内核 → NMSE 对比。

**端到端也已在安卓真机验证**（完整 LLM 推理，11.55 tok/s）：`android-e2e-run.txt`。

- `gen_testdata_ffn.py` — 生成 A.bin（量化权重）/ B.bin（激活）/ S.bin（scale）/ Cref.bin（参考答案）
- `run_test_dev.cpp` — 跑 preprocessor + 分块 qgemm，打印逐层诊断 + NMSE，可带参考对比

用法与判读见 `docs/ANDROID-FIX.md` §二（三条线索定位法）。

基线成绩（本工具实测）：

| 配置 | NMSE | 结论 |
|---|---|---|
| aarch64 NEON 内核，chunk=64（qemu 验证） | 8.4e-05 | PASS |
| **aarch64 NEON 内核，chunk=64（vivo V2323A 真机，Android 16）** | **8.397e-05** | **PASS——与 qemu 逐位一致**（原始记录：`android-device-run.txt`） |
| x64-ags64-**f32** 内核，chunk=64（Kali WSL / i7 实测） | **8.765e-05** | **PASS** |
| 同上但 chunk=128 | 2.17 | FAIL —— 分块约定坑 |
| x64-ags64 内核直喂 fp32 激活 | ~1e72 | FAIL —— fp16 契约坑（ggml 集成层转 fp16 后才正确） |

后两行是**故意保留的演示**：它们正是"返回 0、不报错、输出垃圾"的两大经典失败模式。
`selfcheck.sh` 可一键复现整张表（需 OHOS SDK + WSL；路径用环境变量覆盖）。

> 注意：`--kcfg`（默认 aarch64-hf-bitnet-3b）必须与你编译进二进制的 kernels.cc 配套，
> 否则必错——这本身就是坑 1 的演示。
