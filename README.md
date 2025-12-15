鸿蒙玲珑核 — 基于 MLLM 引擎与 T-MAC 的手机端 AI 推理系统

项目简介
鸿蒙玲珑核是一个面向鸿蒙 5.0+ 设备（华为 P70 / Dayu200）的端侧推理方案，聚焦多模态大模型（MLLM）的轻量化与落地。项目采用 MindSpore Lite 作为端侧推理引擎，结合自研 T-MAC 执行框架，实现从模型量化、转换到鸿蒙应用集成与运维的全链路闭环。

目录结构（顶层）
- `Harmonyos-Application/`：鸿蒙应用前端（ArkTS/接口适配层）。
- `HarmonyOS-intelligent-operation/`：鸿蒙智能运维与可视化面板。
- `HarmonyOS-Linglong-Kernel/`：核心推理与量化代码（含 `mllm-main``t-mac`、量化脚本、C++/Python 推理逻辑）。

总体架构
- 端上前端：`Harmonyos-Application` 提供 UI 与推理调用入口，面向鸿蒙原生接口。
- 端上运维：`HarmonyOS-intelligent-operation` 负责模型/算子状态可视化、日志与异常告警。
- 量化与转换：在 Ubuntu 或华为云侧完成模型蒸馏与量化，生成 MindSpore Lite `.ms` 模型。
- 推理内核：`HarmonyOS-Linglong-Kernel` 集成 MLLM 引擎与 T-MAC，调用 MindSpore Lite runtime 完成推理。
- 设备部署：将量化模型与可执行包下发到鸿蒙 5.0+ 设备，运行端侧 AI。

环境准备
- 主机：Ubuntu 22.04+/华为云环境，用于量化与模型转换；需 Python 3.8+、CMake、MindSpore（训练/量化工具）。
- 端侧：HarmonyOS 5.0+（P70 或 Dayu200），DevEco Studio / 鸿蒙 NDK，MindSpore Lite runtime。
- 外部依赖：
  - MindSpore Lite 工具与 runtime（参考官网 https://www.mindspore.cn/ 下载对应架构包）。
  - 编译链：`clang`/`gn`（随鸿蒙 NDK），`cmake`/`ninja`（如在 C++ 侧集成）。

典型工作流
1) 训练/蒸馏：获得基础 MLLM 模型（如 MindIR/ONNX）。
2) 量化（Ubuntu/云侧）：
   - 对权重/激活做 INT8 或混合精度量化，产出中间模型。
3) 转换为 MindSpore Lite：
   - 使用 `converter_lite` 将模型转为 `.ms`，可附加 `--quantType=WeightQuant` / `FullQuant`。
4) 集成到端侧：
   - 将 `.ms` 模型与推理动态库（或静态库）拷贝到 `HarmonyOS-Linglong-Kernel`，通过 C++/ArkTS 封装给前端。
5) 前端/运维联调：
   - `Harmonyos-Application` 触发推理，`HarmonyOS-intelligent-operation` 观测运行状态与指标。
6) 部署：
   - 通过 DevEco/ADB 将应用与模型下发到 P70/Dayu200，实机验证时延与功耗。


集成要点：
- 在 HarmonyOS CMake 里链接 MindSpore Lite 提供的头文件与静态/动态库。
- 若使用 NPU/GPU，请按设备芯片选择对应 runtime 包与 `delegate` 配置。
- ArkTS 层通过 NAPI/JS 接口调用上述 C++ 封装函数，完成 UI 触发推理。

构建与运行（示意）
- 量化/转换：在 Ubuntu/云环境执行上述脚本，生成 `.ms` 模型。
- 端侧编译：在 `HarmonyOS-Linglong-Kernel` 中配置 CMake，链接 MindSpore Lite，生成 so/har。
- 前端打包：在 `Harmonyos-Application` 用 DevEco 打包，携带模型与 so。
- 部署：`hdc install xxx.hap` 或 DevEco 一键部署到 P70/Dayu200。

测试与验证
- 功能：前端推理调用正常返回、UI 展示推理结果。
- 性能：端侧冷/热启动延迟、单帧/单次推理耗时、峰值/平均功耗。
- 稳定性：长时间循环推理无崩溃/内存泄漏，异常输入的降级策略。
- 运维：`HarmonyOS-intelligent-operation` 能收集日志与告警，并可视化吞吐、延迟与温升。

参考
- MindSpore 官网：https://www.mindspore.cn/ （含 Lite 工具链与文档）
- HarmonyOS SDK/NDK & DevEco Studio 文档
- 项目内 `HarmonyOS-Linglong-Kernel/mllm-main` 量化与推理示例代码

