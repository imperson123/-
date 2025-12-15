# 类图设计文档

## 1. 核心类关系图

### 1.1 整体类图

```
┌─────────────────────────────────────────┐
│         OHLightLLM (ArkTS)              │
└──────────────┬──────────────────────────┘
               │ uses
               ▼
┌─────────────────────────────────────────┐
│      InferenceEngine (C++)              │
│  - init()                                │
│  - loadModel()                           │
│  - inference()                          │
└──────────────┬──────────────────────────┘
               │ uses
               ▼
┌─────────────────────────────────────────┐
│      BackendManager (C++)               │
│  - registerBackend()                    │
│  - selectBackend()                      │
└──────┬──────────────┬───────────────────┘
       │              │
       ▼              ▼
┌──────────────┐  ┌──────────────┐
│ HiAIBackend │  │ NNRuntimeBackend│
└──────────────┘  └──────────────┘
```

### 1.2 详细类定义

#### InferenceEngine
```cpp
class InferenceEngine {
public:
    bool init();
    bool loadModel(const std::string& path);
    Tensor inference(const Tensor& input);
    
private:
    BackendManager* backend_manager_;
    Model* model_;
};
```

#### BackendManager
```cpp
class BackendManager {
public:
    void registerBackend(Backend* backend);
    Backend* selectBackend(Operator* op);
    
private:
    std::vector<Backend*> backends_;
};
```

#### HiAIBackend
```cpp
class HiAIBackend : public Backend {
public:
    bool init() override;
    Tensor execute(const Tensor& input) override;
    
private:
    void* hiai_engine_;
    void* hiai_model_;
};
```

---

*最后更新：2025-01-XX*



