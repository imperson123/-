# ArkTS API 文档

## 1. 完整 API 列表

### OHLightLLM 类

#### initEngine()
初始化推理引擎。

**参数**：无

**返回值**：`Promise<void>`

**示例**：
```typescript
await OHLightLLM.initEngine();
```

#### loadModel(config)
加载模型。

**参数**：
- `config: InferenceConfig` - 模型配置

**返回值**：`Promise<void>`

**示例**：
```typescript
await OHLightLLM.loadModel({
    modelPath: '/data/models/qwen3.mllm',
    deviceType: 'NPU',
    optimizationLevel: 'HIGH'
});
```

---

*最后更新：2025-01-XX*



