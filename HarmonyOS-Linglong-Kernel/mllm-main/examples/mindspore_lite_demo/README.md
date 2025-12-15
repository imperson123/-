# MindSpore Lite 推理示例（结合 T-MAC / MLLM）

## 作用
- 演示如何在端侧使用 MindSpore Lite runtime 加载 `.ms` 量化模型。
- 示例中通过一个极简的 `TMacPipeline` 结构模拟 T-MAC 调度，便于后续与 `mllm` 主干对接。

## 依赖
- MindSpore Lite runtime 与 `include` 头文件（从官方发布包中获取）。
- C++17 编译器、CMake。
- 已经完成量化并转换好的模型文件，例如 `models/demo_int8.ms`。

## 构建（示意）
```bash
cd HarmonyOS-Linglong-Kernel/mllm-main/examples/mindspore_lite_demo

cmake -B build -DMSLITE_ROOT=/path/to/mindspore-lite
cmake --build build -j
```

`MSLITE_ROOT` 需包含：
- `include/`：MindSpore Lite 头文件
- `lib/`：`mindspore-lite.a` 或对应动态库

## 运行
```bash
./build/mindspore_lite_demo \
  --model ./models/demo_int8.ms \
  --input ./data/input.bin
```

输出的 logits/embedding 将打印到日志，并可在后续接入 ArkTS / NAPI 进行 UI 呈现。

