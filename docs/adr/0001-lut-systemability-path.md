# ADR 0001: LUT kernel + SystemAbility 路径

- Status: Accepted
- Date: 2026-07-22
- Scope: T-MAC → SystemAbility 主线（无独立 theme 名）

## Context

方向四要求系统级 AI 效率与可演示落地。初赛窗口极短。已有 T-MAC 开源 LUT 范式与评测数据。

## Decision

1. **主计算引擎**：二开本仓 T-MAC LUT kernel，不从零实现 mpGEMM。
2. **系统落点**：用户态 SystemAbility（或等价系统服务）+ NDK Native 库；不修改 OS 内核。
3. **创新叙事**：LUT 计算范式系统服务化 + 轻量行为/资源调度；复赛再考虑软总线/NPU 对照。
4. **基线**：llama.cpp（及公开 T-MAC 数据）用于测试报告。

## Consequences

- 工程焦点转为移植、封装、调度与测评，而非算法从零发明
- 需补齐 OH/Harmony SA 与 DevEco Native 技能
- 评审可能质疑「套壳」——文档必须讲清系统层接口、调度闭环与可复现数据
