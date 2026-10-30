# T-MAC 鸿蒙真机移植 · 全程技术总结报告

> 队伍：翻斗花园（中北大学）· 赛题：LUT-SA（查找表低比特 LLM 端侧推理）
> 报告日期：2026-09-27 · 工作周期：2026-07-26 → 2026-09-27
> 仓库：`fandou-t-mac-main`（基于 microsoft/T-MAC 的适配 fork）

---

## 0. 执行摘要（一句话 + 一张表）

**把微软 T-MAC（LUT 查找表低比特推理）从"PC + Python + TVM"的原生形态，移植到了 HarmonyOS 真机上的原生 App（ArkTS + NAPI + C++），端到端跑通 BitNet-b1.58-3B（2.44 BPW，966MB），真机生成速度 18.62 tok/s——超过桌面 i7 CPU 上 llama.cpp 自家 Q4_0 量化（15.94 tok/s），期间修复上游与集成层 bug 共 13 处。**

| 环境 | 配置 | 生成速度 | 相对 f16 |
|---|---|---|---|
| **华为真机 HBN-AL00（arm64）** | T-MAC 2.44 BPW | **18.62 tok/s** | — |
| 安卓真机 vivo V2323A（SD 8 Gen 2） | T-MAC 2.44 BPW | 11.55 tok/s（持续 ~10.6） | — |
| 桌面 i7-14650HX（4 线程） | T-MAC 2.44 BPW | 25.10 tok/s | 5.69× |
| 桌面 i7-14650HX（4 线程） | llama.cpp Q4_0 | 15.94 tok/s | 3.61× |
| 桌面 i7-14650HX（4 线程） | llama.cpp f16 | 4.41 tok/s | 1× |
| 鸿蒙模拟器（x86_64） | T-MAC 2.44 BPW | 10.5~11.9 tok/s | — |
| 鸿蒙模拟器 同模型 A/B | llama.cpp Q4_0 | 5.4~5.6 tok/s | T-MAC 快 2.1× |
| qemu-aarch64（功能验证） | T-MAC 2.44 BPW | 0.43 tok/s（TCG 无性能意义） | — |

---

## 1. 背景与技术选型研究

### 1.1 T-MAC 是什么，为什么它适合手机

LLM 推理在端侧是**内存带宽受限**问题：每生成一个 token，都要把全部权重从内存流读一遍。传统低比特方案（如 Q4_0）的每步计算是"反量化 → 浮点乘加"，计算和访存都省不干净。

T-MAC 的思路：**把乘法变成查表**。权重是 2-bit 三值（-1/0/+1），把"激活 × 所有可能的权重组合"预先算成查找表（LUT），推理时对每组激活只做一次 LUT 构造 + 查表累加：

- 权重从 4.5~16 BPW 压到 **2.44 BPW** → 每 token 读的字节数直接砍到 1/2~1/6
- 查表用 SIMD 的 shuffle/table 指令实现，不需要反量化
- **验证了"优势来自位宽，不是魔法"**：冷数据下 T-MAC 与 Q4_0/Q8_0 的每字节带宽持平（13.7~15.7 GB/s），加速比 ≈ 字节数之比（见 §3）

### 1.2 模型选型：BitNet-b1.58-3B

三值权重（ternary）模型，是 T-MAC 论文的原始目标模型。HF 原始权重 12.6GB（fp32 分片）。

**附带研究结论（重要机会点）**：BitNet-3B 的 hidden=3200、intermediate=8640 都**不是 256 的倍数**，而 llama.cpp 的 Q2_K~Q6_K、IQ 系列全部要求 `K % 256 == 0` → **该模型在 llama.cpp 生态里根本无法使用 K-quant**，只能用老式 Q4_0/Q8_0。T-MAC 的 2-bit 布局无此限制——这本身就是赛题价值论证的一部分。

### 1.3 官方仓库现状分析（决定了移植策略）

拿到仓库后先做了一轮"地雷排查"，结论：

| 官方路径 | 状态 | 结论 |
|---|---|---|
| aarch64 NEON 内核（`deploy/tuned/aarch64-hf-bitnet-3b`） | 官方主干 | 可用，**零源码修改**即可集成 |
| x86 内核 | **半成品支线** | 集成层多处类型/契约错误，修了 9 处才跑通 |
| 推理运行时 | 绑死 TVM + Python | 端侧不可用 → 必须走 llama.cpp C++ 集成（fork 自带但已过时且带 bug） |
| 文档 | 未写明任何数据契约 | fp16/fp32、尺度广播、kcfg 架构绑定全靠逆向调试（见 §2.3） |

由此定下移植策略：**分五阶段推进，每阶段只引入一个变量**——PC 打通算法 → 内核微基准 → 模拟器打通应用链路 → qemu 验证 arm64 正确性 → 真机测速。任何一阶段失败都能立刻定位是哪层引入的。

---

## 2. 阶段一：x86 宿主机打通（算法正确性 + PC 基准）

### 2.1 TVM 内核生成（研究 + 工程）

T-MAC 的计算内核不是手写的，是 TVM 代码生成器按目标形状产出的。在 WSL 里从零编译了 **TVM 0.17 + LLVM 17**，为 x86_64-ohos 目标生成 BitNet LUT 内核，产出两个变体：

- `-ags -1`（int32 累加变体，`deploy/tuned/ohos-x64-bitnet-3b/`）
- `-ags 64`（float 累加变体，`deploy/tuned/ohos-x64-ags64/`，**最终采用**——符号与队友的 aarch64 内核完全一致，便于双架构维护）

### 2.2 llama.cpp 集成：修复 9 处 bug（x86 路径）

模型转换链打通：`convert_hf_to_gguf.py --outtype int_n --kcfg <kcfg.ini> --enable-t-mac`，12.6GB HF → **966MB T-MAC GGUF**（288 张量，2.44 BPW）。

随后在 llama.cpp fork（`GGML_TMAC=ON`）里逐个排掉 9 处集成 bug，按层归类：

**上游内核生成器的 bug（我们提交修复）：**
1. `tbl_float_reset` 的 `memset(c, 0, m * sizeof(float_type))`：非 NEON 构建下 `float_type=float`（4B）但目标缓冲是 `half`（2B）→ **memset 双倍越界写坏栈上返回地址**（`ret` 跳向 0 → SIGSEGV）。ARM 上二者恰好同为 2B 所以官方从未暴露。修复：改用 `sizeof(_Float16)`，同步修了源码模板 `python/t_mac/intrins/tbl.cc`
2. ags64 生成器把 qgemm 中间缓冲声明为 `half CBits[N]`（2B/元素），但 AVX2 内核 `tbl_g4_int8_float_update_impl` 用 `_mm256_storeu_ps` 写 **float32**（4B/元素）→ **写入量 = 缓冲 2 倍 → 栈越界**，症状随机二选一：覆盖返回地址（SIGSEGV）或读到零页（输出全 0）。修复：3 处 `half CBits[N]` → `float CBits[N]`

**fork 集成层 bug：**

3. `tmac_float_type` 在 x86 误定义为 `float`，应为 `_Float16`（与 fp16 内核匹配）
4. `ggml_tmac_transform_tensor` 在 mul_mat 分支漏调（补调，幂等）
5. mmap 只读区 inplace scales 转换写崩（条件化跳过 / `--no-mmap`）
6. musl 无 `pthread_setaffinity_np`（glibc 扩展），`#if` 补 `!defined(__MUSL__)`
7. FMA intrinsic 缺兜底宏、`half` typedef 条件编译错误
8. `TMAC_KCFG_FILE` kcfg 路径环境变量支持

**数值契约（文档完全没有，逆向出来的）：**

9. **scales 广播**：模型里 per-tensor 只有 1 个 scale，但内核会按索引读到 S[120] → 运行时必须把单值广播成 256 份缓冲区

### 2.3 "能运行但不出结果"攻坚（本项目最险的一战） 🔍

**现象**：编译通过、`qgemm_lut_int8` 返回 0（成功）、不崩溃、不报错——但输出 C 全 0 或从第 0 层起 `dst[0]=-inf`。**静默失败**，无任何报错信息。

**方法**：不信任任何中间层，直接对照官方 `tests/test_e2e.py` 的数学构造端到端测试：

- `gen_testdata_ffn.py`（WSL）：从 HF 真实 `model.layers.0.mlp.gate_proj` 权重（8640×3200）生成量化输入 A、激活 B、尺度 S，用 NumPy 算出参考答案 Cref
- `run_test_dev.cpp`（设备侧）：跑我们 GCC/OHOS-clang 编译的 `kernels.cc`，逐层打印诊断（qlut 非零计数、lut_scales/lut_biases 非零计数、S 广播值、C 与 Cref 对比）

**逐层定位过程**：
1. 发现 `qlut`（激活生成的查找表）全零 → 问题在 preprocessor 或它的输入
2. 检查调用参数全部对齐 → 问题在**数据格式假设**
3. 最终确认两个隐性契约：x86 内核要求激活是 fp16 指针流（`tmac_float_type`），喂 fp32 就是垃圾；输出 C 类型也按架构不同（x86 float32 / ARM fp16）——正是 bug 1、2、3 的组合

**修复后的验证（铁证级）**：
- attn_q 输出 vs 官方数学参考：**NMSE 0.0027%**
- 设备 dump 真实激活 vs NumPy RMSNorm：差 **1e-7**
- 模型数据 vs 重新转换：**逐字节零差异**
- 调用参数实录（m_bits/K/A_off/C_off stride）全部对齐
- 最后一块拼图：**调试探针本身污染了结果**——探针的文件 IO/printf 在 4 线程推理中干扰时序，清掉探针后输出立即正确

**产出文本质量佐证**（BitNet 是 base 模型，无 chat 模板）：

```
The capital of France is → Paris. It is the largest city in France and the second
                            largest city in Europe...
Once upon a time         → , there was a little girl who was very sad...
The sun rises in the     → east and sets in the west.
```

甚至**超过**手工参考实现：连补了 sub_norm 的 HF transformers 版本都只能输出乱码（HF 并非 BitNet 的正确实现）。

### 2.4 同机三方基准（PC 基准，演示视频核心数据）

**方法学**：同一台机器（WSL2 / i7-14650HX / 4 线程）、同一份 BitNet-b1.58-3B、同一 prompt、固定 `-n 32 -t 4 -c 512 -s 42 --no-mmap`（定长定种子，可复现）。

| 配置 | 体积 | BPW | ms/token | **生成速度** | 加载 |
|---|---|---|---|---|---|
| ① llama.cpp f16 | 6.19 GiB | 16.0 | 226.72 | 4.41 tok/s | 67.9 s |
| ② llama.cpp Q4_0 | 1.79 GiB | 4.63 | 62.75 | 15.94 tok/s | 18.7 s |
| ③ **T-MAC LUT** | **965 MiB** | **2.44** | **39.84** | **25.10 tok/s** | **8.2 s** |

- T-MAC vs f16 = **5.69×**；T-MAC vs Q4_0 = **1.57×**
- **机理解释（无争议）**：三者有效带宽都在 25~31 GB/s（全部内存带宽受限），T-MAC 每 token 只读 1012 MB（807MB 二比特层权重 + 205MB F16 嵌入），Q4_0 要读 1.92 GB，f16 要读 6.65 GB → **加速比 ≈ 字节数之比**
- 诚实注记：f16 在 16GB 内存下与系统争内存（67.9s 加载即证据），最干净的对比是 T-MAC vs Q4_0

---

## 3. 阶段二：内核级微基准研究（SIMD vs LUT）

**目的**：端到端快可能来自"运气"（集成差异），内核级同形状对照才是硬证据。

**设计**：取 BitNet-3B 的 `ffn_gate` 真实形状（权重 N=8640 × K=3200，激活 M=1 单 token），T-MAC LUT 引擎与 llama.cpp SIMD 引擎在同一设备跑同一形状。**5 轮交错执行取 MIN**（对抗模拟器分钟级 ±2× 漂移），SIMD 侧用**持久线程池**（`ggml_graph_compute_with_ctx` 每次调用重建线程池的开销足以淹没测量——这本身是个方法学坑）。

### 3.1 鸿蒙模拟器实测（x86_64，4 vCPU）

**缓存热**（权重可驻留 L3）：

| 配置 | MIN ms/张量 | 有效带宽 |
|---|---|---|
| SIMD Q4_0 1 线程 | 3.291 | 4.7 GB/s |
| SIMD Q4_0 4 线程 | 0.739 | 21.0 GB/s |
| SIMD Q8_0 4 线程 | 1.148 | 25.6 GB/s |
| **T-MAC LUT 1 线程** | **1.045** | 6.6 GB/s |
| **T-MAC LUT 2 线程** | **0.547** | 12.6 GB/s |
| **T-MAC LUT 3 线程** | **0.411** | 16.8 GB/s |
| **T-MAC LUT 4 线程** | **0.338** | 20.4 GB/s |

- 1 线程：LUT 比 Q4_0 快 **3.15×**；4 线程：快 **2.19×**（vs Q8_0 快 3.40×）
- **LUT 用 2 线程（0.547ms）已经超过 Q4_0 用满 4 线程（0.739ms）** → 同样算力预算下 LUT 的性价比翻倍

**冷数据**（流式读 46.7~221MB 权重，缓存失效——对应真实推理"每 token 流一遍权重"）：

| 配置 | MIN ms | 带宽 | 相对 T-MAC |
|---|---|---|---|
| SIMD Q4_0 4 线程 | 1.133 | 13.7 GB/s | 慢 2.33× |
| SIMD Q8_0 4 线程 | 1.930 | 15.2 GB/s | 慢 3.97× |
| **T-MAC LUT 3 线程** | **0.440** | **15.7 GB/s** | 最快 |

**关键研究结论**：冷数据下每字节带宽 LUT ≈ Q8_0 ≈ Q4_0（13.7~15.7 GB/s 持平）→ **T-MAC 的胜势完全来自少读 1.9× 字节（2.00 vs 4.50 bpw），不是每字节更快**。这给"T-MAC 为什么快"提供了一个可答辩的、无争议的解释。

### 3.2 模拟器惩罚量化

同一 LUT 内核在 WSL 原生（i7-14650HX，AVX2+FMA）实测：热 4t 0.149ms（46.4 GB/s）、冷 4t 0.219ms（31.6 GB/s）→ **模拟器虚拟化惩罚 ≈ 2.0~2.3×**。这个系数后来成为"真机预期显著快于模拟器"论断的依据（真机实测 18.62 证实）。

---

## 4. 阶段三：鸿蒙模拟器（x86_64）—— 应用链路打通

### 4.1 HAP 应用工程（从命令行到正经 App）

`ohos/hap/`：ArkTS 界面 + NAPI 桥 + 双架构自适应内核。

- **NAPI 桥**（`napi_init.cpp`）：同步 + 异步（Promise，跑在 NAPI 工作线程）两套入口——`loadModelAsync / generateAsync`（完整 LLM 推理：加载 GGUF、tokenize、decode、top-k+温度采样、tok/s 计时）、`selfTest / bench`、`copyFdAsync`（大文件拷贝）
- **kcfg 自包含**：按 ABI 把 `kcfg.ini` 编进 `.so`，首次运行写入沙箱并 `setenv("TMAC_KCFG_FILE", …)` → 不依赖主机构建路径
- **双 ABI 静态库**：`prebuilt/{x86_64,arm64-v8a}/lib{llama,ggml}.a`，一个 HAP 同时装模拟器和真机
- **日志全透明**：`llama_log_set` 回调把 llama.cpp 全部 verbose 输出转发到 App 屏幕滚动控制台（带时间戳）→ 后面真机 hilog 被禁时救了命（§6.4）
- **CPUID 探针**（`cpuid_probe.c`）发现模拟器虚拟 CPU 为 **AVX2 ✅ / FMA ❌ / F16C ✅** → x86 内核编译**故意不加 `-mfma`**（加了直接 SIGILL）；这一探针思路后来复用到 arm64（HWCAP 解码）

### 4.2 模拟器上的两轮性能演进（Qwen2.5-0.5B 阶段）

| 指标 | 标量版 | SIMD 优化后 | 提升 |
|---|---|---|---|
| 文本生成 | 13.04 tok/s | **55.80 tok/s** | 4.3× |
| prompt 处理 | 17.15 tok/s | 115.32 tok/s | 6.7× |
| 模型加载 | 5272 ms | 2294 ms | 2.3× |

（SIMD 基线本身也做了认真优化，保证对比公平。）

### 4.3 同模型 A/B 对决（模拟器上的赛题核心实验）

同一台模拟器、同一份 BitNet-b1.58-3B、同 prompt 同参数，**两轮换序复测**：

| 指标 | llama.cpp SIMD（Q4_0） | **T-MAC LUT** | 对比 |
|---|---|---|---|
| 文件大小 | 1923.6 MB | **1012.8 MB** | 1.9× |
| **生成速度** | 5.60 / 5.43 tok/s | **11.67 / 11.86 tok/s** | **2.08× / 2.18×** |
| 有效带宽 | 10.4~10.8 GB/s | 11.8~12.0 GB/s | 1.13× |
| 加载 | 7.3~31.9 s | 1.4~3.7 s | 文件小 + 预计算 |
| 输出 | "…the city of Paris…" ✅ | "…the city of Paris…" ✅ | 均连贯 |

端到端 2.1× 与内核级 2.2×（§3.1）**互相印证** → 内核优势完整传导，集成层没有引入损耗（双方各发挥自身内核带宽的 76%~85%）。

**附带发现的 fork 缺陷（记录）**：T-MAC fork 的 `is_type_supported()` 误含 `GGML_TYPE_Q4_0`，加载器对每个张量无条件 `transform_tensor` → 缺 `bits=4` kcfg 时 `LOG(FATAL)` 中止：**T-MAC 版 llama-cli 无法加载任何标准 Q4_0 模型**。因此 SIMD 基线改用未编 T-MAC 的纯 SIMD 构建。

### 4.4 性能数字的诚实性修正（重要方法学事件）

模拟器 HAP 上测得一次 **21.79 tok/s**，未直接采用。交叉验证（同模型每 token 固定读 1012 MB 的物理约束）：五次测量 3.84 / 21.79 / 10.38 / 10.52 / 10.51 → 可信值 **≈10.5 tok/s**（96 ms/token → 10.5 GB/s 有效带宽，与实测内存带宽自洽），三次独立测量误差 <1.5%。

2× 波动根因：模拟器 4 个 vCPU 是宿主线程，i7 的 P 核/E 核性能差 ~2×，调度落点每次不同。**结论写进了报告：模拟器数字波动可达 ±2×，比赛/论文数字必须真机。**

**"是否真跑"的证伪实验**：把沙箱模型偷换成 1KB 随机文件 → App 立刻 `❌ 加载失败`；恢复真模型 → ✅ 加载 + 出文本。排除了"缓存假跑"的可能。

### 4.5 鸿蒙系统层踩坑（HAP 形态）

| 问题 | 现象 | 根因 | 修复 |
|---|---|---|---|
| **THREAD_BLOCK_6S** | 导出 966MB 模型时 App 被杀，导出文件截断（实测 793MB/383MB 半截文件，faultlog 有 appfreeze 记录） | `fs.copyFileSync` 阻塞 UI 线程 >6s，鸿蒙直接杀进程 | 拷贝下沉到 NAPI 工作线程（新增 `copyFdAsync`）+ `backgroundTaskManager.requestSuspendDelay` 瞬时任务保活 |
| 磁盘塞爆 | 模型导入截断 | 模拟器 /data 分区 90% 占用（剩 597MB） | 清理 `/data/local/tmp` 旧产物；App 增加"拷贝字节数 vs 源大小"校验 |
| IDE 网络全断 | DevEco 无法联网 | 用户级 `http_proxy` 环境变量劫持 IDE 请求 | 清理代理变量（备份为 `http_proxy_bak`） |
| 沙箱互访 | App 读不了 `/data/local/tmp`；shell 不能在 App 沙箱**新建**文件 | SELinux 策略（非权限问题） | 见 §6.3 的"0666 占位文件"通道 |

---

## 5. 阶段四：aarch64 验证（qemu-user，不碰真机验证正确性）

### 5.1 方法论

鸿蒙真机是 arm64，模拟器是 x86_64 —— 两条路各说一半真话：

- x86 模拟器：性能可参考，但**不是**真机指令集
- qemu-user 跑 aarch64：TCG 软件翻译慢 5~20×（性能作废），但**指令语义 100% 正确** → 恰好用于"对不对"的验证

策略：**把 arm64 的坑在 PC 上全部排掉，只留给真机一件事——测速。**

### 5.2 三层证据（全部通过）

1. **内核级**：aarch64 NEON 内核 vs 官方 NumPy 参考 **NMSE 8.397e-05**（相关系数 0.999964）；vs x86 输出 NMSE 1.819e-05（两架构互相印证）
2. **构建级**：aarch64-ohos 静态 `llama-cli`（7.9MB，无 INTERP），`llvm-objdump` 确认 **1085 条 NEON/fp16 指令**（`fmla v3.4s` fp32 累加 / fp16 存储），且含 ARM 专属内核符号 `qgemm_lut_t1_int8_m320_k3200_n1_b2`
3. **端到端**：qemu 里输出 `The capital of France is the city of Paris…` ✅，`system_info` 确认 NEON=1 / ARM_FMA=1 / FP16_VA=1

### 5.3 arm64 移植要点（逆向出的契约 + 拆掉的雷）

1. **fp16 契约（最容易踩）**：ARM 的 `preprocessor` 把激活读作 `half*`（fp16），x86 是 `float*` → 首版测试输出全 NaN 就是这个原因；ARM 内核输出 C 也是 fp16（集成层 `ggml_fp16_to_fp32_row` 转换接住）
2. **kcfg 布局按架构不同**：`m6400_*` 条目 ARM 用 bm=128/320、x86 用 bm=256 → **模型必须按目标架构重新转换**（已产出 `bitnet-3b-tmac-arm64.gguf`，966MB）
3. **CMake 假阳性雷**：fork 的 `check_cxx_source_compiles` 测的是基础 NEON `vmlaq_f32`（永远通过）→ 对 aarch64+T-MAC 恒选 `-march=armv8.7-a+fp16` → **armv8.2 的主流手机会 SIGILL**。修法：`-DGGML_COMPILER_SUPPORT_MATMUL_INT8=OFF` 强制走 armv8.2a+fp16 分支；重编后安检 **SVE/i8mm/v8.7 指令 0 条**、NEON fmla 807 条
4. **静态构建配方**：`BUILD_SHARED_LIBS=OFF + OHOS_STL=c++_static + -static`（qemu-user 下 musl 动态 loader 会段错误；只加 `-static` 而库还是 .so 会报 `attempted static link of dynamic object`）
5. **aarch64 路径零源码修改**（对比 x86 修了 9 处）——NEON 是官方主干
6. 调试工具坑：早前"OHOS clang 代码生成缺陷（vpaddw 缺失）"的结论**被推翻**——那是 `llvm-objdump.exe` 工具名写错且错误被 `2>/dev/null` 吞掉导致的**假数据**；真实机器码 OHOS 与 GCC 版本逐条相同。教训：**先验证工具链本身，再怪编译器**

另产出真机加固件：设备探针 `arm64_cpu_probe.c`（HWCAP 解码 + fork 隔离的 fp16 冒烟测试）、一键部署脚本 `deploy_arm64_device.ps1`。

---

## 6. 阶段五：真机部署（HUAWEI HBN-AL00 · HarmonyOS 6.1.1 · arm64）

### 6.1 SELinux 调查：CLI 路线被否决

原计划走"命令行二进制"路线（`hdc shell` 直接跑静态 llama-cli），在零售版 HarmonyOS NEXT 上验证失败：**`sh` 域（`u:r:sh:s0`）不允许执行 `/data/local/tmp` 标签的二进制**。开发板通常放行，零售机不放行。

结论：真机唯一可行形态是 **HAP 应用**（原生库跑在应用沙箱内，不受该策略限制）。好在阶段三已把 HAP 做完——分阶段策略在此兑现价值。

### 6.2 签名链修复（三个坑）

| 坑 | 现象 | 修复 |
|---|---|---|
| 缺 signingConfig 引用 | 构建报 "no signature file" | `build-profile.json5` products 补 `"signingConfig": "default"` |
| targetSdkVersion 空串 | IDE 同步失败（"值不正确"） | 删除空值（IDE 后续自动填 26.0.0） |
| 设备未授权（错误码 9568423） | 真机安装被拒 | 调试 profile 未包含真机 UDID → 手机连接状态下 IDE 重新生成自动签名 |

### 6.3 模型注入应用沙箱（966MB 怎么进手机）

SELinux 规则（实测确认）：App 读不了 `/data/local/tmp`；shell 不能在 App 沙箱**创建**文件；**但 shell 可以覆盖沙箱里已存在的可写文件**。

通道设计：

```
① App 启动时 chmod 沙箱目录 0777，创建 0666 的 model.gguf 占位文件
② hdc file send bitnet-3b-tmac-arm64.gguf \
     /data/app/el2/100/base/com.fandou.lutsa/haps/entry/files/model.gguf   # 覆盖占位
③ App 内点击「加载模型」
```

注意路径视图差异：App 内是 `/data/storage/el2/base/haps/entry/files/…`，shell 侧要用 `/data/app/...` 真实路径。App 同时提供产品级闭环：「📂 选择模型文件」（系统选择器导入）+「📤 导出到手机」（时间戳文件名保存）。

### 6.4 数据提取：hilog 被禁后的替代方案

零售机 hilog 不输出应用日志（SELinux）。好在阶段三已在 App 里内置**屏幕滚动控制台**（`llama_log_set` 回调转发全部 llama.cpp verbose 输出，带时间戳）→ 性能数据直接显示在屏幕上，靠截屏读取。真机最终数据即来自屏幕上的 `llama_perf_context_print` 输出。

### 6.5 真机成绩 🏆

**HUAWEI HBN-AL00 · HarmonyOS 6.1.1 · arm64 · T-MAC 2.44 BPW · 4 线程**

```
llama_perf_context_print:        load time =  1955.44 ms
llama_perf_context_print:  prompt eval time =   239.87 ms /     7 tokens (   34.27 ms per token,   29.18 tokens per second)
llama_perf_context_print:        eval time =   859.51 ms /    16 runs (   53.72 ms per token,   18.62 tokens per second)
llama_perf_context_print:       total time =  1099.38 ms /    23 tokens
```

输入 `The capital of china is` → 输出 `Beijing, Beijing is known for being very china's`（正确续写）。

三个杀手级结论：
1. **手机上的 T-MAC（18.62）超过桌面 i7 CPU 上的 llama.cpp Q4_0（15.94）** —— 端侧低比特让手机达到桌面级吞吐
2. T-MAC vs f16 = 4.22×（18.62 / 4.41，同模型跨平台）
3. 真机比模拟器快 1.7~1.8×（18.62 / 10.5~11.9）——与 §3.2 预测的"模拟器惩罚 ≈2×、真机 NEON+FMA+真实带宽会显著更快"一致；真机加载仅 1.96s

---

## 7. 全平台数据总对比（最终版）

### 7.1 端到端生成速度（同一 BitNet-b1.58-3B 模型系）

| 环境 | 引擎/量化 | BPW | 体积 | 生成 tok/s | ms/tok | 加载 |
|---|---|---|---|---|---|---|
| **🏆 真机 HBN-AL00（arm64）** | **T-MAC LUT** | **2.44** | **966 MB** | **18.62** | **53.72** | **1.96 s** |
| 真机 HBN-AL00（prompt 处理） | T-MAC LUT | 2.44 | 966 MB | 29.18 | 34.27 | — |
| 安卓真机 vivo V2323A（SD8G2，t4，指南 `docs/ANDROID-FIX.md`） | T-MAC LUT | 2.44 | 966 MB | 11.55 峰值 / ~10.6 持续 | 86.58 | 1.46 s |
| 桌面 i7-14650HX（4t，WSL） | T-MAC LUT | 2.44 | 965 MiB | 25.10 | 39.84 | 8.2 s |
| 桌面 i7-14650HX（4t） | llama.cpp Q4_0 | 4.63 | 1.79 GiB | 15.94 | 62.75 | 18.7 s |
| 桌面 i7-14650HX（4t） | llama.cpp f16 | 16.0 | 6.19 GiB | 4.41 | 226.72 | 67.9 s |
| 模拟器 CLI（x86_64） | T-MAC LUT | 2.44 | 1013 MB | 11.67~11.86 | ~85 | 1.4~3.7 s |
| 模拟器 CLI（A/B 对手盘） | llama.cpp Q4_0 | 4.63 | 1924 MB | 5.43~5.60 | ~180 | 7.3~31.9 s |
| 模拟器 HAP App | T-MAC LUT | 2.44 | 966 MB | ~10.5（交叉验证） | ~96 | 1.9~5.0 s |
| qemu-aarch64（TCG） | T-MAC LUT | 2.44 | 966 MB | 0.43（无性能意义） | 2334 | 11.6 s |
| 队友 Ubuntu VM（不同机，仅参考） | f16 | 16 | 6.34 GiB | 5.97 | 167.6 | — |

### 7.2 内核级（同形状 N=8640×K=3200，M=1）

模拟器冷数据 MIN：T-MAC 3 线程 **0.440 ms**（15.7 GB/s）vs Q4_0 4 线程 1.133 ms（慢 2.33×）vs Q8_0 4 线程 1.930 ms（慢 3.97×）；1 线程对决 LUT 快 3.15×；**LUT 2 线程 > Q4_0 4 线程**。WSL 原生同内核冷 4t 0.219 ms（31.6 GB/s）→ 模拟器惩罚 2.0~2.3×。

### 7.3 数据自洽性检验（防杠三连）

- 真机 18.62 tok/s = 53.72 ms/token，每 token 读 1012 MB → 有效带宽 **18.8 GB/s**，落在"桌面 25~31 GB/s 与模拟器 10~12 GB/s 之间"的合理区间（手机 LPDDR 带宽本就低于桌面双通道）
- 端到端加速比（2.1×）与内核级加速比（2.19×）互相印证
- 所有 tok/s 均有原始 `llama_perf_context_print` 输出可查（PC/模拟器存档于 BENCH 文档；真机为屏幕控制台截图）

---

## 8. 问题全清单（现象 → 根因 → 修复）

### 8.1 上游 / 集成层（13 处）

| # | 层 | 现象 | 根因 | 修复 |
|---|---|---|---|---|
| 1 | 上游内核 | 非 NEON 构建随机 SIGSEGV | `tbl_float_reset` memset 双倍越界（float_type=4B vs half=2B）写坏返回地址 | 改 `sizeof(_Float16)`，同步修模板 `tbl.cc` |
| 2 | 上游生成器 | qgemm SIGSEGV 或输出全 0（二选一随机） | `half CBits[N]`（2B）被 AVX2 内核按 float32（4B）写 → 栈越界 | 3 处改 `float CBits[N]` |
| 3 | fork | x86 数值错 | `tmac_float_type` 误定义 float，应为 `_Float16` | 修正定义 |
| 4 | fork | 内核从未被调用 | mul_mat 分支漏调 `transform_tensor` | 补调（幂等） |
| 5 | fork | mmap 加载即崩 | 只读区 inplace scales 写入 | 条件化跳过 / `--no-mmap` |
| 6 | fork | 链接失败 | musl 无 `pthread_setaffinity_np` | `#if !defined(__MUSL__)` 补丁 |
| 7 | fork | 编译错 | FMA intrinsic 无兜底、half typedef 条件错 | 补宏 + 条件编译 |
| 8 | fork | T-MAC 版无法加载 Q4_0 模型 | `is_type_supported` 误含 Q4_0 → 缺 kcfg 即 FATAL | SIMD 基线用纯 SIMD 构建（记录待上游修） |
| 9 | fork CMake | 真机潜在 SIGILL | armv8.7 检测假阳性（测的是基础 NEON） | `-DGGML_COMPILER_SUPPORT_MATMUL_INT8=OFF` 强制 armv8.2a+fp16 |
| 10 | 契约 | ARM 首测全 NaN | ARM preprocessor 读激活为 fp16（x86 为 fp32） | 按 `sizeof(tmac_float_type)==2` 分支处理 |
| 11 | 契约 | 输出垃圾 | ARM 内核输出 C 为 fp16 | `ggml_fp16_to_fp32_row` 转换 |
| 12 | 契约 | 数值错 | per-tensor scale 单值，内核索引到 S[120] | 广播成 256 份 |
| 13 | 契约 | 跨架构结果错 | kcfg 布局架构绑定（bm 128/320 vs 256） | 模型按目标架构重转 |

**另有调试方法学教训 2 条**：① 调试探针的文件 IO 干扰 4 线程时序，清掉探针输出才正确；② "编译器代码生成缺陷"结论被推翻——是 objdump 工具名写错 + `2>/dev/null` 吞错造成的假数据。

### 8.2 鸿蒙系统层（8 处）

| # | 现象 | 根因 | 修复 |
|---|---|---|---|
| 1 | 真机 CLI 二进制无法执行 | SELinux：sh 域禁 exec /data/local/tmp | 改走 HAP 应用形态 |
| 2 | App 读不到 /data/local/tmp | SELinux 域隔离 | 模型导入沙箱 |
| 3 | shell 无法给 App 塞模型 | 沙箱禁止 shell 创建文件 | App 建 0666 占位 → hdc 覆盖 |
| 4 | 大文件导出 App 被杀、文件截断 | `copyFileSync` 阻塞 UI >6s → THREAD_BLOCK_6S | NAPI 工作线程 `copyFdAsync` + `requestSuspendDelay` |
| 5 | 模型导入截断 | 模拟器 /data 90% 满 | 清理 + 拷贝字节数校验 |
| 6 | 安装报 no signature file | products 缺 signingConfig 引用 | 补 `"signingConfig": "default"` |
| 7 | IDE 同步失败 | targetSdkVersion 空串 | 删除空值 |
| 8 | 错误码 9568423 unauthorized | 调试 profile 缺真机 UDID | 连机重新生成自动签名 |

（另有：hdc 路径必须反斜杠 + Git Bash 需 `MSYS_NO_PATHCONV=1`；`http_proxy` 劫持 IDE 网络；零售机 hilog 禁用 → 屏幕控制台替代。）

---

## 9. 官方 T-MAC 的问题 vs 本项目贡献

**官方内核的数学是对的**（NMSE 0.0027% 级对齐官方参考），问题在工程层：

| 官方问题 | 本项目工作 |
|---|---|
| 隐性数据契约零文档（fp16/fp32 按架构、S 广播、kcfg 绑定、分块调用约定） | 全部逆向定位并文档化（本文 §2.3、§5.3） |
| x86 路径是带 bug 的半成品（栈越界 ×2、类型错误） | 修复并同步到源码模板（13 处之一） |
| 运行时绑死 TVM + Python，无纯 C++ 端侧路径 | llama.cpp C++ 全链路集成 + 鸿蒙 NAPI 应用化 |
| 无错误校验，喂错格式静默输出垃圾 | 建立 e2e 参考对比测试方法（NumPy 参考 + 设备侧逐层诊断） |
| CMake 架构检测假阳性（真机 SIGILL 雷） | 强制 armv8.2a+fp16 + 指令集安检（SVE/i8mm/v8.7 归零） |
| 未验证过任何 HarmonyOS 环境 | 模拟器 + qemu + **零售真机**三环境实测 |

**一句话答辩版**：官方 T-MAC 提供了正确的算法内核；我们补上了它缺失的工程化层——修掉了上游 bug、逆向并适配了全部隐性契约、替换了不可端侧化的运行时，最终让它跑进了鸿蒙真机。

---

## 10. 诚实性与可复现性声明

1. **所有加速比均为同机同模型同参数实测**，f16/Q4_0/T-MAC 三方对比固定 `-n 32 -t 4 -c 512 -s 42 --no-mmap`（定种子可复现）
2. 曾测得模拟器 21.79 tok/s，经五次交叉验证判定为 P/E 核调度离群值，**主动修正为 10.5**；波动机理（模拟器 vCPU 是宿主线程）已查明并记录
3. qemu 数据明确标注"无性能意义"（TCG 软件翻译）
4. 真机数据来自屏幕 `llama_perf_context_print` 原始输出（截图存档），未经二次加工
5. "是否真跑"做过证伪实验（换 1KB 假模型立即加载失败）
6. 复现命令全部收录于：`ohos/BENCH-native-3way.md`（PC 三方）、`ohos/BENCH-tmac-vs-simd.md`（模拟器 A/B + 内核级）、`ohos/ARM64-VALIDATION.md`（aarch64）、`ohos/hap/README.md`（App）

---

## 11. 关键文件索引

| 路径 | 内容 |
|---|---|
| `ohos/hap/` | HAP 应用工程（ArkTS UI + NAPI 桥 + 双 ABI prebuilt 静态库） |
| `ohos/hap/entry/src/main/cpp/napi_init.cpp` | NAPI 核心：异步推理、屏幕日志、kcfg 自包含、异步拷贝 |
| `ohos/hap/entry/src/main/ets/pages/Index.ets` | ArkTS UI：控制台、文件导入导出、requestSuspendDelay |
| `ohos/patches/llama_cpp_ohos.patch` | llama.cpp 鸿蒙 + T-MAC 集成全部补丁 |
| `ohos/staging-arm64/t-mac/` · `ohos/staging-x64/t-mac/` | 双架构内核 CMake 包 |
| `deploy/tuned/ohos-x64-ags64/` · `deploy/tuned/aarch64-hf-bitnet-3b/` | TVM 生成的目标内核 |
| `ohos/selftest/` | 探针与测试（cpuid_probe / arm64_cpu_probe / arm64_kernel_test / simd_vs_lut） |
| `ohos/scripts/` | build_kernels / deploy_arm64_device 等自动化脚本 |
| `D:\ohos-models\` | bitnet-3b-tmac-{ags64,arm64}.gguf / f16 / Q4_0 / 测试数据 / qemu |
| `ohos/screenshots/` | 实测截图证据（device/ 真机 + emulator/ 模拟器，含索引 README） |
| `ohos/PROGRESS.md` | 逐日过程记录（本报告的事件级底稿） |

## 12. 五阶段路线图（一图流）

```
阶段一 PC打通        阶段二 内核微基准      阶段三 模拟器HAP      阶段四 qemu-arm64     阶段五 真机
┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│ TVM内核生成   │   │ LUT vs SIMD  │   │ ArkTS+NAPI   │   │ NEON NMSE    │   │ SELinux调查  │
│ 13处bug修复  │ → │ 热冷×线程    │ → │ 双ABI静态库  │ → │ 8.4e-05 ✓    │ → │ 签名链修复   │
│ NMSE 0.0027%│   │ 1t 3.15×     │   │ THREAD_BLOCK │   │ 拆armv8.7雷  │   │ 0666占位注入 │
│ PC基准25.10  │   │ 4t 2.19×     │   │ A/B 2.1×     │   │ e2e出文本 ✓  │   │ 18.62 tok/s │
└──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘
   算法正确            机理证明            应用形态            跨架构正确          最终交付
```

每阶段只引入一个变量；任何阶段失败可立即定位责任层——这是整个移植零返工的根本原因。

## 13. 应用层能力增强与「双形态」交付口径 · 2026-09-28/29

### 13.1 三项新能力（应用级形态）

| 能力 | 实现 | 对应文档承诺 |
|---|---|---|
| **流式 token 回传** | NAPI `napi_threadsafe_function`：推理工作线程逐 token 推送 JS 线程，对话气泡打字机效果 | 设计文档 3.3 `Generate → token 流` |
| **TTFT + 峰值内存** | native 计时（eval 起点→首 token）+ `/proc/self/status` VmHWM；指标卡 5 格 | 设计文档 3.3 `GetMetrics {TTFT, tokens/s, 峰值内存}` |
| **L2 感知调度闭环** | Ability 前后台回调 → AppStorage → 页面规则引擎：切后台释放引擎（~1GB 内存归还系统，真降载）、回前台自动预热重载（~2s）；双开关提供「无调度」对照组 | 设计文档 L2「感知→调度→加速闭环」 |

### 13.2 双形态交付口径（答辩统一说法）

**同一个推理引擎，两种交付形态，各自匹配各自的环境：**

- **形态一 · 应用级引擎（已完整交付）**：NAPI 模块跑在应用进程内，任何零售鸿蒙设备可装可跑（HBN-AL00 真机 18.62 tok/s）。演示与上架以此形态。
- **形态二 · 系统服务化预研（技术已验证）**：同一引擎封装为用户态 SystemAbility（`libtmac_sa.so`），在 qemu 完成「信号→QoS→会话→warm kernel→释放」全链路冒烟；零售版鸿蒙 SELinux 不允许第三方注册系统服务（本项目实测探明的权限边界），开源鸿蒙开发板/标准系统环境可直接部署。

**答辩一句话**：“应用级形态负责今天可用，系统服务级形态负责明天可扩展，两者共用同一个已在双真机验证的内核。”

### 13.3 链式 KV：跨轮前缀复用（2026-09-29）🆕

**动机**：原实现每轮 `kv_cache_clear` 全量重算 prefill；多轮对话中重复前缀（上下文）被白白重算。链式 KV 让 KV 缓存成为一条**可续用的 token 链**：新一轮 prompt 与链求最长公共前缀，全命中零解码 / 部分命中 `seq_rm` 只删分歧段 + 只解码增量。

**实测（模拟器，同一 prompt 连续两轮）**：

| 轮次 | kv chain | prompt eval | TTFT |
|---|---|---|---|
| 首轮（建链） | reused 0 of 6 | 421.41 ms | ~422 ms |
| 次轮（复用） | **reused 6 of 6** | **0.00 ms** | **0.46 ms** |

**配套修复**：L2 调度加 60s 滞回（快速切后台再切回不再释放引擎/冲掉 KV 链；后台停留 >60s 才降载）。TTFT 指标改诚实口径（请求起点→首 token，含 prompt 处理）。

**意义**：单轮 demo 之外的**多轮会话场景**获得数量级 TTFT 改善，且 KV 读取流从"每轮全量"变"每轮增量"——对端侧内存带宽受限环境双重红利。演进方向（页级 block 哈希 + refcount 共享）已列入下一阶段。

### 13.4 与初赛设计文档的差距对照（收口状态）

| 文档承诺 | 状态 |
|---|---|
| L1 应用层（经代理调用） | ✅ 应用形态完整交付（代理路径随 L3 环境） |
| L2 感知与调度 | ✅ 应用内闭环 + 对照组演示（§13.1） |
| L3 SystemAbility | ◐ 代码完成 + qemu 全链路验证；真机部署受零售机权限约束（双形态口径） |
| L4 计算内核 | ✅ 双真机验证（18.62 / 11.55 tok/s） |
| 流式 token / GetMetrics | ✅（§13.1） |
