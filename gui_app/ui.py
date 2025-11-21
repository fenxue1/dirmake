from gui_app.qt_compat import QtWidgets, QtCore
from gui_app.qt_compat import API_NAME
import os
try:
    from PyQt5 import QtGui as _QtGui
except Exception:
    try:
        from PySide6 import QtGui as _QtGui
    except Exception:
        _QtGui = None


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Ty Text Extractor GUI")
        self.setAcceptDrops(True)

        self._run_all_active = False
        self._current_tag = None

        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        root_layout = QtWidgets.QVBoxLayout(central)

        # 面包屑位置指示
        self.breadcrumb = QtWidgets.QLabel("首页 / 提取CSV")
        b_font = self.breadcrumb.font()
        b_font.setBold(True)
        self.breadcrumb.setFont(b_font)
        root_layout.addWidget(self.breadcrumb)

        # 分页容器
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.setTabPosition(QtWidgets.QTabWidget.North)
        root_layout.addWidget(self.tabs)

        # 懒加载标记
        self._page_built = {}
        # 预置四个页签占位
        for name in ("提取CSV", "从CSV生成C", "设置", "日志"):
            w = QtWidgets.QWidget()
            w.setLayout(QtWidgets.QVBoxLayout())
            w.layout().addWidget(QtWidgets.QLabel(f"正在加载 {name}…"))
            self.tabs.addTab(w, name)

        # 快捷键切换分页 Ctrl+Tab / Ctrl+Shift+Tab
        if _QtGui is not None:
            QtWidgets.QShortcut(_QtGui.QKeySequence("Ctrl+Tab"), self, activated=self._next_tab)
            QtWidgets.QShortcut(_QtGui.QKeySequence("Ctrl+Shift+Tab"), self, activated=self._prev_tab)

        # 恢复上次页签
        self.settings = QtCore.QSettings("TyTextGUI", "TyTextGUI")
        last_index = int(self.settings.value("last_tab_index", 0))
        self.tabs.setCurrentIndex(last_index)
        self.tabs.currentChanged.connect(self._on_tab_changed)
        self._on_tab_changed(last_index)

    def choose_root(self):
        d = QtWidgets.QFileDialog.getExistingDirectory(self, "选择根目录")
        if d:
            self.root_edit.setText(d)

    def choose_file(self, line_edit: QtWidgets.QLineEdit, title: str, filter_str: str, save: bool=False):
        if save:
            fn, _ = QtWidgets.QFileDialog.getSaveFileName(self, title, "", filter_str)
        else:
            fn, _ = QtWidgets.QFileDialog.getOpenFileName(self, title, "", filter_str)
        if fn:
            line_edit.setText(fn)

    def choose_project(self):
        d = QtWidgets.QFileDialog.getExistingDirectory(self, "选择项目目录")
        if d:
            self.project_edit.setText(d)

    def autofill_outputs(self):
        proj = self.project_edit.text().strip()
        if not proj:
            self.log.append("请先选择项目目录")
            return
        # 组合项目内常见路径
        csv_out = os.path.join(proj, "ty_text_out.csv")
        c_out = os.path.join(proj, "generated_text_vars_preserved.c")
        self.csv_out_edit.setText(csv_out)
        self.c_out_edit.setText(c_out)

    # ============ 分页构建 ============
    def _on_tab_changed(self, index: int):
        name = self.tabs.tabText(index)
        self.breadcrumb.setText(f"首页 / {name}")
        self.settings.setValue("last_tab_index", index)
        if not self._page_built.get(index):
            self._build_page(index)
            self._page_built[index] = True
        self._animate_current_page()

    def _build_page(self, index: int):
        name = self.tabs.tabText(index)
        container = self.tabs.widget(index)
        lay: QtWidgets.QVBoxLayout = container.layout()
        # 清空占位
        while lay.count():
            item = lay.takeAt(0)
            w = item.widget()
            if w:
                w.setParent(None)
        if name == "提取CSV":
            self._build_extract_page(lay)
        elif name == "从CSV生成C":
            self._build_emit_page(lay)
        elif name == "设置":
            info = QtWidgets.QLabel("设置：分页状态持久化、快捷键、触控滑动已启用。")
            lay.addWidget(info)
        elif name == "日志":
            self.log = QtWidgets.QTextEdit()
            self.log.setReadOnly(True)
            lay.addWidget(self.log)

    def _build_extract_page(self, layout: QtWidgets.QVBoxLayout):
        # 根目录与项目目录
        root_row = QtWidgets.QHBoxLayout()
        self.root_edit = QtWidgets.QLineEdit()
        btn_root = QtWidgets.QPushButton("选择根目录")
        btn_root.clicked.connect(self.choose_root)
        root_row.addWidget(QtWidgets.QLabel("根目录"))
        root_row.addWidget(self.root_edit)
        root_row.addWidget(btn_root)

        proj_row = QtWidgets.QHBoxLayout()
        self.project_edit = QtWidgets.QLineEdit()
        btn_proj = QtWidgets.QPushButton("选择项目目录")
        btn_proj.clicked.connect(self.choose_project)
        btn_autofill = QtWidgets.QPushButton("一键填充输出路径")
        btn_autofill.clicked.connect(self.autofill_outputs)
        proj_row.addWidget(QtWidgets.QLabel("项目目录"))
        proj_row.addWidget(self.project_edit)
        proj_row.addWidget(btn_proj)
        proj_row.addWidget(btn_autofill)

        # 提取类型与保留转义
        opt_row = QtWidgets.QHBoxLayout()
        self.type_combo = QtWidgets.QComboBox()
        self.type_combo.addItems(["_Tr_TEXT"])  # 可扩展
        self.chk_preserve = QtWidgets.QCheckBox("提取保留转义 (--preserve-escapes)")
        opt_row.addWidget(QtWidgets.QLabel("类型"))
        opt_row.addWidget(self.type_combo)
        opt_row.addWidget(self.chk_preserve)

        # 提取参数
        adv_row_extract = QtWidgets.QHBoxLayout()
        self.extensions_edit = QtWidgets.QLineEdit()
        self.extensions_edit.setPlaceholderText(".h,.hpp,.c,.cpp")
        self.extensions_edit.setText(".h,.hpp,.c,.cpp")
        self.proc_mode_combo = QtWidgets.QComboBox()
        self.proc_mode_combo.addItems(["effective", "all"])  # --mode
        self.defines_edit = QtWidgets.QLineEdit()
        self.defines_edit.setPlaceholderText("示例：LASER_MODE_SELECT=1,FEATURE_X=ON")
        adv_row_extract.addWidget(QtWidgets.QLabel("扩展名"))
        adv_row_extract.addWidget(self.extensions_edit)
        adv_row_extract.addWidget(QtWidgets.QLabel("提取模式"))
        adv_row_extract.addWidget(self.proc_mode_combo)
        adv_row_extract.addWidget(QtWidgets.QLabel("宏定义"))
        adv_row_extract.addWidget(self.defines_edit)

        # CSV 输出
        csv_row = QtWidgets.QHBoxLayout()
        self.csv_out_edit = QtWidgets.QLineEdit()
        btn_csv_out = QtWidgets.QPushButton("输出CSV")
        btn_csv_out.clicked.connect(lambda: self.choose_file(self.csv_out_edit, "保存CSV", "CSV Files (*.csv)", save=True))
        csv_row.addWidget(QtWidgets.QLabel("CSV输出"))
        csv_row.addWidget(self.csv_out_edit)
        csv_row.addWidget(btn_csv_out)

        action_row = QtWidgets.QHBoxLayout()
        btn_extract = QtWidgets.QPushButton("从代码提取到CSV")
        btn_extract.clicked.connect(self.on_extract)
        action_row.addWidget(btn_extract)

        for row in (root_row, proj_row, opt_row, adv_row_extract, csv_row, action_row):
            layout.addLayout(row)

    def _build_emit_page(self, layout: QtWidgets.QVBoxLayout):
        # CSV 输入与 C 输出
        csv_row = QtWidgets.QHBoxLayout()
        self.csv_in_edit = QtWidgets.QLineEdit()
        btn_csv_in = QtWidgets.QPushButton("选择CSV")
        btn_csv_in.clicked.connect(lambda: self.choose_file(self.csv_in_edit, "CSV 文件", "CSV Files (*.csv)"))
        csv_row.addWidget(QtWidgets.QLabel("CSV输入"))
        csv_row.addWidget(self.csv_in_edit)
        csv_row.addWidget(btn_csv_in)

        c_row = QtWidgets.QHBoxLayout()
        self.c_out_edit = QtWidgets.QLineEdit()
        btn_c_out = QtWidgets.QPushButton("输出C文件")
        btn_c_out.clicked.connect(lambda: self.choose_file(self.c_out_edit, "保存C", "C Files (*.c)", save=True))
        c_row.addWidget(QtWidgets.QLabel("C输出"))
        c_row.addWidget(self.c_out_edit)
        c_row.addWidget(btn_c_out)

        # 生成选项
        opt_row1 = QtWidgets.QHBoxLayout()
        self.type_combo = QtWidgets.QComboBox()
        self.type_combo.addItems(["_Tr_TEXT"])  # 可扩展
        self.chk_preserve = QtWidgets.QCheckBox("提取保留转义 (--preserve-escapes)")
        self.chk_verbatim = QtWidgets.QCheckBox("CSV到C逐字输出 (--emit-verbatim)")
        self.chk_utf8 = QtWidgets.QCheckBox("使用UTF-8字面量 (--emit-utf8-literal)")
        self.chk_null = QtWidgets.QCheckBox("末尾使用NULL哨兵 (--emit-null-sentinel)")
        self.chk_no_static = QtWidgets.QCheckBox("不使用static (--emit-no-static)")
        opt_row1.addWidget(QtWidgets.QLabel("类型"))
        opt_row1.addWidget(self.type_combo)
        opt_row1.addWidget(self.chk_preserve)
        opt_row1.addWidget(self.chk_verbatim)
        opt_row1.addWidget(self.chk_utf8)
        opt_row1.addWidget(self.chk_null)
        opt_row1.addWidget(self.chk_no_static)

        # 头文件与注册表
        adv_row_emit = QtWidgets.QHBoxLayout()
        self.header_out_edit = QtWidgets.QLineEdit()
        self.header_out_edit.setPlaceholderText("可选：输出头文件路径")
        self.registry_name_edit = QtWidgets.QLineEdit()
        self.registry_name_edit.setPlaceholderText("可选：注册表数组名称，例如 TEXT_REGISTRY")
        self.chk_registry = QtWidgets.QCheckBox("生成注册表数组 (--emit-registry)")
        adv_row_emit.addWidget(QtWidgets.QLabel("头文件输出"))
        adv_row_emit.addWidget(self.header_out_edit)
        adv_row_emit.addWidget(QtWidgets.QLabel("注册表名称"))
        adv_row_emit.addWidget(self.registry_name_edit)
        adv_row_emit.addWidget(self.chk_registry)

        # 使用模式与统一入口
        mode_row = QtWidgets.QHBoxLayout()
        self.mode_combo = QtWidgets.QComboBox()
        self.mode_combo.addItems(["仅提取CSV", "仅生成C", "提取并生成"])
        self.mode_combo.setCurrentIndex(2)
        self.btn_run_all = QtWidgets.QPushButton("生成")
        self.btn_run_all.clicked.connect(self.on_generate)
        mode_row.addWidget(QtWidgets.QLabel("使用模式"))
        mode_row.addWidget(self.mode_combo)
        mode_row.addWidget(self.btn_run_all)

        for row in (csv_row, c_row, opt_row1, adv_row_emit, mode_row):
            layout.addLayout(row)

    # 拖拽进入：只要包含文件/目录URL就接受
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            event.ignore()

    # 放下：根据类型自动运行
    def dropEvent(self, event):
        import os
        urls = event.mimeData().urls()
        if not urls:
            return
        # 仅处理第一个条目
        p = urls[0].toLocalFile()
        if not p:
            return
        if os.path.isdir(p):
            # 拖入目录：作为根目录与项目目录，自动填充并一键运行
            self.root_edit.setText(p)
            self.project_edit.setText(p)
            self.autofill_outputs()
            self.log.append(f"检测到目录拖入：{p}，开始提取并生成...")
            self.on_run_all()
        elif p.lower().endswith('.csv'):
            # 拖入CSV：作为输入CSV，若无项目目录则使用其父目录
            self.csv_in_edit.setText(p)
            parent = os.path.dirname(p)
            if not self.project_edit.text().strip():
                self.project_edit.setText(parent)
            if not self.c_out_edit.text().strip():
                # 在项目目录下生成默认C文件
                self.c_out_edit.setText(os.path.join(self.project_edit.text().strip() or parent, "generated_text_vars_preserved.c"))
            self.log.append(f"检测到CSV拖入：{p}，开始从CSV生成C...")
            self.on_emit()
        else:
            self.log.append(f"拖入的路径不支持自动生成：{p}")

    # 轻量滑动切换（鼠标/触控）：按压-释放位移判断
    def mousePressEvent(self, event):
        self._press_pos = event.pos()
        self._press_time = QtCore.QTime.currentTime()
        return super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if hasattr(self, "_press_pos") and self._press_pos is not None:
            dx = event.pos().x() - self._press_pos.x()
            dt = self._press_time.msecsTo(QtCore.QTime.currentTime()) if hasattr(self, "_press_time") else 0
            if abs(dx) > 60 and dt < 500:
                if dx < 0:
                    self._next_tab()
                else:
                    self._prev_tab()
        self._press_pos = None
        self._press_time = None
        return super().mouseReleaseEvent(event)

    def on_extract(self):
        from gui_app.runner import build_extract_command
        root_path = self.project_edit.text().strip() or self.root_edit.text() or "."
        # 若未显式设置CSV输出，使用项目目录默认文件名
        if not self.csv_out_edit.text().strip() and self.project_edit.text().strip():
            self.csv_out_edit.setText(os.path.join(self.project_edit.text().strip(), "ty_text_out.csv"))
        defines = self._split_defines(self.defines_edit.text())
        cmd = build_extract_command(
            root=root_path,
            csv_out=self.csv_out_edit.text() or "ty_text_out.csv",
            type_name=self.type_combo.currentText(),
            preserve_escapes=self.chk_preserve.isChecked(),
            extensions=self.extensions_edit.text().strip() or None,
            mode=self.proc_mode_combo.currentText(),
            defines=defines if defines else None,
        )
        self.log.append("$ " + " ".join(cmd))
        self._start_process(cmd, tag="extract")

    def on_emit(self):
        from gui_app.runner import build_emit_command
        root_path = self.project_edit.text().strip() or self.root_edit.text() or "."
        # 若未显式设置C输出，使用项目目录默认文件名
        if not self.c_out_edit.text().strip() and self.project_edit.text().strip():
            self.c_out_edit.setText(os.path.join(self.project_edit.text().strip(), "generated_text_vars_preserved.c"))
        cmd = build_emit_command(
            csv_in=self.csv_in_edit.text() or "ty_text_preserved.csv",
            c_out=self.c_out_edit.text() or "generated_text_vars_preserved.c",
            type_name=self.type_combo.currentText(),
            verbatim=self.chk_verbatim.isChecked(),
            utf8_literal=self.chk_utf8.isChecked(),
            null_sentinel=self.chk_null.isChecked(),
            no_static=self.chk_no_static.isChecked(),
            root=root_path,
            csv_out_intermediate=self.csv_out_edit.text() or None,
            header_output=self.header_out_edit.text().strip() or None,
            registry_name=self.registry_name_edit.text().strip() or None,
            registry_emit=self.chk_registry.isChecked(),
        )
        self.log.append("$ " + " ".join(cmd))
        self._start_process(cmd, tag="emit")

    def _split_defines(self, text: str):
        items = []
        for part in (text or "").split(','):
            p = part.strip()
            if p:
                items.append(p)
        return items

    def on_generate(self):
        # 统一生成入口，根据模式执行
        mode = self.mode_combo.currentText()
        if mode == "仅提取CSV":
            self._run_all_active = False
            self.on_extract()
        elif mode == "仅生成C":
            self._run_all_active = False
            # 若未设置CSV输入但设置了项目目录，尝试使用项目目录下默认CSV
            if not self.csv_in_edit.text().strip() and self.project_edit.text().strip():
                default_csv = os.path.join(self.project_edit.text().strip(), "ty_text_out.csv")
                self.csv_in_edit.setText(default_csv)
            self.on_emit()
        else:
            self.on_run_all()

    def on_run_all(self):
        # 若未填写输出路径，则根据项目目录自动填充
        proj = self.project_edit.text().strip()
        if proj:
            if not self.csv_out_edit.text().strip():
                self.csv_out_edit.setText(os.path.join(proj, "ty_text_out.csv"))
            if not self.c_out_edit.text().strip():
                self.c_out_edit.setText(os.path.join(proj, "generated_text_vars_preserved.c"))
        self._run_all_active = True
        self.log.append("开始一键运行：先提取CSV，再从CSV生成C...")
        self.on_extract()

    def _start_process(self, cmd, tag=None):
        program = cmd[0]
        args = cmd[1:]
        self._current_tag = tag
        # 禁用交互，避免重复触发
        self.tabs.setEnabled(False)
        if hasattr(self, "btn_run_all"):
            self.btn_run_all.setEnabled(False)
        self.proc = QtCore.QProcess(self)
        self.proc.setProcessChannelMode(QtCore.QProcess.MergedChannels)
        self.proc.readyReadStandardOutput.connect(self._read_stream)
        self.proc.finished.connect(self._proc_finished)
        self.proc.start(program, args)

    def _read_stream(self):
        data = self.proc.readAllStandardOutput()
        text = bytes(data).decode(errors="ignore")
        if text:
            self.log.append(text)

    def _proc_finished(self):
        self.tabs.setEnabled(True)
        if hasattr(self, "btn_run_all"):
            self.btn_run_all.setEnabled(True)
        # 一键运行：提取完成后继续生成；生成完成后复位
        if self._run_all_active and self._current_tag == "extract":
            # 用刚生成的CSV作为输入
            if self.csv_out_edit.text().strip():
                self.csv_in_edit.setText(self.csv_out_edit.text().strip())
            self.log.append("提取完成，继续从CSV生成C...")
            self.on_emit()
            return
        if self._run_all_active and self._current_tag == "emit":
            self.log.append("一键运行完成。")
        self._run_all_active = False
        self._current_tag = None

    # 辅助：切换动画与快捷切页
    def _animate_current_page(self):
        w = self.tabs.currentWidget()
        if not w:
            return
        effect = QtWidgets.QGraphicsOpacityEffect(w)
        w.setGraphicsEffect(effect)
        anim = QtCore.QPropertyAnimation(effect, b"opacity", self)
        anim.setDuration(200)
        anim.setStartValue(0.0)
        anim.setEndValue(1.0)
        anim.start(QtCore.QAbstractAnimation.DeleteWhenStopped)

    def _next_tab(self):
        i = self.tabs.currentIndex()
        self.tabs.setCurrentIndex((i + 1) % self.tabs.count())

    def _prev_tab(self):
        i = self.tabs.currentIndex()
        self.tabs.setCurrentIndex((i - 1) % self.tabs.count())