# 设计稿 · 系统架构

**Phase 1 · LUT-SA** · 版本 v1.0 · 2026-07-23

源文件（draw.io XML）：`assets/images/readme/architecture-phase1.drawio`
渲染（PNG，导出后放回本目录同名 `.png`）：`assets/images/readme/architecture-phase1.png`

> draw.io 在线编辑：复制 `architecture-phase1.drawio` 内容到 https://app.diagrams.net 即可继续编辑。

## 视图说明

- 配色：单色灰阶（`#1A1A1A` 主字、`#222222` 边线、`#6B6B6B` 次字、`#FAFAFA` 图例底），便于打印与黑白阅读。
- 分层：L1 应用 → L2 感知与调度 → L3 SystemAbility → L4 计算核心，垂直单向依赖。
- 边线：实线 = 主调用；细实线 = 内部连接；虚线 = 调度/控制/对照。
- 右栏「对照与验证」：与主链路并列，强调可复现与对照基线（llama.cpp → 指标 → 测试报告）。
- 底部「边界声明」：不修改 OS 内核，仅在用户态封装 LUT 内核并以 SA 暴露能力。

## 分层职责

| 层 | 职责 | 关键节点 |
|----|------|----------|
| L1 应用层 | 演示客户端 / Agent 任务入口 | 演示 App（ArkTS / Native Client） |
| L2 感知与调度 | 通知 / 传感器 / 桌面状态 → 优先级与预取策略 | 行为信号、调度策略 |
| L3 系统服务层 | 用户态 SystemAbility，SAMgr 注册，对外 IPC | LUT SystemAbility、Native 加速库 |
| L4 计算核心 | T-MAC 二开 LUT Kernel，低比特 mpGEMM | LUT Kernel、t-man 工具链 |
| 对照 | 基线与公开评测对照 → 测试报告 | llama.cpp、TTFT / 吞吐 / 能耗 |

## 出图与导出

| 步 | 动作 |
|----|------|
| 1 | 用 draw.io Desktop 或 https://app.diagrams.net 打开 `.drawio` |
| 2 | File → Export as → PNG，分辨率选 2×（1600×1040 左右） |
| 3 | 落盘同名 `.png` 到本目录 |
| 4 | 在 `docs/output/report/phase1-design.md` 中确认引用一致 |