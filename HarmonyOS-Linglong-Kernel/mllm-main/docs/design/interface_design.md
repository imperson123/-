# 接口设计文档

## 1. ArkTS API 接口定义

### 1.1 OHLightLLM 类

```typescript
export class OHLightLLM {
    // 初始化引擎
    static async initEngine(): Promise<void>;
    
    // 加载模型
    static async loadModel(config: InferenceConfig): Promise<void>;
    
    // 文本推理
    static async inferenceText(input: string): Promise<string>;
    
    // 图像推理
    static async inferenceImage(imagePath: string): Promise<string>;
    
    // 获取性能指标
    static getPerformanceMetrics(): PerformanceMetrics;
}
```

### 1.2 配置接口

```typescript
export interface InferenceConfig {
    modelPath: string;
    deviceType: 'CPU' | 'NPU' | 'GPU';
    optimizationLevel: 'LOW' | 'MEDIUM' | 'HIGH';
}
```

## 2. Native API 接口定义

### 2.1 核心接口

```cpp
// 初始化引擎
bool ohlighthlm_init_engine();

// 加载模型
bool ohlighthlm_load_model(const char* model_path);

// 推理
const char* ohlighthlm_inference(const char* input);
```

---

*最后更新：2025-01-XX*



