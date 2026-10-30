# P0: cross-compile hello for aarch64-ohos with DevEco OHOS NDK
# Usage: powershell -File ohos\scripts\build_hello.ps1

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "env.ps1")

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$OhosNative = $env:OHOS_SDK_NATIVE
$Toolchain = Join-Path $OhosNative "build\cmake\ohos.toolchain.cmake"
if (-not (Test-Path $Toolchain)) {
    throw "ohos.toolchain.cmake not found: $Toolchain"
}

$SrcDir = Join-Path $RepoRoot "ohos\hello"
$BuildDir = Join-Path $SrcDir "build"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Write-Host "RepoRoot=$RepoRoot"
Write-Host "Toolchain=$Toolchain"

Push-Location $BuildDir
try {
    cmake -G Ninja `
        -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
        -DOHOS_ARCH=arm64-v8a `
        -DOHOS_STL=c++_shared `
        -DCMAKE_BUILD_TYPE=Release `
        "$SrcDir"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Ninja generator failed, retrying default generator..."
        cmake `
            -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
            -DOHOS_ARCH=arm64-v8a `
            -DOHOS_STL=c++_shared `
            -DCMAKE_BUILD_TYPE=Release `
            "$SrcDir"
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    }

    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
}
finally {
    Pop-Location
}

$Bin = $null
foreach ($c in @("hello", "hello.exe", "Release\hello", "Release\hello.exe")) {
    $p = Join-Path $BuildDir $c
    if (Test-Path $p) { $Bin = $p; break }
}
if (-not $Bin) { throw "hello binary not found under $BuildDir" }

Write-Host ""
Write-Host "Built: $Bin"
$ReadElf = Join-Path $OhosNative "llvm\bin\llvm-readobj.exe"
if (Test-Path $ReadElf) {
    & $ReadElf --file-headers $Bin 2>&1 | Select-String -Pattern "Class|Machine|OS/ABI|Type:|Format:"
}

$OutNote = Join-Path $RepoRoot "ohos\hello\BUILD_STATUS.md"
@"
# P0 hello build status

- Built: ``$Bin``
- Host time: $(Get-Date -Format o)
- Device: run ``ohos\scripts\push_hello.ps1`` when ``hdc list targets`` is non-empty
"@ | Set-Content -Encoding utf8 $OutNote

Write-Host ""
Write-Host "Next: ohos\scripts\push_hello.ps1 (requires device)"
