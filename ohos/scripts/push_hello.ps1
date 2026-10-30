# Push and run P0 hello on connected OHOS device
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "env.ps1")

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BuildDir = Join-Path $RepoRoot "ohos\hello\build"
$Bin = $null
foreach ($c in @("hello", "hello.exe", "Release\hello", "Release\hello.exe")) {
    $p = Join-Path $BuildDir $c
    if (Test-Path $p) { $Bin = $p; break }
}
if (-not $Bin) { throw "Build hello first: powershell -File ohos\scripts\build_hello.ps1" }

$targets = & hdc list targets 2>&1 | Out-String
if ($targets -match "\[Empty\]" -or [string]::IsNullOrWhiteSpace($targets.Trim())) {
    Write-Host "No device connected (hdc list targets empty). Host build OK; board gate pending."
    exit 2
}

& hdc file send $Bin /data/local/tmp/hello
if ($LASTEXITCODE -ne 0) { throw "hdc file send failed" }
& hdc shell chmod 755 /data/local/tmp/hello
& hdc shell /data/local/tmp/hello
