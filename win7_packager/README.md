# Windows 7 Packager

目标：为 Windows 7 SP1 及以上环境提供 Python 应用的打包工具，支持 CLI 与 GUI 两种方式，生成可在 Win7 x86/x64 运行的单文件可执行程序或安装包，同时包含必要运行时库、依赖项检测、详细日志与错误上报，并提供 Win7 兼容性测试脚本。

功能概览：
- 支持 Python 3.8+（建议 3.8）打包，自动包含依赖项（PyInstaller）
- 生成 Win7 32 位和 64 位可执行文件（需对应位数的 Python 解释器）
- 提供命令行工具 `packager_cli.py` 与图形界面 `packager_gui.py`
- 可生成单文件（onefile）或目录（onedir），可选生成 NSIS 安装包，无管理员权限安装（RequestExecutionLevel user）
- 自动检测依赖项并记录到日志与快照文件
- 自定义图标与版本信息（PyInstaller `--icon` 与 `--version-file`）
- 详细打包日志与错误报告；打包后生成兼容性测试套件（运行于 Win7 环境）

快速开始（CLI）：
```
python win7_packager/packager_cli.py \
  --entry gui_app/main.py \
  --output-dir dist_win7 \
  --onefile \
  --icon assets/app.ico \
  --version-file win7_packager/versioninfo.txt \
  --python32 "C:\\Python38-32\\python.exe" \
  --python64 "C:\\Python38\\python.exe" \
  --installer
```

注意事项：
- PyInstaller 不跨架构编译：构建 32 位需使用 32 位 Python；构建 64 位需使用 64 位 Python。
- 为确保 Win7 兼容性，推荐在 Win10/11 上用 Python 3.8 + PyInstaller 4.10 或 5.13.2（不同环境可能有差异）；如遇 Win7 运行问题，可尝试降级 PyInstaller。
- 生成安装包需要安装 NSIS（`makensis` 在 PATH 中）。如未安装，脚本会跳过安装包生成并提示。

兼容性测试：
- 在 Win7 SP1 机器上运行 `dist_win7/compat_tests/*.py`，验证 OS 版本、无管理员写入能力、依赖项导入等。