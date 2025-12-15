# NN Runtime 集成文档

## 1. NN Runtime API 使用

### 1.1 NN Runtime 概述

NN Runtime（Neural Network Runtime）是 OpenHarmony 提供的统一神经网络推理框架，支持多种硬件后端（CPU、GPU、NPU），提供统一的 API 接口。

### 1.2 核心 API

#### 1.2.1 初始化 API

```cpp
// 创建 NN Runtime 会话
int nnrt_create_session(NNRTSession** session);

// 配置会话
int nnrt_set_config(NNRTSession* session, 
                    const NNRTSessionConfig* config);

// 销毁会话
int nnrt_destroy_session(NNRTSession* session);
```

#### 1.2.2 模型加载 API

```cpp
// 加载模型
int nnrt_load_model(NNRTSession* session,
                   const char* model_path);

// 从内存加载模型
int nnrt_load_model_from_memory(NNRTSession* session,
                                const void* model_data,
                                size_t model_size);
```

#### 1.2.3 推理执行 API

```cpp
// 准备输入
int nnrt_set_input(NNRTSession* session,
                  uint32_t input_index,
                  const NNRTTensor* tensor);

// 执行推理
int nnrt_run(NNRTSession* session);

// 获取输出
int nnrt_get_output(NNRTSession* session,
                   uint32_t output_index,
                   NNRTTensor* tensor);
```

## 2. 计算图构建流程

### 2.1 流程图

```
MLLM IR Graph
    │
    │ Parse Operators
    ▼
NN Runtime Operators
    │
    │ Build Graph
    ▼
NN Runtime Graph
    │
    │ Optimize
    ▼
Optimized Graph
    │
    │ Compile
    ▼
Executable Graph
```

### 2.2 实现步骤

#### Step 1: IR 解析
```cpp
auto ir_graph = loadIRGraph("model.mllm");
```

#### Step 2: 算子映射
```cpp
std::map<std::string, NNRT_OP_TYPE> op_mapping = {
    {"Linear", NNRT_OP_FULLY_CONNECTED},
    {"Conv2d", NNRT_OP_CONV2D},
    {"ReLU", NNRT_OP_RELU},
    // ...
};

for (auto& op : ir_graph.operators()) {
    auto nnrt_op_type = mapOperatorType(op.type(), op_mapping);
    // 创建 NN Runtime 算子
    createNNRTOperator(nnrt_op_type, op);
}
```

#### Step 3: 图构建
```cpp
NNRTGraph* graph = nnrt_create_graph();

// 添加输入节点
for (auto& input : ir_graph.inputs()) {
    nnrt_add_input_node(graph, input.name(), input.shape());
}

// 添加算子节点
for (auto& op : operators) {
    nnrt_add_operator_node(graph, op);
}

// 添加输出节点
for (auto& output : ir_graph.outputs()) {
    nnrt_add_output_node(graph, output.name());
}
```

#### Step 4: 图优化
```cpp
// 算子融合
nnrt_fuse_operators(graph);

// 常量折叠
nnrt_fold_constants(graph);

// 图剪枝
nnrt_prune_graph(graph);
```

#### Step 5: 编译
```cpp
NNRTCompileOptions options = {
    .optimization_level = NNRT_OPT_HIGH,
    .target_device = NNRT_DEVICE_AUTO
};

int ret = nnrt_compile_graph(graph, &options);
```

## 3. 混合执行策略

### 3.1 策略概述

混合执行策略根据算子特性和硬件能力，将计算图分割为多个子图，分别在不同的设备上执行。

### 3.2 执行流程图

```
Input Tensor
    │
    ▼
Graph Partition
    │
    ├─► Subgraph 1 (CPU) ──► Intermediate Result 1
    │
    ├─► Subgraph 2 (NPU) ──► Intermediate Result 2
    │
    └─► Subgraph 3 (CPU) ──► Intermediate Result 3
    │
    ▼
Result Merge
    │
    ▼
Output Tensor
```

### 3.3 实现代码

```cpp
class HybridExecutor {
public:
    struct Subgraph {
        DeviceType device;
        std::vector<Operator*> operators;
    };
    
    std::vector<Tensor> execute(const Tensor& input) {
        // 1. 图分割
        auto subgraphs = partitionGraph(graph_);
        
        // 2. 执行子图
        std::vector<Tensor> intermediate_results;
        Tensor current_input = input;
        
        for (auto& subgraph : subgraphs) {
            // 选择执行设备
            auto executor = getExecutor(subgraph.device);
            
            // 执行子图
            auto result = executor->execute(current_input, subgraph);
            intermediate_results.push_back(result);
            current_input = result;
        }
        
        return intermediate_results;
    }
    
private:
    std::vector<Subgraph> partitionGraph(Graph* graph) {
        // 分割策略：
        // 1. Attention 相关算子 → NPU
        // 2. 其他算子 → CPU
        std::vector<Subgraph> subgraphs;
        
        Subgraph npu_subgraph;
        Subgraph cpu_subgraph;
        
        for (auto& op : graph->operators()) {
            if (isAttentionOp(op)) {
                npu_subgraph.operators.push_back(&op);
                npu_subgraph.device = DeviceType::NPU;
            } else {
                cpu_subgraph.operators.push_back(&op);
                cpu_subgraph.device = DeviceType::CPU;
            }
        }
        
        if (!npu_subgraph.operators.empty()) {
            subgraphs.push_back(npu_subgraph);
        }
        if (!cpu_subgraph.operators.empty()) {
            subgraphs.push_back(cpu_subgraph);
        }
        
        return subgraphs;
    }
};
```

### 3.4 设备选择算法

```cpp
DeviceType selectDevice(Operator* op) {
    // 规则 1: 支持 NPU 的算子优先使用 NPU
    if (isNPUSupported(op)) {
        return DeviceType::NPU;
    }
    
    // 规则 2: 计算密集型算子使用 NPU
    if (isComputeIntensive(op)) {
        return DeviceType::NPU;
    }
    
    // 规则 3: 其他算子使用 CPU
    return DeviceType::CPU;
}
```

## 4. 性能分析

### 4.1 Profiling API

```cpp
// 启用性能分析
NNRTProfilingOptions prof_options = {
    .enable_profiling = true,
    .profile_level = NNRT_PROFILE_DETAILED
};

nnrt_set_profiling_options(session, &prof_options);

// 执行推理
nnrt_run(session);

// 获取性能数据
NNRTProfilingResult* result = nullptr;
nnrt_get_profiling_result(session, &result);

// 打印性能数据
for (int i = 0; i < result->num_operators; i++) {
    printf("Operator %s: %.2f ms\n", 
           result->operators[i].name,
           result->operators[i].execution_time_ms);
}
```

### 4.2 性能指标

| 指标 | 说明 |
|-----|------|
| 总执行时间 | 整个推理过程耗时 |
| 算子执行时间 | 每个算子的执行时间 |
| 内存使用 | 峰值内存占用 |
| 设备利用率 | NPU/CPU 利用率 |

### 4.3 性能优化建议

1. **算子融合**：减少算子数量，降低调度开销
2. **批处理**：增加 batch size 提高吞吐量
3. **异步执行**：重叠计算和数据传输
4. **内存优化**：减少内存拷贝，使用内存池

---

*最后更新：2025-01-XX*



