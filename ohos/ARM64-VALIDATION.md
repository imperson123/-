# aarch64（真鸿蒙架构）验证报告 · 2026-09-27

**结论：不碰真机，已在 PC 上完成 aarch64 的"功能正确性"全链路验证（内核数值 + 端到端出文本）；性能数字仍必须真机（qemu 是 TCG 软件模拟，0.43 tok/s 无意义）。**

---

## 0. 为什么做这个

鸿蒙真机是 **arm64**，而 DevEco 模拟器是 x86_64 虚拟机（宿主是 x86，近原生加速）。两条路都不说谎：
- x86_64 模拟器：同架构 → 性能数字可参考，但**不是**真机的指令集（无 FMA、内核是 x86）
- arm64 上跑 QEMU：跨架构 → **TCG 软件翻译，慢 5~20×**，性能作废，但**指令语义正确** → 恰好用于"对不对"的验证

本次走第二条：把 aarch64 的东西全部跑起来，把坑排干净，只留给真机一件事——测速。

---

## 1. 环境（全部在现有 PC 上）

| 组件 | 说明 |
|---|---|
| 交叉工具链 | OHOS Linux SDK clang（`--target=aarch64-linux-ohos`，musl，含 aarch64 sysroot） |
| qemu-user | `qemu-aarch64-static` v7.2（Debian deb 解包，`/mnt/d/ohos-models/qemu/`） |
| aarch64 T-MAC 内核 | 仓库自带 `deploy/tuned/aarch64-hf-bitnet-3b/`（NEON + fp16 变体） |
| 目标三元组 | `aarch64-linux-ohos`（与真机一致） |

---

## 2. 验证结果（三个层次）

### 2.1 内核级：数值正确 ✅
`arm64_kernel_test`（静态 aarch64 二进制）跑 ffn_gate（N=8640×K=3200, M=1, 135 块）：

| 对比 | NMSE | 相关系数 |
|---|---|---|
| ARM64 输出 vs 官方 numpy 参考 | **8.397e-05** | 0.999964 |
| ARM64 输出 vs x86 输出（已逐位验证过） | 1.819e-05 | 0.999990 |

fp16 与 fp32 标量的舍入差异（ls[0]: 0.040710 vs 0.040736；lb[0]: 1.533203 vs 1.527270）在预期内；`qlut[0..3]` 与 x86 **完全一致**（41/127/27/113）。

### 2.2 构建级：真 NEON 代码 ✅
`llama-cli`（aarch64-ohos，静态，7.9 MB）：
- ELF type=AArch64、**无 INTERP（纯静态）**
- 符号表含 ARM 专属内核 `qgemm_lut_t1_int8_m320_k3200_n1_b2`（x86 无此形状）
- `llvm-objdump` 反汇编：**1085 条 NEON/fp16 指令**，如 `fmla v3.4s, v0.4s, v1.4s`（fp32 累加 / fp16 存储）

### 2.3 端到端 ✅
`qemu-aarch64-static -cpu max llama-cli -m bitnet-3b-tmac-arm64.gguf -n 16 -t 4 -c 512 -p 'The capital of France is'`：

```
 The capital of France is the city of Paris. It is the largest city in the European Union in terms
```
```
system_info: n_threads = 4 | NEON = 1 | SVE = 1 | ARM_FMA = 1 | FP16_VA = 1 | MATMUL_INT8 = 1 | AVX2 = 0
load time = 11644.97 ms   (含 288 张量 transform)
prompt eval = 10474.00 ms / 6 tokens
eval time   = 35012.77 ms / 15 runs  (2334.18 ms/token, 0.43 tok/s)  ← TCG 软件模拟，无性能意义
total 57.6 s
```

---

## 3. 移植要点（本次踩到/发现，真机移植必看）

1. **fp16 契约（最容易踩）**：ARM 的 `preprocessor` 把激活读作 **`half*`**（fp16），x86 是 `float*`。本次第一版测试输出全 NaN 就是这个原因。ggml 集成层已按 `sizeof(tmac_float_type)==2` 分支处理。
2. **ARM 内核输出 C 是 fp16**，x86 是 fp32（我们 x86 修复的结果）。集成层已有对应转换：`ggml.c` 中 `sizeof(tmac_float_type)==2 → ggml_fp16_to_fp32_row(...)`。
3. **kcfg 布局按架构不同**：`m6400_*` 条目 ARM 用 bm=128/320，x86 用 bm=256 → **模型必须按目标架构重新转换**。已产出 arm64 布局模型：`bitnet-3b-tmac-arm64.gguf`（966 MB，288 张量）。
   （ffn_gate 的 `m17280_k3200` 两边一致 → 现成测试数据 A.bin/Cref.bin 可直接复用。）
4. **fork 的 CMake 对 aarch64+T-MAC 总会选 `-march=armv8.7-a+fp16`**：其 `check_cxx_source_compiles` 测的是 `vmlaq_f32`（基础 NEON，永远通过）→ 假阳性。**armv8.2 的真机（多数手机）会 SIGILL**，真机构建时必须强制 `-march=armv8.2a+fp16`（改 CMake 或把该分支短路）。
5. **静态构建配方**（qemu-user 直接跑，不需要 loader）：
   `-DBUILD_SHARED_LIBS=OFF -DOHOS_STL=c++_static -DCMAKE_EXE_LINKER_FLAGS="-static"`
   （只加 `-static` 而库还是 .so 会报 `attempted static link of dynamic object`）
6. **aarch64 路径零源码修改**：不需要任何补丁（对比 x86 路径修了 9 处）——因为 NEON 是 T-MAC 的官方主干，x86 才是半成品支线。
7. `TMAC_KCFG_FILE` 解析顺序：环境变量 → 编译期内置路径（`TMAC_DIR/lib/kcfg.ini`）→ FATAL。qemu 下用内置路径即可。

---

## 4. 复现

```bash
# 0) qemu-user（若 apt 代理损坏，直接解包 deb）
curl -f -o qemu-user-static.deb https://mirrors.aliyun.com/debian/pool/main/q/qemu/qemu-user-static_7.2+dfsg-7+deb12u18+b3_amd64.deb
dpkg-deb -x qemu-user-static.deb ./x && cp x/usr/bin/qemu-aarch64-static .

# 1) 内核级验证（复用 ffn_gate 数据，布局两 arch 相同）
clang++ --target=aarch64-linux-ohos -O2 -static -march=armv8.2a+fp16 -nostdlib++ \
  -I ohos/staging-arm64/t-mac/include arm64_kernel_test.cpp \
  deploy/tuned/aarch64-hf-bitnet-3b/kernels.cc -o arm64_kernel_test -lm
qemu-aarch64-static -cpu max ./arm64_kernel_test /mnt/d/ohos-models/tmactest 64

# 2) 模型：按 aarch64 kcfg 重新转换
python convert_hf_to_gguf.py /path/to/bitnet-3b --outtype int_n \
  --kcfg deploy/tuned/aarch64-hf-bitnet-3b/kcfg.ini --enable-t-mac \
  --outfile bitnet-3b-tmac-arm64.gguf

# 3) 端到端：arm64-ohos 静态构建（T-MAC 开）+ qemu 运行
cmake -B build-ohos-arm64 -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=<ohos-sdk>/native/build/cmake/ohos.toolchain.cmake \
  -DOHOS_ARCH=arm64-v8a -DOHOS_STL=c++_static -DCMAKE_EXE_LINKER_FLAGS="-static" \
  -DCMAKE_C_FLAGS="-march=armv8.2a+fp16" -DCMAKE_CXX_FLAGS="-march=armv8.2a+fp16" \
  -DGGML_OPENMP=OFF -DGGML_TMAC=ON -DTMAC_DIR=<repo>/ohos/staging-arm64/t-mac/lib/cmake/t-mac
cmake --build build-ohos-arm64 --target llama-cli -j 16
qemu-aarch64-static -cpu max build-ohos-arm64/bin/llama-cli \
  -m bitnet-3b-tmac-arm64.gguf --no-mmap -n 16 -t 4 -c 512 -p "The capital of France is"
```

产物位置：
- `ohos/staging-arm64/t-mac/`（aarch64 内核 CMake 包，已归档进仓库）
- `D:\ohos-models\bitnet-3b-tmac-arm64.gguf`（arm64 布局模型）
- `D:\ohos-models\qemu\qemu-aarch64-static`、`D:\ohos-models\tmactest\arm64_kernel_test`
- `~/llama-arm64/llama.cpp/build-ohos-arm64/bin/llama-cli`（WSL 内，静态 aarch64）

---

## 5. 真机可用性加固（2026-09-27 追加，"推上真机能不能跑"）

**先修了一个会让真机直接崩的雷**：fork 的 CMake 对 aarch64+T-MAC 恒选 `-march=armv8.7-a+fp16`（`check_cxx_source_compiles` 假阳性）——armv8.2 的主流手机执行 v8.6+/v8.7 指令会 **SIGILL**。

修法与验证（不改进 CMake，靠预置缓存变量走正确分支）：
```bash
cmake ... -DGGML_COMPILER_SUPPORT_MATMUL_INT8=OFF     # → 走 armv8.2a+fp16 分支
```
重编后二进制指令集安全检查：**SVE 0 条、i8mm 0 条、ld64b/st64b 0 条，NEON `fmla` 807 条** → 只依赖 armv8-a + fp16（几乎所有 ARM64 手机都满足）。再用 qemu 重跑端到端：`The capital of France is the city of Paris. It is the…` ✅

**新增两个交付件**：
1. `ohos/selftest/arm64_cpu_probe.c` — 设备探针（静态、任意 arm64 可跑）：打印 uname / cpuinfo / HWCAP+HWCAP2 解码（NEON、FP16-SIMD、dotprod、i8mm、SVE…），并**在 fork 出的子进程里**做 fp16 NEON 冒烟测试（设备不支持时只报 SIGILL，不会把探针本身打死），最后给出判定：`this device can run the armv8.2a+fp16 build` ✓/✗。已用 qemu 验证（-cpu max 下全 YES + PASS）。
2. `ohos/scripts/deploy_arm64_device.ps1` — 一键真机部署：设备检查 → **编译并运行探针（不通过就中止）** → 推 T-MAC 二进制+模型（模型存在则跳过） → 跑推理；`-WithSimdBaseline` 再跑一遍纯 SIMD（Q4_0）做同模型 A/B。二进制默认从 WSL 构建目录自动拷贝（`\\wsl.localhost\...`）。无设备时会明确报错而不是静默失败（已测）。

**"推上真机能跑吗？"——诚实回答**：

| 层面 | 状态 |
|---|---|
| 指令集/ABI（aarch64、musl 静态、无动态依赖） | ✅ 已验证（反汇编 + qemu 端到端） |
| 数值正确性（内核 NMSE 8.4e-05） | ✅ 已验证 |
| 模型布局（arm64 kcfg 重转） | ✅ 已验证 |
| 真机 OS 用户态（鸿蒙内核/沙箱） | ⚠️ **未验证**：qemu-user 跑的是 WSL 的 Linux 内核；且 HarmonyOS NEXT 对 `hdc shell` 执行 `/data/local/tmp` 下二进制可能有限制（开发板/OpenHarmony 通常放开） |
| 设备算力/内存（966MB 模型 + 1GB 级 RSS） | ⚠️ 预计可行（8GB+ 手机/2GB+ 板子），但 shell 域的资源上限未知 |
| **性能数字** | ❌ 必须真机（TCG 0.43 tok/s 无意义） |

结论：**"能跑"的概率很高，但第一件事是插上设备跑 `deploy_arm64_device.ps1`——探针 30 秒就能给出"能不能跑"的确定答案**；若卡在"shell 不允许执行二进制"，则改走 HAP+NAPI 形态（arm64 静态库已就绪）。

## 6. 下一步

| 目标 | 途径 |
|---|---|
| **性能数字**（唯一真机才能给的） | RK3588/RK3568 开发板刷 OpenHarmony，或开发者模式鸿蒙手机：推同一套 arm64 二进制 + 模型，跑同参数 A/B |
| 真鸿蒙系统环境（不只用户态） | 自建官方 `qemu-arm64-linux-min` 镜像（`vendor_ohemu`）在 QEMU 全系统下跑（TCG，仍只验功能） |
| 交付形态 | HAP + NAPI 封装（arm64 静态库已就绪，可直接被 NAPI 调用） |
