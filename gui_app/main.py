import sys
import os
# 允许作为脚本运行：将项目根目录加入sys.path
if __package__ in (None, ""):
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
from gui_app.qt_compat import QtWidgets, API_NAME, QtCore
from gui_app.ui import MainWindow


def _write_crash_log(exc: Exception):
    try:
        import traceback
        # 写入到可执行同目录（冻结）或项目根目录
        base_dir = os.path.dirname(sys.executable) if getattr(sys, 'frozen', False) else os.path.dirname(os.path.dirname(__file__))
        log_path = os.path.join(base_dir, 'TyTextGUI_crash.log')
        with open(log_path, 'a', encoding='utf-8') as f:
            f.write("\n==== Crash ===\n")
            f.write('API: ' + str(API_NAME) + '\n')
            f.write(traceback.format_exc())
    except Exception:
        pass


def _message_box(title: str, text: str):
    try:
        # 尝试使用 Qt 消息框（若可用）
        if QtWidgets:
            msg = QtWidgets.QMessageBox()
            msg.setIcon(QtWidgets.QMessageBox.Critical)
            msg.setWindowTitle(title)
            msg.setText(text)
            msg.exec_() if hasattr(msg, 'exec_') else msg.exec()
            return
    except Exception:
        pass
    try:
        # 退回到 Win32 原生消息框（在窗口模式无控制台时也可见）
        import ctypes
        ctypes.windll.user32.MessageBoxW(0, text, title, 0x10)
    except Exception:
        pass


def main():
    try:
        if not QtWidgets:
            _message_box("启动失败", "未找到 PyQt5/PySide6 依赖，请安装后再运行。")
            sys.exit(1)
        print(f"使用 {API_NAME} 运行 GUI")
        # 改善 Win7 兼容性与渲染稳定性
        if QtCore:
            try:
                QtWidgets.QApplication.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling)
            except Exception:
                pass
        app = QtWidgets.QApplication(sys.argv)
        win = MainWindow()
        win.show()
        # 兼容不同 Qt 版本的事件循环入口
        if hasattr(app, 'exec'):
            rc = app.exec()
        else:
            rc = app.exec_()
        sys.exit(rc)
    except Exception as e:
        _write_crash_log(e)
        _message_box("程序崩溃", "发生未处理的异常，已写入 TyTextGUI_crash.log")
        raise


if __name__ == "__main__":
    main()