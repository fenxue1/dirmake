Windows 7 可执行文件打包与运行指南

前提条件
- Python 3.8（建议）
- pip 可用
- 建议安装 PyInstaller 4.x（例如 4.10）以兼容 Win7

打包步骤
1) 安装依赖：
   - 如果网络受限，可在有网络的机器上下载 wheel 后离线安装。
   - 建议：`pip install pyinstaller==4.10`

2) 执行打包脚本：
   - 在 PowerShell 中运行：
     `powershell -ExecutionPolicy Bypass -File packaging/build_win7.ps1`
   - 可选：加入 `-OneFile` 参数生成单文件可执行（可能增大体积）。

产物说明
- `dist/TyTextGUI/TyTextGUI.exe`：GUI 主程序
- `dist/TyTextGUI/ty_text_extractor.exe`：CLI 工具，GUI 在冻结环境通过它执行任务

运行要求
- Windows 7 SP1，安装适当的 VC++ 运行库（若缺失可能导致启动失败）
- 图形环境需要基础的图形驱动；如果启动提示 `libpng` 警告为正常现象

离线/内网环境建议
- 使用私有 PyPI 镜像或提前下载好 `PyInstaller` 的 wheel 包
- 如需嵌入 Python 运行时与依赖，推荐 `--onefile` 或保留 `--onedir` 的 `dist` 目录整体分发

注意
- GUI 会在打包环境下自动调用同目录的 `ty_text_extractor.exe`（已在 `gui_app/runner.py` 适配）
- 若你更希望 GUI 直接包含 CLI 逻辑，可改为将脚本合并，但体积与维护成本会增大

DirModeEx (Qt/C++) 在 Win7 的打包

- 选择与你构建所用工具链匹配的脚本：
  - MinGW (建议用于 `D:\Qt\5.14.2\mingw73_32`，32 位 Win7 兼容)：
    - 运行：
      `powershell -ExecutionPolicy Bypass -File packaging/build_dir_mode_ex_win7_mingw.ps1 -QtBin "D:\Qt\5.14.2\mingw73_32\bin" -QtPrefix "D:\Qt\5.14.2\mingw73_32"`
    - 产物目录：`build/DirModeEx/release_pack`，内含 `DirModeEx.exe`、Qt 依赖、平台插件。
  - MSVC (如 `C:\Qt\5.15.10\msvc2017_64`)：
    - 运行：
      `powershell -ExecutionPolicy Bypass -File packaging/build_dir_mode_ex_win7.ps1 -QtBin "C:\Qt\5.15.10\msvc2017_64\bin" -Generator "Visual Studio 16 2019" -Arch "x64"`
    - 产物目录：`build/DirModeEx/release_pack`。

- 重要提示：
  - 必须用与构建相同套件版本的 `windeployqt.exe`，否则 DLL 不兼容。
  - MinGW 脚本已包含 `--compiler-runtime`，确保拷贝 `libstdc++-6.dll` 等运行时。
  - MSVC 脚本同样启用 `--compiler-runtime`，拷贝所需 VC++ 运行库。
  - 如程序含网络/TLS，请在包中加入匹配版本的 OpenSSL 库。

- Win7 验证建议：
  - 在干净的 Win7 SP1 虚拟机运行 `release_pack/DirModeEx.exe`。
  - 如遇插件加载问题：设置 `QT_DEBUG_PLUGINS=1` 观察详细原因。
  - 依赖排查：使用 `Dependencies` 工具打开 `DirModeEx.exe` 查看缺失 DLL。