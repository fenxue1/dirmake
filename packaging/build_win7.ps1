Param(
    [switch]$OneFile
)

$ErrorActionPreference = 'Stop'

# 切换到项目根目录
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $root

Write-Host "项目根目录: $root"

# 检查 PyInstaller
if (-not (Get-Command pyinstaller -ErrorAction SilentlyContinue)) {
    Write-Host "未检测到 PyInstaller。请先安装(建议 4.x 以兼容 Win7):" -ForegroundColor Yellow
    Write-Host "    pip install pyinstaller==4.10" -ForegroundColor Yellow
    exit 1
}

# 清理旧构建
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, dist

# 构建 CLI (ty_text_extractor)
$cliArgs = @('-y','--noconfirm','--clean','--name','ty_text_extractor','text/ty_text_extractor.py')
if ($OneFile) { $cliArgs += '--onefile' } else { $cliArgs += '--onedir' }
pyinstaller @cliArgs

# 构建 GUI (TyTextGUI)
$guiArgs = @('-y','--noconfirm','--clean','--name','TyTextGUI','--windowed','gui_app/main.py')
if ($OneFile) { $guiArgs += '--onefile' } else { $guiArgs += '--onedir' }
pyinstaller @guiArgs

# 将 CLI 可执行文件复制到 GUI 发布目录，供冻结环境调用
$cliExe = Join-Path 'dist/ty_text_extractor' 'ty_text_extractor.exe'
$guiDir = 'dist/TyTextGUI'
if (Test-Path $cliExe) {
    Copy-Item -Force $cliExe $guiDir
    Write-Host "已复制 CLI 到 GUI 目录: $guiDir\ty_text_extractor.exe"
} else {
    Write-Host "未找到 CLI 可执行文件: $cliExe" -ForegroundColor Red
}

Write-Host "打包完成。GUI 路径: dist/TyTextGUI" -ForegroundColor Green
Write-Host "运行: dist/TyTextGUI/TyTextGUI.exe (确保同目录存在 ty_text_extractor.exe)" -ForegroundColor Green