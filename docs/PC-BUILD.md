# PC（桌面 Linux / WSL / x86）构建与基准指南

> 本仓**桌面路径专区** · 其他平台：安卓 [`docs/ANDROID-FIX.md`](ANDROID-FIX.md) · 鸿蒙 [`ohos/FULL-REPORT.md`](../ohos/FULL-REPORT.md)
>
> 本指南聚焦 x86 桌面环境的**原生构建、三方基准（f16 / Q4_0 / T-MAC）与内核验证**。
> 全程方法论见 [`ohos/FULL-REPORT.md`](../ohos/FULL-REPORT.md)。

## 一、成果基线（i7-14650HX · WSL2 · 4 线程 · 同一 BitNet-b1.58-3B）

| 配置 | 体积 | BPW | ms/token | 生成速度 | 加载 |
|---|---|---|---|---|---|
| llama.cpp f16 | 6.19 GiB | 16.0 | 226.72 | 4.41 tok/s | 67.9 s |
| llama.cpp Q4_0 | 1.79 GiB | 4.63 | 62.75 | 15.94 tok/s | 18.7 s |
| **T-MAC LUT** | **965 MiB** | **2.44** | **39.84** | **25.10 tok/s** | **8.2 s** |

T-MAC vs f16 = **5.69×**；vs Q4_0 = **1.57×**。机理：三方有效带宽都在 25~31 GB/s
（内存带宽受限），T-MAC 每 token 只读 1012 MB，Q4_0 读 1.92 GB，f16 读 6.65 GB——
**优势来自位宽/字节数**。详见 [`ohos/BENCH-native-3way.md`](../ohos/BENCH-native-3way.md)。

## 二、内核准备（二选一）

**A. 用仓库预生成内核（最快）**：`deploy/tuned/ohos-x64-ags64*/`（AVX2 + fp16 契约版）。
注意 kernels.cc 的 `SignedHalvingAdder` 等在 AVX2 宏守卫内——**编译必须带 `-mavx2 -mf16c`**，
否则"undeclared identifier"式静默失败（加 `-mfma` 需 CPU 支持）。

**B. 自己用 TVM 生成**（WSL，从零编译 TVM 0.17 + LLVM 17，见上游 `docs/codegen.md`）：
产出 `deploy/tuned/<artifact>/{kernels.cc, kernels.h, kcfg.ini}` 三件套——
**kcfg 必须与模型转换时配套**（数据契约第一坑，跨 artifact 不可混用）。

## 三、模型转换

```bash
python 3rdparty/llama.cpp/convert_hf_to_gguf.py /path/to/bitnet-b1_58-3B \
    --outtype int_n --kcfg deploy/tuned/ohos-x64-ags64/kcfg.ini --enable-t-mac \
    --outfile bitnet-3b-tmac-ags64.gguf     # 12.6GB HF → 966MB / 288 张量 / 2.44 BPW
```

对照组：`--outtype f16`（6.65GB）→ `llama-quantize Q4_0`（1.83GB）。

## 四、构建（llama.cpp + T-MAC，宿主原生）

```bash
cmake -B build-native-tmac -DGGML_TMAC=ON \
      -DTMAC_DIR=<repo>/ohos/staging-x64/t-mac/lib/cmake/t-mac \
      -DCMAKE_C_FLAGS="-mavx2 -mf16c -mfma" -DCMAKE_CXX_FLAGS="-mavx2 -mf16c -mfma"
cmake --build build-native-tmac --target llama-cli -j 16
```

- T-MAC 集成补丁：`ohos/patches/llama_cpp_ohos.patch`（含 musl 守卫、gcc 宿主构建许可等）
- 纯 SIMD 基线（Q4_0/f16 对比用）：不加 `GGML_TMAC` 直接编

## 五、三方基准（定种子可复现）

```bash
./build/bin/llama-cli              -m bitnet-3b-f16.gguf  -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is'
./build/bin/llama-cli              -m bitnet-3b-q4_0.gguf -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is'
./build-native-tmac/bin/llama-cli  -m bitnet-3b-tmac-ags64.gguf -n 32 -t 4 -c 512 -s 42 --no-mmap -p 'The capital of France is'
```

## 六、内核级验证（tests/lut-verify/，10 分钟）

```bash
# 生成已知良好输入 + NumPy 参考答案
python tests/lut-verify/gen_testdata_ffn.py --hf-model ~/models/bitnet-b1_58-3B \
       --outdir ./vdata --kcfg deploy/tuned/ohos-x64-ags64-f32/kcfg.ini
# 主机直跑（f32 变体 + chunk 64 为正确配对；NMSE 应 < 1e-3）
g++ -O2 -mavx2 -mf16c -mfma -I deploy/tuned/ohos-x64-ags64-f32 \
    tests/lut-verify/run_test_dev.cpp deploy/tuned/ohos-x64-ags64-f32/kernels.cc -o rt
./rt ./vdata 64
```

基线（本仓实测）：x64-ags64-f32 + chunk=64 **NMSE 8.765e-05 PASS**；
chunk=128 与 fp16 契约违背的 FAIL 见 `tests/lut-verify/README.md`（静默失败坑活教材）。
跨架构基线：aarch64 8.397e-05（qemu 与安卓真机逐位一致）。

## 七、跨平台指针

| 平台 | 指南 |
|---|---|
| 桌面 x86（本指南） | 本文档 |
| 安卓（adb / NDK，真机已验证 11.55 tok/s） | [`docs/ANDROID-FIX.md`](ANDROID-FIX.md) |
| 鸿蒙（HAP / 真机 18.62 tok/s） | [`ohos/FULL-REPORT.md`](../ohos/FULL-REPORT.md) |
| 跨平台内核验证工具 | [`tests/lut-verify/`](../tests/lut-verify/) |

同一 aarch64 静态产物可横跨 鸿蒙/安卓/qemu 运行（2026-09-28 实证）。
