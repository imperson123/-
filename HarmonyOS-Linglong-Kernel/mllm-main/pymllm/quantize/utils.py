"""Utilities for quantize module: tensor normalization helpers.

Provides normalize_tensor_dict which converts tensors from torch/numpy/mindspore
into internal `mllm.Tensor` via FFI helpers (prefer DLPack when available).
"""
from typing import Dict
import importlib

from ..ffi import (
    Tensor,
    from_numpy,
    from_torch,
    from_mindspore,
    MLLM_FIND_NUMPY_AVAILABLE,
    MLLM_FIND_TORCH_AVAILABLE,
    MLLM_FIND_MINDSPORE_AVAILABLE,
)


def normalize_tensor_dict(tensor_dict: Dict) -> Dict:
    """Normalize mapping name->tensor to mllm.Tensor where possible.

    Order of attempts per value:
      1. If already `mllm.Tensor`, keep.
      2. If torch.Tensor -> from_torch(...)
      3. If numpy.ndarray -> from_numpy(...)
      4. If MindSpore Tensor -> from_mindspore(...)
      5. Fallback: keep original object

    The function is defensive and will not raise if optional libs are missing.
    """
    normalized: Dict = {}

    ms = None
    if MLLM_FIND_MINDSPORE_AVAILABLE:
        try:
            import mindspore as ms  # type: ignore
        except Exception:
            if importlib.util.find_spec("mindspore_lite") is not None:
                import mindspore_lite as ms  # type: ignore
            else:
                ms = None

    np = None
    if MLLM_FIND_NUMPY_AVAILABLE:
        import numpy as np  # type: ignore

    for k, v in tensor_dict.items():
        try:
            if isinstance(v, Tensor):
                normalized[k] = v
                continue
        except Exception:
            pass

        if MLLM_FIND_TORCH_AVAILABLE:
            try:
                import torch

                if isinstance(v, torch.Tensor):
                    normalized[k] = from_torch(v.to(torch.float32))
                    continue
            except Exception:
                pass

        if MLLM_FIND_NUMPY_AVAILABLE and np is not None:
            try:
                if isinstance(v, np.ndarray):
                    normalized[k] = from_numpy(v.astype(np.float32))
                    continue
            except Exception:
                pass

        if MLLM_FIND_MINDSPORE_AVAILABLE and ms is not None:
            try:
                if isinstance(v, getattr(ms, "Tensor", (object,))):
                    normalized[k] = from_mindspore(v)
                    continue
            except Exception:
                pass

        # fallback: keep original
        normalized[k] = v

    return normalized
