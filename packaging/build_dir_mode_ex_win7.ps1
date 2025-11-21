Param(
  [string]$QtBin="C:\Qt\5.15.10\msvc2017_64\bin",
  [string]$Generator="Visual Studio 16 2019",
  [string]$Arch="x64"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$projRoot = Resolve-Path "$root\..\DirModeEx"
$buildDir = Resolve-Path "$root\..\build\DirModeEx"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

Push-Location $buildDir
try {
  cmake -G "$Generator" -A $Arch $projRoot
  cmake --build . --config Release
  $exe = Join-Path $buildDir "Release\DirModeEx.exe"
  if (!(Test-Path $exe)) { $exe = Join-Path $buildDir "DirModeEx.exe" }
  if (!(Test-Path $exe)) { throw "构建失败，未找到 DirModeEx.exe" }
  Write-Host "构建完成：$exe"

  # 部署 Qt 依赖
  $windeployqt = Join-Path $QtBin "windeployqt.exe"
  if (!(Test-Path $windeployqt)) { throw "未找到 windeployqt：$windeployqt" }
  & $windeployqt --release --compiler-runtime --dir "$buildDir\release_pack" $exe
  Write-Host "依赖复制完成：$buildDir\release_pack"
}
finally {
  Pop-Location
}