# Native API 设计文档

## 1. NAPI 接口设计

### 1.1 接口概览

OHLightLLM 通过 NAPI（Native API）提供 C++ 到 ArkTS 的接口桥接。

### 1.2 核心接口

#### 1.2.1 引擎初始化

```cpp
// C++ 实现
static napi_value InitEngine(napi_env env, napi_callback_info info) {
    // 创建全局引擎实例
    g_engine = std::make_unique<InferenceEngine>();
    
    if (!g_engine->init()) {
        napi_throw_error(env, nullptr, "Failed to initialize engine");
        return nullptr;
    }
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}
```

#### 1.2.2 模型加载

```cpp
static napi_value LoadModel(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    // 获取模型路径
    char model_path[256];
    size_t path_len;
    napi_get_value_string_utf8(env, argv[0], model_path, 
                               sizeof(model_path), &path_len);
    
    // 加载模型
    bool success = g_engine->loadModel(model_path);
    
    napi_value result;
    napi_get_boolean(env, success, &result);
    return result;
}
```

#### 1.2.3 推理执行

```cpp
static napi_value Inference(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    // 解析输入（文本或图像）
    std::string input = parseInput(env, argv[0]);
    
    // 执行推理
    std::string output = g_engine->inference(input);
    
    // 返回结果
    napi_value result;
    napi_create_string_utf8(env, output.c_str(), 
                            output.length(), &result);
    return result;
}
```

## 2. 数据转换规范

### 2.1 ArkTS → C++ 转换

#### 字符串转换
```cpp
std::string arktsToString(napi_env env, napi_value value) {
    size_t len;
    napi_get_value_string_utf8(env, value, nullptr, 0, &len);
    
    std::string result(len, '\0');
    napi_get_value_string_utf8(env, value, result.data(), 
                               len + 1, nullptr);
    return result;
}
```

#### 图像转换
```cpp
Tensor arktsImageToTensor(napi_env env, napi_value image) {
    // 获取 PixelMap
    napi_value pixel_map;
    napi_get_named_property(env, image, "pixelMap", &pixel_map);
    
    // 读取像素数据
    void* pixel_data = nullptr;
    size_t data_size = 0;
    // ... 读取像素数据
    
    // 转换为 Tensor
    return Tensor::fromData(pixel_data, data_size);
}
```

### 2.2 C++ → ArkTS 转换

#### 字符串转换
```cpp
napi_value stringToArkTS(napi_env env, const std::string& str) {
    napi_value result;
    napi_create_string_utf8(env, str.c_str(), 
                           str.length(), &result);
    return result;
}
```

#### 对象转换
```cpp
napi_value createResultObject(napi_env env, 
                              const InferenceResult& result) {
    napi_value obj;
    napi_create_object(env, &obj);
    
    // 添加属性
    napi_value text;
    napi_create_string_utf8(env, result.text.c_str(), 
                           result.text.length(), &text);
    napi_set_named_property(env, obj, "text", text);
    
    napi_value confidence;
    napi_create_double(env, result.confidence, &confidence);
    napi_set_named_property(env, obj, "confidence", confidence);
    
    return obj;
}
```

## 3. 内存管理策略

### 3.1 内存生命周期

```
ArkTS Object
    │
    │ NAPI Call
    ▼
C++ Object (RAII)
    │
    │ Auto Destroy
    ▼
Memory Freed
```

### 3.2 RAII 管理

```cpp
class NAPITensor {
public:
    NAPITensor(napi_env env, napi_value value) 
        : env_(env), value_(value) {
        // 引用计数 +1
        napi_reference_ref(env_, value_, &ref_);
    }
    
    ~NAPITensor() {
        // 引用计数 -1
        napi_reference_unref(env_, ref_);
    }
    
private:
    napi_env env_;
    napi_value value_;
    napi_ref ref_;
};
```

### 3.3 内存池

```cpp
class MemoryPool {
public:
    void* allocate(size_t size) {
        // 从池中分配
        if (auto ptr = findFreeBlock(size)) {
            return ptr;
        }
        // 分配新内存
        return allocateNew(size);
    }
    
    void deallocate(void* ptr) {
        // 归还到池中
        markAsFree(ptr);
    }
};
```

## 4. 错误处理机制

### 4.1 错误码定义

```cpp
enum class ErrorCode {
    SUCCESS = 0,
    ENGINE_NOT_INITIALIZED = 1001,
    MODEL_LOAD_FAILED = 1002,
    INFERENCE_FAILED = 1003,
    INVALID_INPUT = 1004,
    // ...
};
```

### 4.2 错误处理流程

```cpp
static napi_value SafeInference(napi_env env, 
                                napi_callback_info info) {
    try {
        // 检查引擎是否初始化
        if (!g_engine) {
            throwError(env, ErrorCode::ENGINE_NOT_INITIALIZED,
                      "Engine not initialized");
            return nullptr;
        }
        
        // 执行推理
        auto result = g_engine->inference(input);
        return createResult(env, result);
        
    } catch (const std::exception& e) {
        throwError(env, ErrorCode::INFERENCE_FAILED, e.what());
        return nullptr;
    }
}
```

### 4.3 错误信息格式

```json
{
    "code": 1003,
    "message": "Inference failed",
    "details": "Model execution timeout"
}
```

---

*最后更新：2025-01-XX*



