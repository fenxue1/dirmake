/**
 * @file language_settings.cpp
 * @brief 项目语言设置实现（Language settings implementation）
 *
 * 提供：新增语言字段、撤销最近一次初始化、用英文填充缺失项；
 * 保留原文件编码与 BOM，生成日志与备份目录，输出统计结果。
 */
#include "language_settings.h"
#include <QDir>
#include <QFileInfoList>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDateTime>
#include <QRegularExpression>
#include "text_extractor.h"

namespace ProjectLang
{

    // forward declarations for helpers defined later in file
    static QStringList listSourceFiles(const QString &root);
    static QStringList collectAliasesWithTextFields(const QString &root);
    static QString rewriteInitializersInText(const QString &text,
                                             const QString &alias,
                                             const QStringList &structLangs,
                                             const QString &newLangCode,
                                             bool &changed);
    static QString readFileAutoCodec(const QString &path, QString &chosenCodec, bool &utf8Bom);
    static bool writeTextWithCodec(const QString &path, const QString &text, const QString &codec, bool utf8Bom);

    static QString ensureLogsDir(const QString &root)
    {
        QDir r(root);
        QString logs = r.absoluteFilePath("logs");
        QDir().mkpath(logs);
        return QDir(logs).absoluteFilePath("lang_init.log");
    }

    static QString backupsSessionDir(const QString &root)
    {
        QDir r(root);
        QString base = r.absoluteFilePath(".lang_init_backups");
        QDir().mkpath(base);
        QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString sess = QDir(base).absoluteFilePath(ts);
        QDir().mkpath(sess);
        return sess;
    }

    static QString backupsFillEngDir(const QString &root)
    {
        QDir r(root);
        QString base = r.absoluteFilePath(".lang_fill_eng_backups");
        QDir().mkpath(base);
        QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString sess = QDir(base).absoluteFilePath(ts);
        QDir().mkpath(sess);
        return sess;
    }

    static QStringList listCandidateFiles(const QString &root)
    {
        QStringList exts{".h", ".hpp"};
        QStringList result;
        QList<QString> stack;
        stack << QDir(root).absolutePath();
        while (!stack.isEmpty())
        {
            QString path = stack.takeLast();
            QDir d(path);
            QFileInfoList infos = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo &fi : infos)
            {
                if (fi.isDir())
                {
                    stack << fi.absoluteFilePath();
                    continue;
                }
                QString name = fi.fileName().toLower();
                bool ok = false;
                for (const QString &e : exts)
                    if (name.endsWith(e))
                    {
                        ok = true;
                        break;
                    }
                if (!ok)
                    continue;
                result << fi.absoluteFilePath();
            }
        }
        return result;
    }

    static bool containsTextField(const QString &body)
    {
        QRegularExpression re(R"((const\s+char\s*\*\s*(?:p_)?text_\w+\s*;))");
        return re.match(body).hasMatch();
    }

    static QString insertNewLanguageField(const QString &text, const QString &langCode, bool &changed)
    {
        changed = false;
        // Match typedef struct blocks and alias
        QRegularExpression reBody(R"(typedef\s+struct\s*\{(.*?)\}\s*(\w+)\s*;)",
                                  QRegularExpression::DotMatchesEverythingOption);
        QString out = text;
        auto it = reBody.globalMatch(text);
        int offsetAdjust = 0;
        while (it.hasNext())
        {
            auto m = it.next();
            int start = m.capturedStart(1) + offsetAdjust;
            int end = m.capturedEnd(1) + offsetAdjust;
            QString body = out.mid(start, end - start);
            if (!containsTextField(body))
                continue; // skip non-translation structs

            // Collect lines and find positions
            QStringList lines = body.split('\n');
            int idxOther = -1;
            int lastLangIdx = -1;
            QString indent = "    ";
            QRegularExpression reField(R"(^([ \t]*)const\s+char\s*\*\s*(?:p_)?text_(\w+)\s*;\s*(//.*)?$)");
            bool usePrefixP = false;
            for (int i = 0; i < lines.size(); ++i)
            {
                auto mm = reField.match(lines[i]);
                if (!mm.hasMatch())
                    continue;
                if (lines[i].contains("p_text_"))
                    usePrefixP = true;
                QString name = mm.captured(2);
                if (mm.captured(1).size() > 0)
                    indent = mm.captured(1);
                if (name.compare("other", Qt::CaseInsensitive) == 0)
                {
                    idxOther = i;
                    break;
                }
                lastLangIdx = i;
            }

            if (idxOther < 0)
            {
                // If no p_text_other, append after last language field if any
                if (lastLangIdx < 0)
                    continue; // nothing to do
                idxOther = lastLangIdx + 1;
            }

            // Check if target language already exists
            bool exists = false;
            QString prefix = usePrefixP ? QStringLiteral("p_text_") : QStringLiteral("text_");
            QRegularExpression reExist(QString(R"(\b%1%2\b)").arg(prefix, QRegularExpression::escape(langCode)));
            for (const QString &ln : lines)
            {
                if (reExist.match(ln).hasMatch())
                {
                    exists = true;
                    break;
                }
            }
            if (exists)
                continue;

            QString newLine = QStringLiteral("%1const char *%2%3;   // 待翻译").arg(indent, prefix, langCode);
            lines.insert(idxOther, newLine);
            QString newBody = lines.join('\n');
            out.replace(start, end - start, newBody);
            int delta = newBody.size() - (end - start);
            offsetAdjust += delta;
            changed = true;
        }
        return out;
    }

    InitResult addLanguageAndInitialize(const QString &root, const QString &langCode)
    {
        InitResult res;
        res.success = false;
        if (langCode.trimmed().isEmpty())
        {
            res.message = QStringLiteral("语言代码为空");
            return res;
        }
        QString code = langCode.trimmed();
        QString logPath = ensureLogsDir(root);
        QFile logF(logPath);
        logF.open(QIODevice::Append);
        QTextStream log(&logF);
        log.setCodec("UTF-8");
        QString sessDir = backupsSessionDir(root);

        log << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
            << " BEGIN addLanguage '" << code << "' root=" << root << "\n";

        QStringList files = listCandidateFiles(root);
        int changedCount = 0;
        for (const QString &fp : files)
        {
            QString codec;
            bool bom = false;
            QString text = readFileAutoCodec(fp, codec, bom);
            if (text.isEmpty())
                continue;
            bool changed = false;
            QString out = insertNewLanguageField(text, code, changed);
            if (!changed)
                continue;
            // backup
            QDir s(sessDir);
            QString rel = QDir(root).relativeFilePath(fp);
            QString backupPath = s.absoluteFilePath(rel);
            QDir().mkpath(QFileInfo(backupPath).dir().absolutePath());
            QFile bf(backupPath);
            if (bf.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                QTextStream bts(&bf);
                bts.setCodec(codec.isEmpty() ? "UTF-8" : codec.toUtf8().constData());
                if (bom && codec.compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) == 0)
                    bts << QChar(0xFEFF);
                bts << text;
                bf.close();
            }
            // write
            writeTextWithCodec(fp, out, codec, bom);
            res.modifiedFiles << fp;
            log << "  modified: " << rel << "\n";
            changedCount++;
        }

        // 更新源文件中的初始化（批量处理）
        QStringList aliases = collectAliasesWithTextFields(root);
        for (const QString &alias : aliases)
        {
            // 发现语言顺序（当前头文件顺序，已包含新语言）
            QStringList structLangs = TextExtractor::discoverLanguageColumns(root, QStringList{QStringLiteral(".h"), QStringLiteral(".hpp"), QStringLiteral(".c"), QStringLiteral(".cpp")}, alias);
            QStringList srcs = listSourceFiles(root);
            for (const QString &sp : srcs)
            {
            QString codec;
            bool bom = false;
            QString text = readFileAutoCodec(sp, codec, bom);
            if (text.isEmpty())
                continue;
            bool ch = false;
            QString out = rewriteInitializersInText(text, alias, structLangs, langCode, ch);
            if (!ch)
                continue;
            // backup
                QDir s(sessDir);
                QString rel = QDir(root).relativeFilePath(sp);
                QString backupPath = s.absoluteFilePath(rel);
                QDir().mkpath(QFileInfo(backupPath).dir().absolutePath());
                QFile bf(backupPath);
                if (bf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    QTextStream bts(&bf);
                    bts.setCodec(codec.isEmpty() ? "UTF-8" : codec.toUtf8().constData());
                    if (bom && codec.compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) == 0)
                        bts << QChar(0xFEFF);
                    bts << text;
                    bf.close();
                }
                // write
                if (writeTextWithCodec(sp, out, codec, bom))
                {
                    res.modifiedFiles << sp;
                    log << "  modified: " << rel << "\n";
                    changedCount++;
                }
            }
        }

        log << "  backups: " << QDir(root).relativeFilePath(sessDir) << "\n";
        log << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << " END\n\n";
        logF.close();
        res.logPath = logPath;
        res.outputDir = sessDir;
        res.success = (changedCount > 0);
        res.message = changedCount > 0 ? QString("已初始化新语言 '%1'，修改 %2 个文件").arg(code).arg(changedCount)
                                       : QString("未发现可修改的结构体或语言已存在: '%1'").arg(code);
        return res;
    }

    InitResult undoLastInitialization(const QString &root)
    {
        InitResult res;
        res.success = false;
        QString base = QDir(root).absoluteFilePath(".lang_init_backups");
        QDir dir(base);
        if (!dir.exists())
        {
            res.message = "未找到备份目录";
            return res;
        }
        QStringList sessions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
        if (sessions.isEmpty())
        {
            res.message = "没有可撤销的会话";
            return res;
        }
        QString last = sessions.first();
        QString sess = dir.absoluteFilePath(last);
        // restore
        QList<QString> stack;
        stack << sess;
        while (!stack.isEmpty())
        {
            QString path = stack.takeLast();
            QDir d(path);
            QFileInfoList infos = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo &fi : infos)
            {
                if (fi.isDir())
                {
                    stack << fi.absoluteFilePath();
                    continue;
                }
                QString rel = QDir(sess).relativeFilePath(fi.absoluteFilePath());
                QString target = QDir(root).absoluteFilePath(rel);
                QFile src(fi.absoluteFilePath());
                QFile dst(target);
                if (src.open(QIODevice::ReadOnly))
                {
                    QByteArray bytes = src.readAll();
                    src.close();
                    if (dst.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        dst.write(bytes);
                        dst.close();
                        res.modifiedFiles << target;
                    }
                }
            }
        }
        res.success = !res.modifiedFiles.isEmpty();
        res.message = res.success ? QStringLiteral("已撤销会话 %1").arg(last) : QStringLiteral("撤销失败：无文件恢复");
        res.logPath = ensureLogsDir(root);
        return res;
    }

}
namespace ProjectLang
{
    // Helpers to preserve original codec/BOM when reading/writing
    static QString decodeWithCodec(const QByteArray &data, const char *name)
    {
        QTextCodec *c = QTextCodec::codecForName(name);
        return c ? c->toUnicode(data) : QString();
    }

    static QString readFileAutoCodec(const QString &path, QString &chosenCodec, bool &utf8Bom)
    {
        chosenCodec.clear();
        utf8Bom = false;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        QByteArray data = f.readAll();
        f.close();
        // BOM checks
        if (data.startsWith("\xEF\xBB\xBF"))
        {
            utf8Bom = true;
            chosenCodec = QStringLiteral("UTF-8");
            return QString::fromUtf8(data.mid(3));
        }
        if (data.startsWith("\xFF\xFE"))
        {
            chosenCodec = QStringLiteral("UTF-16LE");
            QTextCodec *c = QTextCodec::codecForName("UTF-16LE");
            return c ? c->toUnicode(data.mid(2)) : QString();
        }
        if (data.startsWith("\xFE\xFF"))
        {
            chosenCodec = QStringLiteral("UTF-16BE");
            QTextCodec *c = QTextCodec::codecForName("UTF-16BE");
            return c ? c->toUnicode(data.mid(2)) : QString();
        }
        // Try common codecs and pick the one with least replacement chars
        auto countReplacement = [](const QString &s) {
            int cnt = 0;
            for (QChar ch : s)
                if (ch.unicode() == 0xFFFD)
                    cnt++;
            return cnt;
        };
        QString utf8 = decodeWithCodec(data, "UTF-8");
        QString gb18030 = decodeWithCodec(data, "GB18030");
        QString gbk = decodeWithCodec(data, "GBK");
        QString gb2312 = decodeWithCodec(data, "GB2312");
        QList<QPair<QString, QString>> candidates = {
            {QStringLiteral("UTF-8"), utf8},
            {QStringLiteral("GB18030"), gb18030},
            {QStringLiteral("GBK"), gbk},
            {QStringLiteral("GB2312"), gb2312},
        };
        int best = INT_MAX;
        QString bestText;
        QString bestCodec;
        for (const auto &p : candidates)
        {
            int rc = countReplacement(p.second);
            if (rc < best || (rc == best && p.first == QLatin1String("GB18030")))
            {
                best = rc;
                bestText = p.second;
                bestCodec = p.first;
            }
        }
        chosenCodec = bestCodec.isEmpty() ? QStringLiteral("UTF-8") : bestCodec;
        return bestText;
    }

    static bool writeTextWithCodec(const QString &path, const QString &text, const QString &codec, bool utf8Bom)
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        QTextStream ts(&f);
        ts.setCodec(codec.isEmpty() ? "UTF-8" : codec.toUtf8().constData());
        if (utf8Bom && codec.compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) == 0)
            ts << QChar(0xFEFF);
        ts << text;
        f.close();
        return true;
    }

    static QStringList tokenizeInitializerBody(const QString &body)
    {
        QStringList tokens;
        QRegularExpression reTok(R"("([^"\\]|\\.)*"|NULL)", QRegularExpression::DotMatchesEverythingOption);
        auto it = reTok.globalMatch(body);
        while (it.hasNext())
        {
            auto m = it.next();
            tokens << m.captured(0);
        }
        return tokens;
    }

    static bool bodyHasNullSentinel(const QString &body)
    {
        // Check if last non-empty token before closing brace is NULL
        QString t = body;
        t = t.trimmed();
        // Remove trailing comments
        t.replace(QRegularExpression(R"(/\*.*?\*/)", QRegularExpression::DotMatchesEverythingOption), "");
        QStringList parts = t.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (parts.isEmpty())
            return false;
        QString last = parts.last().trimmed();
        return last.contains("NULL");
    }

    static QStringList rebuildBodyFillMissingEnglish(const QStringList &structLangs, const QString &body)
    {
        QStringList tokens = tokenizeInitializerBody(body);
        bool hasNull = bodyHasNullSentinel(body);
        if (hasNull && !tokens.isEmpty() && tokens.last() == QLatin1String("NULL"))
            tokens.removeLast();
        int enIdx = structLangs.indexOf(QStringLiteral("text_en"));
        if (enIdx < 0)
            enIdx = structLangs.indexOf(QLatin1String("en"));
        QString enTok = (enIdx >= 0 && enIdx < tokens.size()) ? tokens[enIdx] : QStringLiteral("\"\"");
        QStringList outTokens;
        outTokens.reserve(structLangs.size());
        for (int i = 0; i < structLangs.size(); ++i)
        {
            QString tok;
            if (i < tokens.size())
                tok = tokens[i];
            // Consider empty or NULL as missing
            bool missing = tok.isEmpty() || tok == QLatin1String("\"\"") || tok.compare(QLatin1String("NULL"), Qt::CaseInsensitive) == 0;
            if (missing && !enTok.isEmpty() && enTok != QLatin1String("\"\""))
                tok = enTok;
            if (tok.isEmpty())
                tok = QStringLiteral("\"\"");
            outTokens << tok;
        }
        // Compose lines preserving simple comma style
        QStringList lines;
        for (int i = 0; i < outTokens.size(); ++i)
        {
            QString val = outTokens[i];
            QString comma = QStringLiteral(",");
            lines << QStringLiteral("    %1%2").arg(val, comma);
        }
        if (hasNull)
        {
            lines << QStringLiteral("    NULL");
        }
        else
        {
            // Remove trailing comma from the last language line
            if (!lines.isEmpty())
            {
                QString prev = lines.takeLast();
                if (prev.endsWith(QLatin1Char(',')))
                    prev.chop(1);
                lines << prev;
            }
        }
        return lines;
    }

    static QString rewriteInitializersFillMissing(const QString &text,
                                                  const QString &alias,
                                                  const QStringList &structLangs,
                                                  bool &changed)
    {
        changed = false;
        QRegularExpression reInit(QString(R"((?:static\s+)?(?:const\s+)?(?:struct\s+)?%1(?:\s+\w+)*\s+(?:\*+\s*)?(\w+)\s*=\s*(?:&\s*\([^)]*\)\s*)?\{(.*?)\};)"
                                    ).arg(QRegularExpression::escape(alias)),
                                  QRegularExpression::DotMatchesEverythingOption);
        QString out = text;
        auto it = reInit.globalMatch(text);
        int offset = 0;
        while (it.hasNext())
        {
            auto m = it.next();
            int start = m.capturedStart(2) + offset;
            int end = m.capturedEnd(2) + offset;
            QString body = out.mid(start, end - start);
            QStringList lines = rebuildBodyFillMissingEnglish(structLangs, body);
            QString newBody = lines.join('\n');
            if (newBody != body)
                changed = true;
            out.replace(start, end - start, newBody);
            int delta = newBody.size() - (end - start);
            offset += delta;
        }
        return out;
    }

    InitResult fillMissingEntriesWithEnglish(const QString &root)
    {
        InitResult res;
        res.success = false;
        QString logPath = ensureLogsDir(root);
        QFile logF(logPath);
        logF.open(QIODevice::Append);
        QTextStream log(&logF);
        log.setCodec("UTF-8");
        QString sessDir = backupsFillEngDir(root);

        log << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
            << " BEGIN fillMissingWithEnglish root=" << root << "\n";

        QStringList aliases = collectAliasesWithTextFields(root);
        QStringList srcs = listSourceFiles(root);
        int changedCount = 0;
        for (const QString &sp : srcs)
        {
            QString codec;
            bool bom = false;
            QString text = readFileAutoCodec(sp, codec, bom);
            if (text.isEmpty())
                continue;
            bool fileChanged = false;
            QString outText = text;
            for (const QString &alias : aliases)
            {
                QStringList structLangs = TextExtractor::discoverLanguageColumns(root,
                    QStringList{QStringLiteral(".h"), QStringLiteral(".hpp"), QStringLiteral(".c"), QStringLiteral(".cpp")}, alias);
                bool ch = false;
                outText = rewriteInitializersFillMissing(outText, alias, structLangs, ch);
                fileChanged = fileChanged || ch;
            }
            if (!fileChanged)
                continue;
            // backup original
            QDir s(sessDir);
            QString rel = QDir(root).relativeFilePath(sp);
            QString backupPath = s.absoluteFilePath(rel);
            QDir().mkpath(QFileInfo(backupPath).dir().absolutePath());
            QFile bf(backupPath);
            if (bf.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                QTextStream bts(&bf);
                bts.setCodec(codec.isEmpty() ? "UTF-8" : codec.toUtf8().constData());
                if (bom && codec.compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) == 0)
                    bts << QChar(0xFEFF);
                bts << text;
                bf.close();
            }
            // write updated
            if (writeTextWithCodec(sp, outText, codec, bom))
            {
                res.modifiedFiles << sp;
                log << "  modified: " << rel << "\n";
                changedCount++;
            }
        }

        log << "  backups: " << QDir(root).relativeFilePath(sessDir) << "\n";
        log << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << " END\n\n";
        logF.close();
        res.logPath = logPath;
        res.outputDir = sessDir;
        res.success = (changedCount > 0);
        res.message = changedCount > 0 ? QStringLiteral("已补齐英文缺失项，修改 %1 个文件").arg(changedCount)
                                       : QStringLiteral("未发现需要补齐的初始化项");
        return res;
    }
    static QStringList listSourceFiles(const QString &root)
    {
        QStringList exts{QStringLiteral(".c"), QStringLiteral(".cpp")};
        QStringList result;
        QList<QString> stack;
        stack << QDir(root).absolutePath();
        while (!stack.isEmpty())
        {
            QString path = stack.takeLast();
            QDir d(path);
            QFileInfoList infos = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo &fi : infos)
            {
                if (fi.isDir())
                {
                    stack << fi.absoluteFilePath();
                    continue;
                }
                QString name = fi.fileName().toLower();
                bool ok = false;
                for (const QString &e : exts)
                    if (name.endsWith(e))
                    {
                        ok = true;
                        break;
                    }
                if (!ok)
                    continue;
                result << fi.absoluteFilePath();
            }
        }
        return result;
    }

    static QStringList collectAliasesWithTextFields(const QString &root)
    {
        QStringList result;
        QStringList hdrs = listCandidateFiles(root);
        QRegularExpression reBody(R"(typedef\s+struct\s*\{(.*?)\}\s*(\w+)\s*;)", QRegularExpression::DotMatchesEverythingOption);
        for (const QString &fp : hdrs)
        {
            QString text = TextExtractor::readTextFile(fp);
            QString nc = TextExtractor::stripComments(text);
            auto it = reBody.globalMatch(nc);
            while (it.hasNext())
            {
                auto m = it.next();
                QString body = m.captured(1);
                QString alias = m.captured(2);
                if (containsTextField(body))
                    result << alias;
            }
        }
        result.removeDuplicates();
        return result;
    }

    static QString rebuildInitializerBody(const QStringList &structLangs,
                                          const QString &newLangCode,
                                          const QString &body)
    {
        QStringList oldTokens = tokenizeInitializerBody(body);
        bool hasNull = bodyHasNullSentinel(body);
        // If there is a null sentinel, exclude it from language tokens
        if (hasNull && !oldTokens.isEmpty() && oldTokens.last() == QLatin1String("NULL"))
            oldTokens.removeLast();
        QString targetLang = QStringLiteral("text_%1").arg(newLangCode);
        int posNew = structLangs.indexOf(targetLang);
        int enCurr = structLangs.indexOf(QStringLiteral("text_en"));
        if (enCurr < 0)
            enCurr = structLangs.indexOf(QLatin1String("en"));
        int enOld = enCurr;
        if (posNew >= 0 && enCurr >= 0 && posNew <= enCurr)
            enOld = enCurr - 1;
        QString enToken = (enOld >= 0 && enOld < oldTokens.size()) ? oldTokens[enOld] : QStringLiteral("\"\"");
        QStringList newTokens;
        newTokens.reserve(structLangs.size());
        for (int i = 0; i < structLangs.size(); ++i)
        {
            if (i == posNew)
            {
                newTokens << enToken; // new language copied from English
            }
            else if (i < posNew)
            {
                newTokens << (i < oldTokens.size() ? oldTokens[i] : QStringLiteral("\"\""));
            }
            else
            { // i > posNew
                int oldIdx = i - 1;
                newTokens << (oldIdx < oldTokens.size() ? oldTokens[oldIdx] : QStringLiteral("\"\""));
            }
        }
        // Build multi-line body with indices comments
        QStringList lines;
        for (int i = 0; i < newTokens.size(); ++i)
        {
            QString idxComment = QStringLiteral("    // [%1]").arg(i + 1);
            if (i == posNew)
                idxComment += QStringLiteral("  待翻译");
            lines << idxComment;
            QString val = newTokens[i];
            QString comma = QStringLiteral(",");
            lines << QStringLiteral("    %1%2").arg(val, comma);
        }
        if (hasNull)
        {
            lines << QStringLiteral("    // [%1]").arg(newTokens.size() + 1);
            lines << QStringLiteral("    NULL");
        }
        else
        {
            // Remove trailing comma from the last language line
            if (!lines.isEmpty())
            {
                QString last = lines.takeLast();
                lines << last; // last was language line without comma handling
                QString prev = lines.takeLast();
                if (prev.endsWith(QLatin1Char(',')))
                    prev.chop(1);
                lines << prev;
            }
        }
        return lines.join("\n");
    }

    static QString rewriteInitializersInText(const QString &text,
                                             const QString &alias,
                                             const QStringList &structLangs,
                                             const QString &newLangCode,
                                             bool &changed)
    {
        changed = false;
        QRegularExpression reInit(QString(R"((?:static\s+)?(?:const\s+)?(?:struct\s+)?%1(?:\s+\w+)*\s+(?:\*+\s*)?(\w+)\s*=\s*(?:&\s*\([^)]*\)\s*)?\{(.*?)\};)"
                                    ).arg(QRegularExpression::escape(alias)),
                                  QRegularExpression::DotMatchesEverythingOption);
        QString out = text;
        auto it = reInit.globalMatch(text);
        int offset = 0;
        while (it.hasNext())
        {
            auto m = it.next();
            int start = m.capturedStart(2) + offset;
            int end = m.capturedEnd(2) + offset;
            QString body = out.mid(start, end - start);
            QString newBody = rebuildInitializerBody(structLangs, newLangCode, body);
            out.replace(start, end - start, newBody);
            int delta = newBody.size() - (end - start);
            offset += delta;
            changed = true;
        }
        return out;
    }
}
