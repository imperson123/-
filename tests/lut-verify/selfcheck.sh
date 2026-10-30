#!/bin/bash
# Harness self-check: cross-compile run_test_dev for two x64 kernel variants
# with the OHOS SDK clang, run the kernel/chunk matrix in WSL (static musl
# binaries run on any Linux kernel), and NMSE-compare with the NumPy reference.
#
# Expected outcome (2026-09-28 measured, Kali WSL / i7-14650HX):
#   f32 kernel  + chunk=64  -> NMSE 8.765e-05 PASS   <- the healthy pairing
#   f32 kernel  + chunk=128 -> FAIL                  <- chunking-convention pitfall
#   ags64 kernel + fp32 act -> garbage (1e30)        <- fp16-contract pitfall
# The two FAILs are intentional demonstrations: they ARE the classic
# "runs fine, returns 0, no output" failure modes this harness exists to catch.
#
# Env overrides: REPO, SDK, DATA
set -e
REPO=${REPO:-$(cd "$(dirname "$0")/../.." && pwd)}
SDK=${SDK:-/mnt/d/ohos-sdk/ohos-sdk/linux/native}
DATA=${DATA:-/mnt/d/ohos-models/tmactest}
CLANG="$SDK/llvm/bin/clang++"          # Linux-host SDK clang; on Windows use
SYSROOT="$SDK/sysroot"                 # windows/native/llvm/bin/clang++.exe
if [ ! -x "$CLANG" ] && [ -x "${CLANG}.exe" ]; then CLANG="${CLANG}.exe"; fi

cd "$REPO"
build() {  # build <artifact> <out>
  "$CLANG" --target=x86_64-linux-ohos --sysroot="$SYSROOT" -O2 -static \
    -mavx2 -mf16c -mfma \
    -I "deploy/tuned/$1" -I ohos/staging-x64/t-mac/include \
    tests/lut-verify/run_test_dev.cpp "deploy/tuned/$1/kernels.cc" -o "$2"
}
build ohos-x64-ags64-f32 /tmp/rt_f32
build ohos-x64-ags64     /tmp/rt_ags64

for ck in 64 128; do
  echo "=== f32 kernel, chunk=$ck ==="
  /tmp/rt_f32 "$DATA" "$ck" | grep -E "C_impl|Cref|NMSE"
done
echo "=== ags64 kernel fed fp32 activation (expected: garbage = pitfall #2 demo) ==="
/tmp/rt_ags64 "$DATA" 64 | grep -E "C_impl|NMSE"
echo "SELF CHECK DONE"
