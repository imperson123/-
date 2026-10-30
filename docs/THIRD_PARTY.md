# 第三方来源与预编译依赖

| 组件 | 本仓位置 | 来源与许可 | 当前用途 |
| --- | --- | --- | --- |
| T-MAC | 仓库根的 `include/`、`python/`、`deploy/` 等 | [microsoft/T-MAC](https://github.com/microsoft/T-MAC)，源码包根 [`LICENSE`](../LICENSE) 为 MIT，保留原版权声明 | 查表内核、代码生成、部署工具 |
| llama.cpp / GGML fork | [`3rdparty/llama.cpp/`](../3rdparty/llama.cpp/) | 源码包声明基于 `kaleid-liner/llama.cpp`，并内嵌本地修改；目录 [`LICENSE`](../3rdparty/llama.cpp/LICENSE) 为 MIT | GGUF 加载、推理与 GGML 运算 |
| HarmonyOS SDK / NAPI | `ohos/hap/` | 由本地 DevEco SDK 提供，**未随仓库分发** | 应用构建与系统接口 |

`ohos/hap/entry/src/main/cpp/CMakeLists.txt` 明确链接以下四个静态库。源码包说明称其来自上述 llama.cpp/GGML 与 T-MAC 内核的 HarmonyOS 构建；本次未做二进制与源码的可重复构建比对。保留它们是为了维持现有 HAP 工程的构建关系。

| 静态库 | SHA-256 |
| --- | --- |
| `ohos/hap/prebuilt/arm64-v8a/libggml.a` | `6C4056425E72511A3C33FE785A6C649C93D9610EDDC08EBBAA40E9CD24800ED5` |
| `ohos/hap/prebuilt/arm64-v8a/libllama.a` | `2B9D1C9BFFD64E59648F8F4ADA6781B55CB511D9DDCEADD5BAD87BE288072C0E` |
| `ohos/hap/prebuilt/x86_64/libggml.a` | `F1D5C958C6DBB9C2061FA36CCC00A53C7C1A6D28419232510F5CEDF6F94C06D9` |
| `ohos/hap/prebuilt/x86_64/libllama.a` | `937CB2A97B7FC3DDFC93C2E2E88705951E2F2DF60D3133C50635AF727F940483` |

库重建入口见 `ohos/scripts/` 和 `ohos/ARM64-VALIDATION.md`。发布者应先在受支持的 SDK 上重建并核对符号、架构、依赖及许可证；当前源码包的声明与现存 MIT 文本支持代码来源说明，但不能代替二进制来源审计。模型权重及其授权由模型提供方单独确定，本仓不分发权重。
