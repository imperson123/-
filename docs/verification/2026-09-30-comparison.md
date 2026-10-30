# 对比试验原始数据（2026-09-30 · 官方模拟器 Pura X View）

## 实验① 链式 KV 部分前缀复用（控制台原文节选）

    kv chain: reused 6 of 13 prompt tokens (prefix chain kept, delta decoded)
    prompt eval time = 564.23 ms / 13 tokens ( 43.40 ms per token )
    ttft = 564.73 ms
    输出：The capital of France is Paris. The capital of Spain is Madrid. / The capital of Germany is Berlin. ...

（前一步「The capital of France is」为 6 token 前缀 → 二次提问复用 6、仅解码增量 7）

## 实验② L2 感知调度 A/B（hidumper --mem 的进程 Pss）

| 状态 | Total Pss | Total Rss |
|---|---|---|
| 调度开启 · 前台（模型已载） | 1403025 kB (≈1370 MB) | 1521576 kB (≈1486 MB) |
| 调度开启 · 后台 70 s（释放后） | **72685 kB (≈71 MB)** | 190900 kB (≈186 MB) |
| 调度开启 · 回前台重载后 | 1396601 kB (≈1364 MB) | 1498368 kB (≈1463 MB) |
| 调度关闭 · 前台（对照） | 1392153 kB (≈1360 MB) | 1478388 kB (≈1443 MB) |
| **调度关闭 · 后台 70 s（对照）** | **1388116 kB (≈1356 MB)** | 1464004 kB (≈1429 MB) |
| 调度关闭 · 回前台 | 1388220 kB (≈1356 MB) | 1464112 kB (≈1429 MB) |

结论：同一设备/模型/后台时长，唯一变量为调度开关 → 释放量 **≈1.30 GB**；对照组不释放。

## 实验③ T-MAC vs Q4_0（应用内）

- 沙箱推入 bitnet-3b-q4_0.gguf（1,923,553,856 B）→ 点「加载模型」→ 进程 cppcrash
- faultlog：cppcrash-com.fandou.lutsa-20020063-20260930142602802 / 142732732 / 142934418
- 定位：T-MAC 构建（GGML_TMAC=ON，kcfg/LUT 内核）只支持自身格式，标准量化格式不在支持面内
- 处置：还原 bitnet-3b-tmac-ags64.gguf，应用复验「模型已加载」+ 正常生成 ✓
