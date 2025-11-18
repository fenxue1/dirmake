"""Qt 兼容层：优先使用 PyQt5（较好适配 Windows 7），失败则回退到 PySide6。"""

API_NAME = None

try:
    from PyQt5 import QtWidgets, QtCore  # 优先 PyQt5
    API_NAME = "PyQt5"
except Exception:
    try:
        from PySide6 import QtWidgets, QtCore
        API_NAME = "PySide6"
    except Exception:
        QtWidgets = None
        QtCore = None
        API_NAME = None