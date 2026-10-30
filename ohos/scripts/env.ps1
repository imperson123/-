# OHOS NDK env — auto-detects the SDK native dir
# Usage: . .\ohos\scripts\env.ps1

$ErrorActionPreference = "Stop"

function Find-OhosSdkNative {
    $candidates = @()
    if ($env:OHOS_SDK_NATIVE) { $candidates += $env:OHOS_SDK_NATIVE }
    $candidates += @(
        "D:\ohos-sdk\ohos-sdk\windows\native",                            # standalone 6.1-LTS SDK
        "D:\DevEco Studio\sdk\default\openharmony\native",                # DevEco bundled (this machine)
        "D:\dev_software\DevEco Studio\sdk\default\openharmony\native",   # DevEco bundled (teammate machine)
        (Join-Path $env:LOCALAPPDATA "OpenHarmony\Sdk\20\native")         # DevEco bundled default
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path (Join-Path $c "build\cmake\ohos.toolchain.cmake"))) { return $c }
    }
    return $null
}

$native = Find-OhosSdkNative
if (-not $native) {
    throw "OpenHarmony SDK native dir not found. Set `$env:OHOS_SDK_NATIVE or install the SDK."
}
$env:OHOS_SDK_NATIVE = $native

# toolchains/ is a sibling of native/ in both standalone and DevEco SDK layouts
$Toolchains = Join-Path (Split-Path $native -Parent) "toolchains"
$LlvmBin = Join-Path $native "llvm\bin"
$env:OHOS_NDK_CC = Join-Path $LlvmBin "clang++.exe"

$BuildTools = Join-Path $native "build-tools\cmake\bin"   # bundled cmake + ninja
$prepend = @($LlvmBin)
if (Test-Path (Join-Path $BuildTools "ninja.exe")) { $prepend = @($BuildTools) + $prepend }
if (Test-Path (Join-Path $Toolchains "hdc.exe")) { $prepend = @($Toolchains) + $prepend }
$env:PATH = ($prepend -join ";") + ";" + $env:PATH

Write-Host "OHOS_SDK_NATIVE=$($env:OHOS_SDK_NATIVE)"
Write-Host "OHOS_NDK_CC=$($env:OHOS_NDK_CC)"
Write-Host "hdc=$(Get-Command hdc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)"
& (Join-Path $LlvmBin "clang++.exe") --version | Select-Object -First 2
