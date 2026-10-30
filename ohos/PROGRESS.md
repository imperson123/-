# P0–P5 落地状态（host 侧 2026-07-26）

| 阶段 | Issue | Host | Board |
|------|-------|------|-------|
| Gate0 PRD+Issues | — | done | — |
| P0 hello | #2 | aarch64 hello 已编 | hdc 空，待板 |
| P1 kernels | #3 | `libtmac_kernels.a` + selftest 已编；nm 无 TVM | selftest 待推 |
| P2 llama | #4 | `llama-cli`/`llama-bench` + `.so` OHOS 已编 | 推理待板+gguf |
| P3 SA | #5 | `libtmac_sa.so` + workspace/tile/QoS + `sa_smoke` | samgr 注册待板 |
| P4 QoS | #6 | `ApplyPerceptionSignal` + 可选 `TMAC_USE_OH_QOS` | 真 QoS 链库待系统镜像 |
| P5 交付 | #7 | 矩阵/报告骨架；**适配 zip 已打** | 实测数待填 |

## 本机零硬件验证 · 2026-09-25（Windows x64）

环境：OpenHarmony SDK 6.1-LTS（独立安装，`env.ps1` 自动探测）+ OHOS clang 15.0.4 + WSL(Kali) qemu-aarch64-static 7.2。

| 项 | 结果 |
|----|------|
| P0 hello 交叉编译 | ✅ `elf64-littleaarch64` / `EM_AARCH64` |
| P1/P3 交叉编译（kernels + selftest + sa + smoke） | ✅ 全绿；`llvm-nm -u` 仅 `memset` + stack-chk |
| **P1 selftest 执行**（qemu 模拟 aarch64-ohos） | ✅ PASS，determinism/zero-ref NMSE = 0 |
| **P3 sa_smoke 执行**（qemu） | ✅ PASS，信号→QoS→会话→warm kernel→释放 全链路 |
| 板侧（hdc + 真机） | 仍待设备 |

复现要点（无开发板）：

1. 编译：`powershell -File ohos\scripts\build_kernels.ps1`（SDK 路径由 `env.ps1` 探测，含 `D:\ohos-sdk\ohos-sdk\windows\native`）。
2. qemu 执行需**静态链接**版本（OHOS musl 动态加载器在 qemu-user 下段错误）：
   `clang++ --target=aarch64-linux-ohos --sysroot=<sdk>/native/sysroot -O2 -static -DNDEBUG -march=armv8.2a+dotprod+fp16 -std=c++17 ... -o tmac_selftest_static`
3. WSL：`qemu-aarch64-static ./tmac_selftest_static`（libc++ 静态库在 `<sdk>/native/llvm/lib/aarch64-linux-ohos`）。

> 已知坑：`ohos/shim/dmlc/logging.h` 的 `DCHECK` 宏三元分支类型不一致，非 `NDEBUG`（Debug）构建编译失败；Release/qemu 路径均需 `-DNDEBUG`。

## x86_64 路径与上游 bug 修复 · 2026-09-25

DevEco 模拟器（Windows 上为 x86_64）路线的内核验证：

| 项 | 结果 |
|----|------|
| `deploy/tuned/kernels.cc`（x86_64/AVX2）编到 `x86_64-linux-ohos` | ✅ |
| `ohos/selftest/main_x86.cpp` 运行（x86_64 静态，WSL 里直接执行） | ✅ PASS |

**修复的上游 bug**：`tbl_float_reset` 里 `memset(c, 0, m * sizeof(float_type))` 在非 NEON 构建下
`float_type = float`（4B），而目标缓冲 `half = _Float16`（2B）→ **memset 双倍越界**，
写坏栈上返回地址（`ret` 跳向地址 0 → SIGSEGV）。ARM 上二者同为 2B，故从未暴露。
已修复 `deploy/tuned/kernels.cc` 与源码模板 `python/t_mac/intrins/tbl.cc`（改用 `sizeof(_Float16)`）。

## 模拟器演示工程（HAP）· 2026-09-25

`ohos/hap/`：ArkTS 界面 + NAPI 桥 + **双架构自适应内核**（x86_64→AVX2 `deploy/tuned` / arm64→NEON `aarch64-hf-bitnet-3b`）。
两个架构的 `libtmac_hap.so` 均用 SDK 工具链构建通过（NEEDED 仅 `libace_napi.z.so` + `libc++_shared.so` + `libc.so`）。
DevEco 打开工程即可跑模拟器/真机，步骤见 `ohos/hap/README.md`。

## 🎉 鸿蒙模拟器实测通过 · 2026-09-26

HarmonyOS 7.0.0 (API 26) 模拟器（x86_64）实测——**T-MAC LUT 内核在鸿蒙系统内第一次执行通过**：

| 步骤 | 结果 |
|---|---|
| hvigor 26.0.0 构建 HAP（双架构 native） | ✅ BUILD SUCCESSFUL |
| `hdc install`（unsigned HAP） | ✅ install bundle successfully |
| 应用启动 | ✅ EntryAbility onCreate → Index loaded |
| **应用内点击「运行内核自测」** | ✅ **PASS: kernel ran, zero-in => zero-out** |

完整链路：ArkTS 界面 → NAPI (`libtmac_hap.so`) → `qgemm_lut_int8` → x86_64 AVX2 内核 → 结果回传界面。

复现命令（模拟器在线时）：

- 连接：`hdc tconn 127.0.0.1:5555`
- 安装：`hdc install <HAP 绝对路径>`（用反斜杠路径，hdc 不认正斜杠）
- 启动：`hdc shell aa start -a EntryAbility -b com.fandou.lutsa`

> 环境坑记录：DevEco 的网络请求会被用户级 `http_proxy` 环境变量劫持（代理一挂 IDE 网络全断），已清理并备份为 `http_proxy_bak`。

## 🎉 鸿蒙模拟器完整 LLM 推理成功 · 2026-09-26

llama.cpp（T-MAC fork `eb07ecf`）交叉编译到 **x86_64-ohos**，模拟器上跑通完整推理（Qwen2.5-0.5B-Instruct-Q4_K_M，463 MiB）：

| 指标 | 实测 |
|---|---|
| 模型加载 | 5272 ms |
| prompt 处理 | 17.15 tok/s（7 tokens） |
| **文本生成** | **13.04 tok/s**（63 tokens，4 线程） |
| 输出 | 中文流畅生成 ✅ |

**移植补丁**（子模块内，重拉需重新应用）：`ohos/patches/llama_cpp_ohos_musl.patch`
- `common/common.cpp` 两处 `#if` 增加 `!defined(__MUSL__)`（musl 无 `pthread_setaffinity_np`，属 glibc 扩展；**证实可行性文档待核实项①：OHOS musl 不放开该 API**）

复现命令（模拟器在线）：

- 编译：`cmake .. -DCMAKE_TOOLCHAIN_FILE=<sdk>/native/build/cmake/ohos.toolchain.cmake -DOHOS_ARCH=x86_64 -DOHOS_STL=c++_shared -DGGML_OPENMP=OFF`（在 `3rdparty/llama.cpp/build-ohos-x86`）
- 推送：`hdc file send <llama-cli|libllama.so|libggml.so|model.gguf> /data/local/tmp/llm/`
- 运行：`hdc shell "cd /data/local/tmp/llm && LD_LIBRARY_PATH=. ./llama-cli -m model.gguf -n 64 -t 4 -p '你好'"`
- 注意：hdc 在 Git Bash 下需 `MSYS_NO_PATHCONV=1`，本地路径用反斜杠

**SIMD 优化（同日）**：CPUID 探针（`ohos/selftest/cpuid_probe.c`）发现模拟器虚拟 CPU 为 **"AVX2 ✅ / FMA ❌ / F16C ✅"** 组合；补 `-mavx2 -mf16c` 并在 `ggml-cpu-impl.h` 加 FMA intrinsic 的 mul+add 兜底宏后重编：

| 指标 | 标量版 | SIMD 版 | 提升 |
|---|---|---|---|
| 文本生成 | 13.04 tok/s | **55.80 tok/s** | **4.3×** |
| prompt 处理 | 17.15 tok/s | 115.32 tok/s | 6.7× |
| 模型加载 | 5272 ms | 2294 ms | 2.3× |

编译配方：`cmake .. -DCMAKE_C_FLAGS="-mavx2 -mf16c" -DCMAKE_CXX_FLAGS="-mavx2 -mf16c"`（**勿加 `-mfma`**：虚拟 CPU 不支持会 SIGILL）。

**T-MAC 集成待做**（GGML_TMAC=ON）需匹配的 x86 内核（TVM 生成）+ BitNet-3B 模型转换。

## T-MAC 加速集成攻坚 · 2026-09-26 夜

**结论：技术链全通，卡在内核执行层最后一个 bug（下游均为上游未验证区）。**

已打通：WSL 从零编译 TVM 0.17 + LLVM 17 → **x86_64-ohos BitNet LUT 内核生成**（两个分支：
`-ags -1` int32 变体 / `-ags 64` float 变体，后者与队友 aarch64 内核符号完全一致）→
BitNet-3B 转换（12.6GB HF → 966MB T-MAC GGUF，288 张量，2.44 BPW）→
llama.cpp `GGML_TMAC=ON` 集成编译 → **全链路激活**（transform / 内核 dispatch 匹配 / 4 线程 / 数据流验证）。

**修复的集成层 bug（5 处，完整补丁见 `ohos/patches/llama_cpp_ohos.patch`）**：

1. `tmac_float_type` x86 定义错误（float → `_Float16`；与 fp16 内核匹配）
2. `ggml_tmac_transform_tensor` 在 mul_mat 分支补调（幂等）
3. mmap 只读区 inplace scales 转换写崩（运行加 `--no-mmap`）
4. 生成阶段：`pthread_setaffinity_np`（musl 无此 glibc 扩展）、FMA 兜底宏、`half` typedef 条件错误
5. 转换阶段：yaml/safetensors/torch 依赖链、权重分片完整性校验

**遗留**（已于当日深夜定位）：`qgemm_lut` 特化内核执行内部 SIGSEGV / 输出全 0。

**根因（第二轮调试最终定位 + 修复）**：

⚠️ 早前"OHOS clang 代码生成缺陷（vpaddw 缺失）"的结论**已推翻**——那是 objdump 工具名写错
（`llvm-objdump.exe` 不存在，错误被 `2>/dev/null` 吞掉）导致指令统计为 0 的**假数据**。
真实的 impl 机器码在 OHOS 与 GCC 两个版本中逐条相同。

**真正的根因：生成器的输出缓冲类型不匹配（栈越界）**
- x86/AVX2 的 `tbl_g4_int8_float_update_impl` 输出 **float32**（`_mm256_storeu_ps`，4 字节/元素）
- 但 ags64 生成器把 qgemm 的中间缓冲声明为 `half CBits[N]`（**fp16，2 字节**）
- → 内核写入量 = 缓冲的 2 倍 → **栈越界** → 覆盖返回地址（SIGSEGV）/ 读到零页（全 0）
- 对照：bitnet-3b 分支用 `uint64_t temp_CBits[N]`（8 字节/元素）→ 长度足够 → 不崩

**修复（已落地）**：
1. `deploy/tuned/ohos-x64-ags64/kernels.cc` 3 处 `half CBits[N]` → `float CBits[N]`
2. 同步 `ohos/staging-x64/t-mac/lib/kernels.cc`（llama.cpp 集成用）
3. 重编内核 + 重编 llama.cpp（GGML_TMAC）→ 推送模拟器

**验证**：
- musl（OHOS clang 链接）与 glibc 输出**完全一致**、无崩溃 → 工具链无罪
- **T-MAC 端到端在模拟器执行成功**：`transform_tensor`（288 张量）→ kcfg 查表（bm=256）
  → scales 转换（fp32→fp16 正确）→ qgemm 分块 compute → 输出
- 性能（含调试探针）：prompt 0.49 tok/s / eval 0.15 tok/s（探针去除后待重测）

**遗留（下一步）**：数值 NaN/-inf（第 0 层起 `dst[0]=-inf`）——内核数值层面问题，
候选：非 permute 分支的数据布局、激活 scale 处理、chunk 偏移。

**运行方式**（模拟器）**：
```
TMAC_KCFG_FILE=/data/local/tmp/llm-tmac/kcfg.ini LD_LIBRARY_PATH=. ./llama-cli \
  -m model.gguf --no-mmap -p '...' -n 20 -t 4
```

产物：`deploy/tuned/ohos-x64-bitnet-3b/`（ags=-1）、`deploy/tuned/ohos-x64-ags64/`（ags=64，已修复）。

## 🏆 最终成功：T-MAC 在鸿蒙上完整跑通（2026-09-27）

**结果**：BitNet-3B（966MB，2.44 BPW）在 HarmonyOS 模拟器上输出**完全正确**的文本：

```
The capital of France is → Paris. It is the largest city in France and the second
                            largest city in Europe. It is the capital of the country...
Once upon a time         → , there was a little girl who was very sad...
The sun rises in the     → east and sets in the west.
```

**验证证据（铁证级）**：
- attn_q 输出 vs 官方数学参考（T-MAC tests/test_e2e 的公式）：**NMSE 0.0027%**
- 设备 dump 真实激活 vs numpy RMSNorm：差 1e-7；模型数据 vs 重新转换：**逐字节零差异**
- 调用参数实录（m_bits/K/A_off/C_off stride）全部对齐
- 输出质量**超越**手工参考：连补了 sub_norm 的 HF transformers 实现都只能输出乱码（HF 并非 BitNet 的正确实现），本实现输出通顺百科文本

**性能**（模拟器 x86_64，4 线程）：eval **11.24 tok/s**、load 11.3s（冷）、prompt 1.93 tok/s（6 token 短 prompt）

**完整修复清单（9 项）**：
1. float32 类型统一（平台 out_dtype=float32 + tmac_float_type=float）—— 上游 x86 半成品路径
2. scales 广播（模型 per-tensor 单值 → 内核读取所需的 120 个）—— 上游 BitNet 路径
3. `tbl_float_reset` 尺寸（`m * sizeof(float_type)`，修复栈垃圾入 FMA）
4. memset 越界（上游）+ ⑤ half typedef（上游）
6. `transform_tensor` 漏调（fork 集成）
7. mmap 只读页写崩（条件化 cast 跳过）
8. kcfg 路径（TMAC_KCFG_FILE 环境变量）
9. **调试探针清理** —— 探针的文件 IO/打印在 4 线程推理中干扰时序；清理后输出立即正确（最后的关键一步）

**archived**：`ohos/patches/llama_cpp_ohos.patch`（llama.cpp 子模块提交 `181ad23a`）

## 对比实测：llama.cpp SIMD vs T-MAC（2026-09-27）

完整报告：**`ohos/BENCH-tmac-vs-simd.md`**（同模型 A/B + 同形状内核对照 + 复现命令 + 原始输出）

**🏆 同模型正面对决（BitNet-b1.58-3B，同一模拟器，同参数，两轮换序复测）**：
T-MAC LUT（2.44 BPW，966MB）**11.7~11.9 tok/s** vs llama.cpp SIMD Q4_0（4.63 BPW，1.83GB）**5.4~5.6 tok/s** → **T-MAC 快 2.1×**，输出均连贯（`…the city of Paris…`）。
有效带宽两者相近（T-MAC 12.0 vs SIMD 10.8 GB/s，都逼近模拟器冷流式上限 13.7~15.7 GB/s）→ 差距来自**位宽/字节数 1.9×** 与 ~13% 带宽利用率优势；两者各达自身内核带宽的 85% / 79%，集成效率相当。

**内核级**（同形状 N=8640×K=3200、M=1、交错 5 轮取 MIN）：T-MAC 快 **2.19×**（热，4t；1t 3.15×）/ **2.33×**（冷）——与端到端 2.1× 吻合，内核优势完整传导。**模拟器对同内核的惩罚 ≈2.2×**（WSL 原生 4t 冷 0.219ms vs 模拟器 0.486ms）。

**方法学修正**（重要）：设备上 `run_test_ffn` 是 WSL 路径版（`ERR open A` 空跑 7ms 假数据），真测试程序是 `run_test_dev`；SIMD 侧必须用持久线程池（`ggml_graph_compute_with_ctx` 每调用重建线程池会污染 4 线程数据）；模拟器噪声大 → 同会话交错取 MIN、A/B 换序各跑一遍。

**附带发现（fork 缺陷）**：T-MAC 补丁的 `is_type_supported()` 含 `GGML_TYPE_Q4_0`（bits=4），而加载器对每个张量无条件 `transform_tensor` → 缺 `bits=4` kcfg 时 `LOG(FATAL)` 中止：**T-MAC 版 llama-cli 无法加载任何标准 Q4_0 模型**。SIMD 基线因此改用未编 T-MAC 的纯 SIMD 构建。

**模型转换链（新增产物）**：HF safetensors → `convert_hf_to_gguf.py --outtype f16`（6.65GB，288 张量，arch=bitnet，保留 sub_norm）→ `llama-quantize Q4_0`（1.83GB）。产物在 `D:\ohos-models\`。

## aarch64（真鸿蒙架构）功能验证 · 2026-09-27

完整报告：**`ohos/ARM64-VALIDATION.md`**（三层证据 + 移植要点 + 复现命令）

**不碰真机，在 PC 上用 qemu-user 把 aarch64 全链路跑通：**
1. **内核级**：aarch64 NEON 内核 vs 官方 numpy 参考 **NMSE 8.4e-05**（corr 0.99996；与 x86 输出 corr 0.99999）
2. **构建级**：aarch64-ohos 静态 llama-cli（7.9MB），`llvm-objdump` 确认 **1085 条 NEON/fp16 指令**（`fmla v3.4s,...`）
3. **端到端**：qemu 里输出 `The capital of France is the city of Paris…` ✅，system_info 确认 **NEON=1/ARM_FMA=1/FP16_VA=1/MATMUL_INT8=1**（TCG 0.43 tok/s，性能无意义）

**关键发现（真机移植必看）**：
- ARM 的 preprocessor 读激活为 **fp16**（x86 为 fp32，我们改的）→ 首次测试 NaN 的根因
- ARM 内核输出 C 为 **fp16**（集成层的 `fp16_to_fp32_row` 转换已接住）
- **kcfg 布局按架构不同**（m6400_* 的 bm：ARM 128/320 vs x86 256）→ 模型已按 arm64 重转：`bitnet-3b-tmac-arm64.gguf`
- fork CMake 对 aarch64+T-MAC **总会选 armv8.7**（check 假阳性）→ armv8.2 真机需强制 `-march=armv8.2a+fp16`，否则 SIGILL
- 静态构建配方：`BUILD_SHARED_LIBS=OFF + OHOS_STL=c++_static + -static`
- **aarch64 路径零源码修改**（对比 x86 修了 9 处）——NEON 是官方主干，x86 是支线

产物：`ohos/staging-arm64/t-mac/`（已归档）· `D:\ohos-models\bitnet-3b-tmac-arm64.gguf` · `D:\ohos-models\qemu\qemu-aarch64-static`

**真机可用性加固（同日追加）**：
- 修掉会让真机 SIGILL 的雷：fork CMake 恒选 `-march=armv8.7-a` → 用 `-DGGML_COMPILER_SUPPORT_MATMUL_INT8=OFF` 强制走 **armv8.2a+fp16**；重编后指令集安检 **SVE/i8mm/v8.7 全 0**，NEON fmla 807 条，端到端复跑通过
- 新增设备探针 `ohos/selftest/arm64_cpu_probe.c`（HWCAP 解码 + fork 隔离的 fp16 冒烟测试 + 判定）
- 新增一键真机部署 `ohos/scripts/deploy_arm64_device.ps1`（探针门禁 → 推二进制/模型 → 推理，`-WithSimdBaseline` 出同模型 A/B）；`env.ps1` 补本机 SDK 路径
- 结论：指令集/ABI/数值/布局已验证；**唯一未验证项 = 真机 shell 是否允许执行 /data/local/tmp 二进制**（开发板通常可以，HarmonyOS NEXT 较严）→ 插上设备跑脚本 30 秒即知

## HAP 应用形态：T-MAC LLM 推理 App · 2026-09-27

**目标**：把 T-MAC 从"命令行二进制"升级成正经鸿蒙应用（ArkTS + NAPI），让 DevEco 的 Run 能一键部署。

**已完成**：
- **NAPI 桥重写**（`ohos/hap/entry/src/main/cpp/napi_init.cpp`）：同步 + **异步（Promise，跑在 NAPI 工作线程，避免卡死 UI）** 两套入口
  —— `loadModelAsync / generateAsync`（真 LLM 推理：加载 T-MAC GGUF、tokenize、decode、top-k+温度采样、tok/s 计时）、`selfTest / bench`（LUT 内核冒烟与基准）、`nativeVersion / release`
- **kcfg 自包含**：按 ABI 把 `kcfg.ini` 字符串编进 `.so`，首次加载写入应用沙箱并 `setenv("TMAC_KCFG_FILE", …)` → 不依赖主机构建路径
- **双 ABI 静态库**（`ohos/hap/prebuilt/{x86_64,arm64-v8a}/lib{llama,ggml}.a`）：静态 llama.cpp（T-MAC 开）+ 对应架构 LUT 内核；x86 特意**不带 `-mfma`**（模拟器无 FMA）
- **ArkTS UI**（`Index.ets`）：模型路径 / prompt / 线程 / 生成长度 / mmap 开关 + 5 个按钮；输出区显示文本与 tok/s
- **命令行构建验证**：`hvigorw assembleHap` → `BUILD SUCCESSFUL`，产物含**双 ABI 的 `libtmac_hap.so`**（已确认库内含 T-MAC/llama 代码串）
- 文档：`ohos/hap/README.md`（DevEco 上手 + CLI 构建 + 模型准备 + 排障表）

**✅ 已在模拟器（Mate X7 / HarmonyOS 7.0 / x86_64）实测通过**：
- 「内核自测」→ **✅ 通过**；「加载模型」→ **✅ load=4956 ms**（含 288 张量内核变换）
- 「生成」（`The capital of France is`，16 token，4 线程）→ **`the city of Paris. The city is located in France. Paris was a Roman`**，
  **eval ~10.5 tok/s**（见下方交叉验证）；截图 `ohos/screenshots/emulator/hap-demo-screenshot.png`
- 验证方式：DevEco 点 Run（自动签名）安装 → 用 `Emulator.exe -instance "Mate X7" -click/-uiLayout/-screenshot` 驱动与读结果

**踩通的关键坑（HarmonyOS 7 沙箱）**：App **读不了 `/data/local/tmp`**（SELinux，非文件问题）；shell 也**不能在 App 沙箱新建文件**。
可行通道：App 启动时 chmod 沙箱到 0777 并创建 **0666 的 `model.gguf` 占位** → shell **覆盖**它：
```
hdc file send bitnet-3b-tmac-ags64.gguf \
  /data/app/el2/100/base/com.fandou.lutsa/haps/entry/files/model.gguf
```
（App 内路径视图是 `/data/storage/el2/base/haps/entry/files/…`，shell 侧要用 `/data/app/...` 真实路径）

**性能数字交叉验证（重要，2026-09-27 晚）**：同一模型每 token 固定读 **1012 MB**（807MB 2-bit 层权重 + 205MB F16 嵌入，gguf 工具实测）。
五次测量：CLI 3.84（刚开机，系统忙）/ **App 21.79（异常值，偏高 2×）** / CLI 10.38 / App 10.52 / App 10.51 →
**可信值 ≈10.5 tok/s（96 ms/token → 10.5 GB/s 有效带宽，与实测内存带宽区间自洽）**，三次独立测量误差 <1.5%。
2× 波动来源：模拟器 4 个 vCPU 是宿主线程，i7-14650HX 的 P 核/E 核性能差 ~2×，调度位置每次不同。
**结论：模拟器数字波动可达 ±2×，比赛/论文数字必须真机。**

**"是否真跑"的证伪实验**：把沙箱模型覆盖成 1KB 随机文件 → App **立刻 `❌ 加载失败`**；恢复真模型 → ✅ 加载 + 出文本。

**真机就绪化**：
- **模型导入产品级闭环**：App 内「📂 选择模型文件」（系统选择器 → 拷进沙箱 → 自动加载）+
  「📤 导出到手机」（保存对话框，唯一时间戳文件名避免系统"重名"拦截）
- **⚠️ 重要鸿蒙坑（实测踩到并修复）**：1GB 级文件拷贝**绝不能同步做**——
  `fs.copyFileSync` 阻塞 UI 线程 >6 秒会被系统按 **`THREAD_BLOCK_6S`（appfreeze）直接杀掉**，
  表现为"导出的文件被截断"（实测出现 793MB / 383MB 的半截文件，faultlog 里有 appfreeze 日志为证）。
  修法：**拷贝放到 NAPI 工作线程**（新增 `copyFdAsync(srcFd, dstFd)`）+ 瞬时任务（`requestSuspendDelay`）保护。
  修复后：导出 1GB 完整成功、选回导入自动加载成功（`OK: loaded … n_vocab=32002 … load=1861 ms`）
- **arm64 指令集体检**：HAP 内 `libtmac_hap.so`(arm64) 用 `-march=armv8.2a+fp16` 编译，
  **SVE/i8mm/v8.7 指令 0 条**、NEON fmla 966 条 → 不会被 fork 的 armv8.7 误选坑到，主流手机都能跑
- 部署能力：模拟器允许 `bm install` 装**未签名** HAP → 我已能自行部署验证（真机仍需 IDE 自动签名）

## 🏆 真机落地：HUAWEI HBN-AL00 实测 18.62 tok/s · 2026-09-27

**真机（HarmonyOS 6.1.1 零售版，arm64）完整跑通**，全程总结见 **`ohos/FULL-REPORT.md`**。

| 步骤 | 结果 |
|---|---|
| CLI 直跑二进制 | ❌ 零售版 SELinux：`sh` 域禁 exec `/data/local/tmp` → 唯一形态 = HAP 应用 |
| 签名链修复 | ① products 补 `signingConfig: default`；② 删空 `targetSdkVersion`；③ 错误码 9568423 → 真机 UDID 未入 profile，连机重新生成自动签名 |
| 模型注入沙箱 | App 建 0666 占位 → `hdc file send` 覆盖（966MB arm64 布局模型） |
| 数据提取 | 零售机 hilog 被禁 → App 屏幕滚动控制台（`appendLog`）显示 + 截屏取数 |

**真机成绩**（屏幕 `llama_perf_context_print` 原始输出，截图 `ohos/screenshots/device/`）：

```
load time   =  1955.44 ms
prompt eval =   239.87 ms /  7 tokens ( 34.27 ms/token,  29.18 tokens per second)
eval time   =   859.51 ms / 16 runs   ( 53.72 ms/token,  18.62 tokens per second)
```

- 真机 T-MAC（18.62）**超过**桌面 i7 的 llama.cpp Q4_0（15.94）
- 比模拟器快 1.7~1.8×（与预测的模拟器惩罚 ≈2× 一致）；加载仅 1.96 s
- 有效带宽 18.8 GB/s（1012 MB/token ÷ 53.72 ms），落在模拟器（10~12）与桌面（25~31）之间，自洽 ✓

> 注：`ohos/hap/build-profile.json5` 的本机签名材料（绝对路径 + DPAPI 密码串）**只留在本地**，不入公开仓库。

## 🤖 安卓真机端到端（跨平台可移植性实证）· 2026-09-28

同一套 aarch64 产物（静态 llama-cli + bitnet-3b-tmac-arm64.gguf + staging-arm64 内核，**零改动**）推上
vivo V2323A（iQOO Neo9 / SD 8 Gen 2 / Android 16）直接跑通：

- 内核级 NMSE **8.397e-05**，与 qemu **逐位一致**（qlut 41/127/27/113 三环境一致）
- 端到端输出正确文本（`…Paris…`），**峰值 11.55 tok/s / 持续 ~10.6（t4 最优）**，load 1.46 s
- 线程扫描：t2/t3/t4≈10.5~11.5，t6=9.35，t8=7.37 → >4 线程被 A510 小核锁 barrier
- 跨平台对照：麒麟 18.62 vs SD8G2 11.55（同二进制同模型，+61%）→ 麒麟内存子系统/调度占优
- 证据：`tests/lut-verify/android-{device,e2e}-run.txt`；排坑指南 `docs/ANDROID-FIX.md`（2026-09-28 三分支并回单 main，平台改专区制）
- 当年"安卓能跑不输出"的病根（数据契约层）按本仓修复集使用后根治 ✅

## 🧠 KV 三连击工作日 · 2026-09-29

**背景**：识别 KV 为下一个数量级入口（每 token KV = 325 KiB，n_ctx≈3000 时 KV 读取量追平全部权重；纯 MHA 无 GQA，属最坏情况）。

| 招式 | 结果 | 证据 |
|---|---|---|
| ① **链式 KV v1**（跨轮前缀复用，省计算） | ✅ **大成功** | 同 prompt 二轮：`reused 6 of 6`、prompt eval **421.41ms → 0.00ms**、TTFT 0.46ms（控制台铁证） |
| ② KV 量化 f16→q8_0（省内存） | ❌ **模型几何封死**（根因修正 2026-09-30） | 崩溃来自 **K 行对齐断言** `GGML_ASSERT(n_embd_head_k % ggml_blck_size(type_k) == 0)`（src/llama.cpp:19307）：head_dim=100 ÷ q8_0 块 32 除不尽 → abort。V/flash-attn 门（19199）是 LOG_ERROR+return nullptr（非 abort）且 K-only 试验中未参与。**新版 fork（llama-context.cpp:201）同款无条件断言仍在 → 升级不解除**；解锁前提 = KV head padding（100→128），独立工作项（详见 fork-upgrade 分支评估报告 §17） |
| ③ 上下文 512→1024 | ✅ 保留 | 配合链式复用装更长对话 |

**配套修复**：L2 调度加 60s 滞回（修"回前台重载冲掉 KV 链"）、TTFT 改诚实口径（含 prompt）。实现：`LlmState.kv_tokens` 追踪 KV 链 + 最长公共前缀 → `seq_rm` 删分歧段 → 只解码增量。

**回退点**：`v1.0-submission-verified`（13:09 已验 HAP 精确坐标）· `v1.1-chained-kv` · `v1.2-final-candidate`（本日终态）。
**代码冻结**：截止前不再动 native 主线；换 fork（新版 llama.cpp + T-MAC 补丁重移植）为周级工程，列入复赛后。

## 📦 提交打包夜 · 2026-09-29（截止前 1 天）

**触发**：官方通知四连（可安装性自查 / 视频规范 / 交付件完整性 / 10 次刷新）。打包全程 hvigorw CLI 完成；真机不在场 → 以官方模拟器三闸门 + 离线签名校验 + 真机级功能跑通为证据。

| 项 | 结果 | 证据 |
|---|---|---|
| ① **剔除 ASan**（DevEco Run 注入 `-p ohos-debug-asan=true`：`__asan_*` 全量插桩 + `libclang_rt.asan.so` 依赖） | ✅ 性能 2.4× | 同模拟器同模型：ASan 版 10.5 tok/s → CLI 纯净版 **25.01 tok/s**（`_verify/ui4.jpeg`） |
| ② **四指标卡修复** | ✅ | 根因：`@Builder chip(label, value, wide)` 按值传参 → ArkUI 不刷新（气泡能刷是因为 ForEach key 含 `text.length` 强制重建）。改按引用传对象 `ChipVM`：23.35 tok/s / TTFT 186.34ms / 加载 4.55s / 峰值 1469.0MB 全亮（`_verify/ui7.jpeg`） |
| ③ **品牌残渣清零** | ✅ | `string.json` module_desc / EntryAbility_desc 去 "T-MAC"（应用信息页可见处）；HAP 资源内 T-MAC 命中归零 |
| ④ 三闸门（模拟器） | ✅ | 安装 `install bundle successfully` · 启动 `start ability successfully` · 渲染整屏（`_verify/ui1-ui3.jpeg`） |
| ⑤ 二轮链式 KV 复盘 | ✅ | 最终 HAP 实测 `reused 6 of 6`、prompt eval **0.00 ms**、TTFT **0.57 ms**（`_verify/ui9.jpeg`） |
| ⑥ 离线签名校验 | ✅ | `hap-sign-tool verify-app` → `Verify success`（终版 SHA256 见交付说明） |
| ⑦ 源码包 | ✅ | git archive + **内嵌 3rdparty/llama.cpp 全量源码**（子模块指向本地提交 181ad23a，上游不可达 → 必须内嵌，否则评委拉不到引擎）；脚本 `ohos/scripts/make_source_zip.ps1` |

**签名实况（评委须知）**：HAP 为 debug 自动签名，profile `type=debug`、UDID 白名单 2 台（本队手机 + 官方模拟器）、有效期至 **2026-10-13**。真机装包需在 DevEco 用自己的证书重签；无 UDID 限制的发布版签名需 AGC 发布证书（复赛后补）。

## 提交包

`docs/output/report/submission/03-LUT-SA翻斗花园-ohos-adapt.zip`

产物目录：`ohos/build/` · `ohos/hello/build/` · `3rdparty/llama.cpp/build-ohos-check/`

## 📱 真机交付版实测 + 系统服务边界复证 · 2026-09-30

设备：HUAWEI Pura 70 Pro（HBN-AL00，API 24，arm64-v8a）；对象：交付版 HAP（原地更新安装成功）。

| 项 | 实测 | 备注 |
|---|---|---|
| 模型加载 | **1.09 s**（冷启动；热重载 1.01 s） | 模拟器同模型 4.55 s |
| 首轮 TTFT | 248.80 ms | 含 10 token 前缀处理 |
| **二轮 TTFT（链式 KV）** | **1.15 ms** | 控制台 `reused 8 of 8`，降幅 216× |
| 生成吞吐 | 20.39 tokens/s（首轮 256 token）· 17.55（二轮） | 采样开销 0.40 ms/token |
| 峰值内存 | 1413.6 / 1415.5 MB | hidumper --mem 独立核验 Pss 1393 MB |
| **内核自测 / 基准** | PASS（m128-k3200, NEON）· 200 次 **1.56 ms**（平均 **7.8 µs/次**） | 内核级证据 |
| L2 调度 | 快速切回（<60 s）×3 保持引擎与 KV 链；后台停留 >60 s 触发释放（降载）；回前台自动重载 1.37 s | 日志实证 |
| 模型装载 | 288 张量 / 29 元数据键；head_count_kv = 32（纯 MHA，无 GQA） | loader 日志 |
| 功耗 | ❌ 未采集：sysfs 拒绝、BatteryService/ThermalService 无 dump | 建议 DevEco Profiler / 外接功耗仪 |

**系统服务边界（真机复证）**：`id`=uid 2000 shell、无 su、`getenforce` **Permission denied**（比模拟器更严）、
`/system` 与 `profile/init` 目录不可访问、`hdc target mount` → E007100（需 debug 镜像）。
完整证据链见 `ohos/SA-BOUNDARY-EVIDENCE.md`（含模拟器与两套 SDK 的六条锁）。

## 🔬 对比试验日（模拟器侧）· 2026-09-30

| 实验 | 方法 | 结果 |
|---|---|---|
| ① 链式 KV **部分复用** | 先以「The capital of France is」建链，再以追加式提问「… Paris. The capital of Spain is」二次生成 | **`reused 6 of 13 prompt tokens`**（前缀 6 复用、增量 7 解码）——LCP 精确裁剪对"部分相同前缀"成立；产出连贯续写（Madrid→Berlin→Washington DC） |
| ② L2 调度 **A/B 内存对照** | `hidumper --mem` 采样进程 Pss：调度开 vs 关，同样后台 70 s | **调度开：1370 MB → 71 MB（释放 1.30 GB）→ 回前台 1364 MB**；**调度关（对照）：1356 MB → 1356 MB（不释放）**；唯一变量=开关 ✓ |
| ③ T-MAC vs Q4_0（同机同模型） | 沙箱推入 `bitnet-3b-q4_0.gguf`（1.92 GB）后点加载 | ❌ **加载即 cppcrash**（14:26/14:27/14:29 三条 faultlog）：T-MAC 构建只支持自身 kcfg/LUT 格式，标准量化格式不在支持面内 → 应用内 Q4_0 基线不可测；跨平台反量化基线沿用初赛 llama.cpp CLI 对照（表 7）。模型已还原为 `bitnet-3b-tmac-ags64.gguf` 并复验加载 ✓ |

附带确认：模型装载日志 288 张量 / 29 元数据键；`head_count = head_count_kv = 32`（纯 MHA 无 GQA，印证 KV 成本模型）。

## 📊 数据补全 · 2026-09-30（模拟器侧）

- **模拟器内核级数据补齐**：内核自测 **PASS**（x86_64 AVX2）；内核基准 200 次 **1.41 ms（平均 0.0071 ms/次）**
  —— 与真机 NEON（1.56 ms / 0.0078 ms）构成双 ISA 内核级对照
- **多次测量值入表**：模拟器加载 4.55/5.22 s、首轮 TTFT 186.34/239.78 ms、峰值内存 1469/1466 MB；
  真机同配置重复偏差 <1%（17.55/17.66 tok/s）
- **测量稳定性条款**：模拟器吞吐受宿主负载影响（同配置实测 **10.21–25.01 tok/s 区间**），跨平台结论以真机为准；
  提示词/生成长度随数字标注（256 token 比 32 token 低约 13%，属 KV 增长）
- 文档同步：作品说明文档（表 8/表 10/说明段）· 内容 md · 交付 PDF（17 页）· BENCHMARK-PLAN · 证据 15 份（含模拟器内核截图）

## 🔧 新 fork 数值缺口修复（上游 WIP 定罪 + 6 行分发表补丁）· 2026-09-30

**症状**：新版 fork（202504_tmac）端到端跑通、速度快（46.93 tok/s），但输出退化为 `The capital of France is is is is…`（作者自标 "[WIP] Wrong outputs"）。

**定罪链**（读源码 + A/B 实证）：
1. `ggml-tmac.cpp:346 get_scales_size` 只有三种口径：`one_scale` → **1 个 scale/整张量**；`has_zero_point` → 每组 (scale+zp)；否则 → **每组 1 个 scale**（= BitNet 三值 + q_group_size=64，正是我们的模型）
2. `tbl.cpp:819 qgemm_lut_int8_g4` 分发表 12 个条目只覆盖 `{有zp+每组}` 与 `{无zp+单scale}` 两个极端；**无 else 兜底、不抛错** → 我们的组合整段循环空转，`CBits` 恒为 0 → 输出退化
3. 而 `tbl_g4_int8_float_update_impl<has_scale,K,Bits,ActK,FastAgg,ZeroPoint,OneScale>` 的**最终 else 分支已实现「每组 scale、无 zp」**（scales 指针寻址亦然）——**只缺分发表路由** 6 条
4. 46.93 tok/s 之谜同源：空转 = 跳过矩阵乘 → 又快又错

**修复**：`tbl.cpp` 补 bits=2/4 各 3 条 `ZeroPoint=false, OneScale=false` 路由（补丁：`ohos/patches/tmac-newfork-pergroup-nozp-dispatch.patch`，含新 fork 全部改动）；
重编译（OH SDK clang 15 + Ninja 增量）→ **输出恢复通顺**（WSL 复现两次）：
`The capital of France is the city of Paris. It is the largest city in the European Union…`

**速度实况（WSL i7，宿主同时在跑模拟器，偏保守）**：修复版 eval **18.66–20.37 tok/s**（-t 4 / -t 8）→ **不高于旧 fork 的 PC 基线 25.10**，
故**本版不并入提交包**；后续优化项：内核调优（kcfg）+ 128 组量化变体（同一补丁已支持）。

### 采样器升级（同日，输出质量修复）

- **问题**：App 自研采样器只有 top-k + 温度（`repeat_penalty=1.0` 等于没开、`top_p` 仅打印未实现）→ 3B base 模型 256 token 长生成必然复读
- **改动**（`napi_init.cpp`）：`SampleCfg`（top_k/top_p/temp/repeat_penalty=1.15/repeat_last_n=64）+ `sample_token()`（重复惩罚 → top-k → softmax → top-p 核采样 → 采样）+ 生成循环维护 recent 窗口 + 报告输出真实参数
- **效果（模拟器 A/B，同默认提示）**：升级前 `…the city of Paris. It is the most visited city in the world attracts millions of tourist every year from every corner of the world and attracts millions of…`（复读）→ 升级后 `the city where all things French are made. From the bistros with crêpes and croissants, to the boutiques brimming with Chanel`（通顺零复读）
- **交付同步**：HAP 重出（`d5798121…`）+ 源码包重生成（`61f73657…`）+ 证据 16/17 + 提交说明更新
- **构建经验**：CLI 重打 HAP 必须 `--no-daemon` + `JAVA_HOME=D:\DevEco Studio\jbr`（守护进程残留会带 JDK8 环境 → `11014003 Init keystore failed`）

### 系统服务化脚手架编译级验证 + 源码包重打（9/30 傍晚，收尾）

- **SA 脚手架补全**：`ohos/sa/component/` 全量落地（SystemAbility + IDL 桩 + profile/cfg/sepolicy/BUILD.gn）
- **编译级验证通过**（真实 OH 头文件 + SDK `idl.exe` 生成的桩 + OH SDK clang，target `aarch64-linux-ohos`，零接口错误）：
  修正 3 处真实差异 —— ① `SystemAbility` 位于 `namespace OHOS`；② dump 钩子为 `OnSvcCmd(fd,args)`；③ `Publish(this)` 需继承 IDL 生成的 `IRemoteStub` 桩。生成物归档 `ohos/sa/component/idl/`
- **源码包重打**：纳入 SA 脚手架（+17 条目）与技术文档索引（TECH-CORE / VERSION-COMPARISON / BENCHMARK-PLAN / STRENGTHS）→ 交付包与 `03-作品名称+翻斗花园-复赛.zip` 同步（最终 SHA256 见交付包 `03-提交说明/提交说明.md`）
- 说明：包内文档不引用本包自身哈希（避免自引用失效），哈希统一由交付包《提交说明》外引
