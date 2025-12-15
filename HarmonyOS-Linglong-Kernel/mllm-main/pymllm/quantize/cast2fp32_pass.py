# Copyright (c) MLLM Team.
# Licensed under the MIT License.
from typing import Dict
from .quantize_pass import QuantizeBasePass, QuantizePlanPayload
from ..ffi import (
    Tensor,
    MLLM_FIND_NUMPY_AVAILABLE,
    MLLM_FIND_TORCH_AVAILABLE,
)

if MLLM_FIND_TORCH_AVAILABLE:
    import torch
if MLLM_FIND_NUMPY_AVAILABLE:
    import numpy as np
from ..ffi import MLLM_FIND_MINDSPORE_AVAILABLE

if MLLM_FIND_MINDSPORE_AVAILABLE:
    try:
        import mindspore as ms
    except Exception:
        import importlib
        if importlib.util.find_spec("mindspore_lite") is not None:
            import mindspore_lite as ms
        else:
            ms = None


class Cast2Fp32QuantizePass(QuantizeBasePass):
    def __init__(self):
        super().__init__()

    def prepare(
        self, quantize_config, tensor_dict: Dict, **kwargs
    ) -> QuantizePlanPayload:
        assert len(tensor_dict) == 1
        return QuantizePlanPayload(
            1,
            1,
            {
                tensor_dict.keys()[0]: tensor_dict[tensor_dict.keys()[0]],
            },
            {
                tensor_dict.keys()[0],
                None,
            },
        )

    def match(self, quantize_config, tensor_dict: Dict, **kwargs) -> bool:
        if quantize_config["quant_method"] != "cast_2_fp32":
            return False
        ret = False
        for k, v in tensor_dict.items():
            if MLLM_FIND_TORCH_AVAILABLE and isinstance(v, torch.Tensor):
                if v.dtype is not torch.float32:
                    ret = True
            if MLLM_FIND_MINDSPORE_AVAILABLE and ms is not None:
                try:
                    if isinstance(v, ms.Tensor):
                        # try to get dtype name if available
                        if getattr(v, "dtype", None) is not None:
                            # compare via str repr or use asnumpy
                            try:
                                if v.dtype != ms.float32:
                                    ret = True
                            except Exception:
                                ret = True
                except Exception:
                    pass
            if MLLM_FIND_NUMPY_AVAILABLE and isinstance(v, np.ndarray):
                if v.dtype is not np.float32:
                    ret = True
        return ret

    def run(self, quantize_config, tensor_dict: Dict, **kwargs) -> Dict:
        name = tensor_dict.keys()[0]
        weight = tensor_dict[name]
        if MLLM_FIND_TORCH_AVAILABLE and isinstance(weight, torch.Tensor):
            weight = weight.to(torch.float32)
        if MLLM_FIND_MINDSPORE_AVAILABLE and ms is not None and isinstance(weight, getattr(ms, "Tensor", (object,))):
            try:
                weight = weight.astype(ms.float32)
            except Exception:
                try:
                    if MLLM_FIND_NUMPY_AVAILABLE:
                        weight = weight.asnumpy().astype(np.float32)
                    else:
                        weight = weight.asnumpy()
                except Exception:
                    pass
        if MLLM_FIND_NUMPY_AVAILABLE and isinstance(weight, np.ndarray):
            weight = weight.astype(np.float32)
        return {name: weight}
