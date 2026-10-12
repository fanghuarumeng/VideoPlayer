# Build VideoPlayer server and/or client (Windows).
# Usage (from repo root):
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Target server
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Target client
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Target all -QtPrefix D:/Qt/Qt5.12.11/5.12.11/msvc2017_64
param(
  [ValidateSet("server", "client", "all")]
  [string]$Target = "all",
  [string]$Config = "Release",
  [string]$QtPrefix = "",
  [string]$ServerBuildDir = "build-server",
  [string]$ClientBuildDir = "build-client",
  [string]$Generator = "Visual Studio 16 2019",
  [string]$Arch = "x64"
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

function Resolve-QtPrefix([string]$Hint) {
  if ($Hint -and (Test-Path $Hint)) { return $Hint }
  if ($env:CMAKE_PREFIX_PATH -and (Test-Path $env:CMAKE_PREFIX_PATH)) {
    return $env:CMAKE_PREFIX_PATH
  }
  $candidates = @(
    "D:/Qt/Qt5.12.11/5.12.11/msvc2017_64",
    "C:/Qt/Qt5.12.11/5.12.11/msvc2017_64",
    "D:/Qt/5.12.11/msvc2017_64",
    "C:/Qt/5.12.11/msvc2017_64"
  )
  foreach ($path in $candidates) {
    if (Test-Path $path) { return $path }
  }
  throw @"
Qt5 prefix not found. Pass -QtPrefix <path> (msvc kit root that contains lib/cmake/Qt5),
or set CMAKE_PREFIX_PATH. Example:
  -QtPrefix D:/Qt/Qt5.12.11/5.12.11/msvc2017_64
"@
}

$CMake = Find-Tool "cmake"

function Stop-BuildLocks {
  # windeployqt patches Qt5Core.dll next to the exe; a running app locks it and fails the build.
  $names = @(
    "VideoPlayerApp",
    "VideoPlayerServer",
    "VideoPlayerClientTests",
    "SquareDanceApp",
    "SquareDanceServer"
  )
  $procs = Get-Process -Name $names -ErrorAction SilentlyContinue
  if (-not $procs) { return }
  Write-Host "==> stopping running processes that lock build outputs:"
  $procs | ForEach-Object { Write-Host ("    {0} (pid {1})" -f $_.Name, $_.Id) }
  $procs | Stop-Process -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 1
}

function Build-Server {
  Write-Host "==> configure server ($ServerBuildDir)"
  & $CMake -S . -B $ServerBuildDir `
    -DBUILD_SERVER=ON `
    -DBUILD_CLIENT=OFF
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> build VideoPlayerServer ($Config)"
  & $CMake --build $ServerBuildDir --config $Config --target VideoPlayerServer
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  $exe = Join-Path $ServerBuildDir "$Config\VideoPlayerServer.exe"
  if (-not (Test-Path $exe)) {
    $exe = Join-Path $ServerBuildDir "VideoPlayerServer.exe"
  }
  Write-Host "==> server ok: $exe"
}

function Build-Client {
  $qt = Resolve-QtPrefix $QtPrefix
  Write-Host "==> configure client ($ClientBuildDir) Qt=$qt"
  & $CMake -S . -B $ClientBuildDir `
    -G $Generator -A $Arch `
    -DBUILD_SERVER=OFF `
    -DBUILD_CLIENT=ON `
    "-DCMAKE_PREFIX_PATH=$qt"
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> build VideoPlayerApp ($Config)"
  & $CMake --build $ClientBuildDir --config $Config --target VideoPlayerApp
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  $exe = Join-Path $ClientBuildDir "$Config\VideoPlayerApp.exe"
  if (-not (Test-Path $exe)) {
    $exe = Join-Path $ClientBuildDir "VideoPlayerApp.exe"
  }
  Write-Host "==> client ok: $exe"
}

Write-Host "==> VideoPlayer build target=$Target config=$Config"
Stop-BuildLocks
switch ($Target) {
  "server" { Build-Server }
  "client" { Build-Client }
  "all" {
    Build-Server
    Build-Client
  }
}
Write-Host "==> build done"
