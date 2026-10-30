# 同机对比：f16 vs Q4_0 vs T-MAC（演示视频用数据）

**同一台机器**：WSL2 / Intel i7-14650HX（16C，AVX2+FMA，4 线程）· 模型同一份 BitNet-b1.58-3B ·
prompt 同为 `The capital of France is` · `-n 32 -t 4 -c 512 -s 42 --no-mmap`

| 配置 | 模型大小 | BPW | eval（ms/token） | **生成速度** | prompt 处理 | 加载 |
|---|---|---|---|---|---|---|
| ① f16 + llama.cpp（SIMD） | 6.19 GiB | 16.00 | 226.72 | **4.41 tok/s** | 1.01 tok/s | 67.9 s |
| ② Q4_0 + llama.cpp（SIMD） | 1.79 GiB | 4.63 | 62.75 | **15.94 tok/s** | 37.82 tok/s | 18.7 s |
| ③ **T-MAC 2.44bpw（LUT）** | **965 MiB** | 2.44 | **39.84** | **25.10 tok/s** | 52.74 tok/s | 8.2 s |

**加速比**：
- T-MAC vs f16 = **5.69×**（25.10 / 4.41）
- T-MAC vs llama.cpp 自家 Q4_0 = **1.57×**（25.10 / 15.94）

**为什么快（无争议的解释）**：三者有效带宽都在 25~31 GB/s（都是内存带宽受限型），
**T-MAC 每 token 只读 1012 MB**（2.44bpw），Q4_0 要读 1.92 GB，f16 要读 6.65 GB。
即：**优势来自位宽/字节数，不是"魔法"** —— 这正是 T-MAC 论文的立论。

**复现命令**：
```bash
# 构建（宿主原生，T-MAC 需要 clang；用 gcc 做基准构建需允许非 clang，见补丁）
cmake -B build-native-tmac -DGGML_TMAC=ON -DTMAC_DIR=<repo>/ohos/staging-x64/t-mac/lib/cmake/t-mac \
      -DCMAKE_C_FLAGS="-mavx2 -mf16c -mfma" -DCMAKE_CXX_FLAGS="-mavx2 -mf16c -mfma"
cmake --build build-native-tmac --target llama-cli -j 16
# 三方各跑一遍
./build/bin/llama-cli           -m bitnet-3b-f16.gguf      -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is'   # ①
./build/bin/llama-cli           -m bitnet-3b-q4_0.gguf     -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is'   # ②
./build-native-tmac/bin/llama-cli -m bitnet-3b-tmac-ags64.gguf -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is' # ③
```

**演示视频建议拍法**：左右分屏 ——
- 左：终端里 `llama.cpp（f16 或 Q4_0）` 的输出（有 `llama_perf_context_print: eval time = … ms per token, … tokens per second`）
- 右：鸿蒙 App（`ohos/hap`）里的同款控制台输出（格式逐字一致，可直接对比数字）
- 画面上同步跑，T-MAC 那侧明显更快吐字 ✓

**注意（诚实说明）**：
- f16 在本机 16 GB 内存下与系统争内存（加载 67.9 s 就是证据）；f16 的绝对速度受内存压力影响，
  所以最干净的对比是 **T-MAC vs Q4_0（1.57×）**（两者都能轻松装进内存）。
- 鸿蒙模拟器上同一 T-MAC 模型是 **10.05 tok/s**（模拟器有 ~2.2× 虚拟化惩罚 + 无 FMA），
  真机（NEON + FMA + 真实带宽）预期明显更高。
- 与队友 Ubuntu 虚拟机上的 f16（5.97 tok/s，6.34 GiB 模型）不是同机数据，仅作参考。
