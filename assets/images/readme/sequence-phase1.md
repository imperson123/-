# 设计稿 · 交互 / 时序

**Phase 1 · LUT-SA** · 版本 v1.0 · 2026-07-23

源文件（draw.io XML）：`assets/images/readme/sequence-phase1.drawio`
渲染（PNG，导出后放回本目录同名 `.png`）：`assets/images/readme/sequence-phase1.png`

> draw.io 在线编辑：复制 `sequence-phase1.drawio` 内容到 https://app.diagrams.net 即可继续编辑。

## 视图说明

- 配色：与架构图保持一致的单色灰阶，不混色。
- 角色：Demo App、SAMgr、LUT SA、Native Lib、LUT Kernel 五条 lifeline，按主调用链从左到右排布。
- 主调用链编号 1–5：CreateSession → InitBuffers/LoadLUT → Generate → InferTokenBatch → mpGEMM/LUT lookup。
- TTFT 区间在右侧用双向开口箭头标出，覆盖 CreateSession 完成 → 首 token 返回。
- 边线：实线 = 调用；虚线 = 返回/异步；双向开口 = 指标区间。

## 时序要点

1. App 向 SAMgr 拿 LUT SA 的 proxy。
2. App → LUT SA：CreateSession(model_id, bits)，SA 触发 InitBuffers/LoadLUT 到 Native。
3. App → LUT SA：Generate(prompt, opts)；SA 内部按调度策略调整优先级。
4. SA → Native → LUT Kernel：InferTokenBatch + 比特级 mpGEMM / LUT lookup。
5. 首 token 沿原路异步返回 App；后续 token 流式回传，计时埋点用于报告。

## 出图与导出

| 步 | 动作 |
|----|------|
| 1 | 用 draw.io Desktop 或 https://app.diagrams.net 打开 `.drawio` |
| 2 | File → Export as → PNG，分辨率选 2× |
| 3 | 落盘同名 `.png` 到本目录 |