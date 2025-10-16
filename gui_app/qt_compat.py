'''
Author: fenxue1 99110925+fenxue1@users.noreply.github.com
Date: 2025-10-08 23:47:52
LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
LastEditTime: 2025-10-09 00:27:24
FilePath: \test_mooc-clin\gui_app\qt_compat.py
Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
'''
"""
Qt 兼容层：优先使用 PyQt5（较好适配 Windows 7），失败则回退到 PySide6。
"""

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