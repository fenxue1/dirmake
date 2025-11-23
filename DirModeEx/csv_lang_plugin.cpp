/**
 * @file csv_lang_plugin.cpp
 * @brief CSV 语言应用插件实现（Apply translations from CSV implementation）
 *
 * 根据 CSV 的语言列，将翻译应用到结构体初始化；
 * 保留格式与注释，生成日志与差异；支持 dry-run 配置与备份会话目录。
 */
#include "csv_lang_plugin.h"
#include "csv_parser.h"
#include "text_extractor.h"
#include "diff_utils.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>
#include <QDateTime>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>

// Forward declaration for function used before its definition
static QString readFileAutoCodec(const QString &path, QString &chosenCodec, bool &utf8Bom);

namespace CsvLangPlugin
{

/**
 * @brief 确保并返回日志文件路径
 * 目录：项目根下 logs；文件：csv_lang_plugin.log
 */
static QString ensureLogsDir(const QString &root)
    {
        QDir r(root);
        QString logs = r.absoluteFilePath(QStringLiteral("logs"));
        QDir().mkpath(logs);
        return QDir(logs).absoluteFilePath(QStringLiteral("csv_lang_plugin.log"));
    }

/**
 * @brief 创建并返回备份会话目录
 * 基础目录：项目根下 .csv_lang_backups/时间戳
 */
static QString backupsSessionDir(const QString &root)
    {
        QDir r(root);
        QString base = r.absoluteFilePath(QStringLiteral(".csv_lang_backups"));
        QDir().mkpath(base);
        QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
        QString sess = QDir(base).absoluteFilePath(ts);
        QDir().mkpath(sess);
        return sess;
    }

/**
 * @brief 判断匹配是否出现在宏定义体内（粗略启发式）
 */
static bool inMacroContext(const QString &text, int matchStart)
    {
        // Skip if the initializer appears within a macro body (naive heuristic)
        int lineStart = text.lastIndexOf(QLatin1Char('\n'), matchStart);
        if (lineStart < 0)
            lineStart = 0;
        int prevDefine = text.lastIndexOf(QStringLiteral("#define"), matchStart);
        if (prevDefine < 0)
            return false;
        // If #define is after last newline, we consider match within macro
        return prevDefine > lineStart;
    }

/**
 * @brief 判断位置是否处于注释中（// 或  ）
 */
static bool inCommentContext(const QString &text, int pos)
    {
        int lineStart = text.lastIndexOf(QLatin1Char('\n'), pos);
        if (lineStart < 0) lineStart = 0;
        int slc = text.lastIndexOf(QStringLiteral("//"), pos);
        if (slc >= 0 && slc > lineStart)
            return true;
        int open = text.lastIndexOf(QStringLiteral("/*"), pos);
        int close = text.lastIndexOf(QStringLiteral("*/"), pos);
        return open >= 0 && open > close;
    }

/**
 * @brief 发现结构体语言字段顺序
 */
static QStringList discoverLangOrder(const QString &root, const QString &alias)
    {
        return TextExtractor::discoverLanguageColumns(root, QStringList{QStringLiteral(".h"), QStringLiteral(".hpp"), QStringLiteral(".c"), QStringLiteral(".cpp")}, alias);
    }

static bool findNthBraceBlock(const QString &body, int nth, int &startContent, int &endContent)
    {
        int level = 0; int count = 0; bool inq = false;
        for (int i = 0; i < body.size(); ++i)
        {
            QChar c = body[i];
            if (!inq && c == QLatin1Char('"')) { inq = true; continue; }
            if (inq)
            {
                if (c == QLatin1Char('"'))
                {
                    if (i + 1 < body.size() && body[i + 1] == QLatin1Char('"')) { ++i; }
                    else { inq = false; }
                }
                continue;
            }
            if (c == QLatin1Char('{'))
            {
                if (level == 0)
                {
                    ++count;
                    if (count == nth)
                    {
                        startContent = i + 1;
                        int j = i + 1; int inner = 1; bool inq2 = false;
                        while (j < body.size())
                        {
                            QChar d = body[j];
                            if (!inq2 && d == QLatin1Char('"')) { inq2 = true; ++j; continue; }
                            if (inq2)
                            {
                                if (d == QLatin1Char('"'))
                                {
                                    if (j + 1 < body.size() && body[j + 1] == QLatin1Char('"')) { j += 2; continue; }
                                    else { inq2 = false; ++j; continue; }
                                }
                                ++j; continue;
                            }
                            if (d == QLatin1Char('{')) { ++inner; }
                            else if (d == QLatin1Char('}')) { --inner; if (inner == 0) { endContent = j; return true; } }
                            ++j;
                        }
                        return false;
                    }
                }
                ++level;
            }
            else if (c == QLatin1Char('}'))
            {
                --level;
            }
        }
        return false;
    }

/**
 * @brief 解析 CSV 行（简版，支持双引号与转义）
 */
static QStringList parseCsvLineSimple(const QString &line)
    {
        QStringList out; QString cur; bool inq = false;
        for (int i = 0; i < line.size(); ++i)
        {
            QChar c = line[i];
            if (inq)
            {
                if (c == QLatin1Char('"'))
                {
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                    { cur.append(QLatin1Char('"')); ++i; }
                    else { inq = false; }
                }
                else if (c == QLatin1Char('\\') && i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                { cur.append(QLatin1Char('"')); ++i; }
                else { cur.append(c); }
            }
            else
            {
                if (c == QLatin1Char('"')) { inq = true; }
                else if (c == QLatin1Char(',')) { out << cur; cur.clear(); }
                else { cur.append(c); }
            }
        }
        out << cur; return out;
    }

/**
 * @brief 将字符串转为 C 字面量安全形式（转义 \\"、\n、\r、\t）
 */
static QString cEscape(const QString &s)
    {
        QString out;
        out.reserve(s.size() * 2);
        for (int i = 0; i < s.size(); ++i)
        {
            QChar ch = s.at(i);
            if (ch == QLatin1Char('"')) out.append(QLatin1String("\\\""));
            else if (ch == QLatin1Char('\n')) out.append(QLatin1String("\\n"));
            else if (ch == QLatin1Char('\r')) out.append(QLatin1String("\\r"));
            else if (ch == QLatin1Char('\t')) out.append(QLatin1String("\\t"));
            else out.append(ch);
        }
        return out;
    }

/**
 * @brief 用 CSV 值替换初始化体，尽量保留原缩进/格式
 */
static QString replaceInitializerBodyPreservingFormat(const QString &body,
                                                          const QStringList &structLangs,
                                                          const QStringList &csvValues,
                                                          const QMap<QString, int> &colMap,
                                                          const QString &annotateMode)
    {
        QString indent = QStringLiteral("\n    ");
        int nl = body.indexOf(QLatin1Char('\n'));
        if (nl >= 0)
        {
            int i = nl + 1; int spaces = 0;
            while (i < body.size() && (body.at(i) == QLatin1Char(' ') || body.at(i) == QLatin1Char('\t'))) { spaces++; i++; }
            indent = QStringLiteral("\n") + QString(spaces, QLatin1Char(' '));
        }
        bool hasTailNull = QRegularExpression(QStringLiteral("\n\s*NULL\s*$")).match(body.trimmed()).hasMatch();

        int enIdx = colMap.value(QStringLiteral("en"), -1);
        QString out;
        for (int k = 0; k < structLangs.size(); ++k)
        {
            QString lang = structLangs.at(k);
            int idx = colMap.value(lang, -1);
            QString v = (idx >= 0 && idx < csvValues.size()) ? csvValues.at(idx) : QString();
            if (v.isEmpty() && enIdx >= 0 && enIdx < csvValues.size())
                v = csvValues.at(enIdx);
            QString tail;
            if (annotateMode == QStringLiteral("names")) tail = QStringLiteral(" // %1").arg(lang);
            else if (annotateMode == QStringLiteral("indices")) tail = QStringLiteral(" // [%1]").arg(k);
            out += indent + QStringLiteral("\"%1\",%2").arg(cEscape(v), tail);
        }
        QString sentinelTail;
        if (annotateMode == QStringLiteral("names")) sentinelTail = QStringLiteral(" // sentinel");
        else if (annotateMode == QStringLiteral("indices")) sentinelTail = QStringLiteral(" // [sentinel]");
        out += indent + QStringLiteral("NULL%1").arg(sentinelTail);
        if (!hasTailNull)
            out += QStringLiteral("\n");
        return out;
    }

/**
 * @brief 根据提取的数组元素重建数组初始化体
 * 规则：每元素 { values, NULL }，保留尾部 NULL 与缩进。
 */
static QString buildArrayBody(const QString &origBody,
                                  const QList<QStringList> &elements,
                                  const QMap<QString, int> &colMap)
    {
        QString indent = QStringLiteral("\n    ");
        int nl = origBody.indexOf(QLatin1Char('\n'));
        if (nl >= 0)
        {
            int i = nl + 1; int spaces = 0;
            while (i < origBody.size() && (origBody.at(i) == QLatin1Char(' ') || origBody.at(i) == QLatin1Char('\t'))) { spaces++; i++; }
            indent = QStringLiteral("\n") + QString(spaces, QLatin1Char(' '));
        }
        bool hasTailNull = QRegularExpression(QStringLiteral("\n\s*NULL\s*$")).match(origBody.trimmed()).hasMatch();
        int enIdx = colMap.value(QStringLiteral("en"), -1);
        QString out;
        for (int k = 0; k < elements.size(); ++k)
        {
            const QStringList &vals = elements.at(k);
            QStringList items;
            int n = vals.size();
            for (int i = 0; i < n; ++i)
            {
                QString v = vals.at(i);
                if (v.isEmpty() && enIdx >= 0 && enIdx < n) v = vals.at(enIdx);
                items << QStringLiteral("\"%1\"").arg(cEscape(v));
            }
            items << QStringLiteral("NULL");
            QString elem = QStringLiteral("{ %1 }").arg(items.join(QStringLiteral(", ")));
            if (hasTailNull || k < elements.size() - 1) elem += QStringLiteral(",");
            out += indent + elem;
        }
        if (hasTailNull)
            out += indent + QStringLiteral("NULL") + QStringLiteral("\n");
        else
            out += QStringLiteral("\n");
        return out;
    }

    static QMutex gFileWriteMutex;

/**
 * @brief 将 CSV 翻译应用至 C 源文件中的初始化块
 * 过程：解析 CSV→定位初始化（按行/邻近窗口）→替换正文→写差异与日志→备份或沙箱输出。
 */
CsvProcessStats applyTranslations(const QString &projectRoot,
                                  const QString &csvPath,
                                  const QJsonObject &config)
    {
        CsvProcessStats stats;
        QString logPath = config.contains(QStringLiteral("log_path"))
                                  ? config.value(QStringLiteral("log_path")).toString()
                                  : ensureLogsDir(projectRoot);
        QFile logF(logPath);
        logF.open(QIODevice::Append);
        QTextStream log(&logF);
        log.setCodec("UTF-8");
        bool disableBackups = config.value(QStringLiteral("disable_backups")).toBool();
        QString sandboxDir = config.value(QStringLiteral("sandbox_output_dir")).toString();
        bool useSandbox = !sandboxDir.isEmpty();
        QString sessDir;
        if (!disableBackups)
        {
            sessDir = config.contains(QStringLiteral("backups_dir"))
                          ? config.value(QStringLiteral("backups_dir")).toString()
                          : backupsSessionDir(projectRoot);
            if (!sessDir.isEmpty()) QDir().mkpath(sessDir);
        }
            QString diffPath = config.contains(QStringLiteral("diff_path"))
                                   ? config.value(QStringLiteral("diff_path")).toString()
                                   : QDir(projectRoot).absoluteFilePath(QStringLiteral("logs/csv_lang_plugin.diff"));
            stats.logPath = logPath;
            stats.diffPath = diffPath;
        QFile diffF(diffPath);
        diffF.open(QIODevice::WriteOnly | QIODevice::Truncate);
        QTextStream diffOut(&diffF);
        diffOut.setCodec("UTF-8");

        log << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
            << QStringLiteral(" BEGIN csv import root=") << projectRoot << QStringLiteral(" csv=") << csvPath << QStringLiteral("\n");

        QString err;
        CsvParseReport parseReport;
        QList<CsvRow> rows = Csv::parseFileWithReport(csvPath, err, parseReport);
        if (!err.isEmpty())
        {
            log << QStringLiteral("CSV解析失败: ") << err << QStringLiteral("\n");
            stats.failCount = 0;
            stats.logPath = logPath;
            stats.diffPath = diffPath;
            stats.outputDir = sessDir;
            logF.close();
            diffF.close();
            return stats;
        }

        // 记录解析统计信息（不做任何去重，仅报告）
        stats.totalCsvLines = parseReport.totalLines;
        stats.nonEmptyCsvLines = parseReport.nonEmptyLines;
        stats.parsedRowCount = parseReport.parsedRows;
        stats.duplicateKeyCount = parseReport.duplicateKeyCount;
        QString integPath = QDir(projectRoot).absoluteFilePath(QStringLiteral("logs/csv_integrity_report.log"));
        QFile integF(integPath);
        integF.open(QIODevice::WriteOnly | QIODevice::Truncate);
        QTextStream integOut(&integF);
        integOut.setCodec("UTF-8");
        integOut << QStringLiteral("CSV 完整性报告\n");
        integOut << QStringLiteral("总行数(含空行): ") << stats.totalCsvLines << QStringLiteral("\n");
        integOut << QStringLiteral("非空行数: ") << stats.nonEmptyCsvLines << QStringLiteral("\n");
        integOut << QStringLiteral("解析行数: ") << stats.parsedRowCount << QStringLiteral("\n");
        integOut << QStringLiteral("重复键行数: ") << stats.duplicateKeyCount << QStringLiteral("\n\n");
        integOut << QStringLiteral("键频次分布(key|count):\n");
        for (auto it = parseReport.keyHistogram.begin(); it != parseReport.keyHistogram.end(); ++it)
        {
            integOut << it.key() << QStringLiteral("|") << it.value() << QStringLiteral("\n");
        }
        integF.close();
        stats.integrityReportPath = integPath;
        log << QStringLiteral("CSV解析统计: 总行%1 非空%2 解析%3 重复键行%4 报告=%5\n")
               .arg(stats.totalCsvLines)
               .arg(stats.nonEmptyCsvLines)
               .arg(stats.parsedRowCount)
               .arg(stats.duplicateKeyCount)
               .arg(integPath);

        // config
        QString aliasHint = config.value(QStringLiteral("struct_alias")).toString();
        bool dryRun = config.value(QStringLiteral("dry_run")).toBool();
        bool strictLineOnly = config.value(QStringLiteral("strict_line_only")).toBool();
        int lineWindow = config.value(QStringLiteral("line_window")).toInt();
        bool ignoreVarNameDefault = config.value(QStringLiteral("ignore_variable_name")).toBool();
        QString annotateMode = config.value(QStringLiteral("annotate_mode")).toString();
        if (annotateMode.isEmpty()) annotateMode = QStringLiteral("none");
        bool onlyInfo = config.value(QStringLiteral("only_info")).toBool();
        QString sourcePathFilter = config.value(QStringLiteral("source_path_filter")).toString();
        int filterLineStart = config.value(QStringLiteral("line_start")).toInt();
        int filterLineEnd = config.value(QStringLiteral("line_end")).toInt();
        if (lineWindow <= 0) lineWindow = 50;
        QJsonObject mapObj = config.value(QStringLiteral("column_mapping")).toObject();
        QMap<QString, int> colMap;
        for (auto it = mapObj.begin(); it != mapObj.end(); ++it)
            colMap[it.key()] = it.value().toInt();

        if (colMap.isEmpty())
        {
            QString chosenCodec; bool bom = false;
            QString csvText = readFileAutoCodec(csvPath, chosenCodec, bom);
            QStringList lines = csvText.split(QLatin1Char('\n'));
            QString header;
            for (const QString &ln : lines)
            {
                QString raw = ln;
                if (!raw.isEmpty() && raw.endsWith(QLatin1Char('\r'))) raw.chop(1);
                if (raw.trimmed().isEmpty()) continue;
                header = raw; break;
            }
            if (!header.isEmpty())
            {
                if (header[0].unicode() == 0xFEFF) header.remove(0, 1);
                QStringList hdrs = parseCsvLineSimple(header);
                if (hdrs.size() > 3)
                {
                    for (int i = 3; i < hdrs.size(); ++i)
                    {
                        QString h = hdrs[i].trimmed();
                        QString shortName = h.startsWith(QLatin1String("text_")) ? h.mid(5) : h;
                        colMap[shortName] = i - 3; // index relative to CsvRow.values
                    }
                }
            }
        }

        int iRow = 0;
        while (iRow < rows.size())
        {
            const CsvRow &r = rows.at(iRow);
            QRegularExpression reNested(QStringLiteral(R"(^([A-Za-z_]\w*)\._(title|info)$)"));
            auto nestedMatch = reNested.match(r.variableName);
            if (nestedMatch.hasMatch())
            {
                QString baseVar = nestedMatch.captured(1);
                QString which = nestedMatch.captured(2);
                if (onlyInfo && which != QStringLiteral("info")) { iRow++; continue; }
                QString absPath = r.sourcePath;
                if (QDir::isRelativePath(absPath))
                    absPath = QDir(projectRoot).absoluteFilePath(absPath);
                if (!sourcePathFilter.isEmpty())
                {
                    QString normRow = QDir::fromNativeSeparators(absPath);
                    QString normFilter = QDir::fromNativeSeparators(sourcePathFilter);
                    if (!normRow.endsWith(normFilter) && normRow != normFilter)
                    { iRow++; continue; }
                }
                QFileInfo fi(absPath);
                if (!fi.exists() || !fi.isFile())
                {
                    stats.failedFiles << absPath; stats.failCount++;
                    iRow++; continue;
                }
                QString codec; bool bom = false; QFile f(absPath);
                if (!f.open(QIODevice::ReadOnly)) { stats.failedFiles << absPath; stats.failCount++; iRow++; continue; }
                f.close();
                QString text = readFileAutoCodec(absPath, codec, bom);
                QRegularExpression reOuter(QStringLiteral(R"((?:static\s+)?(?:const\s+)?DispMessageInfo\s+%1\s*=\s*\{(.*?)\};)" ).arg(QRegularExpression::escape(baseVar)), QRegularExpression::DotMatchesEverythingOption);
                auto m = reOuter.match(text);
                if (!m.hasMatch()) { stats.skippedFiles << absPath; stats.skipCount++; iRow++; continue; }
                int declPos = m.capturedStart(0);
                int declLine = text.left(declPos).count(QLatin1Char('\n')) + 1;
                if (strictLineOnly)
                {
                    int anchor = declLine;
                    Q_UNUSED(anchor);
                }
                if (filterLineStart > 0 && filterLineEnd > 0)
                {
                    if (!(declLine >= filterLineStart && declLine <= filterLineEnd))
                    { iRow++; continue; }
                }
                int sBody = m.capturedStart(1);
                int eBody = m.capturedEnd(1);
                QString body = text.mid(sBody, eBody - sBody);
                int s1=-1,e1=-1,s2=-1,e2=-1;
                if (!findNthBraceBlock(body, 1, s1, e1)) { stats.skippedFiles << absPath; stats.skipCount++; iRow++; continue; }
                if (!findNthBraceBlock(body, 2, s2, e2)) { stats.skippedFiles << absPath; stats.skipCount++; iRow++; continue; }
                int contentStart = (which == QStringLiteral("title")) ? s1 : s2;
                int contentEnd   = (which == QStringLiteral("title")) ? e1 : e2;
                QString inner = body.mid(contentStart, contentEnd - contentStart);
                QRegularExpression reQuoted(QStringLiteral("\"(?:\\.|[^\"\\])*\""));
                int present = 0; auto qi = reQuoted.globalMatch(inner); while (qi.hasNext()) { qi.next(); ++present; }
                QStringList structLangs = TextExtractor::defaultLanguageColumns();
                if (present > 0 && present < structLangs.size())
                    structLangs = structLangs.mid(0, present);
                QString newInner = replaceInitializerBodyPreservingFormat(inner, structLangs, r.values, colMap, annotateMode);
                QString newBody = body;
                newBody.replace(contentStart, contentEnd - contentStart, newInner);
                QString before = text; QString after = text;
                after.replace(sBody, eBody - sBody, newBody);
                diffOut << DiffUtils::unifiedDiff(absPath, before, after);
                QMutexLocker writeLock(&gFileWriteMutex);
                QFile wf(absPath);
                if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    if (bom && codec == QStringLiteral("UTF-8")) wf.write("\xEF\xBB\xBF");
                    QTextStream wr(&wf); wr.setCodec(codec.toUtf8().constData()); wr << after; wf.close();
                    stats.successFiles << absPath; stats.successCount++;
                }
                else { stats.failedFiles << absPath; stats.failCount++; }
                iRow++;
                continue;
            }
            QString absPath = r.sourcePath;
            if (QDir::isRelativePath(absPath))
                absPath = QDir(projectRoot).absoluteFilePath(absPath);
            QFileInfo fi(absPath);
            if (!fi.exists() || !fi.isFile())
            {
                stats.failedFiles << absPath;
                stats.failCount++;
                log << QStringLiteral("  路径无效: ") << absPath << QStringLiteral("\n");
                continue;
            }
            QFile f(absPath);
            if (!f.open(QIODevice::ReadOnly))
            {
                stats.failedFiles << absPath;
                stats.failCount++;
                log << QStringLiteral("  无法读取: ") << absPath << QStringLiteral("\n");
                continue;
            }
            f.close();
            QString codec;
            bool bom = false;
            QString text = readFileAutoCodec(absPath, codec, bom);
            if (text.isEmpty())
            {
                stats.failedFiles << absPath;
                stats.failCount++;
                log << QStringLiteral("  无法解码: ") << absPath << QStringLiteral("\n");
                iRow++;
                continue;
            }

            // 处理数组CSV：variableName 形如 "...var[]" 作为头行，后续空var行作为元素
            QRegularExpression reArrHeader(QStringLiteral("([A-Za-z_]\\w*)\\s*\\[\\s*\\]$"));
            auto arrMatch = reArrHeader.match(r.variableName);
            if (arrMatch.hasMatch())
            {
                QString varName = arrMatch.captured(1);
                QString patt = QStringLiteral(R"((?:static\s+)?(?:const\s+)?(?:struct\s+)?(\w+)(?:\s+\w+)*\s+(?:\*+\s*)?%1\s*\[[^\]]*\]\s*=\s*\{(.*?)\};)" ).arg(QRegularExpression::escape(varName));
                QRegularExpression reArr(patt, QRegularExpression::DotMatchesEverythingOption);
                int anchorLine = r.lineNumber > 0 ? r.lineNumber : 1;
                QStringList linesText = text.split(QLatin1Char('\n'));
                QVector<int> offsets(linesText.size() + 1);
                int acc = 0;
                for (int i = 0; i < linesText.size(); ++i) { offsets[i] = acc; acc += linesText[i].size() + 1; }
                offsets[linesText.size()] = acc;
                int startIdx = qBound(0, anchorLine - 1 - 50, linesText.size());
                int endIdx = qBound(0, anchorLine - 1 + 200, linesText.size());
                int subStart = offsets[startIdx];
                int subLen = offsets[endIdx] - subStart;
                if (subLen < 0 || subStart < 0 || subStart >= text.size()) subStart = 0, subLen = text.size();
                QString sub = text.mid(subStart, subLen);
                auto m = reArr.match(sub);
                if (!m.hasMatch())
                {
                    m = reArr.match(text);
                    if (!m.hasMatch())
                    {
                        stats.skippedFiles << absPath;
                        stats.skipCount++;
                        iRow++;
                        continue;
                    }
                    int s = m.capturedStart(2);
                    int e = m.capturedEnd(2);
                    QString origBody = text.mid(s, e - s);
                    QList<QStringList> elems;
                    iRow++;
                    while (iRow < rows.size())
                    {
                        const CsvRow &ri = rows.at(iRow);
                        if (!ri.variableName.isEmpty()) break;
                        elems.append(ri.values);
                        iRow++;
                    }
                    QString newBody = buildArrayBody(origBody, elems, colMap);
                    QString before = text;
                    QString after = text;
                    after.replace(s, e - s, newBody);
                    diffOut << DiffUtils::unifiedDiff(absPath, before, after);
                    QMutexLocker writeLock(&gFileWriteMutex);
                    QFile f2(absPath);
                    if (f2.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        if (bom && codec == QStringLiteral("UTF-8")) f2.write("\xEF\xBB\xBF");
                        QTextStream wr(&f2); wr.setCodec(codec.toUtf8().constData()); wr << after; f2.close();
                        stats.successFiles << absPath; stats.successCount++;
                        log << QStringLiteral("  修改(数组): ") << QDir(projectRoot).relativeFilePath(absPath) << QStringLiteral("  var=") << varName << QStringLiteral("\n");
                    }
                    else
                    {
                        stats.failedFiles << absPath; stats.failCount++;
                    }
                    continue;
                }
                int s = subStart + m.capturedStart(2);
                int e = subStart + m.capturedEnd(2);
                QString origBody = text.mid(s, e - s);
                // 收集元素行
                QList<QStringList> elems;
                iRow++;
                while (iRow < rows.size())
                {
                    const CsvRow &ri = rows.at(iRow);
                    if (!ri.variableName.isEmpty()) break;
                    QStringList vals = ri.values;
                    if (!vals.isEmpty())
                    {
                        QString v0 = vals.first().trimmed();
                        if (v0 == (varName + QStringLiteral("[]"))) vals.removeFirst();
                    }
                    elems.append(vals);
                    iRow++;
                }
                QString newBody = buildArrayBody(origBody, elems, colMap);
                QString before = text;
                QString after = text;
                after.replace(s, e - s, newBody);
                diffOut << DiffUtils::unifiedDiff(absPath, before, after);
                QMutexLocker writeLock(&gFileWriteMutex);
                QFile f2(absPath);
                if (f2.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    if (bom && codec == QStringLiteral("UTF-8")) f2.write("\xEF\xBB\xBF");
                    QTextStream wr(&f2); wr.setCodec(codec.toUtf8().constData()); wr << after; f2.close();
                    stats.successFiles << absPath; stats.successCount++;
                    log << QStringLiteral("  修改(数组): ") << QDir(projectRoot).relativeFilePath(absPath) << QStringLiteral("  var=") << varName << QStringLiteral("\n");
                }
                else
                {
                    stats.failedFiles << absPath; stats.failCount++;
                }
                continue;
            }

            // find initializer at exact lineNumber first, fallback to nearest within a window
            // Pattern characteristics:
            // - optional: static/const/struct and additional qualifiers (e.g., PROGMEM)
            // - optional: array declarator [] after variable name
            // - captures alias/type in group(1), initializer body in group(2)
            QString pattern;
            bool ignoreVarName = ignoreVarNameDefault;
            if (!r.variableName.isEmpty())
                ignoreVarName = false;
            if (ignoreVarName)
            {
                // 支持复合字面量地址取值：=&(type){...}
                pattern = QStringLiteral(R"((?:static\s+)?(?:const\s+)?(?:struct\s+)?(\w+)(?:\s+\w+)*\s+(?:\*+\s*)?\w+\s*(?:\[[^\]]*\])?\s*=\s*(?:&\s*\([^)]*\)\s*)?\{(.*?)\};)" );
            }
            else
            {
                pattern = QStringLiteral(R"((?:static\s+)?(?:const\s+)?(?:struct\s+)?(\w+)(?:\s+\w+)*\s+(?:\*+\s*)?%1\s*(?:\[[^\]]*\])?\s*=\s*(?:&\s*\([^)]*\)\s*)?\{(.*?)\};)" )
                              .arg(QRegularExpression::escape(r.variableName));
            }
            QRegularExpression reInitVar(pattern, QRegularExpression::DotMatchesEverythingOption);
            auto it = reInitVar.globalMatch(text);
            bool matched = false;
            int bestStart = -1;
            int bestEnd = -1;
            QString alias;
            bool matchedExact = false;
            int exactStart = -1;
            int exactEnd = -1;
            QString aliasExact;
            int nearestDist = std::numeric_limits<int>::max();
            int nearestStart = -1;
            int nearestEnd = -1;
            QString aliasNearest;
            while (it.hasNext())
            {
                auto m = it.next();
                int declPos = m.capturedStart(0);
                int s = m.capturedStart(2);
                int e = m.capturedEnd(2);
                int lineAt = text.left(declPos).count(QLatin1Char('\n')) + 1;
                QString declText = m.captured(0);
                int eqPos = declText.indexOf(QLatin1Char('='));
                if (eqPos > 0)
                {
                    QString left = declText.left(eqPos);
                    if (left.contains(QLatin1Char('[')))
                    {
                        continue; // 跳过结构体数组声明
                    }
                }
                if (inMacroContext(text, s))
                {
                    continue;
                }
                if (inCommentContext(text, s))
                {
                    continue;
                }
                int dist = qAbs(lineAt - r.lineNumber);
                if (dist == 0)
                {
                    matchedExact = true;
                    exactStart = s;
                    exactEnd = e;
                    aliasExact = m.captured(1);
                    break; // exact match wins
                }
                if (dist < nearestDist)
                {
                    nearestDist = dist;
                    nearestStart = s;
                    nearestEnd = e;
                    aliasNearest = m.captured(1);
                }
            }
            if (matchedExact)
            {
                matched = true;
                bestStart = exactStart;
                bestEnd = exactEnd;
                alias = aliasExact;
            }
            else if (!strictLineOnly && nearestStart >= 0 && nearestDist <= lineWindow)
            {
                matched = true;
                bestStart = nearestStart;
                bestEnd = nearestEnd;
                alias = aliasNearest;
            }
            if (!matched)
            {
                stats.skippedFiles << absPath;
                stats.skipCount++;
                log << QStringLiteral("  跳过(未匹配初始化/宏/注释/数组): ") << absPath
                    << QStringLiteral("  var=") << r.variableName
                    << QStringLiteral("  ignore_var=") << (ignoreVarName ? QStringLiteral("true") : QStringLiteral("false"))
                    << QStringLiteral("  line=") << r.lineNumber << QStringLiteral("\n");
                iRow++;
                continue;
            }

            QStringList structLangs = aliasHint.isEmpty() ? discoverLangOrder(projectRoot, alias) : discoverLangOrder(projectRoot, aliasHint);
            if (structLangs.isEmpty())
            {
                stats.skippedFiles << absPath;
                stats.skipCount++;
                log << QStringLiteral("  跳过(未发现语言列): ") << absPath << QStringLiteral("\n");
                iRow++;
                continue;
            }

            QString before = text;
            QString body = text.mid(bestStart, bestEnd - bestStart);
            QString newBody = replaceInitializerBodyPreservingFormat(body, structLangs, r.values, colMap, annotateMode);
            QString after = text;
            after.replace(bestStart, bestEnd - bestStart, newBody);

            // write diff
            diffOut << DiffUtils::unifiedDiff(absPath, before, after);

            // backup and write (serialize writes to avoid races)
            QMutexLocker writeLock(&gFileWriteMutex);
            QString rel = QDir(projectRoot).relativeFilePath(absPath);
            if (!sessDir.isEmpty())
            {
                QString backupPath = QDir(sessDir).absoluteFilePath(rel);
                QDir().mkpath(QFileInfo(backupPath).dir().absolutePath());
                QFile bf(backupPath);
                if (bf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    QTextStream bts(&bf);
                    bts.setCodec("GB2312");
                    bts << before;
                    bf.close();
                }
            }
            if (!dryRun)
            {
                if (useSandbox)
                {
                    QString targetPath = QDir(sandboxDir).absoluteFilePath(rel);
                    QDir().mkpath(QFileInfo(targetPath).dir().absolutePath());
                    QFile tf(targetPath);
                    if (tf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        if (bom && codec == QStringLiteral("UTF-8"))
                            tf.write("\xEF\xBB\xBF");
                        QTextStream wr(&tf);
                        wr.setCodec(codec.toUtf8().constData());
                        wr << after;
                        tf.close();
                        stats.successFiles << targetPath;
                        stats.successCount++;
                        log << QStringLiteral("  写入沙箱: ") << QDir(projectRoot).relativeFilePath(targetPath) << QStringLiteral("\n");
                    }
                }
                else if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    // 保留原文件编码与 UTF-8 BOM 状态
                    if (bom && codec == QStringLiteral("UTF-8"))
                        f.write("\xEF\xBB\xBF");
                    QTextStream wr(&f);
                    wr.setCodec(codec.toUtf8().constData());
                    wr << after;
                    f.close();
                    stats.successFiles << absPath;
                    stats.successCount++;
                    log << QStringLiteral("  修改: ") << rel << QStringLiteral("\n");
                }
            }
            else
            {
                stats.successFiles << absPath;
                stats.successCount++;
                log << QStringLiteral("  预览修改: ") << rel << QStringLiteral("\n");
            }
            iRow++;
        }

        log << QStringLiteral("成功:") << stats.successCount << QStringLiteral(" 跳过:") << stats.skipCount << QStringLiteral(" 失败:") << stats.failCount << QStringLiteral("\n");
        log << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) << QStringLiteral(" END\n\n");
        logF.close();
        diffF.close();
        stats.logPath = logPath;
        stats.diffPath = diffPath;
        stats.outputDir = useSandbox ? sandboxDir : sessDir;
        return stats;
    }

}
    static QString decodeWithCodec(const QByteArray &data, const char *name)
    {
        QTextCodec *c = QTextCodec::codecForName(name);
        return c ? c->toUnicode(data) : QString();
    }

    // 自动编码检测：返回文本，并输出所选编码与是否存在 UTF-8 BOM
    static QString readFileAutoCodec(const QString &path, QString &chosenCodec, bool &utf8Bom)
    {
        chosenCodec.clear();
        utf8Bom = false;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        QByteArray data = f.readAll();
        f.close();

        if (data.startsWith("\xEF\xBB\xBF"))
        {
            utf8Bom = true;
            chosenCodec = QStringLiteral("UTF-8");
            return QString::fromUtf8(data.constData() + 3, data.size() - 3);
        }
        if (data.size() >= 2 && (uchar)data[0] == 0xFF && (uchar)data[1] == 0xFE)
        {
            chosenCodec = QStringLiteral("UTF-16LE");
            QTextCodec *c = QTextCodec::codecForName("UTF-16LE");
            return c ? c->toUnicode(data) : QString();
        }
        if (data.size() >= 2 && (uchar)data[0] == 0xFE && (uchar)data[1] == 0xFF)
        {
            chosenCodec = QStringLiteral("UTF-16BE");
            QTextCodec *c = QTextCodec::codecForName("UTF-16BE");
            return c ? c->toUnicode(data) : QString();
        }

        auto countReplacement = [](const QString &s) -> int {
            int cnt = 0;
            for (int i = 0; i < s.size(); ++i)
                if (s.at(i).unicode() == 0xFFFD)
                    ++cnt;
            return cnt;
        };

        QString utf8 = decodeWithCodec(data, "UTF-8");
        int repUtf8 = countReplacement(utf8);
        QString gb18030 = decodeWithCodec(data, "GB18030");
        int repGb18030 = countReplacement(gb18030);
        QString gbk = decodeWithCodec(data, "GBK");
        int repGbk = countReplacement(gbk);
        QString gb2312 = decodeWithCodec(data, "GB2312");
        int repGb2312 = countReplacement(gb2312);

        // 选择替换字符最少的解码结果；持平时偏好 GB18030
        QString best = gb18030;
        int minRep = repGb18030;
        chosenCodec = QStringLiteral("GB18030");
        auto consider = [&](const QString &s, int rep, const QString &codec) {
            if (rep < minRep)
            {
                minRep = rep;
                best = s;
                chosenCodec = codec;
            }
        };
        consider(utf8, repUtf8, QStringLiteral("UTF-8"));
        consider(gbk, repGbk, QStringLiteral("GBK"));
        consider(gb2312, repGb2312, QStringLiteral("GB2312"));
        return best;
    }
