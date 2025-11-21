/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-11 03:22:01
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-11-11 03:23:31
 * @FilePath: \test_mooc-clin\DirModeEx\csv_parser.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @file csv_parser.cpp
 * @brief CSV 解析实现（CSV parsing implementation）
 *
 * 使用示例（Usage）：
 *   QString err; auto rows = Csv::parseFile(path, err);
 *   if (!err.isEmpty())
 */
#include "csv_parser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QTextCodec>
#include <QByteArray>

namespace Csv
{
    /**
     * @brief 行解析算法（Parse a single CSV line with quotes & escapes）
     *
     * 算法逻辑（Algorithm）：
     * - 逐字符遍历，维护 `inQuotes` 与 `escape` 状态；
     * - 双引号内的逗号不作为分隔符；支持反斜杠转义；
     * - 结尾若 `inQuotes` 为真，则判定为错误（未关闭的引号）。
     *
     * 注意事项（Notes）：
     * - 本实现不去除字段外侧空白；调用方可根据需要 `trimmed()`；
     * - 转义目前按字面保留，不做 \xNN 字节解码（保持简单）。
     */
    static bool parseLine(const QString &line, QStringList &outFields, QString &err)
    {
        // RFC4180 风格解析：
        // - 逗号分隔；
        // - 双引号包裹的字段内允许逗号；
        // - 双引号内的转义采用 "" -> "；也兼容 \" -> "；
        outFields.clear();
        QString field;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i)
        {
            const QChar ch = line[i];
            if (inQuotes)
            {
                if (ch == QLatin1Char('"'))
                {
                    // 处理双引号转义："" => "
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                    {
                        field.append(QLatin1Char('"'));
                        ++i; // 跳过第二个引号
                        continue;
                    }
                    // 结束引号字段
                    inQuotes = false;
                    continue;
                }
                // 兼容 \" 形式的转义（非标准 CSV，但常见）
                if (ch == QLatin1Char('\\') && i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                {
                    field.append(QLatin1Char('"'));
                    ++i;
                    continue;
                }
                field.append(ch);
                continue;
            }

            if (ch == QLatin1Char('"'))
            {
                inQuotes = true;
                continue;
            }
            if (ch == QLatin1Char(','))
            {
                outFields << field;
                field.clear();
                continue;
            }
            field.append(ch);
        }
        if (inQuotes)
        {
            err = QStringLiteral("未关闭的引号: %1").arg(line);
            return false;
        }
        outFields << field;
        return true;
    }

    /**
     * @brief 自动编码识别与解码（Auto-detect text codec）
     *
     * 特殊处理（Special Handling）：
     * - BOM 优先：UTF-8、UTF-16LE/BE；
     * - 无 BOM 时在 UTF-8/GB18030/GBK/GB2312 间尝试，选择替换字符最少的解码结果；
     * - 若并列，优先 UTF-8。
     */
    static QString readAllAutoCodec(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        QByteArray data = f.readAll();
        f.close();

        auto countReplacement = [](const QString &s) -> int {
            int cnt = 0;
            for (int i = 0; i < s.size(); ++i)
            {
                if (s.at(i).unicode() == 0xFFFD)
                    ++cnt;
            }
            return cnt;
        };

        // BOM 检测优先
        if (data.startsWith("\xEF\xBB\xBF"))
        {
            return QString::fromUtf8(data.constData() + 3, data.size() - 3);
        }
        if (data.size() >= 2 && (uchar)data[0] == 0xFF && (uchar)data[1] == 0xFE)
        {
            QTextCodec *c = QTextCodec::codecForName("UTF-16LE");
            return c ? c->toUnicode(data) : QString();
        }
        if (data.size() >= 2 && (uchar)data[0] == 0xFE && (uchar)data[1] == 0xFF)
        {
            QTextCodec *c = QTextCodec::codecForName("UTF-16BE");
            return c ? c->toUnicode(data) : QString();
        }

        auto decodeWith = [&](const char *name) -> QString {
            QTextCodec *c = QTextCodec::codecForName(name);
            return c ? c->toUnicode(data) : QString();
        };

        QString utf8 = decodeWith("UTF-8");
        int repUtf8 = countReplacement(utf8);
        QString gb18030 = decodeWith("GB18030");
        int repGb18030 = countReplacement(gb18030);
        QString gbk = decodeWith("GBK");
        int repGbk = countReplacement(gbk);
        QString gb2312 = decodeWith("GB2312");
        int repGb2312 = countReplacement(gb2312);

        // 选择替换字符最少的解码结果，持平时偏好 UTF-8
        QString best = utf8;
        int minRep = repUtf8;
        if (repGb18030 < minRep)
        {
            minRep = repGb18030;
            best = gb18030;
        }
        if (repGbk < minRep)
        {
            minRep = repGbk;
            best = gbk;
        }
        if (repGb2312 < minRep)
        {
            minRep = repGb2312;
            best = gb2312;
        }
        return best;
    }

    /**
     * @brief 解析 CSV 文件（Parse CSV file into rows）
     *
     * 业务规则（Business Rules）：
     * - 第一行允许作为标题，解析时跳过空行；
     * - 每行最少 3 列：sourcePath, lineNumber, variableName，其余为文本列；
     * - 解析失败返回错误信息并中止。
     */
    /**
     * @brief 解析 CSV 主流程（含统计）
     * 处理：自动编码→逐行解析→跳过标题→构造 CsvRow→统计键频次与重复行。
     */
    static QList<CsvRow> doParse(const QString &csvPath, QString &error, int &totalLines, int &nonEmptyLines, QMap<QString,int> &keyHist)
    {
        QList<CsvRow> rows;
        const QString content = readAllAutoCodec(csvPath);
        if (content.isEmpty())
        {
            error = QStringLiteral("无法打开CSV: %1").arg(csvPath);
            return rows;
        }
        const QStringList lines = content.split(QLatin1Char('\n'));
        int lineNo = 0;
        totalLines = lines.size();
        nonEmptyLines = 0;
        bool headerSkipped = false;
        for (QString raw : lines)
        {
            if (!raw.isEmpty() && raw.endsWith(QLatin1Char('\r')))
                raw.chop(1);
            // 移除可能存在的 BOM（U+FEFF），避免首字段包含不可见字符
            if (lineNo == 0 && !raw.isEmpty() && raw[0].unicode() == 0xFEFF)
                raw.remove(0, 1);
            lineNo++;
            if (raw.trimmed().isEmpty())
                continue;
            nonEmptyLines++;
            QStringList fields;
            QString err;
            if (!parseLine(raw, fields, err))
            {
                error = QStringLiteral("第 %1 行解析失败: %2").arg(lineNo).arg(err);
                rows.clear();
                return rows;
            }
            if (fields.size() < 3)
            {
                error = QStringLiteral("第 %1 行字段不足").arg(lineNo);
                rows.clear();
                return rows;
            }

            // 标题行自动跳过：
            // 规则1：首个非空行且第2列非整数，视为标题；
            // 规则2：若包含典型列名（source/line/variable）命中≥2，也视为标题。
            if (!headerSkipped)
            {
                const QString f0 = fields[0].trimmed();
                const QString f1 = fields[1].trimmed();
                const QString f2 = fields[2].trimmed();
                const bool isInt = QRegularExpression(QStringLiteral("^[+-]?\\d+$")).match(f1).hasMatch();
                int tokenHits = 0;
                if (f0.contains(QStringLiteral("source"), Qt::CaseInsensitive)) tokenHits++;
                if (f1.contains(QStringLiteral("line"), Qt::CaseInsensitive)) tokenHits++;
                if (f2.contains(QStringLiteral("variable"), Qt::CaseInsensitive)) tokenHits++;
                if (!isInt || tokenHits >= 2)
                {
                    headerSkipped = true;
                    continue; // 跳过标题行
                }
                headerSkipped = true; // 不是标题也标记，避免后续误跳过
            }

            CsvRow r;
            r.sourcePath = fields[0].trimmed();
            r.lineNumber = fields[1].trimmed().toInt();
            r.variableName = fields[2].trimmed();
            for (int i = 3; i < fields.size(); ++i)
                r.values << fields[i];
            rows << r;
            const QString key = r.sourcePath + QLatin1Char('|') + QString::number(r.lineNumber) + QLatin1Char('|') + r.variableName;
            keyHist[key] = keyHist.value(key, 0) + 1;
        }
        return rows;
    }

    /**
     * @brief 解析 CSV 并返回行（不含统计报告）
     */
    QList<CsvRow> parseFile(const QString &csvPath, QString &error)
    {
        int total = 0, nonEmpty = 0; QMap<QString,int> hist;
        return doParse(csvPath, error, total, nonEmpty, hist);
    }

    /**
     * @brief 解析 CSV 并返回行与统计报告
     */
    QList<CsvRow> parseFileWithReport(const QString &csvPath, QString &error, CsvParseReport &report)
    {
        int total = 0, nonEmpty = 0; QMap<QString,int> hist;
        QList<CsvRow> rows = doParse(csvPath, error, total, nonEmpty, hist);
        report.totalLines = total;
        report.nonEmptyLines = nonEmpty;
        report.parsedRows = rows.size();
        report.keyHistogram = hist;
        int dupCount = 0;
        for (auto it = hist.begin(); it != hist.end(); ++it)
        {
            if (it.value() > 1) dupCount += it.value();
        }
        report.duplicateKeyCount = dupCount;
        return rows;
    }

}
