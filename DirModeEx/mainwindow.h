/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-10 22:29:22
 * @LastEditors: fenxue1 1803651830@qq.com
 * @LastEditTime: 2025-11-20 20:45:57
 * @FilePath: \test_mooc-clin\DirModeEx\mainwindow.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @file mainwindow.h
 * @brief 主窗口模块（Main Window UI module）
 *
 * 功能名称：项目浏览与文本提取/生成的图形界面（Project browser & CSV/C code tools）
 * 主要用途：
 * - 文件系统浏览与基本操作（新建/重命名/删除/复制/移动）；
 * - 提取 CSV 与从 CSV 生成 C/.h 代码；
 * - 语言设置与 CSV 翻译导入；
 *
 * 使用示例：
 *   MainWindow w; w.show();
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include <QMutex>
#include "text_extractor.h"

// 前置声明以避免头文件包含不足导致的类型未识别错误
class QFileSystemModel;
class QTreeView;
class QTableView;
class QLineEdit;
class QToolBar;
class QAction;
class QTabWidget;
class QPushButton;
class QCheckBox;
class QComboBox;
class QTextEdit;
class QSpinBox;
class QLabel;
class QProgressBar;
#include <QFileSystemModel>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
class QTreeView;
class QTableView;
class QLineEdit;
class QToolBar;
class QAction;
class QFile;
class QTextStream;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief 主窗口控制器（Main application window controller）
 *
 * 职责（Responsibilities）：
 * - 浏览与操作项目目录与文件（browse & operate files）；
 * - 文本提取到 CSV 与从 CSV 生成 C 代码；
 * - 语言列发现与“仅中文”快捷提取；
 * - CSV 翻译导入工具与日志显示。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造与析构（Construction & Destruction）
     * @param parent 父窗口
     */
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief 路径输入框按回车后设置当前路径（Set current directory from path edit on Enter）
     */
    void onPathReturnPressed();
    /**
     * @brief 目录树选中项变化时刷新文件表与路径（Refresh file view and path on tree selection change）
     * @param current 当前索引
     * @param previous 之前索引（未使用）
     */
    void onTreeSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    /**
     * @brief 文件表双击：若为目录则进入，否则日志记录选中文件（Enter folder or log selection on double click）
     * @param index 表格索引
     */
    void onTableDoubleClicked(const QModelIndex &index);
    /**
     * @brief 刷新文件系统视图（Refresh filesystem models）
     */
    void onRefresh();
    /**
     * @brief 新建文件夹（Create new folder in current path）
     */
    void onNewFolder();
    /**
     * @brief 重命名选中文件或文件夹（Rename selected item）
     */
    void onRename();
    /**
     * @brief 删除选中文件或文件夹（Delete selected items）
     */
    void onDelete();
    /**
     * @brief 复制选中文件或文件夹（Copy selected items）
     */
    void onCopy();
    /**
     * @brief 移动选中文件或文件夹（Move selected items）
     */
    void onMove();

    // 提取与生成
    /**
     * @brief 执行常规文本提取到 CSV（Run normal extraction to CSV）
     */
    void onExtractRun();
    /**
     * @brief 从 CSV 生成 C 代码与头文件（Generate C and header from CSV）
     */
    void onGenerateRun();

private:
    Ui::MainWindow *ui;
    QFileSystemModel *m_dirModel{nullptr};
    QFileSystemModel *m_fileModel{nullptr};
    QTreeView *m_tree{nullptr};
    QTableView *m_table{nullptr};
    QLineEdit *m_pathEdit{nullptr};
    QToolBar *m_toolbar{nullptr};
    QAction *actRefresh{nullptr};
    QAction *actNewFolder{nullptr};
    QAction *actRename{nullptr};
    QAction *actDelete{nullptr};
    QAction *actCopy{nullptr};
    QAction *actMove{nullptr};

    QFile *m_logFile{nullptr};
    QTextStream *m_logStream{nullptr};

    void setupUiContent();
    void buildTabs();
    QWidget* buildExtractPage();
    QWidget* buildGeneratePage();
    QWidget* buildSettingsPage();
    QWidget* buildLogsPage();
    void setCurrentPath(const QString &path);
    QString currentDirPath() const;
    QStringList selectedFilePaths() const;
    void log(const QString &msg);

    // 分页
    QTabWidget *m_tabs{nullptr};

    // 提取CSV页控件
    QLineEdit *m_projectDirEdit{nullptr};
    QPushButton *m_browseProjectBtn{nullptr};
    QPushButton *m_extractRunBtn{nullptr};
    QPushButton *m_extractChineseBtn{nullptr}; // “一键提取中文”按钮（仅生成包含中文的 CSV）
    QCheckBox *m_extractKeepEscapes{nullptr};
    QComboBox *m_extractTypeCombo{nullptr};
    QComboBox *m_extractModeCombo{nullptr};
    QLineEdit *m_extractDefinesEdit{nullptr};
    QLineEdit *m_extractExtsEdit{nullptr};
    // 提取选项：保留原文语言列，可配置
    QCheckBox *m_extractUtf8Literal{nullptr};
    QLineEdit *m_extractUtf8ColsEdit{nullptr};
    QCheckBox *m_extractReplaceComma{nullptr};
    // 动态语言复选框容器与列表
    QWidget *m_extractLangBox{nullptr};
    QList<QCheckBox*> m_extractLangChecks;
    void refreshExtractLanguageChecks();
    void applyLanguageChecks(const QStringList &langCols);
    QFutureWatcher<QStringList> *m_langDiscoverWatcher{nullptr};
    // 并发提取进度与监视器
    QProgressBar *m_extractProgress{nullptr};
    QFutureWatcher<QList<ExtractedBlock>> *m_extractWatcher{nullptr};
    // 提取完成后所需上下文
    QString m_extractOutCsv;
    QStringList m_extractLangCols;
    QStringList m_extractLiteralCols;
    bool m_extractReplaceCommaFlag{true};
    bool m_extractChineseOnly{false}; // 标志：本次提取是否启用“仅中文筛选”
    int m_lastFiles{0};               // 统计：参与提取的源文件数量
    int m_lastTotalBlocks{0};         // 统计：并发任务返回的原始块数量
    int m_lastFilteredBlocks{0};      // 统计：中文筛选后保留下来的块数量

    // 从CSV生成C页控件
    QLineEdit *m_csvInputEdit{nullptr};
    QLineEdit *m_cOutputEdit{nullptr};
    QPushButton *m_browseCsvBtn{nullptr};
    QPushButton *m_browseCOutBtn{nullptr};
    QPushButton *m_generateRunBtn{nullptr};
    // 生成选项
    QCheckBox *m_genNoStatic{nullptr};
    QCheckBox *m_genEmitRegistry{nullptr};
    QLineEdit *m_genRegistryNameEdit{nullptr};
    QCheckBox *m_genUtf8Literal{nullptr};
    QLineEdit *m_genUtf8ColsEdit{nullptr};
    QCheckBox *m_genNullSentinel{nullptr};
    QCheckBox *m_genVerbatim{nullptr};
    QCheckBox *m_genFillMissingWithEnglish{nullptr};
    QComboBox *m_genAnnotateCombo{nullptr};
    QSpinBox *m_genPerLineSpin{nullptr};

    // 设置 & 日志
    QTextEdit *m_settingsInfo{nullptr};
    QTextEdit *m_logView{nullptr};

    // 项目设置：语言模块
    QLineEdit *m_projRootEdit{nullptr};
    QPushButton *m_projBrowseRootBtn{nullptr};
    QLineEdit *m_projNewLangEdit{nullptr};
    QPushButton *m_projInitLangBtn{nullptr};
    QPushButton *m_projUndoBtn{nullptr};
    QPushButton *m_projFillEnglishBtn{nullptr};
    QLabel *m_projLogLabel{nullptr};

    // CSV 翻译导入模块 UI
    QLineEdit *m_csvProjEdit{nullptr};
    QPushButton *m_csvProjBrowseBtn{nullptr};
    QLineEdit *m_csvFileEdit{nullptr};
    QPushButton *m_csvFileBrowseBtn{nullptr};
    QProgressBar *m_csvProgress{nullptr};
    QPushButton *m_csvRunBtn{nullptr};
    QPushButton *m_csvRunArraysBtn{nullptr};
    QPushButton *m_csvExportLogBtn{nullptr};
    QTextEdit *m_csvReportView{nullptr};

private slots:
    /**
     * @brief 选择项目根目录（Browse project root path）
     */
    void onBrowseProjectRoot();
    /**
     * @brief 初始化新增语言字段（Initialize and insert a new language field）
     */
    void onInitNewLanguage();
    /**
     * @brief 撤销最近一次语言初始化（Undo last language initialization）
     */
    void onUndoLastInit();
    /**
     * @brief 用英文填充缺失项（Fill missing entries with English）
     */
    void onFillEnglishMissing();

    // 一键提取中文
    /**
     * @brief 一键执行中文专用提取（仅保留含中文的条目）（One-click Chinese-only extraction）
     */
    void onExtractChinese();
    void onExtractArrays();

    // CSV 翻译导入
    /**
     * @brief 选择 CSV 导入的项目根目录（Browse project for CSV import）
     */
    void onBrowseCsvProject();
    /**
     * @brief 选择要导入的 CSV 文件（Browse CSV file to import）
     */
    void onBrowseCsvFile();
    /**
     * @brief 运行 CSV 翻译导入流程（Run CSV translations import process）
     */
    void onRunCsvImport();
    void onRunArrayCsvImport();
    /**
     * @brief 导出 CSV 导入报告日志（Export CSV import report log）
     */
    void onExportCsvLog();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};
#endif // MAINWINDOW_H
