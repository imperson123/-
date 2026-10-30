#!/usr/bin/env python3
"""Generate kernel-vs-reference verification data from a REAL HF BitNet weight.

Same math as T-MAC's tests/test_e2e.py, but against OUR compiled kernels.cc
(x86 GCC/clang, OHOS clang, or Android NDK clang) instead of the TVM runtime.

This harness exists because T-MAC failures are SILENT: kernels return 0 and
emit no error while producing all-zero or garbage output. The debugging
method that works: bake a known-good NumPy reference alongside the exact
binary inputs, then diff on-device output against the reference (NMSE).

Usage:
  python gen_testdata_ffn.py --hf-model /path/to/bitnet-b1_58-3B --outdir ./vdata
  # optional: --kcfg deploy/tuned/<your-kernels>/kcfg.ini  (MUST match the
  #            kernels you will run -- kcfg layout is per-artifact!)

Outputs (in --outdir):
  A.bin    quantized+preprocessed weight (layout defined by kcfg)
  B.bin    random activation (fp32, K floats)
  S.bin    per-tensor scale (1 fp32)
  Cref.bin NumPy reference output (Mrows floats) -- the ground truth
"""
import argparse, json, os, struct, sys

import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("--hf-model", required=True, help="HF safetensors dir of bitnet-b1_58-3B")
parser.add_argument("--outdir", default="./vdata")
parser.add_argument("--kcfg", default=None,
                    help="kcfg.ini matching the kernels you will test "
                         "(default: the aarch64-hf BitNet artifact -- Android phones)")
parser.add_argument("--target", default="model.layers.0.mlp.gate_proj.weight",
                    help="which HF tensor to slice (default: ffn_gate 8640x3200)")
args = parser.parse_args()

repo = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(repo, "python"))
from t_mac.model_utils import preprocess_for_t_mac  # noqa: E402

if args.kcfg is None:
    args.kcfg = os.path.join(repo, "deploy", "tuned", "aarch64-hf-bitnet-3b", "kcfg.ini")

# ---- load the real HF weight: (8640, 3200) -> m_bits=17280 ----
w = None
for fname in sorted(os.listdir(args.hf_model)):
    if not fname.endswith(".safetensors"):
        continue
    with open(os.path.join(args.hf_model, fname), "rb") as fp:
        n = struct.unpack("<Q", fp.read(8))[0]
        hdr = json.loads(fp.read(n))
        if args.target not in hdr:
            continue
        info = hdr[args.target]
        fp.seek(8 + n + info["data_offsets"][0])
        raw = fp.read(info["data_offsets"][1] - info["data_offsets"][0])
        w = np.frombuffer(raw, dtype=np.float32).reshape(info["shape"])
        break
assert w is not None, f"tensor {args.target} not found in {args.hf_model}"
print("HF tensor:", w.shape)

def weight_quant(x):
    scale = np.abs(x).mean()
    iscale = 1.0 / max(scale, 1e-5)
    return (np.round(x * iscale).clip(-1, 1) / iscale).astype(np.float32)

q = weight_quant(w)
scale = np.max(np.abs(q))
wq = np.round(q / scale + 2).astype(np.uint8)      # ternary -> {0,1,2,3}
print("scale=%.8f | ternary: 1=%d 2=%d 3=%d" % (scale, (wq == 1).sum(), (wq == 2).sum(), (wq == 3).sum()))

A_t = preprocess_for_t_mac(args.kcfg, wq, scale.reshape(1), bits=2)
print("A_t:", A_t.shape, A_t.dtype, "bytes:", A_t.nbytes)

np.random.seed(11)
Bref = np.random.randn(1, w.shape[1]).astype(np.float32)
Adq = (wq.T.astype(np.float32) - 2.0) * scale
Cref = Bref.dot(Adq)                               # (1, 8640) ground truth
print("Cref[0..4]:", np.round(Cref[0][:5], 4))

os.makedirs(args.outdir, exist_ok=True)
A_t.tofile(os.path.join(args.outdir, "A.bin"))
Bref.tofile(os.path.join(args.outdir, "B.bin"))
np.array([scale], dtype=np.float32).tofile(os.path.join(args.outdir, "S.bin"))
Cref.tofile(os.path.join(args.outdir, "Cref.bin"))
print("VERIFICATION_DATA_READY ->", args.outdir)
print("next: run run_test_dev against this dir; compare C vs Cref (NMSE < 1e-3 expected)")
