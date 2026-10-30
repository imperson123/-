# 鸿蒙玲珑核

**鸿蒙玲珑核——面向 HarmonyOS NEXT 的低比特大模型离线推理引擎**

本仓库提供 HarmonyOS 原生应用工程与端侧推理代码。模型文件预先由用户导入应用沙箱；加载、token 生成和缓存处理由设备上的 ArkTS、NAPI 与 C++ 代码完成，不调用云端大模型 API。下载或导入模型可能需要其他设备或网络，推理路径本身不依赖云端服务。本次源码迁移只做了静态检查，未重新执行断网、真机或完整构建验证。

## 项目与技术边界

鸿蒙玲珑核针对低比特模型在移动设备上的内存与计算约束，集成 [T-MAC](https://github.com/microsoft/T-MAC) 查表计算路线、llama.cpp/GGML 推理代码、HarmonyOS 应用和原生桥接。T-MAC 算法来自已有开源研究（[论文](https://arxiv.org/abs/2407.00088)），并非本项目原创。当前交付的应用形态是用户态 HAP；`ohos/sa/` 是 SystemAbility 实验性原型，不能视为已部署的正式系统服务。

这份源码以**文本生成**为可核查的应用路径。旧仓库中关于 MindSpore Lite、多模态、NPU、系统内核级调度以及任意机型兼容的描述，不对应本次交付的主要实现，不能据此认定这些能力已经完成。

## 软件架构

```text
系统文件选择器 / 本地 GGUF
        ↓
ohos/hap/entry/src/main/ets/pages/Index.ets
        ↓  libtmac_hap.so（ArkTS 类型声明）
ohos/hap/entry/src/main/cpp/napi_init.cpp
        ↓  llama.cpp / GGML API、KV Cache 复用
3rdparty/llama.cpp + T-MAC LUT 内核
        ↓
arm64 NEON / x86_64 预编译静态库
```

`Index.ets` 负责文件选择、模型导入、加载、生成和前后台状态处理；`napi_init.cpp` 实现异步加载、生成、文件复制、采样和缓存链管理。CMake 按架构链接 `ohos/hap/prebuilt/` 内的 `libllama.a`、`libggml.a`，并引用 `ohos/staging-{arm64,x64}/t-mac/include`。这些静态库是当前 HAP 工程的必要输入，不能当作普通构建缓存删除。内核与库的来源、重建说明见 [第三方与预编译依赖](docs/THIRD_PARTY.md)。

## 目录

| 路径 | 内容 |
| --- | --- |
| `ohos/hap/` | DevEco/Hvigor 应用工程、ArkTS 页面、NAPI 桥、双架构静态库 |
| `ohos/sa/`、`ohos/sched/` | 实验性 SystemAbility 组件、会话与资源感知原型 |
| `ohos/staging-arm64/`、`ohos/staging-x64/` | 与预编译库配套的 T-MAC 头文件 |
| `3rdparty/llama.cpp/` | 源码包内嵌的 llama.cpp/GGML fork |
| `include/`、`python/`、`deploy/`、`t-man/` | T-MAC 上游计算核心、生成与部署工具 |
| `docs/`、`tests/` | 技术说明、迁移记录、轻量级测试源码 |

## 离线推理流程与工程入口

1. 在适配的设备上准备 T-MAC 路线使用的 GGUF 模型文件；仓库不附带模型权重。
2. 打开 `ohos/hap/` DevEco 工程。用户通过系统文件选择器选取 `.gguf`，应用复制到自身沙箱。需要独立检查可用空间与文件格式；不能假定任意 GGUF 量化类型都受支持。
3. ArkTS 调用 `loadModelAsync`，经 NAPI 在原生层加载本地模型；调用 `generateStreamAsync` 输出文本。代码维护 token 前缀对应的 KV Cache 链，并在配置的前后台策略下释放或重载模型。

构建入口、SDK 与签名要求见 [`ohos/hap/README.md`](ohos/hap/README.md)。源码包提供 arm64-v8a 与 x86_64 的预编译库；本次未运行 Hvigor、HAP 安装、模型加载或基准测试。当前未对所有 HarmonyOS NEXT 版本和设备型号完成兼容性验证。

## 已有验证记录

源码包附带的 2026-09-30 记录报告：在 HUAWEI Pura 70 Pro（HBN-AL00）上，使用约 966 MB 的 T-MAC GGUF，最终版 HAP 的模型加载为 **1.69 s**，首轮生成 **22.00 token/s**，二轮完整前缀复用时 TTFT **1.32 ms**。这些数字来自既有记录，不是本次迁移复测；条件和原文摘录见 [真机复测记录](docs/verification/2026-09-30-device.md)。

同日官方模拟器 Pura X View 的记录中，开启前后台释放策略时，后台约 70 秒后的进程 Pss 从约 1370 MB 到 71 MB；关闭策略的对照约为 1356 MB。该结果仅适用于记录中的设备、模型和测试过程；真机内存变化不能单独归因于应用主动释放。参见 [对比试验记录](docs/verification/2026-09-30-comparison.md)。源码材料同时记录标准 Q4_0 GGUF 在当前 T-MAC 构建中发生原生崩溃，格式兼容性需单独验证。

## 来源、许可与历史

根目录 [`LICENSE`](LICENSE) 延续源码包的 MIT 许可；内嵌 llama.cpp/GGML fork 保留其 [MIT 许可证](3rdparty/llama.cpp/LICENSE)。第三方版权、静态库和源码坐标见 [第三方与预编译依赖](docs/THIRD_PARTY.md)。迁移前仓库历史保留在 Git 中，旧目录被本次可追溯迁移替换，见 [迁移记录](docs/MIGRATION.md)。

项目团队提供的历史说明涉及第三届开放原子大赛参与经历及 CCF 开源创新大赛相关技术积累；本仓本次源码与验证记录不作为这些经历的获奖证明，故不列具体奖项。当前源码包记录的是 2026 年鸿蒙赛道交付。后续工作应优先验证设备兼容与离线运行、收紧模型导入权限及覆盖行为、复测预编译库的可重建性。已知代码问题见 [待验证事项](docs/KNOWN_ISSUES.md)。
