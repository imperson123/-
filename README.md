# Hongmeng Linglong Core

<p align="center">
  <img src="assets/logo/linglong-calligraphy.png" alt="鸿蒙玲珑核 · 国风金红书法项目标志" width="560">
</p>

<p align="center"><strong>Native on-device low-bit LLM inference for HarmonyOS NEXT.</strong></p>

<p align="center"><code>HarmonyOS NEXT</code> · <code>ARM64 NEON</code> · <code>Low-bit LUT inference</code> · <code>Offline text generation</code></p>

<p align="center">
  <a href="#overview">Overview</a> ·
  <a href="#why-linglong-core">Motivation</a> ·
  <a href="#key-innovations">Innovations</a> ·
  <a href="#system-architecture">Architecture</a> ·
  <a href="#real-device-demonstration">Demo</a> ·
  <a href="#performance-evaluation">Evaluation</a> ·
  <a href="#quick-start">Quick Start</a> ·
  <a href="#documentation">Docs</a>
</p>

---

## Overview

**鸿蒙玲珑核（Hongmeng Linglong Core）** 是面向 **HarmonyOS NEXT** 的原生端侧低比特大语言模型离线推理工程。项目将本地 GGUF 模型、ArkTS 原生界面、N-API 桥接层与 C++ 推理运行时连接为一条完整执行链，并围绕移动终端的计算与内存约束，集成 **T-HarmonyOS 低比特推理适配、Token 级 KV Cache 前缀复用、应用生命周期感知资源管理**等机制。

与仅通过 HTTP API 调用云端模型的应用不同，当前已验证路径直接在设备上读取本地模型权重并执行文本生成。项目的主要成果是一套**真实可执行的原生推理链路及其系统级优化实践**，而不只是聊天应用界面。模型权重需要由使用者另行准备，且必须与当前 T-MAC 计算后端及目标 ABI 兼容。

**Representative results — 2026-09-30 交付版及相关对照实验：**

| **1.69 s** | **22.00 tokens/s** | **6 / 6 tokens** | **≈ 1,299 MiB** |
| :---: | :---: | :---: | :---: |
| 真机模型加载 | 真机首轮生成吞吐 | KV Cache 完全前缀命中 | 模拟器开启调度后后台 PSS 下降 |
| Pura 70 Pro | HarmonyOS 6.1 | 特定完全命中场景 TTFT 1.32 ms | 前台约 1370 → 后台约 71 MiB |

> **读数边界：** 上述数据不是同一类性能测试。1.69 s、22.00 tokens/s 和 6/6 来自最终 HAP 真机复测；约 1,299 MiB 的后台内存变化来自官方 Pura X View 模拟器调度 A/B。下方另外列出的 **222× / 173×** 是历史 i2 与 FP16 **不同精度、不同执行配置**的端到端方案对照，不代表单个 T-MAC 算子的独立加速。详细原始记录见 [Benchmarks](docs/BENCHMARKS.md)。

## Why Linglong Core?

### Four Practical Challenges for On-device LLMs

当大模型从云端迁移至移动设备时，问题不只是“能否运行”。模型的本地交付、计算方式、上下文状态以及应用生命周期会共同影响可用性。鸿蒙玲珑核围绕四项实际挑战组织技术实现：

| 痛点 | 端侧部署的现实问题 | 对应解决机制 |
| --- | --- | --- |
| **01 · 云端依赖 / 本地数据处理需求** | 核心推理依赖远程 API 时，弱网络影响可用性，用户文本也需要离开终端 | 本地 GGUF → ArkTS → N-API → C++；设备侧 Token 生成，核心推理不需要云端模型 API |
| **02 · 模型体积与算力受限** | 高精度权重增加存储、加载与内存压力；移动 CPU 缺少对某些超低比特形式的直接算术支持 | T-HarmonyOS 低比特计算适配，基于上游 T-MAC LUT 方法并整合 ARM64 NEON 原生执行路径 |
| **03 · 连续交互的重复计算** | 共享 Prompt 前缀的连续请求若全部重新 Prefill，会浪费已有计算 | Token ID 最长公共前缀匹配、KV 状态保留、分歧尾部裁剪与增量解码 |
| **04 · 鸿蒙原生适配与系统资源管理** | 模型沙箱文件访问、ArkTS/C++ 跨语言调用、异步生成和应用前后台状态需要协同 | 原生 HAP 集成、N-API 流式桥接与 60 秒后台延迟释放机制 |

**设计思路：** 用低比特方法降低部分权重存储与计算开销，用 KV Cache 复用减少“不必再算”的 Prompt，用生命周期策略避免“不再使用时仍长期占着内存”。这些机制分别处理不同环节，不能用一个笼统的“总加速比”代替评价。

## Key Innovations

### 01 · Native Offline Inference on HarmonyOS NEXT

原生应用使用 ArkTS 承载模型管理、推理配置、Prompt 输入及流式输出。用户通过系统文件选择器导入兼容 GGUF，应用将文件复制到自身沙箱，并经 N-API 向 C++ 后端传递本地模型路径。推理运行时完成模型加载、Tokenizer、Prefill、Decode 和采样，通过线程安全回调逐 Token 更新界面。

```text
Local GGUF in App Sandbox
        │
        ▼
HarmonyOS ArkTS UI
        │  native API / async callbacks
        ▼
N-API Bridge · libtmac_hap.so
        │
        ▼
llama.cpp / GGML + T-HarmonyOS low-bit kernels
        │
        ▼
Local inference result → streamed tokens
```

**工程价值：** 把高开销的原生计算隔离在 C++ 层，把文件权限、界面状态和前后台事件留在 HarmonyOS 应用层；不要求把模型生成请求转发至云端。当前交付形式为**用户态 HAP**，并非已部署的系统级推理服务。

**源码入口：** `ohos/hap/entry/src/main/ets/pages/Index.ets`、`ohos/hap/entry/src/main/cpp/napi_init.cpp`。路径如因仓库外层封装有所不同，请参考 [源码位置说明](docs/SOURCE_PATHS.md)。

### 02 · T-HarmonyOS: Low-bit LUT Inference Adaptation

对于权重量化至少量比特而激活仍采用较高精度的矩阵运算，传统实现可能产生解码或反量化开销。**T-MAC** 研究提出利用低比特权重的有限模式，通过位级表示、查找表（Lookup Table, LUT）及累加执行部分混合精度计算，减少相关数据转换与乘法负担。

**T-HarmonyOS** 是项目面向 HarmonyOS 原生环境形成的低比特推理**工程适配方案**：将 T-MAC 相关计算后端、llama.cpp/GGML、ARM64 NEON 编译与调用接口连接到可运行的 HAP 工程，围绕模型加载、ABI、原生库链接、沙箱路径及推理状态进行集成和验证。

| 层次 | 主要内容 | 对端侧的意义 |
| --- | --- | --- |
| 权重表示 | i2 / 2.44 BPW 的历史测试配置 | 降低权重文件体积 |
| LUT 方法 | 权重编码索引、激活相关查找表和累加 | 为低比特执行提供计算路径 |
| ARM64 集成 | NEON / FP16 目标架构与原生静态库 | 在目标 CPU 上运行相应内核 |
| 原生推理封装 | GGUF 加载、上下文建立、流式生成 | 从单独算子转化为应用可调用的推理运行时 |

### 03 · Token-level KV Cache Prefix Reuse

连续请求可能共享相同 Token 前缀。原生推理上下文维护已缓存 Token 序列，收到新的 Prompt 时按 **Token ID** 而非字符串字符匹配最长公共前缀：命中部分保留对应 KV 状态；如旧缓存尾部与新请求不一致，则裁剪失效尾部；仅针对新增 Token 执行必要的 `llama_decode()`。

```text
Cached:      [A] [B] [C] [D] [E] [F]
              │   │   │   │   │   │
New prompt:  [A] [B] [C] [D] [E] [F] [G] [H]
             └── cached prefix ───┘  └─ new ┘

Common prefix: 6 tokens → reuse existing KV, decode G/H
```

在最终版 Pura 70 Pro 真机测试中，控制台记录：

```text
kv chain: reused 6 of 6 prompt tokens (prefix chain kept, delta decoded)
prompt eval time = 0.00 ms / 6 tokens
ttft = 1.32 ms
```

这是特定**完全前缀命中**请求中的观测值，并不代表冷启动、全新 Prompt 或普通对话的通用 TTFT。模型释放后，旧推理上下文的 KV 状态也不能继续假定有效。

### 04 · Lifecycle-aware Memory Management

模型常驻内存有利于快速返回，但会增加长期后台内存压力；立即释放则导致短时间应用切换也需要重新加载。项目根据 HarmonyOS 前后台事件实施带延迟的资源控制：进入后台时先保留运行时；后台持续约 **60 秒**、启用策略、模型已加载且当前空闲时释放；用户快速返回则取消待执行的释放任务；恢复前台可依据配置按需重载。

```text
Foreground / Model Loaded
             │ switch to background
             ▼
    Background, keep model temporarily
             │
        ┌────┴─────┐
        │          │
   Return early    Background > 60 s
        │          │ loaded + idle + policy on
     Keep KV        ▼
                  Release model & context
                         │
                   Return to foreground
                         │
                  Optional reload
```

该功能已在官方模拟器 A/B 对照中观察到显著的后台 Total PSS 差异。它属于**应用级延迟释放策略**，`ohos/sa/` 中的 SystemAbility 内容仍是实验性原型，不能作为零售设备已部署系统服务的证据。

## System Architecture

完整执行链路从本地模型文件开始，经 ArkTS 原生界面、N-API 桥接层、C++ 推理运行时和低比特计算后端，最终在设备上输出 Token。KV 前缀复用和生命周期策略围绕模型上下文建立状态控制关系。

<p align="center">
  <a href="assets/architecture/system.png"><img src="assets/architecture/system.png" alt="鸿蒙玲珑核系统架构图：GGUF、ArkTS、N-API、低比特引擎、KV Cache 与生命周期策略" width="1050"></a>
</p>

| System Layer | Component / Responsibility |
| --- | --- |
| Model storage | 兼容 GGUF 导入应用沙箱；模型本体不随仓库分发 |
| HarmonyOS application | ArkTS 页面、参数设置、文件选择、生命周期事件 |
| N-API bridge | 原生加载/生成接口、异步执行、线程安全 Token 回调 |
| Inference runtime | llama.cpp/GGML、Tokenizer、Prefill、Decode、采样 |
| Low-bit computing | T-MAC LUT 相关内核与 ARM64 NEON 适配 |
| Runtime state | KV Cache 链、模型/上下文释放及恢复状态 |

[Architecture Notes](docs/ARCHITECTURE.md) · [Original Architecture PNG](assets/architecture/system.png)

## Real-device Demonstration

### HarmonyOS Application Interface

以下是实际应用界面的静态记录，展示本地文本生成应用的 UI 形态。**当前提供的源文件虽然以 `.gif` 命名，但其实际编码为单帧 PNG，并不包含生成过程动画**；本仓库以 PNG 形式保留，避免误称为可播放动图。若后续提供了真实的多帧录屏 GIF，可在此位置替换。

<p align="center">
  <a href="assets/screenshots/harmonyos-app-ui.png"><img src="assets/screenshots/harmonyos-app-ui.png" alt="鸿蒙玲珑核真机应用对话页面静态截图" width="280"></a>
</p>

### Final HAP · Real-device Verification

最终版 HAP 的四张关键截图分别对应本地模型加载、首轮生成、链式 KV Cache 复用日志与 ARM64 内核自测。这组证据展示从应用操作到原生输出的实际执行过程。

<p align="center">
  <a href="assets/screenshots/verification-final-4.png"><img src="assets/screenshots/verification-final-4.png" alt="Pura 70 Pro 最终 HAP 四宫格：模型加载、文本生成、KV Cache 6/6 命中、内核 PASS" width="1100"></a>
</p>

| Verification | Observed Evidence |
| --- | --- |
| Local model | 约 966 MB GGUF；最终版加载耗时 **1.69 s** |
| Text generation | 首轮 **22.00 tok/s**；二轮 **22.38 tok/s** |
| KV prefix reuse | 原生日志 `reused 6 of 6 prompt tokens` |
| Full-prefix TTFT | 对应特定二轮请求 **1.32 ms** |
| ARM64 kernel smoke test | `m=128 k=3200 n=1 b=2`，**PASS: zero-in → zero-out** |

<details>
<summary><strong>查看四张原始真机复测截图（点击展开）</strong></summary>

| [模型加载 1.69 s](assets/screenshots/device-model-loading.jpeg) | [首轮生成 22.00 tok/s](assets/screenshots/device-first-generation.jpeg) | [KV Cache 日志 6/6](assets/screenshots/device-kv-log.jpeg) | [ARM64 内核自测 PASS](assets/screenshots/device-kernel-selftest.jpeg) |
|:---:|:---:|:---:|:---:|
| <a href="assets/screenshots/device-model-loading.jpeg"><img src="assets/screenshots/device-model-loading.jpeg" width="165" alt="真机模型加载截图"></a> | <a href="assets/screenshots/device-first-generation.jpeg"><img src="assets/screenshots/device-first-generation.jpeg" width="165" alt="真机文本生成截图"></a> | <a href="assets/screenshots/device-kv-log.jpeg"><img src="assets/screenshots/device-kv-log.jpeg" width="165" alt="真机 KV 命中日志"></a> | <a href="assets/screenshots/device-kernel-selftest.jpeg"><img src="assets/screenshots/device-kernel-selftest.jpeg" width="165" alt="原生计算内核自测截图"></a> |

</details>

[Real-device Raw Record](docs/verification/2026-09-30-device.md)

## Performance Evaluation

所有指标均来自项目历史交付与现有记录，并非在本次 README 排版过程中重新执行。这里将**最终真机复测、官方模拟器 A/B、历史跨平台基线、真机内存辅助观测**分开报告。不同设备、软件版本和内存统计口径的结果不可直接合并。

### A · Current HarmonyOS 6.1 Real-device Inference

**设备：** HUAWEI Pura 70 Pro（HBN-AL00） · **记录日期：** 2026-09-30 · **模型：** 约 966 MB T-MAC 兼容 GGUF · **执行：** ARM64 CPU。

| Measurement | Result | Scope |
| --- | ---: | --- |
| Model load | **1.69 s** | 已安装 HAP，本地 GGUF |
| Generation, first round | **22.00 tok/s** | 指定英文 Prompt，32-token 生成测试 |
| Generation, second round | **22.38 tok/s** | 连续请求，已有前缀状态 |
| Mean token latency | **45.46 / 44.68 ms/token** | 首轮 / 二轮 |
| Second-round TTFT | **1.32 ms** | **仅当次 6/6 完全前缀命中** |
| Peak VmHWM | **1395.1 MB** | 进程峰值常驻高水位，非 PSS |
| ARM64 kernel self-test | **PASS** | 指定形状的零输入冒烟测试 |

<p align="center">
  <a href="assets/figures/fig03_device_inference_slope.png"><img src="assets/figures/fig03_device_inference_slope.png" width="1050" alt="真机两轮生成吞吐和平均 Token 延迟：22.00 到 22.38 tok/s、45.46 到 44.68 ms/token"></a>
</p>

**Interpretation.** 两轮生成吞吐相近，说明该测试配置下能够连续完成本地推理。二轮速度变化不等于 KV 复用对稳态 Decode 的独立加速；KV 复用主要减少重叠 Prompt 的计算。图中只有两个离散数据点，不能由此声称长期稳定性或多设备统计结论。

[Device Verification](docs/verification/2026-09-30-device.md)

### B · Historical Android / HarmonyOS Baselines

历史对照包含两种完整部署配置：T-HarmonyOS 采用 **i2 / 2.44 BPW**，原生 llama.cpp 采用 **FP16 / 16 BPW**。因此此处**不是同精度 A/B**，而是权重表示、模型体积、计算路径共同变化的方案级对照。

| Metric | Android / T-HarmonyOS | Android / FP16 | HarmonyOS / T-HarmonyOS | HarmonyOS / FP16 |
| --- | ---: | ---: | ---: | ---: |
| Model size | 965.21 MiB | 6.19 GiB | 965.21 MiB | 6.19 GiB |
| Load time | 2.03 s | 15.91 s | 0.53 s | 117.41 s |
| Prompt throughput | 4.17 tok/s | 1.64 tok/s | 8.87 tok/s | 0.07 tok/s |
| Generation throughput | 3.92 tok/s | 1.57 tok/s | 6.90 tok/s | 0.04 tok/s |
| Per-token latency | 254.90 ms | 638.58 ms | 144.91 ms | 23181.20 ms |
| Fixed-length generation | 32.88 s | 82.36 s | 18.64 s | 1812.26 s |

<p align="center">
  <a href="assets/figures/fig01_historical_baseline.png"><img src="assets/figures/fig01_historical_baseline.png" width="1080" alt="历史 Android 与 HarmonyOS 跨平台端到端基线四子图"></a>
</p>

**历史 HarmonyOS 方案间主要对比：**

| Metric | T-HarmonyOS | Selected FP16 Baseline | Ratio |
| --- | ---: | ---: | ---: |
| Model loading | 0.53 s | 117.41 s | **≈ 222×** 耗时比值 |
| Prompt throughput | 8.87 tok/s | 0.07 tok/s | **≈ 127×** 吞吐比值 |
| Token generation | 6.90 tok/s | 0.04 tok/s | **≈ 173×** 吞吐比值 |
| Fixed-length generation | 18.64 s | 1812.26 s | **≈ 97×** 耗时比值 |
| Weight file size | 965.21 MiB | 6.19 GiB | **≈ 6.6×** 大小比值 |

> **严谨性说明：** 上述倍数来自历史的完整配置对比，**不是 T-MAC LUT 算子单因素加速比**，更不能与 2026-09-30 最终真机 22 tok/s 直接相除后宣称新版独立优化幅度。历史图的 `128-token generation` 与部分早期原始统计存在 128/129 Token 标注差异；FP16 极低吞吐的测试环境因素仍需后续复核。

[Full Baseline Notes](docs/BENCHMARKS.md)

### C · Lifecycle-aware Memory: Official Emulator A/B

**环境：** 官方 Pura X View 模拟器 · **比较方式：** 相同模型、相同后台约 70 s，调度**开启 / 关闭**两组。指标为进程 **Total PSS**，不能与真机 VmHWM 或 native heap PSS 混称。

| Lifecycle stage | Scheduling enabled | Scheduling disabled |
| --- | ---: | ---: |
| Foreground loaded | 1370 MiB | 1360 MiB |
| Background ≈ 70 s | **71 MiB** | **1356 MiB** |
| Foreground resumed | 1364 MiB | 1356 MiB |

<p align="center">
  <a href="assets/figures/fig02_emulator_scheduling.png"><img src="assets/figures/fig02_emulator_scheduling.png" width="1050" alt="模拟器生命周期调度 A/B：开启后台约 71 MiB，关闭约 1356 MiB"></a>
</p>

开启策略时，前台约 1370 MiB 下降到后台约 71 MiB，约减少 **1299 MiB（94.8%）**；关闭策略时后台仍为约 1356 MiB。三个状态是**离散测量**，并非连续时间曲线；PSS 下降不能直接换算为功耗节省。

[Raw A/B Evidence](docs/verification/2026-09-30-comparison.md)

### D · Real-device Memory Observations

**环境：** Pura 70 Pro 真机，测量对象是 **native heap PSS**，并非上节的进程 Total PSS。

| Observation | Native heap PSS |
| --- | ---: |
| Foreground / model loaded | 1328 MB |
| Background, 9 s | 1327 MB |
| Background, 30 s | **649 MB** |
| Resumed foreground | 652 MB |

<p align="center">
  <a href="assets/figures/fig04_device_memory.png"><img src="assets/figures/fig04_device_memory.png" width="1050" alt="Pura 70 Pro 真机前后台 native heap PSS 离散观测：1328 到 649 MB"></a>
</p>

后台 30 s 相比前台观察到约 **679 MB** 下降。但应用设定的主动释放阈值约为 **60 s**，原记录没有同步提供明确释放函数触发日志，故**不能从该图单独判定下降原因**；可能同时包含系统回收的影响。模拟器 A/B 才是应用级策略效果的主要对照证据。

[Device Lifecycle Record](docs/verification/2026-09-30-device.md)

### E · Native Runtime Evidence

下面的终端画面来自开发/历史执行材料，是对原生推理工程的补充说明。上传素材实际为**静态 PNG**，不应作为真机实时录屏或多帧生成动画的证据；它也不用于替代上方的最终 HAP 验证截图。

<p align="center">
  <a href="assets/screenshots/native-terminal.png"><img src="assets/screenshots/native-terminal.png" alt="早期原生推理终端运行记录静态截图" width="640"></a>
</p>

<details>
<summary>展开查看验证范围和统计口径</summary>

- 当前 HAP 真机记录中的 1.69 s、22.00 tok/s、22.38 tok/s、6/6、1.32 ms，源自 **2026-09-30 最终版复测**。
- 模拟器 70 s 的 PSS 对照是**应用级资源策略**实验。
- 真机 30 s 的 native heap PSS 变化只构成**现象级观测**。
- 历史 i2 对 FP16 实验是**跨配置、跨环境**的端到端方案对照。
- ARM64 内核 PASS 是**选定矩阵形状的 zero-in → zero-out 冒烟测试**，不等同于全面数值误差验证。
- 图片、截图和日志的原始来源及本轮未复测的情况见 [Reproducibility](docs/REPRODUCIBILITY.md)。

</details>

## Quick Start

> 本节给出基于现有工程的构建与操作路径，**不是本次 README 更新中完成的新构建验证**。开发者需要自行检查 SDK/NDK、签名、模型格式和目标 ABI。

### 1. Prerequisites

| Item | Requirement |
| --- | --- |
| IDE | DevEco Studio |
| SDK / NDK | 与工程配置匹配的 HarmonyOS SDK、原生工具链 |
| Target | `arm64-v8a` 真机；部分模拟器场景为 `x86_64` |
| Runtime | 仓库对应的 llama.cpp / GGML 静态库和 T-MAC 相关头文件 |
| Model | **单独获取**且与目标 T-MAC 内核及 ABI 兼容的 GGUF |
| Signing | 与目标设备对应的 HAP 签名配置 |

### 2. Open and Build

在完整仓库中使用 DevEco Studio 打开工程目录：

```text
ohos/hap/
```

等待工程同步，确认原生静态库及 staging 头文件存在，选择对应 ABI 和签名配置后构建/运行 `entry` 模块。**不要只复制 `ohos/hap/` 子目录**；其 CMake 配置会向仓库根目录查找依赖。若当前仓库将源码封装于 `LUT-SA/`，使用实际路径 `LUT-SA/ohos/hap/`。

### 3. Import and Load a Model

在应用「模型」页选择兼容的 `.gguf` 文件，等待其导入应用沙箱并加载。当前导入代码对固定目标文件采用截断写入，**请保留模型备份、确保空间充足，并勿在写入期间关闭应用**。模型格式不匹配可能导致加载失败或原生错误。

### 4. Run Local Generation

进入「对话」页，输入 Prompt 并生成；ArkTS 经 `generateStreamAsync()` 获取原生 Token 回调。连续请求的 KV 复用比例取决于 Token 前缀是否实际匹配，不保证每次命中。

### 5. Inspect Logs / Troubleshooting

可结合 `hilog` 与应用输出查看本地加载、生成性能、KV 前缀复用与计算内核测试状态。遇到签名、模型路径、量化格式或 ABI 不匹配时，请先参考 [Troubleshooting](docs/TROUBLESHOOTING.md) 与 [Known Issues](docs/KNOWN_ISSUES.md)。

## Repository Guide

以下路径为源码原始工程结构示意；若当前公开仓库把源码放在 `LUT-SA/` 子目录下，请以实际仓库树为准修改源码链接，**素材目录 `assets/` 不需要改**。

```text
.
├── assets/
│   ├── logo/                # 国风项目标志
│   ├── architecture/        # 系统架构图
│   ├── figures/             # 历史、真机和模拟器图表
│   └── screenshots/         # 真机与终端验证材料
├── docs/
│   ├── ARCHITECTURE.md
│   ├── BENCHMARKS.md
│   ├── REPRODUCIBILITY.md
│   ├── SOURCE_PATHS.md
│   ├── TROUBLESHOOTING.md
│   ├── KNOWN_ISSUES.md
│   ├── THIRD_PARTY.md
│   └── verification/
│       ├── 2026-09-30-device.md
│       └── 2026-09-30-comparison.md
├── ohos/hap/               # ArkTS + N-API/C++ HAP（或 LUT-SA/ohos/hap/）
├── ohos/sa/                # 实验性 SystemAbility 原型
└── 3rdparty/llama.cpp/     # 推理运行时依赖或 fork
```

## Documentation

| Guide | Content |
| --- | --- |
| [Architecture](docs/ARCHITECTURE.md) | 原生调用链、状态管理与架构边界 |
| [Benchmarks](docs/BENCHMARKS.md) | 历史跨平台、真机性能及 A/B 数据表 |
| [Reproducibility](docs/REPRODUCIBILITY.md) | 原始证据、测试参数、尚未完成的重复验证 |
| [Device Verification](docs/verification/2026-09-30-device.md) | 2026-09-30 最终 HAP 真机复测原始记录 |
| [Emulator A/B](docs/verification/2026-09-30-comparison.md) | 官方 Pura X View 模拟器比较实验原始记录 |
| [Source Paths](docs/SOURCE_PATHS.md) | 公开仓库中源码目录层级对照与 Codex 迁移检查 |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | 安装、签名、导入、ABI 与格式兼容问题 |
| [Known Issues](docs/KNOWN_ISSUES.md) | 当前功能覆盖与需要收敛的安全和工程问题 |
| [Third-party / License](docs/THIRD_PARTY.md) | 上游技术来源、署名与许可证边界 |

## Open Source & Achievements

鸿蒙玲珑核延续团队在**端侧推理、国产操作系统适配与开源工程实践**中的持续积累。团队曾获得**第三届开放原子大赛基于开源鸿蒙的 AI 应用赛道全国总冠军**，并在 **CCF 开源创新大赛**等活动中取得国家级奖项。相关荣誉涉及团队不同时期的多个开源项目，并非全部由当前仓库同一版本单独获得。

| Team Open-source Achievement | Recognition |
| --- | --- |
| 第三届开放原子大赛 · 基于开源鸿蒙的 AI 应用赛道 | **全国总冠军** |
| 第八届 CCF 开源创新大赛 | **国家级三等奖（团队其他开源项目）** |
| 开源先锋团队相关活动 | **团队荣誉与激励** |
| 累计开源竞赛与创新实践奖励 | **超过 ¥40,000** |

感谢**中北大学**对团队科技创新、项目研发和开源实践提供的支持与资助；也感谢 T-MAC、llama.cpp、GGML 等研究者及社区贡献者。团队相关开源项目持续获得关注，GitHub Star 逐步增长。我们希望通过可核查的实验数据、清晰的技术边界与持续维护，使相关成果能够被更多开发者使用、复现和改进。

> **开源不是竞赛终点，而是持续工程演进的起点。**

## Compatibility & Current Limitations

| Scope | Current Status |
| --- | --- |
| User-facing inference | 本地文本生成；核心路径不依赖云端模型 API |
| Runtime form | 已验证为**用户态 HAP** |
| Model compatibility | 指定 T-MAC 兼容 GGUF；**非任意 GGUF 格式均支持** |
| Compute | 当前真机主要使用 ARM64 CPU / NEON；**未宣称通用 NPU 加速** |
| KV Cache | 同一有效上下文内的 Token 前缀匹配与复用 |
| Lifecycle | 应用级约 60 s 后台延迟释放和可选重载 |
| SystemAbility | `ohos/sa/` 为实验性原型，非已部署零售设备服务 |
| Multimodality | 当前可核查主路径为文本生成 |
| Benchmark coverage | 不同实验阶段分开报告；缺少统一长时间、多设备和同精度消融 |

## Research Foundations, Citation & License

**T-MAC (EuroSys 2025)**  
Jianyu Wei, Shijie Cao, Ting Cao, Lingxiao Ma, Lei Wang, Yanyong Zhang, Mao Yang. *T-MAC: CPU Renaissance via Table Lookup for Low-Bit LLM Deployment on Edge*. Proceedings of the Twentieth European Conference on Computer Systems (EuroSys), 2025, pp. 278–292. DOI: [10.1145/3689031.3696099](https://doi.org/10.1145/3689031.3696099).

**Related open-source foundations:** [Microsoft/T-MAC](https://github.com/microsoft/T-MAC) · [llama.cpp](https://github.com/ggml-org/llama.cpp) · [GGML](https://github.com/ggml-org/ggml)

---

<p align="center"><strong>鸿蒙玲珑核 · Hongmeng Linglong Core</strong></p>
<p align="center"><em>Native inference · Low-bit computing · KV Cache reuse · Lifecycle-aware memory management</em></p>
<p align="center">让大模型，真正运行在鸿蒙设备本地。</p>
