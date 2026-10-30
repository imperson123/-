# Deploy + run the aarch64 T-MAC build on a real OHOS device (board or phone).
#
#   powershell -File ohos\scripts\deploy_arm64_device.ps1                    # T-MAC only
#   powershell -File ohos\scripts\deploy_arm64_device.ps1 -WithSimdBaseline  # + same-model SIMD A/B
#
# Prereq: device in developer mode, USB connected, `hdc list targets` non-empty.
#
# Artifacts — resolved automatically from the WSL build dirs (see README/ARM64-VALIDATION.md),
# or override with -LlamaCli / -LlamaCliSimd / -Model / -ModelQ4:
#   llama-cli-aarch64-tmac  ~/llama-arm64/llama.cpp/build-ohos-arm64-82/bin/llama-cli
#   llama-cli-aarch64-simd  ~/llama-arm64/llama.cpp/build-ohos-arm64-simd/bin/llama-cli
#   bitnet-3b-tmac-arm64.gguf   D:\ohos-models\bitnet-3b-tmac-arm64.gguf   (966 MB, arm64 layout)
#   bitnet-3b-q4_0.gguf         D:\ohos-models\bitnet-3b-q4_0.gguf         (1.83 GB, SIMD baseline)
#
# Notes:
#  * The T-MAC-enabled binary CANNOT load standard Q4_0 models (fork aborts on a missing bits=4 kcfg),
#    so the A/B uses the plain (GGML_TMAC=OFF) build for Q4_0 — same fork, same flags otherwise.
#  * Both arm64 binaries are built for armv8.2a+fp16 (clean instruction set, verified with
#    llvm-objdump). The probe below is the gate: if it fails, do NOT run the binaries.
param(
    [string]$LlamaCli     = "",
    [string]$LlamaCliSimd = "",
    [string]$Model        = "D:\ohos-models\bitnet-3b-tmac-arm64.gguf",
    [string]$ModelQ4      = "D:\ohos-models\bitnet-3b-q4_0.gguf",
    [int]$Threads         = 4,
    [int]$NPredict        = 16,
    [string]$Prompt       = "The capital of France is",
    [switch]$WithSimdBaseline
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "env.ps1")

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Tmp = "/data/local/tmp"
$localTmp = Join-Path $env:TEMP "tmac-deploy"
New-Item -ItemType Directory -Force -Path $localTmp | Out-Null

function Resolve-Artifact([string]$path, [string]$wslFallback, [string]$name) {
    if ($path -and (Test-Path $path)) { return (Resolve-Path $path).Path }
    $distro = (wsl -l -q 2>$null | Where-Object { $_.Trim() } | Select-Object -First 1)
    if ($distro) {
        $distro = $distro.Trim()
        foreach ($prefix in @("\\wsl.localhost\$distro", "\\wsl$\$distro")) {
            $unc = $prefix + ($wslFallback -replace '/', '\')
            if (Test-Path $unc) {
                $dst = Join-Path $localTmp (Split-Path $unc -Leaf)
                if (-not (Test-Path $dst) -or (Get-Item $unc).Length -ne (Get-Item $dst).Length) {
                    Write-Host "  copying from WSL: $unc"
                    Copy-Item $unc $dst -Force
                }
                return $dst
            }
        }
    }
    throw "$name not found: '$path' and no WSL fallback ($wslFallback). Build it first (see ohos/ARM64-VALIDATION.md)."
}

Write-Host "== [1/5] device check =="
$targets = (& hdc list targets 2>&1 | Out-String).Trim()
if ($targets -match "\[Empty\]" -or [string]::IsNullOrWhiteSpace($targets)) {
    throw "No device connected (hdc list targets is empty). Enable developer mode / USB debugging."
}
Write-Host "  target(s): $targets"

Write-Host "== [2/5] CPU probe (gate) =="
$clang = Join-Path (Join-Path $env:OHOS_SDK_NATIVE "llvm\bin") "clang.exe"
$probeLocal = Join-Path $localTmp "arm64_cpu_probe"
& $clang --target=aarch64-linux-ohos -O2 -static -march=armv8.2a+fp16 `
    (Join-Path $RepoRoot "ohos\selftest\arm64_cpu_probe.c") -o $probeLocal
if ($LASTEXITCODE -ne 0) { throw "probe build failed" }
& hdc file send $probeLocal "$Tmp/arm64_cpu_probe" | Out-Null
& hdc shell "chmod 755 $Tmp/arm64_cpu_probe" | Out-Null
$probeOut = (& hdc shell "$Tmp/arm64_cpu_probe" 2>&1 | Out-String)
Write-Host $probeOut
if ($LASTEXITCODE -ne 0) {
    throw "PROBE FAILED — this device lacks NEON/fp16, do NOT run the armv8.2a+fp16 binaries."
}

Write-Host "== [3/5] push T-MAC binary + model =="
$cli = Resolve-Artifact $LlamaCli "~/llama-arm64/llama.cpp/build-ohos-arm64-82/bin/llama-cli" "llama-cli (T-MAC)"
& hdc file send $cli "$Tmp/llama-cli-aarch64-tmac" | Out-Null
& hdc shell "chmod 755 $Tmp/llama-cli-aarch64-tmac" | Out-Null
if (-not (Test-Path $Model)) { throw "model not found: $Model" }
$has = (& hdc shell "test -f $Tmp/bitnet-3b-tmac-arm64.gguf && echo yes || echo no" 2>&1 | Out-String).Trim()
if ($has -notmatch "yes") {
    Write-Host "  pushing model ($([math]::Round((Get-Item $Model).Length / 1MB)) MB, once)..."
    & hdc file send $Model "$Tmp/" | Out-Null
} else {
    Write-Host "  model already on device, skipping push"
}

Write-Host "== [4/5] T-MAC inference =="
$cmd = "cd $Tmp && ./llama-cli-aarch64-tmac -m bitnet-3b-tmac-arm64.gguf --no-mmap -n $NPredict -t $Threads -c 512 -s 42 -p '$Prompt'"
Write-Host "  $cmd"
$tmacOut = (& hdc shell $cmd 2>&1 | Out-String)
Write-Host $tmacOut

if ($WithSimdBaseline) {
    Write-Host "== [5/5] SIMD baseline (Q4_0) for the same-model A/B =="
    $cliSimd = Resolve-Artifact $LlamaCliSimd "~/llama-arm64/llama.cpp/build-ohos-arm64-simd/bin/llama-cli" "llama-cli (SIMD)"
    & hdc file send $cliSimd "$Tmp/llama-cli-aarch64-simd" | Out-Null
    & hdc shell "chmod 755 $Tmp/llama-cli-aarch64-simd" | Out-Null
    if (-not (Test-Path $ModelQ4)) { throw "Q4_0 model not found: $ModelQ4" }
    $has4 = (& hdc shell "test -f $Tmp/bitnet-3b-q4_0.gguf && echo yes || echo no" 2>&1 | Out-String).Trim()
    if ($has4 -notmatch "yes") {
        Write-Host "  pushing Q4_0 model ($([math]::Round((Get-Item $ModelQ4).Length / 1MB)) MB, once)..."
        & hdc file send $ModelQ4 "$Tmp/" | Out-Null
    }
    $cmd2 = "cd $Tmp && ./llama-cli-aarch64-simd -m bitnet-3b-q4_0.gguf --no-mmap -n $NPredict -t $Threads -c 512 -s 42 -p '$Prompt'"
    Write-Host "  $cmd2"
    & hdc shell $cmd2
} else {
    Write-Host "== [5/5] skipped (pass -WithSimdBaseline for the A/B) =="
}

Write-Host ""
Write-Host "=== summary (grep the output above for 'eval time') ==="
Write-Host "  T-MAC : see 'eval time = ... tokens per second' above"
if ($WithSimdBaseline) { Write-Host "  SIMD  : ditto — compare the two 'eval time' lines (same model/prompt/threads)" }
