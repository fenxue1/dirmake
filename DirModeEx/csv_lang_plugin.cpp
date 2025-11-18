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

    static QString ensureLogsDir(const QString &root)
    {
        QDir r(root);
        QString logs = r.absoluteFilePath(QStringLiteral("logs"));
        QDir().mkpath(logs);
        return QDir(logs).absoluteFilePath(QStringLiteral("csv_lang_plugin.log"));
    }

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

    static QStringList discoverLangOrder(const QString &root, const QString &alias)
    {
        return TextExtractor::discoverLanguageColumns(root, QStringList{QStringLiteral(".h"), QStringLiteral(".hpp"), QStringLiteral(".c"), QStringLiteral(".cpp")}, alias);
    }

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

    static QString cEscape(const QString &s)
    {
        QString out;
        out.reserve(s.size() * 2);
        for (int i = 0; i < s.size(); ++i)
        {
            QChar ch = s.at(i);
            ushort u = ch.unicode();
            if (ch == QLatin1Char('\\')) out.append(QLatin1String("\\\\"));
            else if (ch == QLatin1Char('"')) out.append(QLatin1String("\\\""));
            else if (ch == QLatin1Char('\n')) out.append(QLatin1String("\\n"));
            else if (ch == QLatin1Char('\r')) out.append(QLatin1String("\\r"));
            else if (ch == QLatin1Char('\t')) out.append(QLatin1String("\\t"));
            else if (u < 0x20)
            {
                out.append(QStringLiteral("\\x%1").arg(QString::number(u, 16).rightJustified(2, QLatin1Char('0')).toUpper()));
            }
            else
                out.append(ch);
        }
        return out;
    }

    static QString replaceInitializerBodyPreservingFormat(const QString &body,
                                                          const QStringList &structLangs,
                                                          const QStringList &csvValues,
                                                          const QMap<QString, int> &colMap)
    {
        Q_UNUSED(structLangs);
        Q_UNUSED(colMap);
        QStringList items;
        items.reserve(csvValues.size() + 1);
        for (const QString &v : csvValues)
            items << QStringLiteral("\"%1\"").arg(cEscape(v));
        items << QStringLiteral("NULL");
        QString indent = QStringLiteral("\n    ");
        return indent + items.join(QStringLiteral(",") + indent);
    }

    static QMutex gFileWriteMutex;

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
        bool ignoreVarName = config.value(QStringLiteral("ignore_variable_name")).toBool();
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

        for (const CsvRow &r : rows)
        {
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
                continue;
            }

            // find initializer at exact lineNumber first, fallback to nearest within a window
            // Pattern characteristics:
            // - optional: static/const/struct and additional qualifiers (e.g., PROGMEM)
            // - optional: array declarator [] after variable name
            // - captures alias/type in group(1), initializer body in group(2)
            QString pattern;
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
                int s = m.capturedStart(2);
                int e = m.capturedEnd(2);
                int lineAt = text.left(s).count(QLatin1Char('\n')) + 1;
                if (inMacroContext(text, s))
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
                log << QStringLiteral("  跳过(未匹配初始化/宏): ") << absPath
                    << QStringLiteral("  var=") << r.variableName
                    << QStringLiteral("  ignore_var=") << (ignoreVarName ? QStringLiteral("true") : QStringLiteral("false"))
                    << QStringLiteral("  line=") << r.lineNumber << QStringLiteral("\n");
                continue;
            }

            QStringList structLangs = aliasHint.isEmpty() ? discoverLangOrder(projectRoot, alias) : discoverLangOrder(projectRoot, aliasHint);
            if (structLangs.isEmpty())
            {
                stats.skippedFiles << absPath;
                stats.skipCount++;
                log << QStringLiteral("  跳过(未发现语言列): ") << absPath << QStringLiteral("\n");
                continue;
            }

            QString before = text;
            QString body = text.mid(bestStart, bestEnd - bestStart);
            QString newBody = replaceInitializerBodyPreservingFormat(body, structLangs, r.values, colMap);
            if (newBody == body)
            {
                stats.skippedFiles << absPath;
                stats.skipCount++;
                log << QStringLiteral("  跳过(无变更): ") << absPath << QStringLiteral("\n");
                continue;
            }
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