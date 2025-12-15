# 数据流图文档

## 1. 数据流向图

### 1.1 推理数据流

```
User Input (Text/Image)
    │
    ▼
ArkTS Layer (String/PixelMap)
    │
    │ UTF-8 / PixelBuffer
    ▼
NAPI Converter
    │
    │ std::string / Tensor
    ▼
C++ Layer (Tensor)
    │
    │ MLLM Tensor Format
    ▼
Backend Layer
    │
    ├─► HiAI Tensor ──► NPU ──► Result Tensor
    │
    └─► NN RT Tensor ──► CPU/NPU ──► Result Tensor
    │
    ▼
Result Tensor
    │
    │ std::string
    ▼
NAPI Converter
    │
    │ UTF-8
    ▼
ArkTS Layer (String)
    │
    ▼
User Output (Text)
```

## 2. 内存布局

### 2.1 Tensor 内存布局

```
Tensor Memory Layout:
┌─────────────────────────────────┐
│ Header (metadata)               │
├─────────────────────────────────┤
│ Data (row-major)                │
│ [d0, d1, d2, ..., dn]          │
└─────────────────────────────────┘
```

---

*最后更新：2025-01-XX*



