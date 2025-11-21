/**
 * @file csv_parser.h
 * @brief CSV 解析模块接口（CSV Parser Module APIs）
 *
 * 功能名称：CSV 解析（CSV Parsing）
 * 主要用途：
 * - 解析带引号与转义的 CSV 文本；
 * - 输出行结构，支持源位置与变量名；
 *
 * 使用示例：
 *  QString err; auto rows = Csv::parseFile(csvPath, err);
 *  if (!err.isEmpty())
 */
#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

/**
 * @brief CSV 行结构（CSV Row structure）
 * @details 包含源文件位置、变量名以及按列顺序的文本值。
 */
struct CsvRow {
    QString sourcePath;   // absolute or project-relative path
    int lineNumber{0};    // line anchor for initializer
    QString variableName; // variable identifier in C source
    QStringList values;   // language texts in CSV order
};

/**
 * @brief CSV 解析报告（CSV Parse Report）
 * @details 保留原始数据统计，不做任何去重；用于完整性校验与报告生成。
 */
struct CsvParseReport {
    int totalLines{0};           // 文件总行数（含空行）
    int nonEmptyLines{0};        // 非空行数
    int parsedRows{0};           // 成功解析的行数
    int duplicateKeyCount{0};    // 重复键行数（source+line+var）
    QMap<QString, int> keyHistogram; // 键频次分布
};

namespace Csv {

/**
 * @brief 严格 CSV 解析，支持引号、转义与 UTF-8（Strict CSV parsing with quotes/escapes, UTF-8）
 * @param csvPath 输入 CSV 文件路径
 * @param error 若失败，返回错误消息（中文优先，含英文）
 * @return CsvRow 列表；包含源位置与变量名（若可用）
 */
QList<CsvRow> parseFile(const QString &csvPath, QString &error);

/**
 * @brief 解析 CSV 并返回统计报告（Parse CSV and return rows with report）
 * @param csvPath 输入 CSV 文件路径
 * @param error 若失败，返回错误消息
 * @param report 输出解析报告（行计数、重复键统计等）
 * @return CsvRow 列表；严格保留原始顺序与重复项
 */
QList<CsvRow> parseFileWithReport(const QString &csvPath, QString &error, CsvParseReport &report);

}

#endif // CSV_PARSER_H
