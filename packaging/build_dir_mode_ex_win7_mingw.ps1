Param(
  [string]$QtBin="D:\Qt\5.14.2\mingw73_32\bin",
  [string]$QtPrefix="D:\Qt\5.14.2\mingw73_32",
  [string]$Jobs="4"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$projRoot = Resolve-Path "$root\..\DirModeEx"
$buildDir = Resolve-Path "$root\..\build\DirModeEx"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

Push-Location $buildDir
try {
  # 生成 MinGW Makefiles，指定 Qt 前缀以便 CMake 找到 Qt
  cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="$QtPrefix" -DCMAKE_BUILD_TYPE=Release $projRoot
  if ($LASTEXITCODE -ne 0) { throw "CMake 生成失败" }

  # 构建（MinGW 单配置，无 --config 参数）
  cmake --build . -- -j $Jobs
  if ($LASTEXITCODE -ne 0) { throw "构建失败" }

  # 查找可执行文件（可能位于构建根目录）
  $exe = Get-ChildItem -Recurse -Filter DirModeEx.exe | Select-Object -First 1
  if (-not $exe) { throw "未找到 DirModeEx.exe" }
  Write-Host "构建完成：$($exe.FullName)"

  # 部署 Qt 依赖
  $windeployqt = Join-Path $QtBin "windeployqt.exe"
  if (!(Test-Path $windeployqt)) { throw "未找到 windeployqt：$windeployqt" }
  $outDir = Join-Path $buildDir "release_pack"
  if (Test-Path $outDir) { Remove-Item -Recurse -Force $outDir }
  New-Item -ItemType Directory -Path $outDir | Out-Null

  & $windeployqt --release --compiler-runtime --dir "$outDir" $exe.FullName
  if ($LASTEXITCODE -ne 0) { throw "windeployqt 执行失败" }
  Write-Host "依赖复制完成：$outDir"
}
finally {
  Pop-Location
}