/**
 * @file csv_lang_plugin.h
 * @brief CSV 语言应用插件接口（CSV Language Apply Plugin APIs）
 *
 * 功能名称：CSV 翻译应用（Apply translations from CSV）
 * 主要用途：
 * - 将 CSV 中的多语言文本应用到 C 结构体初始化块；
 * - 保留格式与注释，生成差异与日志；
 *
 * 使用示例：
 *  QJsonObject cfg; cfg["dry_run"] = true; auto stats = CsvLangPlugin::applyTranslations(root, csv, cfg);
 */
#ifndef CSV_LANG_PLUGIN_H
#define CSV_LANG_PLUGIN_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>

struct CsvProcessStats {
    int successCount{0};
    int skipCount{0};
    int failCount{0};
    int totalCsvLines{0};
    int nonEmptyCsvLines{0};
    int parsedRowCount{0};
    int duplicateKeyCount{0};
    QStringList successFiles;
    QStringList skippedFiles;
    QStringList failedFiles;
    QString logPath;
    QString diffPath;
    QString outputDir; // backups session folder to open after completion
    QString integrityReportPath; // 数据完整性报告路径
};

namespace CsvLangPlugin {

/**
 * @brief 将 CSV 文本应用至 C 源文件中的结构体初始化（Apply CSV translations to C initializers）
 * @param projectRoot 项目根目录
 * @param csvPath CSV 文件路径
 * @param config 配置项（JSON）
 * - `target_globs`: 目标文件匹配；
 * - `struct_alias`: 结构体别名（可省略自动发现）；
 * - `column_mapping`: 列索引映射（相对 CSV 值列）；
 * - `exclude_macros`: 排除宏包裹部分；
 * - `dry_run`: 仅生成差异不写文件；
 * @return CsvProcessStats 处理统计（成功/跳过/失败及文件列表、日志路径、diff 路径等）
 */
CsvProcessStats applyTranslations(const QString &projectRoot,
                                  const QString &csvPath,
                                  const QJsonObject &config);

}

#endif // CSV_LANG_PLUGIN_H