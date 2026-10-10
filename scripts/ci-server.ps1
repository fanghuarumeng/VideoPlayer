# Server-only CI gate: configure/build without Qt, run server GTests.
# Usage (from repo root):
#   powershell -ExecutionPolicy Bypass -File scripts/ci-server.ps1
param(
  [string]$BuildDir = "build-server-ci",
  [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
Set-Location (Split-Path -Parent $PSScriptRoot)

function Find-Tool([string]$Name) {
  $cmd = Get-Command $Name -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  $candidates = @(
    "C:\Program Files\CMake\bin\$Name.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\$Name.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\$Name.exe"
  )
  foreach ($path in $candidates) {
    if (Test-Path $path) { return $path }
  }
  throw "Cannot find $Name on PATH or known Visual Studio CMake locations."
}

$CMake = Find-Tool "cmake"
$CTest = Find-Tool "ctest"

Write-Host "==> cmake configure (BUILD_CLIENT=OFF)"
& $CMake -S . -B $BuildDir `
  -DBUILD_SERVER=ON `
  -DBUILD_CLIENT=OFF
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> cmake build SquareDanceServer + tests"
& $CMake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> ctest (server)"
& $CTest --test-dir $BuildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> server CI green"
