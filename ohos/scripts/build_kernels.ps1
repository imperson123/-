# P1: build libtmac_kernels.a + selftest + sa stub for aarch64-ohos
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "env.ps1")

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$OhosNative = $env:OHOS_SDK_NATIVE
$Toolchain = Join-Path $OhosNative "build\cmake\ohos.toolchain.cmake"
$SrcDir = Join-Path $RepoRoot "ohos"
$BuildDir = Join-Path $SrcDir "build"

if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Push-Location $BuildDir
try {
    cmake -G Ninja `
        -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
        -DOHOS_ARCH=arm64-v8a `
        -DOHOS_STL=c++_shared `
        -DCMAKE_BUILD_TYPE=Release `
        -DT_MAC_ROOT="$RepoRoot" `
        "$SrcDir"
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
}
finally {
    Pop-Location
}

$Lib = Join-Path $BuildDir "libtmac_kernels.a"
$Nm = Join-Path $OhosNative "llvm\bin\llvm-nm.exe"
Write-Host "Library: $Lib"
if (Test-Path $Nm) {
    Write-Host "=== undefined symbols (filter TVM/pthread) ==="
    & $Nm -u $Lib 2>&1 | Select-String -Pattern "TVM|pthread|tvm" -CaseSensitive:$false
    Write-Host "(empty filter output means no TVM/pthread undefs matched)"
}

Write-Host "Selftest binary: $(Join-Path $BuildDir 'tmac_selftest')"
Write-Host "Push: hdc file send .../tmac_selftest /data/local/tmp/ && hdc shell /data/local/tmp/tmac_selftest"
