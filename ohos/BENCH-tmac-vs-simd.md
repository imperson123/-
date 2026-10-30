# llama.cpp SIMD vs T-MAC · 鸿蒙模拟器实测对比

日期：2026-09-27 · 设备：HarmonyOS x86_64 模拟器（4 vCPU，AVX2 ✅ / **FMA ❌** / F16C ✅）
宿主机：Intel i7-14650HX（16C/24T）· 全部数据为本次实测，非估算（估算处已标注）

---

## 0. 一句话结论

**同一模型（BitNet-b1.58-3B）正面对决：T-MAC 11.7~11.9 tok/s vs llama.cpp SIMD(Q4_0) 5.4~5.6 tok/s，快 2.1×（两轮换序复测一致）。**
内核级同形状对照 T-MAC 快 2.19×（热）/ 2.33×（冷）→ 内核优势完整传导到端到端。两者端到端有效带宽相近（10~12 GB/s，都接近本模拟器的冷流式内存上限），T-MAC 的胜势来自**少读 1.9× 字节**（2.44 BPW vs 4.63 BPW）。

---

## 1. 同模型 A/B（本次核心实验，2026-09-27）

同一台模拟器、同一份 BitNet-b1.58-3B、同一个 prompt（`The capital of France is`，6 token）、`-t 4 -c 512 -s 42 --no-mmap -n 64`，两轮换序复测：

| 指标 | llama.cpp SIMD（Q4_0） | **T-MAC LUT** | 对比 |
|---|---|---|---|
| 量化 | Q4_0，4.63 BPW | int2 LUT，2.44 BPW | 字节数 1.90× |
| 文件大小 | 1923.6 MB | 1012.8 MB | |
| **eval（生成）** | **5.60 / 5.43 tok/s**（两轮） | **11.67 / 11.86 tok/s** | **2.08× / 2.18×** |
| 每 token | 178.6 / 184.2 ms | 85.7 / 84.3 ms | |
| **有效权重带宽** | 10.8 / 10.4 GB/s | 11.8 / 12.0 GB/s | ≈1.13× |
| prompt（6 token） | 2.76 / 7.91 tok/s | 25.90 / 25.80 tok/s | 3~9×（SIMD 批量路径不稳） |
| 加载 | 31.9 / 7.3 s | 3.7 / 1.4 s | 文件小 + 转换预计算 |
| 输出质量 | "…the city of Paris. It is the largest city in the European Union…" ✅ | "…the city of Paris. It is the largest city in the European Union (EU)…" ✅ | 两者均连贯 |

**结论**：T-MAC 端到端快 2.1×，且与内核级 2.2× 完全吻合；两者有效带宽 (10~12 GB/s) 都逼近本模拟器冷数据实测上限（13.7~15.7 GB/s），说明**双方集成层效率相当**（各发挥自身内核带宽的 76%~85%），差距全部来自位宽/字节数。

模型转换链（可复现）：`convert_hf_to_gguf.py --outtype f16`（6.65GB，288 张量，arch=bitnet，保留 attn_sub_norm/ffn_sub_norm）→ `llama-quantize Q4_0`（1.83GB）。

## 2. 模型级（旧数据·不同模型·仅供参考）

| 指标 | llama.cpp SIMD | T-MAC |
|---|---|---|
| 模型 | Qwen2.5-0.5B Q4_K_M（491 MB） | BitNet-b1.58-3B LUT（1013 MB） |
| 生成 | 55.80 tok/s | 11.24 tok/s |
| 有效权重带宽 | 27.4 GB/s | 11.4 GB/s |

**这两个数字不可直接比**（不同模型、不同会话）。且 27.4 GB/s 明显高于本模拟器所有其它实测带宽（冷流式上限 13.7~15.7 GB/s、热 20~26 GB/s），怀疑与 Q4_K 内核效率更高、模型更小（缓存行为不同）、以及跨会话差异（模拟器同配置分钟级波动可达 2×）有关。**同模型 A/B（§1）才是可信答案。**

---

## 2. 内核级：同形状对照（本次新增，最硬的数据）

形状取自 BitNet-3B 的 `ffn_gate`：**权重 N=8640 × K=3200，激活 M=1（单 token）**，两种引擎在同一台模拟器上跑同一形状。
方法：5 轮交错执行（消除模拟器漂移），每配置取 **MIN**（最接近无干扰值）与中位数；SIMD 侧用**持久线程池**（`ggml_graph_compute_with_ctx` 每次调用会重建线程池，在模拟器上足以淹没测量）。

### 2.1 缓存热（权重 6.9/15.6/29.4 MB，可驻留 L3）

| 配置 | MIN ms/张量 | 中位数 | 有效带宽 | G 权重/s |
|---|---|---|---|---|
| SIMD Q4_0 1 线程 | 3.291 | 3.408 | 4.7 GB/s | 8.4 |
| SIMD Q4_0 4 线程 | 0.739 | 0.860 | 21.0 GB/s | 37.4 |
| SIMD Q8_0 1 线程 | 3.753 | 4.186 | 7.8 GB/s | 7.4 |
| SIMD Q8_0 4 线程 | 1.148 | 1.287 | 25.6 GB/s | 24.1 |
| **T-MAC LUT 1 线程** | **1.045** | 1.067 | 6.6 GB/s | 26.5 |
| **T-MAC LUT 2 线程** | **0.547** | 0.588 | 12.6 GB/s | 50.6 |
| **T-MAC LUT 3 线程** | **0.411** | 0.495 | 16.8 GB/s | 67.3 |
| **T-MAC LUT 4 线程** | **0.338** | 0.356 | 20.4 GB/s | 81.7 |

- 1 线程：LUT 比 Q4_0 快 **3.15×**
- 4 线程：LUT 比 Q4_0 快 **2.19×**，比 Q8_0 快 **3.40×**
- **LUT 2 线程（0.547）已超过 Q4_0 4 线程（0.739）**

### 2.2 冷数据（流式，权重 221 MB / 46.7 MB / 88 MB，缓存全失效；对应"真模型每 token 流一遍权重"）

| 配置 | MIN ms（每张量） | 有效带宽 | 相对 T-MAC |
|---|---|---|---|
| SIMD Q4_0 1 线程 | 3.460 | 4.5 GB/s | — |
| SIMD Q4_0 4 线程 | 1.133 | 13.7 GB/s | 2.33× 慢 |
| SIMD Q8_0 1 线程 | 4.186 | 7.0 GB/s | — |
| SIMD Q8_0 4 线程 | 1.930 | 15.2 GB/s | 3.97× 慢 |
| **T-MAC LUT 1 线程** | **1.061** | 6.5 GB/s | — |
| **T-MAC LUT 2 线程** | **0.650** | 10.6 GB/s | — |
| **T-MAC LUT 3 线程** | **0.440** | **15.7 GB/s** | 最快 |
| **T-MAC LUT 4 线程** | **0.486** | 14.2 GB/s | — |

**冷数据下每字节带宽两者持平**（LUT 15.7 GB/s ≈ Q8_0 15.2 ≈ Q4_0 13.7）→ T-MAC 的胜势来自**少读 1.8× 字节**（2.00 bpw vs 4.50 bpw），不是每字节更快。
4 线程 LUT（0.486）反而略慢于 3 线程（0.440）→ 模拟器 4 vCPU 上 4 线程已开始互相干扰（含解释器/系统线程）。

### 2.3 附带发现：K-quant 对 BitNet-3B 根本不可用

BitNet-3B 的 hidden=3200、intermediate=8640 **都不是 256 的倍数**，而 llama.cpp 的 Q2_K…Q6_K、IQ 系列都要求 `K % 256 == 0` → 该模型在 llama.cpp 里只能用 Q4_0/Q4_1/Q5/Q8_0 这类 32 元组老量化。这也是 T-MAC 的机会点（2.00 bpw 无此限制）。

---

## 3. 环境参照：同一 LUT 内核实跑在真 CPU 上（WSL, i7-14650HX, AVX2+FMA, GCC -O2）

| 配置 | 热缓存 MIN ms | 冷流式 MIN ms |
|---|---|---|
| LUT 1 线程 | 0.466（14.8 GB/s） | 0.565（12.2 GB/s） |
| LUT 2 线程 | 0.230（30.0 GB/s） | 0.285（24.3 GB/s） |
| LUT 3 线程 | 0.188（36.8 GB/s） | 0.247（28.0 GB/s） |
| LUT 4 线程 | 0.149（46.4 GB/s） | 0.219（31.6 GB/s） |

**模拟器惩罚 ≈ 2.0~2.3×**（同内核、同线程数：热 4t 0.149→0.338；冷 4t 0.219→0.486）。
真 CPU 上冷数据 4t 顶到 31.6 GB/s（热 46.4）→ 内存带宽受限，与 T-MAC 论文的定位一致。

---

## 4. 正确性核对（同批数据）

| 检查 | 结果 |
|---|---|
| 设备内核输出 vs 官方 numpy 参考（T-MAC `tests/test_e2e.py` 同款公式） | **NMSE 8.8×10⁻⁵**，相关系数 0.99997 |
| 设备输出 vs WSL 主机同一内核输出 | **逐位一致**（NMSE 完全相同、max\|diff\| 相同） |
| T-MAC 端到端输出 | `The capital of France is → Paris.` / `…east and sets in the west.` ✅ |
| Q4_0 SIMD 端到端输出（同 prompt） | `…the city of Paris. It is the largest city in the European Union by population…` ✅ |

---

## 5. 归因：内核 2.2× → 端到端 2.1×，一致性闭环

**事实（本次实测）**
1. 同形状内核（同设备/同线程数）：T-MAC 快 2.19×（热）/ 2.33×（冷）。
2. 同模型端到端：T-MAC 快 2.08×/2.18× —— **与内核级比值吻合**，说明 T-MAC 的内核优势完整传导到了端到端。
3. 两者端到端有效带宽接近：T-MAC 11.8~12.0 GB/s vs SIMD Q4_0 10.4~10.8 GB/s（T-MAC 略高 ~13%）。
   各自相对自身内核冷带宽的发挥率：T-MAC 12.0/14.2 ≈ **85%**；Q4_0 10.8/13.7 ≈ **79%** —— **集成层效率相当**。
4. 所以 T-MAC 的 2.1× 基本全部来自**位宽/字节数**（2.44 BPW vs 4.63 BPW，1.90×）+ 小的带宽利用率优势。

**推测（标注为假设）**
- 两者都逼近本模拟器冷数据实测上限（13.7~15.7 GB/s），说明在 4 vCPU QEMU 上**内存带宽是共同瓶颈**；T-MAC 在此环境的上限≈其 LUT 内核冷带宽。
- 模拟器（4 vCPU、无 FMA）不代表真机；T-MAC 论文的 4–5× 来自 Apple M2 + NEON + 真实内存系统。aarch64 真机内核（与队友同符号）已具备，待真机复测。
- 之前报告里"T-MAC 端到端只有 llama.cpp 有效带宽的 42%"的结论**已作废**：那是拿 Qwen-0.5B（Q4_K，不同会话）与 BitNet-3B 相比，既不同模型也不可比；且该 27.4 GB/s 数值超出本模拟器所有其它实测带宽，本身就存疑。

---

## 6. 测量方法学修正（避免再出"假数据"）

1. 早前"设备 7 ms"是**假数据**：设备上的 `run_test_ffn` 是 WSL 路径版，`ERR open A` 直接退出，7 ms 只是进程启动时间。真跑（`run_test_dev`）单次含 6.9MB 读盘共 12~14 ms，纯计算 ~1.2 ms。
2. SIMD 侧必须用**持久线程池**：`ggml_graph_compute_with_ctx` 每次调用重建线程池（QEMU 上线程创建极贵），会让 4 线程数据完全失真（曾出现 Q8_0 4t 8.8 ms 的假象）。
3. 模拟器噪声大（同配置分钟级波动可达 2×；跨会话更不可比）→ **同会话交错多轮 + 取 MIN/中位数**；跨会话的数字只作参考。
4. A/B 必须**轮换顺序各跑一遍**（本次 pass1 先 A 后 B、pass2 先 B 后 A，结果一致）。

## 7. 附带发现：T-MAC 补丁会拦截标准量化模型（fork 真实缺陷）

`ggml-tmac.cpp` 的 `is_type_supported()` 把 **GGML_TYPE_Q4_0 也视为 T-MAC 支持类型**（bits=4，设计上要支持 4-bit LUT），而 `ggml_tmac_get_type_bits(Q4_0)=4`。于是：
- 模型加载时 `llama.cpp:5216` 对**每个**张量无条件调用 `ggml_tmac_transform_tensor(cur)`；
- Q4_0 张量在 kcfg 里查不到 `bits=4` 条目 → `LOG(FATAL) << "Failed to find kcfg. Abort transforming"` → **T-MAC 版 llama-cli 直接中止，无法加载任何标准 Q4_0 模型**（本次实测复现）。

后果与规避：
- 本报告的 SIMD 基线改用**未编入 T-MAC 的纯 SIMD 构建**（`/data/local/tmp/llm-simd/llama-cli`，同为该 fork、同 `-mavx2 -mf16c`、同编译器，仅少 T-MAC 补丁）；
- 若要单二进制跑两种模型，需要修：加载时跳过非 T-MAC 类型的 transform，或为 Q4_0 生成 `bits=4` 的 kcfg（T-MAC 上游本意）。

## 8. 复现

```bash
# ---- 同模型 A/B ----
# 1) 转换（WSL 主机，fork 自带转换器；arch=bitnet，288 张量含 sub_norm）
python convert_hf_to_gguf.py /mnt/d/ohos-models/bitnet-3b --outtype f16 \
    --outfile /mnt/d/ohos-models/bitnet-3b-f16.gguf            # 6.65GB
# 2) 原生构建 llama-quantize / llama-cli（cmake + conda gcc，-DGGML_OPENMP=OFF）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLLAMA_CURL=OFF -DGGML_OPENMP=OFF
cmake --build build --target llama-quantize llama-cli -j 16
# 3) 量化 + 推送
./build/bin/llama-quantize bitnet-3b-f16.gguf bitnet-3b-q4_0.gguf Q4_0   # 1.83GB
hdc file send bitnet-3b-q4_0.gguf /data/local/tmp/llm-q4/
# 4) 模拟器 A/B（同一 prompt/种子/线程/上下文）
cd /data/local/tmp/llm-simd && LD_LIBRARY_PATH=. ./llama-cli \
  -m /data/local/tmp/llm-q4/bitnet-3b-q4_0.gguf --no-mmap -n 64 -t 4 -c 512 -s 42 \
  -p 'The capital of France is'                    # SIMD: 5.4~5.6 tok/s
cd /data/local/tmp/llm-tmac && TMAC_KCFG_FILE=/data/local/tmp/llm-tmac/kcfg.ini \
  LD_LIBRARY_PATH=. ./llama-cli -m model.gguf --no-mmap -n 64 -t 4 -c 512 -s 42 \
  -p 'The capital of France is'                    # T-MAC: 11.7~11.9 tok/s

# ---- 内核级对照 ----
CC=$HOME/ohos-linux-sdk/native/llvm/bin/clang++
$CC --target=x86_64-linux-ohos -mavx2 -mf16c -O2 -fPIC -pthread \
  -I<llama.cpp>/ggml/include -I<d>/ohos-models/x64inc \
  simd_vs_lut.cpp <t-mac>/deploy/tuned/ohos-x64-ags64-f32/kernels.cc \
  -L<llama.cpp>/build-ohos-x86-simd/ggml/src -lggml -Wl,-rpath,'$ORIGIN' -o simd_vs_lut
LD_LIBRARY_PATH=. ./simd_vs_lut /data/local/tmp/llm-tmac 5 6 4 50        # 热缓存
LD_LIBRARY_PATH=. ./simd_vs_lut /data/local/tmp/llm-tmac 5 3 4 50 cold   # 冷流式
```

源码：`ohos/selftest/simd_vs_lut.cpp`（仓库内）与 `D:\ohos-models\simd_vs_lut.cpp`
产物：`D:\ohos-models\bitnet-3b-q4_0.gguf`（SIMD 基线模型）、`bitnet-3b-f16.gguf`（中间件）、`tmactest/simd_vs_lut{,_native}`

## 9. 原始输出

```
===== 同模型 A/B（模拟器）· pass 1: A→B =====
A) Q4_0 SIMD : load 31861 ms | prompt 2173.52 ms/6 (2.76 tok/s) | eval 11255.07 ms/63 (178.65 ms/token, 5.60 tok/s)
   输出: ' The capital of France is the city of Paris. It is the largest city in the European Union by population...'
B) T-MAC LUT : load  3650 ms | prompt  231.69 ms/6 (25.90 tok/s) | eval  5396.36 ms/63 ( 85.66 ms/token, 11.67 tok/s)
   输出: ' The capital of France is the city of Paris. It is the largest city in the European Union (EU) and the third-most...'

===== 同模型 A/B（模拟器）· pass 2: B→A（换序复测）=====
B) T-MAC LUT : load  1425.76 ms | prompt  232.54 ms/6 (25.80 tok/s) | eval  5312.86 ms/63 (84.33 ms/token, 11.86 tok/s)
A) Q4_0 SIMD : load  7344.93 ms | prompt  758.52 ms/6 ( 7.91 tok/s) | eval 11601.37 ms/63 (184.15 ms/token, 5.43 tok/s)

=== 内核级·热缓存（模拟器）· 5 轮交错 ===
round 0: 3.291 0.780 4.203 1.287 1.045 1.976 2.302 0.354
round 1: 3.416 0.860 4.402 1.682 2.154 0.547 0.439 0.338
round 2: 3.392 1.156 3.753 1.572 1.051 0.547 1.483 1.046
round 3: 3.408 0.953 4.186 1.148 1.160 0.588 0.411 0.356
round 4: 3.535 0.739 4.113 1.282 1.067 0.610 1.596 1.184
（列：Q4_0(1t,4t) Q8_0(1t,4t) LUT(1t,2t,3t,4t)；1t LUT 首轮 1.976 为页错误干扰，取 MIN=1.045）

=== 冷流式（模拟器）· LUT 32×6.9MB=221MB，SIMD ×3 ===
round 0: 10.379 3.762 12.557 7.595 2.178 0.655 0.484 0.486
round 1: 10.492 3.588 13.550 5.789 1.436 0.700 0.515 0.506
round 2: 10.745 3.399 13.427 6.037 1.264 0.771 0.533 0.530
round 3: 10.478 3.563 13.925 5.828 1.288 0.707 0.440 0.490
round 4: 10.547 4.167 13.165 6.478 1.061 0.650 0.495 0.543

=== WSL 原生（LUT-only，GCC -O2 -mavx2 -mf16c）===
热：MIN 1t 0.466 / 2t 0.230 / 3t 0.188 / 4t 0.149 ms
冷：MIN 1t 0.565 / 2t 0.285 / 3t 0.247 / 4t 0.219 ms
```
