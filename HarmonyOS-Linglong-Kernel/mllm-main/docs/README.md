# OHLightLLM 文档索引

本文档提供所有项目文档的索引和位置说明。

---

## 📁 文档目录结构

```
docs/
├── architecture/              # 架构设计文档
│   ├── overall_architecture.md        # 总体架构
│   ├── hiai_integration.md           # HiAI 集成
│   ├── nnrt_integration.md           # NN Runtime 集成
│   └── native_api_design.md          # Native API 设计
│
├── design/                    # 设计文档
│   ├── class_diagram.md              # 类图
│   ├── sequence_diagram.md           # 时序图
│   ├── interface_design.md           # 接口设计
│   └── data_flow.md                  # 数据流图
│
├── api/                       # API 文档
│   ├── arkts_api.md                  # ArkTS API
│   └── native_api.md                 # Native API
│
└── deployment/                # 部署文档
    ├── build_guide.md                # 构建指南
    └── demo_guide.md                  # Demo 使用指南
```

---

## 📚 文档列表

### 架构设计文档

#### 1. 总体架构文档
- **文件**：`docs/architecture/overall_architecture.md`
- **内容**：
  - 系统整体架构图
  - 各层职责说明
  - 数据流向图
  - 技术选型说明
  - 架构设计原则
  - 关键技术决策

#### 2. HiAI 集成文档
- **文件**：`docs/architecture/hiai_integration.md`
- **内容**：
  - HiAI 引擎架构
  - 集成方案设计
  - 模型转换流程
  - 性能优化策略
  - 代码示例
  - 性能基准测试
  - 故障排查

#### 3. NN Runtime 集成文档
- **文件**：`docs/architecture/nnrt_integration.md`
- **内容**：
  - NN Runtime API 使用
  - 计算图构建流程
  - 混合执行策略
  - 性能分析

#### 4. Native API 设计文档
- **文件**：`docs/architecture/native_api_design.md`
- **内容**：
  - NAPI 接口设计
  - 数据转换规范
  - 内存管理策略
  - 错误处理机制

---

### 设计文档

#### 5. 类图设计文档
- **文件**：`docs/design/class_diagram.md`
- **内容**：
  - 核心类关系图
  - 接口定义
  - 继承关系
  - 依赖关系

#### 6. 时序图文档
- **文件**：`docs/design/sequence_diagram.md`
- **内容**：
  - 模型加载时序图
  - 推理执行时序图
  - 错误处理时序图

#### 7. 接口设计文档
- **文件**：`docs/design/interface_design.md`
- **内容**：
  - ArkTS API 接口定义
  - Native API 接口定义
  - 参数说明
  - 返回值说明
  - 使用示例

#### 8. 数据流图文档
- **文件**：`docs/design/data_flow.md`
- **内容**：
  - 数据流向图
  - 内存布局
  - 数据格式转换

---

### API 文档

#### 9. ArkTS API 文档
- **文件**：`docs/api/arkts_api.md`
- **内容**：
  - 完整 API 列表
  - 参数说明
  - 使用示例
  - 最佳实践

#### 10. Native API 文档
- **文件**：`docs/api/native_api.md`
- **内容**：
  - 完整 API 列表
  - 参数说明
  - 使用示例
  - 最佳实践

---

### 部署文档

#### 11. 构建指南
- **文件**：`docs/deployment/build_guide.md`
- **内容**：
  - 环境要求
  - 构建步骤
  - 常见问题

#### 12. Demo 使用指南
- **文件**：`docs/deployment/demo_guide.md`
- **内容**：
  - Demo 使用说明
  - 功能演示
  - 故障排查

---

## 🗂️ 项目根目录文档

### 项目总览
- **README_OHLightLLM.md** - 项目总览和快速导航

### 项目规划
- **OHLightLLM_项目大纲.md** - 完整项目规划文档
- **OHLightLLM_开发教程.md** - 详细开发指南
- **OHLightLLM_快速开始.md** - 快速上手指南
- **OHLightLLM_团队分工.md** - 团队协作指南

### 参考资料
- **OHLightLLM_检索词指南.md** - 论文和博客检索词

---

## 📖 文档使用指南

### 对于新成员
1. 先阅读 `README_OHLightLLM.md` 了解项目
2. 阅读 `OHLightLLM_快速开始.md` 快速上手
3. 查看 `docs/architecture/overall_architecture.md` 了解架构

### 对于开发人员
1. 参考 `docs/architecture/` 下的架构文档
2. 查看 `docs/design/` 下的设计文档
3. 查阅 `docs/api/` 下的 API 文档

### 对于文档人员
1. 参考所有文档作为模板
2. 根据项目进展更新文档
3. 确保文档与代码同步

---

## 🔄 文档更新规范

1. **及时更新**：代码变更后及时更新文档
2. **版本控制**：文档与代码一起版本管理
3. **审查机制**：重要文档需要团队审查
4. **格式统一**：使用 Markdown 格式，保持风格一致

---

*最后更新：2025-01-XX*



