# Android / PC 路径「能跑但不输出」排坑与修复指南

> 本仓**安卓路径专区** · 其他平台：桌面 [`docs/PC-BUILD.md`](PC-BUILD.md) · 鸿蒙 [`ohos/FULL-REPORT.md`](../ohos/FULL-REPORT.md)
>
> **✅ 端到端已真机验证（2026-09-28）**：vivo V2323A（SD 8 Gen 2 · Android 16）跑通完整 LLM 推理，
> 输出正确连贯文本，峰值 **11.55 tok/s**（t4 冷机）/ 持续 ~10.6，load 1.46 s——
> 原始记录 [`tests/lut-verify/android-e2e-run.txt`](../tests/lut-verify/android-e2e-run.txt)。
> 同一静态二进制横跨 鸿蒙/安卓/qemu 三环境可运行。
>
> **症状**：编译通过、程序不崩溃、`qgemm_lut_int8` 返回 0（成功）、无任何报错——但输出全 0 / 全 NaN / 乱码。
> 我们在 x86-ohos 上复现并逐一修复了这套静默失败；aarch64（安卓手机）同理中招。**官方内核的数学是对的，
> 问题全部在数据契约层与集成层，且全部无错误提示。**

---

## 一、五大静默失败坑（按命中率排序）

### 坑 1：kcfg 内核绑定 —— 模型和内核必须配套

kcfg.ini 是 TVM 代码生成的"调度表"，**每个内核产物一份，条目布局互不相同**（例如 m6400_* 条目：
aarch64 用 bm=128/320，x86 用 bm=256）。模型预处理/转换时用的 kcfg 必须与运行时加载的内核产物一致，
否则内核派发或数据布局错位 → 静默输出垃圾。

- 官方 `tools/run_pipeline.py -d android -u`（预置内核）自带配套转换，路径本身没问题；
  **自己手动转换/换内核产物时最容易踩**
- 排查：`TMAC_KCFG_FILE` 指向的 kcfg 是否属于你 `#include` 的那份 kernels.cc

### 坑 2：fp16 激活契约 —— aarch64 的 preprocessor 吃 half*

`preprocessor_int8` 在 **aarch64 内核里把激活指针读作 `half*`（fp16）**，x86 内核读作 `float*`。
把 fp32 激活喂给 aarch64 内核 = 两个 fp16 被拼着解释 → LUT 全 0 → 后面全错。**这是我们 ARM 首测全 NaN 的根因。**

- 修复：进 preprocessor 前把激活转 fp16（`sizeof(tmac_float_type)==2` 分支）

### 坑 3：scales 广播 —— 内核会读到 S[120]

模型里 per-tensor 只有 1 个 scale，但内核内部按索引读到 S[120]。
只给 1 个值 = 越界读垃圾。**必须广播成 256 份缓冲区。**

### 坑 4：输出 dtype —— ARM 出 fp16、x86 出 fp32

aarch64 内核输出 C 是 **fp16**，x86（修复后）是 **float32**。按错误的 dtype 读回 = 乱码。
集成层用 `sizeof(tmac_float_type)==2 → fp16_to_fp32_row(...)` 接住。

### 坑 5：上游 memset 越界 —— 非 NEON 构建崩（已修）

`tbl_float_reset` 的 `memset(c, 0, m * sizeof(float_type))` 在非 NEON 构建下 `float_type=float`（4B）
而目标缓冲是 `half`（2B）→ 双倍越界写坏栈（SIGSEGV 或全 0）。
**本仓已修复**：`python/t_mac/intrins/tbl.cc:711` 与各 kernels 产物改用 `sizeof(_Float16)`。
从官方仓库拉代码记得带上这个修复。

> 走 llama.cpp 集成的再加 4 处 fork 层修复（`tmac_float_type` 定义、`transform_tensor` 漏调、
> mmap 只读页写崩、Q4_0 误 FATAL），完整清单见 `ohos/FULL-REPORT.md` §8.1，补丁见 `ohos/patches/`。

---

## 二、诊断武器：对照参考法（10 分钟定位）

不要盯着集成层猜。用 [`tests/lut-verify/`](../tests/lut-verify/) 直接对内核做 A/B：

```bash
# ① 主机：从真实 HF 权重生成已知良好输入 + NumPy 参考答案
python tests/lut-verify/gen_testdata_ffn.py --hf-model ~/models/bitnet-b1_58-3B --outdir ./vdata

# ② 交叉编译（Android arm64；换 -I 的 artifact 目录即换内核）
$NDK_HOME/toolchains/llvm/prebuilt/*/bin/aarch64-linux-android24-clang++ \
    -O2 -march=armv8.2a+fp16 -I deploy/tuned/aarch64-hf-bitnet-3b \
    tests/lut-verify/run_test_dev.cpp deploy/tuned/aarch64-hf-bitnet-3b/kernels.cc \
    -static-libstdc++ -o run_test_dev_android

# ③ 推到手机跑（Android 允许执行 /data/local/tmp，比鸿蒙零售机宽松）
adb push run_test_dev_android vdata /data/local/tmp/
adb shell /data/local/tmp/run_test_dev_android /data/local/tmp/vdata
```

读输出的三条线索：

| 输出 | 含义 |
|---|---|
| `qlut nonzero: 0/12800` | preprocessor 没吃到有效激活 → 坑 2（dtype 契约） |
| `qgemm ret=0` 但 `C_impl` 全 0 / NaN | 坑 1（kcfg 不配套）或坑 3（scales 没广播） |
| `NMSE < 1e-3 PASS` | 内核层健康，问题在更上层集成 |

我们的基线（本仓 `tests/lut-verify/` 实测）：

- **安卓真机**（vivo V2323A · iQOO Neo9 · Android 16 · arm64）：aarch64 NEON 内核 vs NumPy 参考
  **NMSE 8.397e-05**，与 qemu 模拟**逐位一致**（原始记录 `tests/lut-verify/android-device-run.txt`）
- qemu-aarch64：NMSE 8.4e-05（`ohos/ARM64-VALIDATION.md`）
- x64-ags64-f32 内核 chunk=64：NMSE 8.765e-05（Kali WSL 实测）

错误配对的复现数据（chunk=128 / fp16 契约违背）见 `tests/lut-verify/README.md` 的对照表。

## 三、Android 构建注意

1. **指令集**：主流安卓机是 armv8.2。T-MAC fork 的 CMake 对 aarch64 会误选 `-march=armv8.7-a`
   （检测假阳性）→ 真机 SIGILL。**手动加 `-march=armv8.2a+fp16`**。
2. **bionic ≠ musl**：鸿蒙补丁里的 `__MUSL__` 守卫与 Android 不冲突；bionic 有 `pthread_setaffinity_np`。
3. **执行权限**：Android 允许 `adb shell` 执行 `/data/local/tmp` 二进制（鸿蒙零售机不允许——那边的
   方案是 HAP 应用，见 main 分支）。
4. **模型转换按目标架构**：安卓机跑 aarch64 内核 → 用 `deploy/tuned/aarch64-hf-bitnet-3b/kcfg.ini`
   转换模型；用 x86 的 kcfg 转出来必错（坑 1）。

## 四、本仓修复位置速查

| 修复 | 文件 |
|---|---|
| memset 越界（坑 5） | `python/t_mac/intrins/tbl.cc` + 各 `deploy/tuned/*/kernels.cc` |
| fp16/尺度/输出契约（坑 2/3/4） | llama.cpp fork 集成层（`ohos/patches/llama_cpp_ohos.patch`，架构无关部分通用） |
| CMake armv8.7 假阳性 | 构建时 `-DGGML_COMPILER_SUPPORT_MATMUL_INT8=OFF` 或手动 `-march` |
| 验证工具 | `tests/lut-verify/` |
| 全程方法论与数据 | [`ohos/FULL-REPORT.md`](../ohos/FULL-REPORT.md)（架构无关，安卓同样适用） |
