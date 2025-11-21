import os
import sys
import ctypes
from pathlib import Path

from .utils import is_win7_sp1_or_later, setup_logger


def test_os_version(logger):
    ok = is_win7_sp1_or_later()
    logger.info("OS version check (Win7 SP1+): %s", ok)
    return ok


def test_no_admin_required(logger):
    try:
        # 写入到用户目录 (不需要管理员权限)
        p = Path(os.getenv("LOCALAPPDATA", ".")) / "win7_packager_test.txt"
        p.write_text("ok", encoding="utf-8")
        logger.info("Write to LOCALAPPDATA succeeded: %s", p)
        if p.exists():
            p.unlink()
        return True
    except Exception as e:
        logger.error("Write to LOCALAPPDATA failed: %s", e)
        return False


def test_win7_api_present(logger):
    try:
        # 加载 kernel32 并检查基础 API 是否可用
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        GetVersionEx = getattr(kernel32, "GetVersionExW", None)
        logger.info("Kernel32 loaded, GetVersionExW present: %s", GetVersionEx is not None)
        return True
    except Exception as e:
        logger.error("Kernel32 load failed: %s", e)
        return False


def main():
    logger = setup_logger("packaging/logs/win7_compat_tests.log")
    results = {
        "os_version": test_os_version(logger),
        "no_admin": test_no_admin_required(logger),
        "win7_api": test_win7_api_present(logger),
    }
    ok = all(results.values())
    logger.info("Summary: %s", results)
    print("RESULT:", results)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())