import os
import sys
import json
import subprocess
import platform
import logging
from typing import Optional, Tuple, List


def setup_logger(log_file: str) -> logging.Logger:
    os.makedirs(os.path.dirname(log_file), exist_ok=True)
    logger = logging.getLogger("win7_packager")
    logger.setLevel(logging.INFO)
    # 防重复添加 handler
    if not logger.handlers:
        fh = logging.FileHandler(log_file, encoding="utf-8")
        fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
        fh.setFormatter(fmt)
        logger.addHandler(fh)
        sh = logging.StreamHandler(sys.stdout)
        sh.setFormatter(fmt)
        logger.addHandler(sh)
    return logger


def is_win7_sp1_or_later() -> bool:
    try:
        v = sys.getwindowsversion()
        # Windows 7 = 6.1, SP1 对应 build >= 7601
        major, minor, build = v.major, v.minor, v.build
        if (major, minor) < (6, 1):
            return False
        if (major, minor) == (6, 1) and build < 7601:
            return False
        return True
    except Exception:
        # 非 Windows 或未能检测，默认 True 以不阻塞流程
        return True


def python_info(python_exe: Optional[str]) -> Tuple[str, str]:
    """返回 (version_str, arch_str) 例如 ("3.8.10", "64bit")。"""
    cmd = [python_exe or sys.executable, "-c",
           "import platform,sys; print(platform.python_version()); print(platform.architecture()[0])"]
    out = subprocess.check_output(cmd, shell=False).decode("utf-8", errors="replace").strip().splitlines()
    version = out[0].strip() if out else "unknown"
    arch = out[1].strip() if len(out) > 1 else "unknown"
    return version, arch


def ensure_pyinstaller(python_exe: Optional[str], logger: logging.Logger, preferred_version: Optional[str] = None) -> None:
    """确保目标 Python 安装了 PyInstaller，必要时进行安装/降级。
    优先检测已安装版本，避免不必要的网络安装；安装失败时增加超时时间重试。
    """
    py = python_exe or sys.executable
    # 检查已安装版本
    try:
        out = subprocess.check_output([py, "-c", "import PyInstaller,sys;print(getattr(PyInstaller,'__version__','unknown'))"], shell=False).decode("utf-8", errors="replace").strip()
        installed_ver = out or "unknown"
        if installed_ver != "unknown":
            if preferred_version:
                if installed_ver == preferred_version:
                    logger.info("PyInstaller already present: %s (matches preferred)", installed_ver)
                    return
                else:
                    logger.info("PyInstaller present: %s (preferred: %s) -> will adjust", installed_ver, preferred_version)
            else:
                logger.info("PyInstaller already present: %s", installed_ver)
                return
    except Exception:
        pass

    base_cmd = [py, "-m", "pip", "install", "--default-timeout", "120"]
    pkg = "pyinstaller"
    if preferred_version:
        pkg = f"pyinstaller=={preferred_version}"
    cmd = base_cmd + [pkg]
    logger.info("Installing/ensuring %s via: %s", pkg, " ".join(cmd))
    try:
        subprocess.check_call(cmd, shell=False)
    except subprocess.CalledProcessError:
        # 尝试使用官方源重试
        cmd_retry = base_cmd + ["-i", "https://pypi.org/simple", pkg]
        logger.warning("PyInstaller install failed, retrying with default index: %s", " ".join(cmd_retry))
        subprocess.check_call(cmd_retry, shell=False)


def list_installed_packages(python_exe: Optional[str]) -> List[dict]:
    cmd = [python_exe or sys.executable, "-m", "pip", "list", "--format", "json"]
    try:
        out = subprocess.check_output(cmd, shell=False).decode("utf-8", errors="replace")
        return json.loads(out)
    except Exception:
        return []


def snapshot_dependencies(python_exe: Optional[str], output_path: str, logger: logging.Logger) -> None:
    deps = list_installed_packages(python_exe)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(deps, f, ensure_ascii=False, indent=2)
    logger.info("Saved dependency snapshot to %s (packages: %d)", output_path, len(deps))


def generate_default_version_file(path: str, product_name: str, file_version: str) -> None:
    """生成 PyInstaller 兼容的 versionfile txt。"""
    content = f"""
# UTF-8
VSVersionInfo(
  ffi=FixedFileInfo(
    filevers=({file_version.replace('.', ',')}, 0),
    prodvers=({file_version.replace('.', ',')}, 0),
    mask=0x3f,
    flags=0x0,
    OS=0x40004,
    fileType=0x1,
    subtype=0x0,
    date=(0, 0)
  ),
  kids=[
    StringFileInfo([
      StringTable('040904B0', [
        StringStruct('CompanyName', '{product_name}'),
        StringStruct('FileDescription', '{product_name} Application'),
        StringStruct('FileVersion', '{file_version}'),
        StringStruct('InternalName', '{product_name}'),
        StringStruct('OriginalFilename', '{product_name}.exe'),
        StringStruct('ProductName', '{product_name}'),
        StringStruct('ProductVersion', '{file_version}')
      ])
    ]),
    VarFileInfo([
      VarStruct('Translation', [1033, 1200])
    ])
  ]
)
""".strip()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)