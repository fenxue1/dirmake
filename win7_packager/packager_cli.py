import os
import sys
import shutil
import argparse
import subprocess
from pathlib import Path
from typing import Optional

from .utils import (
    setup_logger,
    is_win7_sp1_or_later,
    python_info,
    ensure_pyinstaller,
    snapshot_dependencies,
    generate_default_version_file,
)


def build_with_pyinstaller(
    python_exe: Optional[str], entry: str, output_dir: str,
    onefile: bool, icon: Optional[str], version_file: Optional[str],
    gui_app: bool, logger
):
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    cmd = [python_exe or sys.executable, "-m", "PyInstaller", entry, "--noconfirm", "--clean"]
    cmd += ["--distpath", os.path.abspath(output_dir), "--workpath", os.path.abspath(os.path.join(output_dir, "build")), "--specpath", os.path.abspath(os.path.join(output_dir, "spec"))]
    cmd += ["--log-level", "INFO"]
    if onefile:
        cmd.append("--onefile")
    else:
        cmd.append("--onedir")
    if icon:
        cmd += ["--icon", os.path.abspath(icon)]
    if version_file:
        cmd += ["--version-file", os.path.abspath(version_file)]
    if gui_app:
        cmd.append("--noconsole")
    logger.info("Running PyInstaller: %s", " ".join(cmd))
    subprocess.check_call(cmd, shell=False)


def generate_nsis_script(installer_nsi: str, app_name: str, exe_path: str):
    content = f"""
; NSIS installer generated for Windows 7 (no admin)
!include "MUI2.nsh"
Name "{app_name}"
OutFile "{app_name}-Setup.exe"
InstallDir "$LOCALAPPDATA\\{app_name}"
RequestExecutionLevel user

Section "Install"
  SetOutPath "$INSTDIR"
  File "{exe_path}"
  CreateShortcut "$DESKTOP\\{app_name}.lnk" "$INSTDIR\\{Path(exe_path).name}"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\\{Path(exe_path).name}"
  Delete "$DESKTOP\\{app_name}.lnk"
  RMDir "$INSTDIR"
SectionEnd
""".strip()
    Path(installer_nsi).parent.mkdir(parents=True, exist_ok=True)
    with open(installer_nsi, "w", encoding="utf-8") as f:
        f.write(content)


def try_build_installer(installer_nsi: str, logger):
    makensis = shutil.which("makensis")
    if not makensis:
        logger.warning("NSIS 'makensis' not found in PATH. Skipping installer build.")
        return False
    cmd = [makensis, installer_nsi]
    logger.info("Building installer via NSIS: %s", " ".join(cmd))
    subprocess.check_call(cmd, shell=False)
    return True


def main(argv=None):
    parser = argparse.ArgumentParser(description="Windows 7 Packager (CLI)")
    parser.add_argument("--entry", required=True, help="入口脚本路径（例如 gui_app/main.py）")
    parser.add_argument("--output-dir", required=True, help="输出目录")
    parser.add_argument("--onefile", action="store_true", help="生成单文件可执行程序")
    parser.add_argument("--onedir", action="store_true", help="生成目录形式（与 --onefile 互斥）")
    parser.add_argument("--icon", help="图标文件 .ico 路径")
    parser.add_argument("--version-file", help="PyInstaller 版本信息文件（.txt）")
    parser.add_argument("--python32", help="32 位 Python3.8+ 解释器路径")
    parser.add_argument("--python64", help="64 位 Python3.8+ 解释器路径")
    parser.add_argument("--preferred-pyi", help="优先使用的 PyInstaller 版本，例如 4.10 或 5.13.2")
    parser.add_argument("--gui-app", action="store_true", help="应用为 GUI（打包时添加 --noconsole）")
    parser.add_argument("--installer", action="store_true", help="生成 NSIS 安装包（需安装 NSIS）")
    parser.add_argument("--log-file", default="packaging/logs/packager.log", help="日志输出文件")

    args = parser.parse_args(argv)
    logger = setup_logger(args.log_file)

    if not is_win7_sp1_or_later():
        logger.warning("当前 OS 非 Win7 SP1 及以上，继续构建但建议在 Win7 环境验证运行。")

    # 选择 onefile/onedir
    onefile = True
    if args.onedir:
        onefile = False
    elif args.onefile:
        onefile = True

    # 准备版本文件
    version_file = args.version_file
    if not version_file:
        default_vf = os.path.join(args.output_dir, "versioninfo.txt")
        generate_default_version_file(default_vf, Path(args.entry).stem, "1.0.0")
        version_file = default_vf

    # 依次构建 32 位与 64 位（根据提供的解释器）
    built_artifacts = []
    for label, py in (("x86", args.python32), ("x64", args.python64)):
        if not py:
            logger.info("Skip %s build (no interpreter provided)", label)
            continue
        ver, arch = python_info(py)
        logger.info("Using Python %s (%s) at %s for %s build", ver, arch, py, label)
        ensure_pyinstaller(py, logger, preferred_version=args.preferred_pyi)
        snapshot_dependencies(py, os.path.join(args.output_dir, f"deps_snapshot_{label}.json"), logger)
        out_dir = os.path.join(args.output_dir, f"build_{label}")
        build_with_pyinstaller(py, args.entry, out_dir, onefile, args.icon, version_file, args.gui_app, logger)
        # 寻找生成的 exe
        dist_dir = Path(out_dir)
        exes = list(dist_dir.glob("*.exe"))
        if not exes:
            # PyInstaller 会在 dist 下创建以入口名为目录或 exe 名，扩大搜索范围
            exes = list(dist_dir.rglob("*.exe"))
        if not exes:
            logger.error("未找到生成的 exe: %s", out_dir)
        else:
            exe_path = str(exes[0])
            logger.info("Generated: %s", exe_path)
            built_artifacts.append((label, exe_path))

    # 生成安装包（可选）
    if args.installer:
        for label, exe in built_artifacts:
            nsi_path = os.path.join(args.output_dir, f"installer_{label}.nsi")
            generate_nsis_script(nsi_path, Path(args.entry).stem + ("-" + label), exe)
            try_build_installer(nsi_path, logger)

    logger.info("All done. Built artifacts: %s", built_artifacts)
    return 0


if __name__ == "__main__":
    sys.exit(main())