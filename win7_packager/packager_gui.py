import os
import sys
import threading
from pathlib import Path
from tkinter import Tk, Label, Entry, Button, Checkbutton, BooleanVar, filedialog, StringVar, Text, END

from .packager_cli import main as cli_main


def run_packager_async(args_line: str, log_widget: Text):
    def _worker():
        try:
            log_widget.insert(END, f"运行命令: {args_line}\n")
            argv = args_line.split()
            rc = cli_main(argv)
            log_widget.insert(END, f"完成，返回码: {rc}\n")
        except Exception as e:
            log_widget.insert(END, f"错误: {e}\n")
    t = threading.Thread(target=_worker, daemon=True)
    t.start()


def make_gui():
    win = Tk()
    win.title("Windows 7 Packager")

    sv_entry = StringVar()
    sv_outdir = StringVar(value="dist_win7")
    sv_icon = StringVar()
    sv_py32 = StringVar()
    sv_py64 = StringVar()
    sv_verfile = StringVar()
    sv_pyi = StringVar(value="4.10")
    is_onefile = BooleanVar(value=True)
    is_guiapp = BooleanVar(value=True)
    is_installer = BooleanVar(value=False)

    def choose_file(var: StringVar, title: str, filetypes=(("All", "*.*"),)):
        p = filedialog.askopenfilename(title=title, filetypes=filetypes)
        if p:
            var.set(p)

    # 布局
    Label(win, text="入口脚本").grid(row=0, column=0, sticky='e')
    Entry(win, textvariable=sv_entry, width=50).grid(row=0, column=1)
    Button(win, text="选择", command=lambda: choose_file(sv_entry, "选择入口脚本", (("Python", "*.py"),))).grid(row=0, column=2)

    Label(win, text="输出目录").grid(row=1, column=0, sticky='e')
    Entry(win, textvariable=sv_outdir, width=50).grid(row=1, column=1)

    Label(win, text="图标(.ico)").grid(row=2, column=0, sticky='e')
    Entry(win, textvariable=sv_icon, width=50).grid(row=2, column=1)
    Button(win, text="选择", command=lambda: choose_file(sv_icon, "选择图标", (("Icon", "*.ico"),))).grid(row=2, column=2)

    Label(win, text="版本文件").grid(row=3, column=0, sticky='e')
    Entry(win, textvariable=sv_verfile, width=50).grid(row=3, column=1)
    Button(win, text="选择", command=lambda: choose_file(sv_verfile, "选择版本文件", (("Txt", "*.txt"),))).grid(row=3, column=2)

    Label(win, text="Python32").grid(row=4, column=0, sticky='e')
    Entry(win, textvariable=sv_py32, width=50).grid(row=4, column=1)
    Button(win, text="选择", command=lambda: choose_file(sv_py32, "选择32位Python", (("python.exe", "python.exe"),))).grid(row=4, column=2)

    Label(win, text="Python64").grid(row=5, column=0, sticky='e')
    Entry(win, textvariable=sv_py64, width=50).grid(row=5, column=1)
    Button(win, text="选择", command=lambda: choose_file(sv_py64, "选择64位Python", (("python.exe", "python.exe"),))).grid(row=5, column=2)

    Checkbutton(win, text="单文件(onefile)", variable=is_onefile).grid(row=6, column=0)
    Checkbutton(win, text="GUI应用(--noconsole)", variable=is_guiapp).grid(row=6, column=1)
    Checkbutton(win, text="生成安装包(NSIS)", variable=is_installer).grid(row=6, column=2)

    Label(win, text="PyInstaller版本").grid(row=7, column=0, sticky='e')
    Entry(win, textvariable=sv_pyi, width=20).grid(row=7, column=1, sticky='w')

    log = Text(win, width=100, height=20)
    log.grid(row=8, column=0, columnspan=3)

    def build(label: str):
        args = []
        args += ["--entry", sv_entry.get()]
        args += ["--output-dir", sv_outdir.get()]
        if is_onefile.get():
            args += ["--onefile"]
        else:
            args += ["--onedir"]
        if sv_icon.get(): args += ["--icon", sv_icon.get()]
        if sv_verfile.get(): args += ["--version-file", sv_verfile.get()]
        if sv_py32.get(): args += ["--python32", sv_py32.get()]
        if sv_py64.get(): args += ["--python64", sv_py64.get()]
        if sv_pyi.get(): args += ["--preferred-pyi", sv_pyi.get()]
        if is_guiapp.get(): args += ["--gui-app"]
        if is_installer.get(): args += ["--installer"]
        args += ["--log-file", os.path.join(sv_outdir.get(), "packager_gui.log")]
        run_packager_async(" ".join(args), log)

    Button(win, text="开始打包", command=lambda: build("all")).grid(row=9, column=0, columnspan=3)
    return win


if __name__ == "__main__":
    app = make_gui()
    app.mainloop()