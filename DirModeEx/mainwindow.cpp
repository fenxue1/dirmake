/**
 * @file mainwindow.cpp
 * @brief 主窗口实现（Main window implementation）
 *
 * 职责：项目浏览、CSV 文本提取与生成、语言列发现、CSV 翻译导入、日志展示。
 * Notes: 双语注释，核心用英文、辅助说明中文；不改变原始逻辑，仅增注释与日志。
 */
// DirModeEx: Windows 7 兼容的目录/文件管理示例
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSplitter>
#include <QTreeView>
#include <QTableView>
#include <QHeaderView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QShortcut>
#include <QSettings>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProcess>
#include <QProgressBar>
#include <QApplication>
#include <QDirIterator>
#include <QtConcurrent>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QFutureWatcher>
#include "text_extractor.h"
#include "language_settings.h"
#include "csv_lang_plugin.h"
#include "csv_parser.h"
#include <QMap>
#include <memory>

namespace {
struct ExtractMapFn {
    QString mode;
    QMap<QString, QString> defines;
    QString typeName;
    bool keepEsc;
    QList<ExtractedBlock> operator()(const QString &fpath) const {
        QList<ExtractedBlock> res;
        QString text = TextExtractor::readTextFile(fpath);
        QString t = (mode == QStringLiteral("effective")) ? TextExtractor::preprocess(text, defines) : text;
        auto b = TextExtractor::extractBlocks(t, fpath, typeName, keepEsc);
        res.append(b);
        return res;
    }
};
struct ExtractReduceFn {
    void operator()(QList<ExtractedBlock> &result, const QList<ExtractedBlock> &mapped) const {
        result.append(mapped);
    }
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    /* 构造机制 | Constructor: 初始化 UI、并发 watcher、信号槽与默认状态。*/
    ui->setupUi(this);
    // 初始化异步语言列发现 watcher
    m_langDiscoverWatcher = new QFutureWatcher<QStringList>(this);
    connect(m_langDiscoverWatcher, &QFutureWatcher<QStringList>::finished, this, [this]
            {
                QStringList langCols = m_langDiscoverWatcher->future().result();
                if (langCols.isEmpty())
                    langCols = TextExtractor::defaultLanguageColumns();
                applyLanguageChecks(langCols);
                QApplication::restoreOverrideCursor();
                statusBar()->clearMessage();
            });
    // 初始化提取流程的并发监视器与进度回调
    m_extractWatcher = new QFutureWatcher<QList<ExtractedBlock>>(this);
    connect(m_extractWatcher, &QFutureWatcher<QList<ExtractedBlock>>::progressRangeChanged, this, [this](int min, int max)
            {
                if (m_extractProgress)
                {
                    m_extractProgress->setMinimum(min);
                    m_extractProgress->setMaximum(max);
                }
            });
    connect(m_extractWatcher, &QFutureWatcher<QList<ExtractedBlock>>::progressValueChanged, this, [this](int value)
            {
                if (m_extractProgress)
                    m_extractProgress->setValue(value);
            });
    connect(m_extractWatcher, &QFutureWatcher<QList<ExtractedBlock>>::finished, this, [this]
            {
                // 1) 取出并发任务结果：所有提取到的块（未经筛选）
                QList<ExtractedBlock> all = m_extractWatcher->result();
                // 记录原始总数，便于新手观察流程统计
                m_lastTotalBlocks = all.size();
                log(QStringLiteral("[完成] 并发提取结束，原始块数=%1").arg(m_lastTotalBlocks));

                // 2) 如启用了“仅中文”，根据语言列位置筛选出含中文字符的条目
                if (m_extractChineseOnly)
                {
                    log(QStringLiteral("[筛选] 开始中文筛选（语言列=%1）").arg(m_extractLangCols.join(QStringLiteral(", "))));
                    auto hasChinese = [](const QString &s) {
                        // 使用 Unicode 范围匹配常用中文字符（基础汉字、扩展A、CJK符号）
                        static const QRegularExpression re(QStringLiteral("[\\x{3400}-\\x{4DBF}\\x{4E00}-\\x{9FFF}\\x{3000}-\\x{303F}]"));
                        return re.match(s).hasMatch();
                    };
                    // 找到中文列的索引（如 *_cn、*_zh、*_chs、*_hans）
                    QList<int> cnIdx;
                    for (int i = 0; i < m_extractLangCols.size(); ++i)
                    {
                        const QString col = m_extractLangCols.at(i).toLower();
                        if (col.endsWith(QLatin1String("_cn")) || col.endsWith(QLatin1String("_zh")) || col.endsWith(QLatin1String("_chs")) || col.endsWith(QLatin1String("_hans")))
                            cnIdx.push_back(i);
                    }
                    QList<ExtractedBlock> filtered;
                    for (const auto &r : all)
                    {
                        bool keep = cnIdx.isEmpty(); // 若未发现中文列，则不过滤
                        for (int idx : cnIdx)
                        {
                            if (idx >= 0 && idx < r.strings.size())
                            {
                                if (hasChinese(r.strings.at(idx))) { keep = true; break; }
                            }
                        }
                        if (keep) filtered.push_back(r);
                    }
                    all.swap(filtered);
                    m_lastFilteredBlocks = all.size();
                    log(QStringLiteral("[筛选] 中文筛选完成，保留块数=%1").arg(m_lastFilteredBlocks));
                }

                // 3) 写入 CSV 文件（包含头、UTF-8 BOM、列处理）
                log(QStringLiteral("[写入] 写入CSV：%1（列=%2；直写列=%3；替换英文逗号=%4）")
                        .arg(m_extractOutCsv)
                        .arg(m_extractLangCols.join(QStringLiteral(", ")))
                        .arg(m_extractLiteralCols.join(QStringLiteral(", ")))
                        .arg(m_extractReplaceCommaFlag ? QStringLiteral("是") : QStringLiteral("否")));
                bool ok = TextExtractor::writeCsv(m_extractOutCsv, all, m_extractLangCols, m_extractLiteralCols, m_extractReplaceCommaFlag);

                // 4) 收尾：恢复光标与状态栏、更新进度条
                QApplication::restoreOverrideCursor();
                statusBar()->clearMessage();
                if (m_extractProgress)
                    m_extractProgress->setValue(m_extractProgress->maximum());

                // 5) 结果提示：根据是否中文模式显示不同统计
                if (ok)
                {
                    if (m_extractChineseOnly)
                    {
                        QFileInfo fi(m_extractOutCsv);
                        QDir outDir = fi.dir();
                        QString base = fi.completeBaseName();
                        QStringList parts = outDir.entryList(QStringList{QString("%1_part*.csv").arg(base)}, QDir::Files, QDir::Name);
                        if (!parts.isEmpty())
                        {
                            log(QStringLiteral("中文提取完成：已分片 %1 个文件，目录=%2").arg(parts.size()).arg(outDir.absolutePath()));
                            QMessageBox::information(this, QStringLiteral("中文提取完成"), QStringLiteral("CSV 已分片为 %1 个文件，目录：%2\n中文条目数：%3\n原始块数：%4")
                                                         .arg(parts.size())
                                                         .arg(outDir.absolutePath())
                                                         .arg(all.size())
                                                         .arg(m_lastTotalBlocks));
                        }
                        else
                        {
                            log(QStringLiteral("中文提取完成：%1，共 %2 项").arg(m_extractOutCsv).arg(all.size()));
                            QMessageBox::information(this, QStringLiteral("中文提取完成"), QStringLiteral("CSV 已生成：%1\n中文条目数：%2\n原始块数：%3")
                                                         .arg(m_extractOutCsv)
                                                         .arg(all.size())
                                                         .arg(m_lastTotalBlocks));
                        }
                        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_extractOutCsv).dir().absolutePath()));
                    }
                    else
                    {
                        QFileInfo fi(m_extractOutCsv);
                        QDir outDir = fi.dir();
                        QString base = fi.completeBaseName();
                        QStringList parts = outDir.entryList(QStringList{QString("%1_part*.csv").arg(base)}, QDir::Files, QDir::Name);
                        if (!parts.isEmpty())
                        {
                            log(QStringLiteral("提取完成：已分片 %1 个文件，目录=%2").arg(parts.size()).arg(outDir.absolutePath()));
                            QMessageBox::information(this, QStringLiteral("提取完成"), QStringLiteral("CSV 已分片为 %1 个文件，目录：%2\n共提取 %3 项")
                                                         .arg(parts.size())
                                                         .arg(outDir.absolutePath())
                                                         .arg(all.size()));
                        }
                        else
                        {
                            log(QStringLiteral("提取完成：%1，共 %2 项").arg(m_extractOutCsv).arg(all.size()));
                            QMessageBox::information(this, QStringLiteral("提取完成"), QStringLiteral("CSV 已生成：%1\n共提取 %2 项")
                                                         .arg(m_extractOutCsv)
                                                         .arg(all.size()));
                        }
                        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_extractOutCsv).dir().absolutePath()));
                    }
                }
                else
                {
                    QMessageBox::critical(this, QStringLiteral("提取失败"), QStringLiteral("写入CSV失败，请检查路径权限。"));
                }
            });
    setupUiContent();
}

MainWindow::~MainWindow()
{
    if (m_logStream)
    {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile)
    {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    delete ui;
}

void MainWindow::setupUiContent()
{
    setWindowTitle(QStringLiteral("DirModeEx - C++/Qt (Win7)"));

    // 日志文件
    QDir().mkpath(QStringLiteral("logs"));
    m_logFile = new QFile(QStringLiteral("logs/dir_app.log"), this);
    if (m_logFile->open(QIODevice::Append | QIODevice::Text))
    {
        m_logStream = new QTextStream(m_logFile);
        log(QStringLiteral("=== 应用启动 ==="));
    }

    // 顶部路径栏与工具栏
    m_toolbar = addToolBar(QStringLiteral("操作"));
    m_toolbar->setMovable(false);
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(QStringLiteral("输入路径并回车，例如 C:/Users"));
    connect(m_pathEdit, SIGNAL(returnPressed()), this, SLOT(onPathReturnPressed()));
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    actRefresh = m_toolbar->addAction(QStringLiteral("刷新"), this, SLOT(onRefresh()));
    actNewFolder = m_toolbar->addAction(QStringLiteral("新建文件夹"), this, SLOT(onNewFolder()));
    actRename = m_toolbar->addAction(QStringLiteral("重命名"), this, SLOT(onRename()));
    actDelete = m_toolbar->addAction(QStringLiteral("删除"), this, SLOT(onDelete()));
    actCopy = m_toolbar->addAction(QStringLiteral("复制到..."), this, SLOT(onCopy()));
    actMove = m_toolbar->addAction(QStringLiteral("移动到..."), this, SLOT(onMove()));
    m_toolbar->addWidget(spacer);
    m_toolbar->addWidget(m_pathEdit);

    // 中心区域：目录树 + 文件表
    QSplitter *splitter = new QSplitter(this);
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_dirModel = new QFileSystemModel(splitter);
    m_dirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    m_dirModel->setReadOnly(false);
    m_dirModel->setRootPath(QLatin1String(""));

    m_fileModel = new QFileSystemModel(splitter);
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDot);
    m_fileModel->setReadOnly(false);
    m_fileModel->setRootPath(QLatin1String(""));

    m_tree = new QTreeView(splitter);
    m_tree->setModel(m_dirModel);
    m_tree->setHeaderHidden(true);
    m_tree->setAnimated(true);
    m_tree->setIndentation(16);

    m_table = new QTableView(splitter);
    m_table->setModel(m_fileModel);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSortingEnabled(true);
    connect(m_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onTableDoubleClicked(QModelIndex)));

    // 同步选择更改
    connect(m_tree->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(onTreeSelectionChanged(QModelIndex, QModelIndex)));

    // 将现有目录树+文件表嵌入“提取CSV”页
    QWidget *extractPage = new QWidget(this);
    QVBoxLayout *extractLayout = new QVBoxLayout(extractPage);
    // 顶部提示与浏览按钮
    QWidget *extractTop = new QWidget(extractPage);
    QHBoxLayout *extractTopLayout = new QHBoxLayout(extractTop);
    QLabel *extractHint = new QLabel(QStringLiteral("选择项目目录，配置参数后执行提取"), extractTop);
    m_browseProjectBtn = new QPushButton(QStringLiteral("浏览..."), extractTop);
    connect(m_browseProjectBtn, &QPushButton::clicked, this, [this]
            {
        QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择项目目录"), QDir::homePath());
        if (!dir.isEmpty()) { m_pathEdit->setText(dir); setCurrentPath(dir); } });
    extractTopLayout->addWidget(extractHint);
    extractTopLayout->addStretch();
    extractTopLayout->addWidget(m_browseProjectBtn);
    extractLayout->addWidget(extractTop);
    // 主体 splitter
    extractLayout->addWidget(splitter, 1);
    // 选项与执行按钮
    QWidget *extractBottom = new QWidget(extractPage);
    QVBoxLayout *extractBottomLayout = new QVBoxLayout(extractBottom);
    // 行1：类型、模式、保留转义
    QWidget *exRow1 = new QWidget(extractBottom);
    QHBoxLayout *exH1 = new QHBoxLayout(exRow1);
    m_extractTypeCombo = new QComboBox(exRow1);
    m_extractTypeCombo->addItems(QStringList{QStringLiteral("动态文本")});
    m_extractModeCombo = new QComboBox(exRow1);
    m_extractModeCombo->addItems(QStringList{QStringLiteral("effective"), QStringLiteral("all")});
    m_extractKeepEscapes = new QCheckBox(QStringLiteral("保留转义字符"), exRow1);
    m_extractKeepEscapes->setChecked(true);
    m_extractKeepEscapes->setToolTip(QStringLiteral("提取时保留原始转义，如\\xNN、\\n、\\t 等"));
    exH1->addWidget(new QLabel(QStringLiteral("类型:")));
    exH1->addWidget(m_extractTypeCombo);
    exH1->addSpacing(12);
    exH1->addWidget(new QLabel(QStringLiteral("模式:")));
    exH1->addWidget(m_extractModeCombo);
    exH1->addSpacing(12);
    exH1->addWidget(m_extractKeepEscapes);
    exH1->addStretch();
    // 行2：扩展名、宏定义、执行按钮
    QWidget *exRow2 = new QWidget(extractBottom);
    QHBoxLayout *exH2 = new QHBoxLayout(exRow2);
    m_extractExtsEdit = new QLineEdit(exRow2);
    m_extractExtsEdit->setPlaceholderText(QStringLiteral(".h,.hpp,.c,.cpp"));
    m_extractExtsEdit->setText(QStringLiteral(".h,.hpp,.c,.cpp"));
    m_extractDefinesEdit = new QLineEdit(exRow2);
    m_extractDefinesEdit->setPlaceholderText(QStringLiteral("示例: FOO=1;BAR;BAZ=hello"));
    m_extractRunBtn = new QPushButton(QStringLiteral("从代码提取到CSV"), exRow2);
    connect(m_extractRunBtn, SIGNAL(clicked()), this, SLOT(onExtractRun()));
    m_extractChineseBtn = new QPushButton(QStringLiteral("一键提取中文"), exRow2);
    connect(m_extractChineseBtn, SIGNAL(clicked()), this, SLOT(onExtractChinese()));
    QPushButton *m_extractArraysBtn = new QPushButton(QStringLiteral("提取结构体数组"), exRow2);
    QPushButton *m_readErrorsBtn = new QPushButton(QStringLiteral("读取报错"), exRow2);
    connect(m_extractArraysBtn, SIGNAL(clicked()), this, SLOT(onExtractArrays()));
    connect(m_readErrorsBtn, SIGNAL(clicked()), this, SLOT(onReadErrors()));
    m_extractProgress = new QProgressBar(exRow2);
    m_extractProgress->setMinimum(0);
    m_extractProgress->setMaximum(100);
    m_extractProgress->setValue(0);
    exH2->addWidget(new QLabel(QStringLiteral("扩展:")));
    exH2->addWidget(m_extractExtsEdit, 1);
    exH2->addSpacing(12);
    exH2->addWidget(new QLabel(QStringLiteral("宏定义:")));
    exH2->addWidget(m_extractDefinesEdit, 1);
    exH2->addStretch();
    exH2->addWidget(m_extractProgress);
    exH2->addWidget(m_extractRunBtn);
    exH2->addWidget(m_extractChineseBtn);
    exH2->addWidget(m_extractArraysBtn);
    exH2->addWidget(m_readErrorsBtn);
    // 行3：保留原文语言列配置
    QWidget *exRow3 = new QWidget(extractBottom);
    QHBoxLayout *exH3 = new QHBoxLayout(exRow3);
    m_extractUtf8Literal = new QCheckBox(QStringLiteral("仅指定语言保留原文，其余写为\\xNN"), exRow3);
    m_extractUtf8Literal->setChecked(true);
    m_extractUtf8ColsEdit = new QLineEdit(exRow3);
    m_extractUtf8ColsEdit->setPlaceholderText(QStringLiteral("例如: text_cn,text_en"));
    m_extractUtf8ColsEdit->setText(QStringLiteral("text_cn,text_en"));
    m_extractReplaceComma = new QCheckBox(QStringLiteral("替换英文逗号为中文逗号"), exRow3);
    m_extractReplaceComma->setToolTip(QStringLiteral("将文本中的 , 替换为 ，，避免CSV处理时误分列或换行"));
    m_extractReplaceComma->setChecked(true);
    exH3->addWidget(m_extractUtf8Literal);
    exH3->addSpacing(12);
    exH3->addWidget(new QLabel(QStringLiteral("保留原文语言列:")));
    exH3->addWidget(m_extractUtf8ColsEdit, 1);
    exH3->addSpacing(12);
    exH3->addWidget(m_extractReplaceComma);
    extractBottomLayout->addWidget(exRow1);
    extractBottomLayout->addWidget(exRow2);
    extractBottomLayout->addWidget(exRow3);
    // 动态语言复选框容器（用于选择保留原文语言）
    m_extractLangBox = new QWidget(extractBottom);
    {
        QVBoxLayout *langLay = new QVBoxLayout(m_extractLangBox);
        langLay->setContentsMargins(0, 0, 0, 0);
        langLay->addWidget(new QLabel(QStringLiteral("选择保留原文语言:")));
        QScrollArea *scroll = new QScrollArea(m_extractLangBox);
        scroll->setWidgetResizable(true);
        QWidget *gridContainer = new QWidget(scroll);
        gridContainer->setObjectName("langGridContainer");
        QGridLayout *grid = new QGridLayout(gridContainer);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(6);
        scroll->setWidget(gridContainer);
    langLay->addWidget(scroll);
    }
    extractBottomLayout->addWidget(m_extractLangBox);
    extractLayout->addWidget(extractBottom);
    m_tabs->addTab(extractPage, QStringLiteral("提取CSV"));

    // “从CSV生成C”页
    QWidget *genPage = new QWidget(this);
    QVBoxLayout *genLayout = new QVBoxLayout(genPage);
    QWidget *genRow1 = new QWidget(genPage);
    QHBoxLayout *genH1 = new QHBoxLayout(genRow1);
    m_csvInputEdit = new QLineEdit(genPage);
    m_browseCsvBtn = new QPushButton(QStringLiteral("选择CSV..."), genPage);
    connect(m_browseCsvBtn, &QPushButton::clicked, this, [this]
            {
        QString f = QFileDialog::getOpenFileName(this, QStringLiteral("选择CSV"), QDir::homePath(), QStringLiteral("CSV (*.csv)"));
        if (!f.isEmpty()) m_csvInputEdit->setText(f); });
    genH1->addWidget(new QLabel(QStringLiteral("CSV 输入:")));
    genH1->addWidget(m_csvInputEdit, 1);
    genH1->addWidget(m_browseCsvBtn);
    QWidget *genRow2 = new QWidget(genPage);
    QHBoxLayout *genH2 = new QHBoxLayout(genRow2);
    m_cOutputEdit = new QLineEdit(genPage);
    m_browseCOutBtn = new QPushButton(QStringLiteral("选择C输出..."), genPage);
    connect(m_browseCOutBtn, &QPushButton::clicked, this, [this]
            {
        QString f = QFileDialog::getSaveFileName(this, QStringLiteral("选择C输出"), QDir::homePath()+QStringLiteral("/generated.c"), QStringLiteral("C (*.c)"));
        if (!f.isEmpty()) m_cOutputEdit->setText(f); });
    genH2->addWidget(new QLabel(QStringLiteral("C 输出:")));
    genH2->addWidget(m_cOutputEdit, 1);
    genH2->addWidget(m_browseCOutBtn);
    // 头文件输出路径
    QWidget *genRow2b = new QWidget(genPage);
    QHBoxLayout *genH2b = new QHBoxLayout(genRow2b);
    m_headerOutputEdit = new QLineEdit(genPage);
    m_browseHeaderOutBtn = new QPushButton(QStringLiteral("选择头文件输出..."), genPage);
    connect(m_browseHeaderOutBtn, &QPushButton::clicked, this, [this]
            {
        QString f = QFileDialog::getSaveFileName(this, QStringLiteral("选择头文件输出"), QDir::homePath()+QStringLiteral("/generated_text_vars_dynamic.h"), QStringLiteral("Header (*.h)"));
        if (!f.isEmpty()) m_headerOutputEdit->setText(f); });
    genH2b->addWidget(new QLabel(QStringLiteral("头文件输出:")));
    genH2b->addWidget(m_headerOutputEdit, 1);
    genH2b->addWidget(m_browseHeaderOutBtn);
    // 生成选项行
    QWidget *genOpts1 = new QWidget(genPage);
    QHBoxLayout *genO1 = new QHBoxLayout(genOpts1);
    m_genNoStatic = new QCheckBox("去除static", genOpts1);
    m_genEmitRegistry = new QCheckBox("生成注册表数组", genOpts1);
    m_genRegistryNameEdit = new QLineEdit(genOpts1);
    m_genRegistryNameEdit->setPlaceholderText(QStringLiteral("注册表数组名称，如 text_registry"));
    genO1->addWidget(m_genNoStatic);
    genO1->addSpacing(12);
    genO1->addWidget(m_genEmitRegistry);
    genO1->addWidget(m_genRegistryNameEdit, 1);

    QWidget *genOpts2 = new QWidget(genPage);
    QHBoxLayout *genO2 = new QHBoxLayout(genOpts2);
    m_genUtf8Literal = new QCheckBox("UTF-8直写", genOpts2);
    m_genUtf8ColsEdit = new QLineEdit(genOpts2);
    m_genUtf8ColsEdit->setPlaceholderText(QStringLiteral("直写列名（逗号分隔），如 cn,en"));
    m_genNullSentinel = new QCheckBox("末尾NULL哨兵", genOpts2);
    m_genNullSentinel->setChecked(true);
    m_genVerbatim = new QCheckBox("保留原样(不转为十六进制)", genOpts2);
    m_genVerbatim->setChecked(true);
    m_genFillMissingWithEnglish = new QCheckBox("缺失语言用英文填充", genOpts2);
    m_genFillMissingWithEnglish->setChecked(true);
    genO2->addWidget(m_genUtf8Literal);
    genO2->addWidget(m_genUtf8ColsEdit, 1);
    genO2->addSpacing(12);
    genO2->addWidget(m_genNullSentinel);
    genO2->addSpacing(12);
    genO2->addWidget(m_genVerbatim);
    genO2->addSpacing(12);
    genO2->addWidget(m_genFillMissingWithEnglish);

    QWidget *genOpts3 = new QWidget(genPage);
    QHBoxLayout *genO3 = new QHBoxLayout(genOpts3);
    m_genAnnotateCombo = new QComboBox(genOpts3);
    m_genAnnotateCombo->addItems({"none", "names", "indices"});
    m_genAnnotateCombo->setCurrentText("indices");
    m_genPerLineSpin = new QSpinBox(genOpts3);
    m_genPerLineSpin->setRange(1, 12);
    m_genPerLineSpin->setValue(1);
    genO3->addWidget(new QLabel(QStringLiteral("注释模式:")));
    genO3->addWidget(m_genAnnotateCombo);
    genO3->addSpacing(12);
    genO3->addWidget(new QLabel(QStringLiteral("每行条目:")));
    genO3->addWidget(m_genPerLineSpin);

    m_generateRunBtn = new QPushButton(QStringLiteral("生成"), genPage);
    connect(m_generateRunBtn, SIGNAL(clicked()), this, SLOT(onGenerateRun()));
    genLayout->addWidget(genRow1);
    genLayout->addWidget(genRow2);
    genLayout->addWidget(genOpts1);
    genLayout->addWidget(genOpts2);
    genLayout->addWidget(genOpts3);
    genLayout->addWidget(genRow2b);
    genLayout->addStretch();
    genLayout->addWidget(m_generateRunBtn);
    m_tabs->addTab(genPage, "从CSV生成C");

    // “CSV 翻译导入”页
    QWidget *csvPage = new QWidget(this);
    QVBoxLayout *csvLayout = new QVBoxLayout(csvPage);
    QWidget *csvRow1 = new QWidget(csvPage);
    QHBoxLayout *csvH1 = new QHBoxLayout(csvRow1);
    m_csvProjEdit = new QLineEdit(csvRow1);
    m_csvProjEdit->setPlaceholderText(QStringLiteral("选择项目根目录"));
    m_csvProjBrowseBtn = new QPushButton(QStringLiteral("浏览..."), csvRow1);
    connect(m_csvProjBrowseBtn, SIGNAL(clicked()), this, SLOT(onBrowseCsvProject()));
    csvH1->addWidget(new QLabel(QStringLiteral("项目根目录:")));
    csvH1->addWidget(m_csvProjEdit, 1);
    csvH1->addWidget(m_csvProjBrowseBtn);
    QWidget *csvRow2 = new QWidget(csvPage);
    QHBoxLayout *csvH2 = new QHBoxLayout(csvRow2);
    m_csvFileEdit = new QLineEdit(csvRow2);
    m_csvFileEdit->setPlaceholderText(QStringLiteral("选择CSV文件"));
    m_csvFileBrowseBtn = new QPushButton(QStringLiteral("选择CSV..."), csvRow2);
    connect(m_csvFileBrowseBtn, SIGNAL(clicked()), this, SLOT(onBrowseCsvFile()));
    csvH2->addWidget(new QLabel(QStringLiteral("CSV 文件:")));
    csvH2->addWidget(m_csvFileEdit, 1);
    csvH2->addWidget(m_csvFileBrowseBtn);
    QWidget *csvRow3 = new QWidget(csvPage);
    QHBoxLayout *csvH3 = new QHBoxLayout(csvRow3);
    m_csvProgress = new QProgressBar(csvRow3);
    m_csvProgress->setMinimum(0);
    m_csvProgress->setMaximum(100);
    m_csvProgress->setValue(0);
    m_csvRunBtn = new QPushButton(QStringLiteral("执行导入"), csvRow3);
    m_csvRunArraysBtn = new QPushButton(QStringLiteral("仅导入结构体数组"), csvRow3);
    QPushButton *m_csvRunErrorsBtn = new QPushButton(QStringLiteral("仅导入报错文本"), csvRow3);
    m_csvExportLogBtn = new QPushButton(QStringLiteral("导出日志"), csvRow3);
    connect(m_csvRunBtn, SIGNAL(clicked()), this, SLOT(onRunCsvImport()));
    connect(m_csvRunArraysBtn, SIGNAL(clicked()), this, SLOT(onRunArrayCsvImport()));
    connect(m_csvRunErrorsBtn, SIGNAL(clicked()), this, SLOT(onRunErrorsCsvImport()));
    connect(m_csvExportLogBtn, SIGNAL(clicked()), this, SLOT(onExportCsvLog()));
    csvH3->addWidget(new QLabel(QStringLiteral("进度:")));
    csvH3->addWidget(m_csvProgress, 1);
    csvH3->addStretch();
    csvH3->addWidget(m_csvRunBtn);
    csvH3->addWidget(m_csvRunArraysBtn);
    csvH3->addWidget(m_csvRunErrorsBtn);
    csvH3->addWidget(m_csvExportLogBtn);
    m_csvReportView = new QTextEdit(csvPage);
    m_csvReportView->setReadOnly(true);
    csvLayout->addWidget(csvRow1);
    csvLayout->addWidget(csvRow2);
    csvLayout->addWidget(csvRow3);
    csvLayout->addWidget(m_csvReportView, 1);
    m_tabs->addTab(csvPage, "CSV翻译导入");

    // “项目设置：语言”页
    QWidget *projPage = new QWidget(this);
    QVBoxLayout *projLayout = new QVBoxLayout(projPage);
    QWidget *prTop = new QWidget(projPage);
    QHBoxLayout *prH1 = new QHBoxLayout(prTop);
    m_projRootEdit = new QLineEdit(prTop);
    m_projRootEdit->setPlaceholderText(QStringLiteral("选择项目根目录，例如 e:/code/project"));
    m_projBrowseRootBtn = new QPushButton(QStringLiteral("浏览..."), prTop);
    connect(m_projBrowseRootBtn, SIGNAL(clicked()), this, SLOT(onBrowseProjectRoot()));
    prH1->addWidget(new QLabel(QStringLiteral("项目根目录:")));
    prH1->addWidget(m_projRootEdit, 1);
    prH1->addWidget(m_projBrowseRootBtn);
    QWidget *prRow2 = new QWidget(projPage);
    QHBoxLayout *prH2 = new QHBoxLayout(prRow2);
    m_projNewLangEdit = new QLineEdit(prRow2);
    m_projNewLangEdit->setPlaceholderText(QStringLiteral("新语言代码，例如: fr"));
    m_projInitLangBtn = new QPushButton(QStringLiteral("一键初始化新语言"), prRow2);
    connect(m_projInitLangBtn, SIGNAL(clicked()), this, SLOT(onInitNewLanguage()));
    m_projUndoBtn = new QPushButton(QStringLiteral("撤销上次初始化"), prRow2);
    connect(m_projUndoBtn, SIGNAL(clicked()), this, SLOT(onUndoLastInit()));
    m_projFillEnglishBtn = new QPushButton(QStringLiteral("一键补齐英文缺失项"), prRow2);
    connect(m_projFillEnglishBtn, SIGNAL(clicked()), this, SLOT(onFillEnglishMissing()));
    prH2->addWidget(new QLabel(QStringLiteral("语言代码:")));
    prH2->addWidget(m_projNewLangEdit, 1);
    prH2->addWidget(m_projInitLangBtn);
    prH2->addWidget(m_projUndoBtn);
    prH2->addWidget(m_projFillEnglishBtn);
    QWidget *prRow3 = new QWidget(projPage);
    QHBoxLayout *prH3 = new QHBoxLayout(prRow3);
    m_projLogLabel = new QLabel(QStringLiteral("日志: logs/lang_init.log"), prRow3);
    prH3->addWidget(m_projLogLabel);
    prH3->addStretch();
    projLayout->addWidget(new QLabel(QStringLiteral("新增语言将插入在 p_text_other 之前，类型与约束保持一致，并以英文为模板初始化（保留格式与占位符），同时标记为待翻译。支持批量处理与撤销。")));
    projLayout->addWidget(prTop);
    projLayout->addWidget(prRow2);
    projLayout->addWidget(prRow3);
    projLayout->addStretch();
    m_tabs->addTab(projPage, "项目设置");

    // “设置”页（显示文档）
    QWidget *settingsPage = new QWidget(this);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    m_settingsInfo = new QTextEdit(settingsPage);
    m_settingsInfo->setReadOnly(true);
    QFile docFile("../docs/ui_paging.md");
    if (docFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_settingsInfo->setPlainText(QString::fromUtf8(docFile.readAll()));
        docFile.close();
    }
    else
    {
        m_settingsInfo->setPlainText("分页式界面说明未找到，路径 ../docs/ui_paging.md");
    }
    settingsLayout->addWidget(m_settingsInfo);
    m_tabs->addTab(settingsPage, "设置");

    // “日志”页
    QWidget *logsPage = new QWidget(this);
    QVBoxLayout *logsLayout = new QVBoxLayout(logsPage);
    m_logView = new QTextEdit(logsPage);
    m_logView->setReadOnly(true);
    logsLayout->addWidget(m_logView);
    m_tabs->addTab(logsPage, "日志");

    // 快捷键（Qt5 需通过信号连接激活事件）
    QShortcut *scNext = new QShortcut(QKeySequence("Ctrl+Tab"), this);
    QObject::connect(scNext, &QShortcut::activated, this, [this]
                     { m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count()); });
    QShortcut *scPrev = new QShortcut(QKeySequence("Ctrl+Shift+Tab"), this);
    QObject::connect(scPrev, &QShortcut::activated, this, [this]
                     { m_tabs->setCurrentIndex((m_tabs->currentIndex() - 1 + m_tabs->count()) % m_tabs->count()); });

    // 面包屑（标题随页签变化）
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx)
            {
        setWindowTitle(QString("DirModeEx - %1").arg(m_tabs->tabText(idx)));
        QSettings s("DirModeEx", "DirApp");
        s.setValue("lastTab", idx); });

    // 允许拖拽进入
    setAcceptDrops(true);

    // 恢复上次页签
    QSettings s("DirModeEx", "DirApp");
    int last = s.value("lastTab", 0).toInt();
    if (last >= 0 && last < m_tabs->count())
        m_tabs->setCurrentIndex(last);

    // 初始路径
    setCurrentPath(QDir::homePath());
}

void MainWindow::log(const QString &msg)
{
    statusBar()->showMessage(msg, 3000);
    if (m_logView)
    {
        m_logView->append(msg);
    }
    if (m_logStream)
    {
        (*m_logStream) << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
                       << QStringLiteral(" | ") << msg << QStringLiteral("\n");
        m_logStream->flush();
    }
}

/**
 * @brief 设置当前浏览路径并刷新目录与文件视图
 * @param path 目标路径
 * 功能：同步树视图与表格根索引、更新路径输入框、触发语言列复选框刷新与日志。
 */
void MainWindow::setCurrentPath(const QString &path)
{
    QModelIndex dirIndex = m_dirModel->index(path);
    if (dirIndex.isValid())
    {
        m_tree->setCurrentIndex(dirIndex);
        m_tree->expand(dirIndex);
        m_table->setRootIndex(m_fileModel->setRootPath(path));
        m_pathEdit->setText(path);
        log(QStringLiteral("进入目录: ") + path);
        // 根据新路径刷新“保留原文语言”复选框
        refreshExtractLanguageChecks();
    }
    else
    {
        QMessageBox::warning(this, QStringLiteral("路径无效"), QStringLiteral("无法访问路径：%1").arg(path));
    }
}

/**
 * @brief 获取当前目录树选中路径
 * @return 绝对路径
 * 说明：用于文件操作与提取流程的默认根。
 */
QString MainWindow::currentDirPath() const
{
    QModelIndex idx = m_tree->currentIndex();
    return m_dirModel->filePath(idx);
}

/**
 * @brief 获取文件表中选中的项目的绝对路径列表
 * @return 路径列表
 * 用于复制、移动、删除等批量操作。
 */
QStringList MainWindow::selectedFilePaths() const
{
    QStringList paths;
    foreach (const QModelIndex &idx, m_table->selectionModel()->selectedRows())
    {
        paths << m_fileModel->filePath(idx);
    }
    return paths;
}

/**
 * @brief 回车设置路径
 * 行为：读取路径输入框内容，调用 setCurrentPath。
 */
void MainWindow::onPathReturnPressed()
{
    setCurrentPath(m_pathEdit->text());
}

/**
 * @brief 目录树选择变化
 * 行为：更新文件表根索引与路径输入框，保持浏览同步。
 */
void MainWindow::onTreeSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    const QString path = m_dirModel->filePath(current);
    m_table->setRootIndex(m_fileModel->setRootPath(path));
    m_pathEdit->setText(path);
}

/**
 * @brief 文件表双击行为
 * 目录则进入；文件则记录日志，留待后续打开。
 */
void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    /* 表格双击 | Double-click behavior: 目录进入；文件留作后续打开/预览。*/
    const QString p = m_fileModel->filePath(index);
    QFileInfo info(p);
    if (info.isDir())
    {
        setCurrentPath(p);
    }
    else
    {
        // 打开文件所在目录并选择
        log(QStringLiteral("选中文件：%1").arg(p));
    }
}

/**
 * @brief 刷新文件系统视图
 * 通过重设根路径触发 QFileSystemModel 重新加载。
 */
void MainWindow::onRefresh()
{
    const QString p = currentDirPath();
    // QFileSystemModel 在 Qt5 中没有 refresh()，通过重设 rootPath 触发重载
    m_dirModel->setRootPath(m_dirModel->rootPath());
    m_fileModel->setRootPath(m_fileModel->rootPath());
    setCurrentPath(p);
    log(QStringLiteral("已刷新"));
}

/**
 * @brief 从源码提取到 CSV 的常规流程
 * 步骤：解析扩展与宏→发现语言列→收集文件→并发提取→写入 CSV →提示
 */
void MainWindow::onExtractRun()
{
    // 入口：普通提取（非中文专用）。逐步输出，帮助新手理解流程。
    const QString dir = m_pathEdit ? m_pathEdit->text() : QString();          // 当前目录输入框内容
    const QString type = m_extractTypeCombo ? m_extractTypeCombo->currentText() : QString(); // 结构体类型选择（可选）
    const bool keepEsc = m_extractKeepEscapes && m_extractKeepEscapes->isChecked(); // 是否保留字符串中的转义符
    if (dir.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提取"), QStringLiteral("请选择或输入项目根目录。"));
        return;
    }
    const QString outCsv = QDir(dir).absoluteFilePath(QStringLiteral("ty_text_out.csv")); // 输出文件名（常规）
    const QString mode = m_extractModeCombo ? m_extractModeCombo->currentText() : QStringLiteral("effective"); // 解析模式（effective/everything）
    log(QStringLiteral("[提取] 目录=%1, 输出=%2, 模式=%3, 保留转义=%4").arg(dir, outCsv, mode).arg(keepEsc ? QStringLiteral("是") : QStringLiteral("否")));
    // 扩展名
    QStringList exts;
    if (m_extractExtsEdit && !m_extractExtsEdit->text().trimmed().isEmpty())
    {
        for (const QString &p : m_extractExtsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
        {
            exts << p.trimmed();
        }
    }
    if (exts.isEmpty())
        exts = QStringList{QLatin1String(".h"), QLatin1String(".hpp"), QLatin1String(".c"), QLatin1String(".cpp")};
    log(QStringLiteral("[提取] 使用扩展名过滤：%1").arg(exts.join(QStringLiteral(", "))));
    // 宏定义解析 A=1;B;C=hello
    QMap<QString, QString> defines;
    if (m_extractDefinesEdit && !m_extractDefinesEdit->text().trimmed().isEmpty())
    {
        for (const QString &item : m_extractDefinesEdit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts))
        {
            QString t = item.trimmed();
            if (t.isEmpty())
                continue;
            int eq = t.indexOf('=');
            if (eq >= 0)
            {
                defines.insert(t.left(eq).trimmed(), t.mid(eq + 1).trimmed());
            }
            else
            {
                defines.insert(t, QString());
            }
        }
    }
    if (!defines.isEmpty())
        log(QStringLiteral("[提取] 宏定义：%1").arg(QStringList(defines.keys()).join(QStringLiteral(", "))));
    const QString typeName = "_Tr_TEXT";
    QStringList langCols = TextExtractor::discoverLanguageColumns(dir, exts, typeName); // 自动发现项目中的语言列
    log(QStringLiteral("[提取] 发现语言列：%1").arg(langCols.join(QStringLiteral(", "))));
    // 按需：保留原文语言列（可配置），其余写为UTF-8十六进制转义
    QStringList literalCols;
    if (m_extractUtf8Literal && m_extractUtf8Literal->isChecked())
    {
        // 优先使用动态复选框的选择结果
        if (!m_extractLangChecks.isEmpty())
        {
            for (QCheckBox *cb : m_extractLangChecks)
            {
                if (cb && cb->isChecked())
                    literalCols << cb->text().trimmed();
            }
        }
        // 如果复选框未提供选择，则回退到文本输入
        if (literalCols.isEmpty())
        {
            if (m_extractUtf8ColsEdit && !m_extractUtf8ColsEdit->text().trimmed().isEmpty())
            {
                for (const QString &c : m_extractUtf8ColsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
                {
                    literalCols << c.trimmed();
                }
            }
            else
            {
                // 默认保留中文/英文
                literalCols << QStringLiteral("text_cn") << QStringLiteral("text_en");
            }
        }
    }
    else
    {
        // 未勾选时：全部列保留原文（不进行十六进制转义）
        literalCols = langCols;
    }
    log(QStringLiteral("[提取] 直写列（不转义）：%1").arg(literalCols.join(QStringLiteral(", "))));
    // 收集文件列表
    QStringList files;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    auto matchExt = [&](const QString &fn)
    { for (const QString &e : exts) if (fn.toLower().endsWith(e.toLower())) return true; return false; };
    while (it.hasNext())
    {
        const QString f = it.next();
        if (f.contains(QStringLiteral("/csv_import_sandbox/")) || f.contains(QStringLiteral("\\csv_import_sandbox\\")) ||
            f.contains(QStringLiteral("/.csv_lang_backups/")) || f.contains(QStringLiteral("\\.csv_lang_backups\\")))
            continue;
        if (matchExt(QFileInfo(f).fileName())) files << f;
    }
    if (files.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提取"), QStringLiteral("未找到匹配的源文件。"));
        return;
    }
    m_lastFiles = files.size();
    log(QStringLiteral("[提取] 文件收集完成：%1 个").arg(m_lastFiles));
    // 记录上下文，启动并发映射归约
    m_extractOutCsv = outCsv;
    m_extractLangCols = langCols;
    m_extractLiteralCols = literalCols;
    m_extractReplaceCommaFlag = m_extractReplaceComma && m_extractReplaceComma->isChecked();
    m_extractChineseOnly = false;
    statusBar()->showMessage(QStringLiteral("正在提取 %1 个文件…").arg(files.size()));
    QApplication::setOverrideCursor(Qt::BusyCursor);
    log(QStringLiteral("[提取] 启动并发任务（QtConcurrent）"));
    ExtractMapFn mapFn{mode, defines, typeName, keepEsc};
    ExtractReduceFn reduceFn{};
    auto future = QtConcurrent::run([files, mapFn, reduceFn]() {
        QList<ExtractedBlock> out;
        for (const QString &f : files)
        {
            QList<ExtractedBlock> mapped = mapFn(f);
            reduceFn(out, mapped);
        }
        return out;
    });
    m_extractWatcher->setFuture(future);
}

/**
 * @brief 一键提取“含中文”的条目至 CSV
 * 筛选：根据语言列中中文字符命中保留；其他逻辑同常规提取。
 */
void MainWindow::onExtractChinese()
{
    // 入口：一键提取中文。逐步输出，帮助新手理解流程。
    const QString dir = m_pathEdit ? m_pathEdit->text() : QString();          // 当前目录
    const bool keepEsc = m_extractKeepEscapes && m_extractKeepEscapes->isChecked(); // 保留转义符
    if (dir.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提取"), QStringLiteral("请选择或输入项目根目录。"));
        return;
    }
    const QString outCsv = QDir(dir).absoluteFilePath(QStringLiteral("ty_text_cn.csv")); // 输出中文专用文件
    const QString mode = m_extractModeCombo ? m_extractModeCombo->currentText() : QStringLiteral("effective"); // 解析模式
    log(QStringLiteral("[中文提取] 目录=%1, 输出=%2, 模式=%3, 保留转义=%4").arg(dir, outCsv, mode).arg(keepEsc ? QStringLiteral("是") : QStringLiteral("否")));
    // 扩展名
    QStringList exts;
    if (m_extractExtsEdit && !m_extractExtsEdit->text().trimmed().isEmpty())
    {
        for (const QString &p : m_extractExtsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
            exts << p.trimmed();
    }
    if (exts.isEmpty())
        exts = QStringList{QLatin1String(".h"), QLatin1String(".hpp"), QLatin1String(".c"), QLatin1String(".cpp")};
    log(QStringLiteral("[中文提取] 使用扩展名过滤：%1").arg(exts.join(QStringLiteral(", "))));
    // 宏定义解析 A=1;B;C=hello
    QMap<QString, QString> defines;
    if (m_extractDefinesEdit && !m_extractDefinesEdit->text().trimmed().isEmpty())
    {
        for (const QString &item : m_extractDefinesEdit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts))
        {
            QString t = item.trimmed();
            if (t.isEmpty()) continue;
            int eq = t.indexOf('=');
            if (eq >= 0) defines.insert(t.left(eq).trimmed(), t.mid(eq + 1).trimmed());
            else defines.insert(t, QString());
        }
    }
    if (!defines.isEmpty())
        log(QStringLiteral("[中文提取] 宏定义：%1").arg(QStringList(defines.keys()).join(QStringLiteral(", "))));
    const QString typeName = "_Tr_TEXT";
    QStringList langCols = TextExtractor::discoverLanguageColumns(dir, exts, typeName); // 自动发现语言列
    log(QStringLiteral("[中文提取] 发现语言列：%1").arg(langCols.join(QStringLiteral(", "))));
    // 仅保留中文列直写，其余写为十六进制；如果未发现中文列，回退为 text_cn
    QStringList literalCols;
    auto isCnCol = [](const QString &col){
        const QString c = col.toLower();
        return c.endsWith(QLatin1String("_cn")) || c.endsWith(QLatin1String("_zh")) || c.endsWith(QLatin1String("_chs")) || c.endsWith(QLatin1String("_hans"));
    };
    for (const QString &c : langCols) if (isCnCol(c)) literalCols << c;
    if (literalCols.isEmpty()) literalCols << QStringLiteral("text_cn");
    log(QStringLiteral("[中文提取] 中文直写列：%1").arg(literalCols.join(QStringLiteral(", "))));
    // 收集文件
    QStringList files;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    auto matchExt = [&](const QString &fn)
    { for (const QString &e : exts) if (fn.toLower().endsWith(e.toLower())) return true; return false; };
    while (it.hasNext())
    {
        const QString f = it.next();
        if (f.contains(QStringLiteral("/csv_import_sandbox/")) || f.contains(QStringLiteral("\\csv_import_sandbox\\")) ||
            f.contains(QStringLiteral("/.csv_lang_backups/")) || f.contains(QStringLiteral("\\.csv_lang_backups\\")))
            continue;
        if (matchExt(QFileInfo(f).fileName())) files << f;
    }
    if (files.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提取"), QStringLiteral("未找到匹配的源文件。"));
        return;
    }
    m_lastFiles = files.size();
    log(QStringLiteral("[中文提取] 文件收集完成：%1 个").arg(m_lastFiles));
    // 上下文与并发
    m_extractOutCsv = outCsv;
    m_extractLangCols = langCols;
    m_extractLiteralCols = literalCols;
    m_extractReplaceCommaFlag = m_extractReplaceComma && m_extractReplaceComma->isChecked();
    m_extractChineseOnly = true;
    statusBar()->showMessage(QStringLiteral("正在提取 %1 个文件（中文筛选）…").arg(files.size()));
    QApplication::setOverrideCursor(Qt::BusyCursor);
    log(QStringLiteral("[中文提取] 启动并发任务（QtConcurrent）"));
    ExtractMapFn mapFn{mode, defines, typeName, keepEsc};
    ExtractReduceFn reduceFn{};
    auto future = QtConcurrent::run([files, mapFn, reduceFn]() {
        QList<ExtractedBlock> out;
        for (const QString &f : files)
        {
            QList<ExtractedBlock> mapped = mapFn(f);
            reduceFn(out, mapped);
        }
        return out;
    });
    m_extractWatcher->setFuture(future);
}

/**
 * @brief 提取结构体数组到独立 CSV
 * 以 `_Tr_TEXT` 数组为目标，写出 `array_variable` 与元素行。
 */
void MainWindow::onExtractArrays()
{
    const QString dir = m_pathEdit ? m_pathEdit->text() : QString();
    const bool keepEsc = m_extractKeepEscapes && m_extractKeepEscapes->isChecked();
    if (dir.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提取"), QStringLiteral("请选择或输入项目根目录。"));
        return;
    }
    const QString outCsv = QDir(dir).absoluteFilePath(QStringLiteral("ty_text_arrays.csv"));
    const QString mode = m_extractModeCombo ? m_extractModeCombo->currentText() : QStringLiteral("effective");
    log(QStringLiteral("[数组提取] 目录=%1, 输出=%2, 模式=%3").arg(dir, outCsv, mode));
    QStringList exts;
    if (m_extractExtsEdit && !m_extractExtsEdit->text().trimmed().isEmpty())
    {
        for (const QString &p : m_extractExtsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
            exts << p.trimmed();
    }
    if (exts.isEmpty())
        exts = QStringList{QLatin1String(".h"), QLatin1String(".hpp"), QLatin1String(".c"), QLatin1String(".cpp")};
    // 发现语言列
    const QString typeName = "_Tr_TEXT";
    QStringList langCols = TextExtractor::discoverLanguageColumns(dir, exts, typeName);
    QStringList literalCols;
    if (m_extractUtf8Literal && m_extractUtf8Literal->isChecked())
    {
        if (!m_extractLangChecks.isEmpty())
        {
            for (QCheckBox *cb : m_extractLangChecks)
                if (cb && cb->isChecked()) literalCols << cb->text().trimmed();
        }
        if (literalCols.isEmpty()) literalCols << QStringLiteral("text_cn") << QStringLiteral("text_en");
    }
    else
    {
        literalCols = langCols;
    }
    // 并发提取数组
    statusBar()->showMessage(QStringLiteral("正在提取结构体数组…"));
    QApplication::setOverrideCursor(Qt::BusyCursor);
    auto future = QtConcurrent::run([dir, exts, mode, typeName, keepEsc]() {
        return TextExtractor::scanDirectoryArrays(dir, exts, mode, QMap<QString, QString>{}, typeName, keepEsc);
    });
    auto watcher = new QFutureWatcher<QList<ExtractedArray>>(this);
    connect(watcher, &QFutureWatcher<QList<ExtractedArray>>::finished, this, [this, watcher, outCsv, langCols, literalCols]() {
        QList<ExtractedArray> arrays = watcher->result();
        bool ok = TextExtractor::writeArraysCsv(outCsv, arrays, langCols, literalCols, m_extractReplaceComma && m_extractReplaceComma->isChecked());
        QApplication::restoreOverrideCursor();
        statusBar()->clearMessage();
        if (ok)
        {
            log(QStringLiteral("数组提取完成：%1，数组数=%2").arg(outCsv).arg(arrays.size()));
            QMessageBox::information(this, QStringLiteral("数组提取完成"), QStringLiteral("CSV 已生成：%1\n数组数：%2").arg(outCsv).arg(arrays.size()));
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outCsv).dir().absolutePath()));
        }
        else
        {
            QMessageBox::warning(this, QStringLiteral("提取失败"), QStringLiteral("写入数组CSV失败：%1").arg(outCsv));
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

/**
 * @brief 从 CSV 生成带注释的 C 源与头文件
 * 选项：去除 static、生成注册表数组、直写列、注释模式、缺失填充等。
 */
void MainWindow::onGenerateRun()
{
    const QString csv = m_csvInputEdit ? m_csvInputEdit->text() : QString();
    const QString outc = m_cOutputEdit ? m_cOutputEdit->text() : QString();
    if (csv.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("生成"), QStringLiteral("请选择或输入CSV路径。"));
        return;
    }
    if (outc.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("生成"), QStringLiteral("请选择或输入C输出文件路径。"));
        return;
    }
    QString headerOut = (m_headerOutputEdit && !m_headerOutputEdit->text().trimmed().isEmpty())
                            ? m_headerOutputEdit->text().trimmed()
                            : QDir(qApp->applicationDirPath()).absoluteFilePath(QStringLiteral("../include/generated_text_vars_dynamic.h"));
    log(QStringLiteral("[生成] CSV=%1, C=%2, 头文件=%3").arg(csv, outc, headerOut));
    const bool noStatic = m_genNoStatic && m_genNoStatic->isChecked();
    const bool regEmit = m_genEmitRegistry && m_genEmitRegistry->isChecked();
    const QString regName = m_genRegistryNameEdit ? m_genRegistryNameEdit->text().trimmed() : QString();
    const bool utf8Lit = m_genUtf8Literal && m_genUtf8Literal->isChecked();
    QStringList litCols;
    if (utf8Lit && m_genUtf8ColsEdit && !m_genUtf8ColsEdit->text().trimmed().isEmpty())
    {
        for (const QString &c : m_genUtf8ColsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
            litCols << c.trimmed();
    }
    const bool nullSent = m_genNullSentinel && m_genNullSentinel->isChecked();
    const bool verbatim = m_genVerbatim && m_genVerbatim->isChecked();
    const QString annotate = m_genAnnotateCombo ? m_genAnnotateCombo->currentText() : QString("names");
    const int perLine = m_genPerLineSpin ? m_genPerLineSpin->value() : 1;
    const bool fillEng = m_genFillMissingWithEnglish && m_genFillMissingWithEnglish->isChecked();
    QString typeName = "_Tr_TEXT";
    QString code = TextExtractor::generateCFromCsv(
        csv,
        typeName,
        outc,
        headerOut,
        noStatic,
        regEmit,
        regName,
        utf8Lit,
        fillEng,
        nullSent,
        verbatim,
        litCols,
        annotate,
        perLine,
        QMap<QString, QPair<QString, QString>>(),
        QString());
    if (!code.isEmpty())
    {
        log(QStringLiteral("生成完成：%1 和 %2").arg(outc, headerOut));
        QMessageBox::information(this, QStringLiteral("生成完成"), QStringLiteral("已生成：\n%1\n%2").arg(outc, headerOut));
    }
    else
    {
        QMessageBox::critical(this, QStringLiteral("生成失败"), QStringLiteral("生成内容为空，请检查CSV格式。"));
    }
}

void MainWindow::onReadErrors()
{
    QString root = currentDirPath();
    if (root.trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("读取报错"), QStringLiteral("请先选择项目目录"));
        return;
    }
    QString outCsv = QDir(root).absoluteFilePath(QStringLiteral("disp_errors.csv"));
    QStringList exts = QStringList{QStringLiteral(".c"), QStringLiteral(".cpp"), QStringLiteral(".h"), QStringLiteral(".hpp")};
    QMap<QString, QString> defines;
    QString mode = QStringLiteral("effective");
    log(QStringLiteral("[读取报错] 扫描 DispMessageInfo 并写入: %1").arg(outCsv));
    QList<ExtractedBlock> rows = TextExtractor::scanDispMessageInfo(root, exts, mode, defines);
    if (rows.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("读取报错"), QStringLiteral("未发现 DispMessageInfo 初始化"));
        return;
    }
    QStringList langCols = TextExtractor::defaultLanguageColumns();
    QStringList literalCols; literalCols << QStringLiteral("text_cn") << QStringLiteral("text_en");
    bool ok = TextExtractor::writeCsv(outCsv, rows, langCols, literalCols, true);
    if (ok)
    {
        QMessageBox::information(this, QStringLiteral("读取报错完成"), QStringLiteral("CSV 已生成：%1\n记录数：%2").arg(outCsv).arg(rows.size()));
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outCsv).dir().absolutePath()));
    }
    else
    {
        QMessageBox::critical(this, QStringLiteral("写入失败"), QStringLiteral("无法写入 CSV：%1").arg(outCsv));
    }
}

/**
 * @brief 浏览并设置“项目设置”页的项目根目录
 */
void MainWindow::onBrowseProjectRoot()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择项目根目录"), QDir::homePath());
    if (!dir.isEmpty())
        m_projRootEdit->setText(dir);
}

/**
 * @brief 初始化插入新语言字段
 * 行为：在结构体中插入 `text_<lang>`，并批量重写初始化，生成备份与日志。
 */
void MainWindow::onInitNewLanguage()
{
    QString root = m_projRootEdit ? m_projRootEdit->text().trimmed() : QString();
    QString code = m_projNewLangEdit ? m_projNewLangEdit->text().trimmed() : QString();
    if (root.isEmpty())
    {
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录"));
        return;
    }
    if (code.isEmpty())
    {
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入新语言代码，例如 fr"));
        return;
    }
    auto res = ProjectLang::addLanguageAndInitialize(root, code);
    log(res.message);
    if (!res.logPath.isEmpty() && m_projLogLabel)
        m_projLogLabel->setText(QString("日志: %1").arg(res.logPath));
    if (res.success)
    {
        QMessageBox::information(this, "完成", res.message);
        if (!res.outputDir.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(res.outputDir));
    }
    else
    {
    QMessageBox::warning(this, QStringLiteral("未变更"), res.message);
    }
}

/**
 * @brief 撤销最近一次语言初始化会话
 * 从备份目录恢复原文件。
 */
void MainWindow::onUndoLastInit()
{
    QString root = m_projRootEdit ? m_projRootEdit->text().trimmed() : QString();
    if (root.isEmpty())
    {
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录"));
        return;
    }
    auto res = ProjectLang::undoLastInitialization(root);
    log(res.message);
    if (!res.logPath.isEmpty() && m_projLogLabel)
        m_projLogLabel->setText(QString("日志: %1").arg(res.logPath));
    if (res.success)
    {
        QMessageBox::information(this, "撤销完成", res.message);
    }
    else
    {
    QMessageBox::warning(this, QStringLiteral("撤销失败"), res.message);
    }
}

/**
 * @brief 用英文填充缺失语言项
 * 在每个初始化中将空/NULL 的项替换为英文列值。
 */
void MainWindow::onFillEnglishMissing()
{
    QString root = m_projRootEdit ? m_projRootEdit->text().trimmed() : QString();
    if (root.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录"));
        return;
    }
    auto res = ProjectLang::fillMissingEntriesWithEnglish(root);
    log(res.message);
    if (!res.logPath.isEmpty() && m_projLogLabel)
        m_projLogLabel->setText(QString("日志: %1").arg(res.logPath));
    if (res.success)
    {
        QMessageBox::information(this, QStringLiteral("完成"), res.message);
        if (!res.outputDir.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(res.outputDir));
    }
    else
    {
        QMessageBox::warning(this, QStringLiteral("未变更"), res.message);
    }
}

/**
 * @brief 浏览 CSV 导入页的项目根目录
 */
void MainWindow::onBrowseCsvProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择项目根目录", QDir::homePath());
    if (!dir.isEmpty())
        m_csvProjEdit->setText(dir);
}

/**
 * @brief 选择要导入的 CSV 文件（允许多选）
 */
void MainWindow::onBrowseCsvFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择CSV", QDir::homePath(), "CSV (*.csv)");
    if (!files.isEmpty())
        m_csvFileEdit->setText(files.join(QLatin1String(";")));
}

/**
 * @brief 执行 CSV 翻译导入流程
 * 合并统计：成功/跳过/失败及日志与差异路径，支持多 CSV 顺序处理。
 */
void MainWindow::onRunCsvImport()
{
    QString root = m_csvProjEdit ? m_csvProjEdit->text().trimmed() : QString();
    QString csvInput = m_csvFileEdit ? m_csvFileEdit->text().trimmed() : QString();
    if (root.isEmpty() || csvInput.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录和CSV文件"));
        return;
    }
    m_csvProgress->setValue(5);
    // 默认配置：按值列顺序映射标准语言代码
    QJsonObject cfg;
    QJsonObject map;
    map["cn"] = 0;
    map["en"] = 1;
    map["vn"] = 2;
    map["ko"] = 3;
    map["tr"] = 4;
    map["ru"] = 5;
    map["pt"] = 6;
    map["es"] = 7;
    map["fa"] = 8;
    map["jp"] = 9;
    map["ar"] = 10;
    map["other"] = 11;
    cfg["column_mapping"] = map;
    cfg["dry_run"] = false;
    cfg["strict_line_only"] = false;
    cfg["line_window"] = 10000;
    cfg["ignore_variable_name"] = true;
    cfg["disable_backups"] = true;
    QStringList csvFiles;
    for (const QString &part : csvInput.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        QString f = part.trimmed();
        if (!f.isEmpty())
            csvFiles << f;
    }
    struct ImportTotals {
        int totalSuccess{0};
        int totalSkip{0};
        int totalFail{0};
        QString lastLogPath;
        QString lastDiffPath;
        QString lastOutputDir;
        QStringList allSuccessFiles;
        QStringList allSkippedFiles;
        QStringList allFailedFiles;
    };
    auto totals = std::make_shared<ImportTotals>();

    m_csvRunBtn->setEnabled(false);
    auto watcher = new QFutureWatcher<void>(this);
    QFuture<void> fut = QtConcurrent::run([this, totals, root, csvFiles, cfg]() {
        for (int i = 0; i < csvFiles.size(); ++i)
        {
            QFileInfo fi(csvFiles.at(i));
            QString base = fi.completeBaseName();
            QJsonObject cfgEach = cfg;
            QString logsDir = QDir(root).absoluteFilePath(QStringLiteral("logs"));
            QDir().mkpath(logsDir);
            cfgEach[QStringLiteral("log_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_%1.log").arg(base));
            cfgEach[QStringLiteral("diff_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_%1.diff").arg(base));
            if (!cfgEach.value(QStringLiteral("disable_backups")).toBool())
            {
                QString backupsBase = QDir(root).absoluteFilePath(QStringLiteral(".csv_lang_backups"));
                QDir().mkpath(backupsBase);
                QString sess = QDir(backupsBase).absoluteFilePath(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_") + base);
                cfgEach[QStringLiteral("backups_dir")] = sess;
            }

            auto stats = CsvLangPlugin::applyTranslations(root, csvFiles.at(i), cfgEach);
            totals->totalSuccess += stats.successCount;
            totals->totalSkip += stats.skipCount;
            totals->totalFail += stats.failCount;
            totals->lastLogPath = stats.logPath;
            totals->lastDiffPath = stats.diffPath;
            totals->lastOutputDir = stats.outputDir;
            totals->allSuccessFiles.append(stats.successFiles);
            totals->allSkippedFiles.append(stats.skippedFiles);
            totals->allFailedFiles.append(stats.failedFiles);

            int prog = 5 + ((i + 1) * 90) / qMax(1, csvFiles.size());
            QMetaObject::invokeMethod(m_csvProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, qMin(95, prog)));
        }
    });
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, totals, csvFiles, watcher]() {
        m_csvProgress->setValue(100);
        QString report = QString("CSV文件数: %1\n成功: %2\n跳过: %3\n失败: %4\n日志: %5\n差异: %6\n备份: %7")
                             .arg(csvFiles.size())
                             .arg(totals->totalSuccess)
                             .arg(totals->totalSkip)
                             .arg(totals->totalFail)
                             .arg(totals->lastLogPath)
                             .arg(totals->lastDiffPath)
                             .arg(totals->lastOutputDir);
        m_csvReportView->setPlainText(report);
        log(QStringLiteral("CSV导入完成：文件 %1 成功 %2 跳过 %3 失败 %4").arg(csvFiles.size()).arg(totals->totalSuccess).arg(totals->totalSkip).arg(totals->totalFail));
        Q_UNUSED(csvFiles);
        m_csvRunBtn->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(fut);
}

/**
 * @brief 仅导入结构体数组的 CSV
 * 日志与差异文件名带 arrays 后缀。
 */
void MainWindow::onRunArrayCsvImport()
{
    QString root = m_csvProjEdit ? m_csvProjEdit->text().trimmed() : QString();
    QString csvInput = m_csvFileEdit ? m_csvFileEdit->text().trimmed() : QString();
    if (root.isEmpty() || csvInput.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录和数组CSV文件"));
        return;
    }
    m_csvProgress->setValue(5);
    QJsonObject cfg;
    QJsonObject map;
    map[QStringLiteral("cn")] = 0;
    map[QStringLiteral("en")] = 1;
    map[QStringLiteral("vn")] = 2;
    map[QStringLiteral("ko")] = 3;
    map[QStringLiteral("tr")] = 4;
    map[QStringLiteral("ru")] = 5;
    map[QStringLiteral("pt")] = 6;
    map[QStringLiteral("es")] = 7;
    map[QStringLiteral("fa")] = 8;
    map[QStringLiteral("jp")] = 9;
    map[QStringLiteral("ar")] = 10;
    map[QStringLiteral("other")] = 11;
    cfg[QStringLiteral("column_mapping")] = map;
    cfg[QStringLiteral("dry_run")] = false;
    cfg[QStringLiteral("strict_line_only")] = false;
    cfg[QStringLiteral("line_window")] = 10000;
    cfg[QStringLiteral("ignore_variable_name")] = true;
    cfg[QStringLiteral("disable_backups")] = true;
    // 专用数组导入：日志文件名带 arrays 后缀
    QStringList csvFiles;
    for (const QString &part : csvInput.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        QString f = part.trimmed();
        if (!f.isEmpty()) csvFiles << f;
    }
    struct ImportTotals { int totalSuccess{0}; int totalSkip{0}; int totalFail{0}; QString lastLogPath; QString lastDiffPath; QString lastOutputDir; };
    auto totals = std::make_shared<ImportTotals>();
    m_csvRunArraysBtn->setEnabled(false);
    auto watcher = new QFutureWatcher<void>(this);
    QFuture<void> fut = QtConcurrent::run([this, totals, root, csvFiles, cfg]() {
        for (int i = 0; i < csvFiles.size(); ++i)
        {
            QFileInfo fi(csvFiles.at(i));
            QString base = fi.completeBaseName();
            QJsonObject cfgEach = cfg;
            QString logsDir = QDir(root).absoluteFilePath(QStringLiteral("logs"));
            QDir().mkpath(logsDir);
            cfgEach[QStringLiteral("log_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_%1_arrays.log").arg(base));
            cfgEach[QStringLiteral("diff_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_%1_arrays.diff").arg(base));
            auto stats = CsvLangPlugin::applyTranslations(root, csvFiles.at(i), cfgEach);
            totals->totalSuccess += stats.successCount;
            totals->totalSkip += stats.skipCount;
            totals->totalFail += stats.failCount;
            totals->lastLogPath = stats.logPath;
            totals->lastDiffPath = stats.diffPath;
            totals->lastOutputDir = stats.outputDir;
            int prog = 5 + ((i + 1) * 90) / qMax(1, csvFiles.size());
            QMetaObject::invokeMethod(m_csvProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, qMin(95, prog)));
        }
    });
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, totals, watcher]() {
        m_csvProgress->setValue(100);
        QString report = QString("数组CSV文件数: %1\n成功: %2\n跳过: %3\n失败: %4\n日志: %5\n差异: %6")
                             .arg(1)
                             .arg(totals->totalSuccess)
                             .arg(totals->totalSkip)
                             .arg(totals->totalFail)
                             .arg(totals->lastLogPath)
                             .arg(totals->lastDiffPath);
        m_csvReportView->setPlainText(report);
        log(QStringLiteral("数组CSV导入完成：成功 %1 跳过 %2 失败 %3").arg(totals->totalSuccess).arg(totals->totalSkip).arg(totals->totalFail));
        m_csvRunArraysBtn->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(fut);
}

void MainWindow::onRunErrorsCsvImport()
{
    QString root = m_csvProjEdit ? m_csvProjEdit->text().trimmed() : QString();
    QString csvInput = m_csvFileEdit ? m_csvFileEdit->text().trimmed() : QString();
    if (root.isEmpty() || csvInput.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择项目根目录和CSV文件"));
        return;
    }
    m_csvProgress->setValue(5);
    QJsonObject cfg;
    QJsonObject map;
    map[QStringLiteral("cn")] = 0;
    map[QStringLiteral("en")] = 1;
    map[QStringLiteral("vn")] = 2;
    map[QStringLiteral("ko")] = 3;
    map[QStringLiteral("tr")] = 4;
    map[QStringLiteral("ru")] = 5;
    map[QStringLiteral("pt")] = 6;
    map[QStringLiteral("es")] = 7;
    map[QStringLiteral("fa")] = 8;
    map[QStringLiteral("jp")] = 9;
    map[QStringLiteral("ar")] = 10;
    map[QStringLiteral("other")] = 11;
    cfg[QStringLiteral("column_mapping")] = map;
    cfg[QStringLiteral("dry_run")] = false;
    cfg[QStringLiteral("strict_line_only")] = true;
    cfg[QStringLiteral("line_window")] = 10;
    cfg[QStringLiteral("ignore_variable_name")] = false;
    cfg[QStringLiteral("disable_backups")] = true;
    cfg[QStringLiteral("only_info")] = true;
    cfg[QStringLiteral("annotate_mode")] = QStringLiteral("names");
    // 可选：设置源文件过滤与行范围（当前选择）
    // 如果用户在 IDE 中选定了某文件与行范围，可手动调整如下两项
    // 这里预填 demo2.c 的路径与范围供你参考，如需修改请在 UI 更改 CSV 后再点击
    QString demoPath = QDir(root).absoluteFilePath(QStringLiteral("tests/demo_proj/src/demo2.c"));
    cfg[QStringLiteral("source_path_filter")] = demoPath;
    cfg[QStringLiteral("line_start")] = 96;
    cfg[QStringLiteral("line_end")] = 117;

    QStringList csvFiles;
    for (const QString &part : csvInput.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        QString f = part.trimmed();
        if (!f.isEmpty()) csvFiles << f;
    }

    struct ImportTotals { int totalSuccess{0}; int totalSkip{0}; int totalFail{0}; QString lastLogPath; QString lastDiffPath; QString lastOutputDir; };
    auto totals = std::make_shared<ImportTotals>();

    m_csvRunBtn->setEnabled(false);
    auto watcher = new QFutureWatcher<void>(this);
    QFuture<void> fut = QtConcurrent::run([this, totals, root, csvFiles, cfg]() {
        for (int i = 0; i < csvFiles.size(); ++i)
        {
            QFileInfo fi(csvFiles.at(i));
            QString base = fi.completeBaseName();
            QJsonObject cfgEach = cfg;
            QString logsDir = QDir(root).absoluteFilePath(QStringLiteral("logs"));
            QDir().mkpath(logsDir);
            cfgEach[QStringLiteral("log_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_errors_%1.log").arg(base));
            cfgEach[QStringLiteral("diff_path")] = QDir(logsDir).absoluteFilePath(QStringLiteral("csv_lang_plugin_errors_%1.diff").arg(base));

            auto stats = CsvLangPlugin::applyTranslations(root, csvFiles.at(i), cfgEach);
            totals->totalSuccess += stats.successCount;
            totals->totalSkip += stats.skipCount;
            totals->totalFail += stats.failCount;
            totals->lastLogPath = stats.logPath;
            totals->lastDiffPath = stats.diffPath;
            totals->lastOutputDir = stats.outputDir;
            int prog = 5 + ((i + 1) * 90) / qMax(1, csvFiles.size());
            QMetaObject::invokeMethod(m_csvProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, qMin(95, prog)));
        }
    });
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, totals, watcher]() {
        m_csvProgress->setValue(100);
        QString report = QString("仅导入报错文本\n成功: %1\n跳过: %2\n失败: %3\n日志: %4\n差异: %5")
                             .arg(totals->totalSuccess)
                             .arg(totals->totalSkip)
                             .arg(totals->totalFail)
                             .arg(totals->lastLogPath)
                             .arg(totals->lastDiffPath);
        m_csvReportView->setPlainText(report);
        log(QStringLiteral("报错文本导入完成：成功 %1 跳过 %2 失败 %3").arg(totals->totalSuccess).arg(totals->totalSkip).arg(totals->totalFail));
        m_csvRunBtn->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(fut);
}
/**
 * @brief 提示导入日志与差异文件的默认位置
 */
void MainWindow::onExportCsvLog()
{
    QMessageBox::information(this, QStringLiteral("日志"), QStringLiteral("日志位于 logs/csv_lang_plugin.log，差异位于 logs/csv_lang_plugin.diff"));
}

// 根据当前目录与扩展，动态发现语言列并刷新复选框
/**
 * @brief 刷新“保留原文语言”动态复选框
 * 异步发现语言列以避免卡顿；清空旧项等待填充。
 */
void MainWindow::refreshExtractLanguageChecks()
{
    if (!m_extractLangBox)
        return;
    m_extractLangChecks.clear();
    // 获取当前路径与扩展
    QString dir = currentDirPath();
    if (dir.isEmpty() && m_pathEdit)
        dir = m_pathEdit->text();
    QStringList exts;
    if (m_extractExtsEdit && !m_extractExtsEdit->text().trimmed().isEmpty())
    {
        for (const QString &p : m_extractExtsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
            exts << p.trimmed();
    }
    if (exts.isEmpty())
        exts = QStringList{QLatin1String(".h"), QLatin1String(".hpp"), QLatin1String(".c"), QLatin1String(".cpp")};
    // 异步发现语言列，避免 UI 卡顿
    statusBar()->showMessage(QStringLiteral("正在扫描语言列..."));
    QApplication::setOverrideCursor(Qt::BusyCursor);
    auto future = QtConcurrent::run(TextExtractor::discoverLanguageColumns, dir, exts, QStringLiteral("_Tr_TEXT"));
    m_langDiscoverWatcher->setFuture(future);
    // 预先准备容器，以便 finished 时直接填充
    // 找到滚动区域中的网格容器
    QWidget *gridContainer = m_extractLangBox->findChild<QWidget *>(QStringLiteral("langGridContainer"));
    QGridLayout *grid = gridContainer ? qobject_cast<QGridLayout *>(gridContainer->layout()) : nullptr;
    if (!gridContainer || !grid)
    {
        // 回退：如果容器缺失，则创建
        QVBoxLayout *langLay = qobject_cast<QVBoxLayout *>(m_extractLangBox->layout());
        if (!langLay)
        {
            langLay = new QVBoxLayout(m_extractLangBox);
            langLay->setContentsMargins(0, 0, 0, 0);
        langLay->addWidget(new QLabel(QStringLiteral("选择保留原文语言:")));
        }
        QScrollArea *scroll = new QScrollArea(m_extractLangBox);
        scroll->setWidgetResizable(true);
        gridContainer = new QWidget(scroll);
        gridContainer->setObjectName("langGridContainer");
        grid = new QGridLayout(gridContainer);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(6);
        scroll->setWidget(gridContainer);
        langLay->addWidget(scroll);
    }
    // 清空旧项并留待 finished 时填充
    while (grid->count() > 0)
    {
        QLayoutItem *it = grid->takeAt(0);
        if (!it)
            break;
        if (QWidget *w = it->widget())
        {
            delete w;
        }
        delete it;
    }
}

/**
 * @brief 根据语言列填充复选框网格
 * 默认勾选 `text_cn` 与 `text_en`。
 */
void MainWindow::applyLanguageChecks(const QStringList &langCols)
{
    if (!m_extractLangBox)
        return;
    QWidget *gridContainer = m_extractLangBox->findChild<QWidget *>(QStringLiteral("langGridContainer"));
    QGridLayout *grid = gridContainer ? qobject_cast<QGridLayout *>(gridContainer->layout()) : nullptr;
    if (!gridContainer || !grid)
        return;
    const int cols = 4;
    int idx = 0;
    for (const QString &col : langCols)
    {
        QCheckBox *cb = new QCheckBox(col, gridContainer);
        if (col == QLatin1String("text_cn") || col == QLatin1String("text_en"))
        {
            cb->setChecked(true);
        }
        m_extractLangChecks.push_back(cb);
        grid->addWidget(cb, idx / cols, idx % cols);
        ++idx;
    }
}

/**
 * @brief 拖拽进入事件
 * 支持拖拽目录与 CSV，分别切换到对应页签与设置输入。
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 拖拽释放事件
 * 目录进入“提取CSV”页并设为当前路径；CSV 进入“从CSV生成C”页并填充输入。
 */
void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
    {
        event->ignore();
        return;
    }
    const QString p = urls.first().toLocalFile();
    QFileInfo fi(p);
    if (fi.isDir())
    {
        m_tabs->setCurrentIndex(0);
        m_pathEdit->setText(p);
        setCurrentPath(p);
        log(QString("拖拽目录：%1").arg(p));
        event->acceptProposedAction();
    }
    else if (fi.suffix().toLower() == "csv")
    {
        m_tabs->setCurrentIndex(1);
        if (m_csvInputEdit)
            m_csvInputEdit->setText(p);
        log(QString("拖拽CSV：%1").arg(p));
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief 新建文件夹于当前路径
 */
void MainWindow::onNewFolder()
{
    const QString base = currentDirPath();
    bool ok = false;
    const QString name = QInputDialog::getText(this, "新建文件夹", "名称：", QLineEdit::Normal, "NewFolder", &ok);
    if (!ok || name.isEmpty())
        return;
    QDir dir(base);
    if (dir.mkdir(name))
    {
        log(QString("已创建文件夹：%1").arg(dir.absoluteFilePath(name)));
        onRefresh();
    }
    else
    {
        QMessageBox::critical(this, "创建失败", "无法创建文件夹");
    }
}

/**
 * @brief 重命名选中项
 */
void MainWindow::onRename()
{
    QStringList files = selectedFilePaths();
    if (files.size() != 1)
    {
        QMessageBox::information(this, "重命名", "请选中一项进行重命名");
        return;
    }
    const QString src = files.first();
    QFileInfo fi(src);
    bool ok = false;
    const QString newName = QInputDialog::getText(this, "重命名", "新名称：", QLineEdit::Normal, fi.fileName(), &ok);
    if (!ok || newName.isEmpty())
        return;
    const QString dst = fi.dir().absoluteFilePath(newName);
    if (QFile::rename(src, dst))
    {
        log(QString("已重命名：%1 -> %2").arg(src, dst));
        onRefresh();
    }
    else
    {
        QMessageBox::critical(this, "重命名失败", "无法重命名文件或文件夹");
    }
}

/**
 * @brief 删除选中项（文件或文件夹）
 * 目录使用 `removeRecursively`，文件使用 `QFile::remove`。
 */
void MainWindow::onDelete()
{
    QStringList files = selectedFilePaths();
    if (files.isEmpty())
    {
        QMessageBox::information(this, "删除", "请选中至少一项进行删除");
        return;
    }
    if (QMessageBox::question(this, "确认删除", QString("将删除 %1 项，是否继续？").arg(files.size())) != QMessageBox::Yes)
    {
        return;
    }
    int okCount = 0;
    foreach (const QString &p, files)
    {
        QFileInfo fi(p);
        bool ok = false;
        if (fi.isDir())
        {
            QDir dir(p);
            ok = dir.removeRecursively();
        }
        else
        {
            ok = QFile::remove(p);
        }
        if (ok)
            okCount++;
    }
    log(QString("删除完成：成功 %1 / 总计 %2").arg(okCount).arg(files.size()));
    onRefresh();
}

/**
 * @brief 复制选中项到目标目录
 * 简化目录复制为创建同名空目录；文件使用 `QFile::copy`。
 */
void MainWindow::onCopy()
{
    QStringList files = selectedFilePaths();
    if (files.isEmpty())
    {
        QMessageBox::information(this, "复制", "请选中至少一项进行复制");
        return;
    }
    const QString dstDir = QFileDialog::getExistingDirectory(this, "选择目标目录", currentDirPath());
    if (dstDir.isEmpty())
        return;
    int okCount = 0;
    foreach (const QString &p, files)
    {
        QFileInfo fi(p);
        const QString dst = QDir(dstDir).absoluteFilePath(fi.fileName());
        bool ok = false;
        if (fi.isDir())
        {
            // 简化：目录复制为创建同名目录，不做递归复制
            ok = QDir(dstDir).mkdir(fi.fileName());
        }
        else
        {
            ok = QFile::copy(p, dst);
        }
        if (ok)
            okCount++;
    }
    log(QString("复制完成：成功 %1 / 总计 %2").arg(okCount).arg(files.size()));
    onRefresh();
}

/**
 * @brief 移动选中项到目标目录
 * 使用 `QFile::rename` 完成移动。
 */
void MainWindow::onMove()
{
    QStringList files = selectedFilePaths();
    if (files.isEmpty())
    {
        QMessageBox::information(this, "移动", "请选中至少一项进行移动");
        return;
    }
    const QString dstDir = QFileDialog::getExistingDirectory(this, "选择目标目录", currentDirPath());
    if (dstDir.isEmpty())
        return;
    int okCount = 0;
    foreach (const QString &p, files)
    {
        QFileInfo fi(p);
        const QString dst = QDir(dstDir).absoluteFilePath(fi.fileName());
        bool ok = QFile::rename(p, dst);
        if (ok)
            okCount++;
    }
    log(QString("移动完成：成功 %1 / 总计 %2").arg(okCount).arg(files.size()));
    onRefresh();
}
