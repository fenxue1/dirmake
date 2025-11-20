/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-10 23:45:24
 * @LastEditors: fenxue1 1803651830@qq.com
 * @LastEditTime: 2025-11-20 20:13:20
 * @FilePath: \test_mooc-clin\DirModeEx\text_extractor.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "text_extractor.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include <QByteArray>
#include <QFileInfo>
#include <QSet>
#include <QTextCodec>

namespace
{
    // 解析 C 字符串字面量，支持 \xHH 按 GBK/CP936 字节解码为 Unicode
    QString decodeCEscapedString(const QString &literal, bool preserveEscapes)
    {
        if (literal.size() < 2 || !literal.startsWith(QLatin1Char('"')) || !literal.endsWith(QLatin1Char('"')))
            return literal;
        QString raw = literal.mid(1, literal.size() - 2);
        if (preserveEscapes)
            return raw;

        QTextCodec *gbk = QTextCodec::codecForName("GBK");
        QString out;
        QByteArray buf; // 缓冲需要按 GBK 解码的字节（例如 \xB2\xE2）

        auto flushBuf = [&]() {
            if (buf.isEmpty()) return;
            QString dec;
            if (gbk)
                dec = gbk->toUnicode(buf);
            if (dec.isEmpty())
            {
                // 回退：尝试按本地8位或UTF-8解码
                dec = QString::fromLocal8Bit(buf);
                if (dec.isEmpty())
                    dec = QString::fromUtf8(buf);
            }
            out.append(dec);
            buf.clear();
        };

        for (int i = 0; i < raw.size(); ++i)
        {
            QChar c = raw[i];
            if (c == QLatin1Char('\\') && i + 1 < raw.size())
            {
                QChar n = raw[i + 1];
                if (n == QLatin1Char('x'))
                {
                    // 累积最多2位十六进制，形成一个字节；GBK中文通常由两个字节顺序出现
                    int j = i + 2;
                    int consumed = 0;
                    while (j + 1 < raw.size() && consumed < 2)
                    {
                        auto h1 = raw[j];
                        auto h2 = raw[j + 1];
                        auto hex = QString(h1) + QString(h2);
                        bool ok = false;
                        int val = hex.toInt(&ok, 16);
                        if (!ok)
                            break;
                        buf.append(char(val));
                        j += 2;
                        consumed += 2;
                    }
                    i = j - 1;
                    continue;
                }
                // 遇到转义控制字符，先刷新 GBK 缓冲，再按 Unicode 附加
                flushBuf();
                switch (n.unicode())
                {
                case 'n':
                    out.append(QLatin1Char('\n'));
                    i++;
                    continue;
                case 'r':
                    out.append(QLatin1Char('\r'));
                    i++;
                    continue;
                case 't':
                    out.append(QLatin1Char('\t'));
                    i++;
                    continue;
                case '\\':
                    out.append(QLatin1Char('\\'));
                    i++;
                    continue;
                case '"':
                    out.append(QLatin1Char('"'));
                    i++;
                    continue;
                case '\'':
                    out.append(QLatin1Char('\''));
                    i++;
                    continue;
                default:
                    out.append(n);
                    i++;
                    continue;
                }
            }
            // 普通字符：先刷新 GBK 缓冲，再附加当前 Unicode 字符
            flushBuf();
            out.append(c);
        }
        // 处理尾部残留的 GBK 字节
        flushBuf();
        return out;
    }

    QStringList parsePointerFields(const QString &body)
    {
        QStringList cols;
        QRegularExpression re(QStringLiteral("(?:const\\s+char|char\\s+const|char)\\s*\\*\\s*(\\w+)\\s*;"));
        auto it = re.globalMatch(body);
        while (it.hasNext())
        {
            auto m = it.next();
            QString name = m.captured(1);
            QRegularExpression m2(QStringLiteral("_(\\w+)$"));
            auto mm = m2.match(name);
            QString lang = mm.hasMatch() ? mm.captured(1) : name;
            // 规范化常见命名：p_text_xx 或 text_xx -> xx
            if (name.startsWith(QLatin1String("p_text_")))
            {
                lang = name.mid(7);
            }
            else if (name.startsWith(QLatin1String("text_")))
            {
                lang = name.mid(5);
            }
            // 跳过末尾的哨兵字段，一般为 other/NULL
            if (lang.compare(QLatin1String("other"), Qt::CaseInsensitive) == 0)
            {
                continue;
            }
            cols << lang;
        }
        return cols;
    }

    QString stripBlockComments(const QString &text)
    {
        QString t = text;
        QRegularExpression reBlock(QStringLiteral("/\\*.*?\\*/"), QRegularExpression::DotMatchesEverythingOption);
        t = t.replace(reBlock, QString());
        QRegularExpression reLine(QStringLiteral("//.*$"));
        QStringList lines = t.split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i)
            lines[i].remove(reLine);
        return lines.join(QLatin1Char('\n'));
    }
}

namespace TextExtractor
{

    QStringList defaultLanguageColumns()
    {
        return {QStringLiteral("cn"), QStringLiteral("en"), QStringLiteral("vn"), QStringLiteral("ko"), QStringLiteral("turkish"), QStringLiteral("russian"), QStringLiteral("spanish"), QStringLiteral("pt"), QStringLiteral("fa"), QStringLiteral("jp"), QStringLiteral("ar"), QStringLiteral("other")};
    }

    QString readTextFile(const QString &path)
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
        QString cp936 = decodeWith("CP936");
        int repCp936 = countReplacement(cp936);

        // 选择替换字符最少的解码结果；持平时偏好 GB18030（更覆盖简体中文）
        QString best = gb18030;
        int minRep = repGb18030;
        auto consider = [&](const QString &s, int rep) {
            if (rep < minRep)
            {
                minRep = rep;
                best = s;
            }
        };
        consider(utf8, repUtf8);
        consider(gbk, repGbk);
        consider(gb2312, repGb2312);
        consider(cp936, repCp936);
        return best;
    }

    QString stripComments(const QString &text)
    {
        return stripBlockComments(text);
    }

    QString preprocess(const QString &text, const QMap<QString, QString> &defines)
    {
        QStringList lines = text.split(QLatin1Char('\n'));
        QStringList out;
        struct Frame
        {
            bool parentActive;
            bool currentActive;
            bool branchTaken;
        };
        QList<Frame> stack;
        bool parentActive = true;
        bool currentActive = true;
        bool branchTaken = false;
        QRegularExpression reIf(QStringLiteral("^\\s*#\\s*if\\b(.*)$"));
        QRegularExpression reIfdef(QStringLiteral("^\\s*#\\s*ifdef\\b\\s*(\\w+).*$"));
        QRegularExpression reIfndef(QStringLiteral("^\\s*#\\s*ifndef\\b\\s*(\\w+).*$"));
        QRegularExpression reElif(QStringLiteral("^\\s*#\\s*elif\\b(.*)$"));
        QRegularExpression reElse(QStringLiteral("^\\s*#\\s*else\\b.*$"));
        QRegularExpression reEndif(QStringLiteral("^\\s*#\\s*endif\\b.*$"));
        auto evalCond = [&](const QString &expr) -> bool
        {
            QString e = expr.trimmed();
            if (e.isEmpty())
                return false;
            QRegularExpression rDefined(QStringLiteral("defined\\s*\\(\\s*(\\w+)\\s*\\)"));
            auto md = rDefined.match(e);
            if (md.hasMatch())
                return defines.contains(md.captured(1));
            QRegularExpression rEq(QStringLiteral("(\\w+)\\s*(==|!=)\\s*(\\S+)"));
            auto me = rEq.match(e);
            if (me.hasMatch())
            {
                QString name = me.captured(1);
                QString op = me.captured(2);
                QString val = me.captured(3);
                val.remove(QLatin1Char('"'));
                // val.remove(QLatin1Char('\'')); 
                QString defval = defines.value(name);
                if (op == QLatin1String("=="))
                    return defval == val;
                return defval != val;
            }
            QRegularExpression rName(QStringLiteral("^\\w+$"));
            if (rName.match(e).hasMatch())
                return defines.contains(e);
            return false;
        };

        for (const QString &line : lines)
        {
            auto mIf = reIf.match(line);
            if (mIf.hasMatch())
            {
                bool cond = evalCond(mIf.captured(1));
                stack.append({parentActive, currentActive, branchTaken});
                parentActive = parentActive && currentActive;
                currentActive = parentActive && cond;
                branchTaken = cond;
                continue;
            }
            auto mIfdef = reIfdef.match(line);
            if (mIfdef.hasMatch())
            {
                bool cond = defines.contains(mIfdef.captured(1));
                stack.append({parentActive, currentActive, branchTaken});
                parentActive = parentActive && currentActive;
                currentActive = parentActive && cond;
                branchTaken = cond;
                continue;
            }
            auto mIfndef = reIfndef.match(line);
            if (mIfndef.hasMatch())
            {
                bool cond = !defines.contains(mIfndef.captured(1));
                stack.append({parentActive, currentActive, branchTaken});
                parentActive = parentActive && currentActive;
                currentActive = parentActive && cond;
                branchTaken = cond;
                continue;
            }
            auto mElif = reElif.match(line);
            if (mElif.hasMatch())
            {
                if (!stack.isEmpty())
                {
                    Frame &f = stack.last();
                    parentActive = f.parentActive;
                    if (f.branchTaken)
                    {
                        currentActive = false;
                    }
                    else
                    {
                        bool cond = evalCond(mElif.captured(1));
                        currentActive = parentActive && cond;
                        f.branchTaken = f.branchTaken || cond;
                    }
                    branchTaken = f.branchTaken;
                }
                continue;
            }
            if (reElse.match(line).hasMatch())
            {
                if (!stack.isEmpty())
                {
                    Frame &f = stack.last();
                    parentActive = f.parentActive;
                    currentActive = parentActive && (!f.branchTaken);
                    f.branchTaken = true;
                    branchTaken = true;
                }
                continue;
            }
            if (reEndif.match(line).hasMatch())
            {
                if (!stack.isEmpty())
                {
                    Frame f = stack.takeLast();
                    parentActive = f.parentActive;
                    currentActive = f.currentActive;
                    branchTaken = f.branchTaken;
                }
                continue;
            }
            if (parentActive && currentActive)
                out << line;
        }
        return out.join(QLatin1Char('\n'));
    }

QList<ExtractedBlock> extractBlocks(const QString &text, const QString &sourceFile, const QString &typeName, bool preserveEscapes)
{
        QString ncAll = stripComments(text);
        QRegularExpression reFull(QStringLiteral("(?:(?:static|const)\s+)*(?:struct\s+)?%1(?:\s+\w+)*\s*(?:\*+)?\s+([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?(?:\s+\w+)*\s*=\s*(?:&\s*\([^)]*\)\s*)?\{(.*?)\};").arg(QRegularExpression::escape(typeName)), QRegularExpression::DotMatchesEverythingOption);
        QRegularExpression reStringsFull(QStringLiteral("\"(?:\\.|[^\"\\])*\""));
        auto itFull = reFull.globalMatch(ncAll);
        QList<ExtractedBlock> fast;
        while (itFull.hasNext())
        {
            auto m = itFull.next();
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
            QString var = m.captured(1);
            QString inner = m.captured(2);
            int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\s,])(NULL|nullptr)(?:[\s,]|$)")));
            QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
            QStringList strs;
            auto sit = reStringsFull.globalMatch(innerFor);
            while (sit.hasNext())
            {
                auto sm = sit.next();
                QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                strs << decoded;
            }
            int startPos = m.capturedStart(0);
            int finalLine = ncAll.left(startPos).count(QLatin1Char('\n')) + 1;
            fast.append({var, strs, sourceFile, finalLine});
        }
        if (!fast.isEmpty())
            return fast;
        QList<ExtractedBlock> results;
        QStringList lines = text.split(QLatin1Char('\n'));
        // 允许类型名与变量名之间的附加限定词（如 PROGMEM），以及指针/数组声明
        QRegularExpression reStart(QStringLiteral("(?:(?:static|const)\\s+)*(?:struct\\s+)?%1(?:\\s+\\w+)*\\s*(?:\\*+)?\\s+([A-Za-z_]\\w*)\\s*(?:\\[[^\\]]*\\])?(?:\\s+\\w+)*\\s*=\\s*").arg(QRegularExpression::escape(typeName)));
        QRegularExpression reStrings(QStringLiteral("\"(?:\\\\.|[^\"\\\\])*\""));
    int i = 0;
    bool inBlockComment = false;
    while (i < lines.size())
    {
        QString line = lines[i];
        QString ltrim = line.trimmed();
        if (ltrim.contains(QLatin1String("/*")) && !ltrim.contains(QLatin1String("*/")))
            inBlockComment = true;
        bool lineStartsComment = ltrim.startsWith(QLatin1String("//")) || inBlockComment;
        if (inBlockComment && ltrim.contains(QLatin1String("*/")))
        {
            inBlockComment = false;
            i++;
            continue;
        }
        if (lineStartsComment)
        {
            i++;
            continue;
        }
        auto m = reStart.match(line);
        if (!m.hasMatch())
        {
            i++;
            continue;
        }
        {
            int eqPos = line.indexOf(QLatin1Char('='));
            if (eqPos > 0)
            {
                QString left = line.left(eqPos);
                if (left.contains(QLatin1Char('[')))
                {
                    i++;
                    continue; // 跳过结构体数组声明
                }
            }
        }
            QString var = m.captured(1);
            int declLine = i + 1;
            QStringList buf;
            buf << line;
            i++;
            bool started = line.contains(QLatin1Char('{'));
            int braceDepth = started ? (line.count(QLatin1Char('{')) - line.count(QLatin1Char('}'))) : 0;
            int bodyStartLine = started ? declLine : 0;
        if (started && braceDepth == 0 && line.contains(QStringLiteral("};")))
        {
            QString nc = stripComments(line);
            QRegularExpression reInner(QStringLiteral("\\{(.*)\\};"), QRegularExpression::DotMatchesEverythingOption);
            auto mi = reInner.match(nc);
            QStringList strs;
            if (mi.hasMatch())
            {
                QString inner = mi.captured(1);
                int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\\s,])(NULL|nullptr)(?:[\\s,]|$)")));
                QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
                auto it2 = reStrings.globalMatch(innerFor);
                while (it2.hasNext())
                {
                    auto sm = it2.next();
                    QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                    strs << decoded;
                }
                int finalLine = declLine;
                results.append({var, strs, sourceFile, finalLine});
            }
            continue;
        }
            while (i < lines.size())
            {
                buf << lines[i];
                if (!started && lines[i].contains(QLatin1Char('{')))
                {
                    started = true;
                    bodyStartLine = i + 1;
                }
                braceDepth += lines[i].count(QLatin1Char('{'));
                braceDepth -= lines[i].count(QLatin1Char('}'));
                if (started && braceDepth == 0 && lines[i].contains(QStringLiteral("};")))
                {
                    i++;
                    break;
                }
                i++;
            }
            QString block = buf.join(QLatin1Char('\n'));
            QString nc = stripComments(block);
            QRegularExpression reInner(QStringLiteral("\\{(.*)\\};"), QRegularExpression::DotMatchesEverythingOption);
            auto mi = reInner.match(nc);
            QStringList strs;
        if (mi.hasMatch())
        {
            QString inner = mi.captured(1);
            int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\\s,])(NULL|nullptr)(?:[\\s,]|$)")));
            QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
            auto it = reStrings.globalMatch(innerFor);
            while (it.hasNext())
            {
                auto sm = it.next();
                QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                strs << decoded;
            }
            int finalLine = (bodyStartLine > 0) ? bodyStartLine : declLine;
            results.append({var, strs, sourceFile, finalLine});
        }
    }
    return results;
}

QList<ExtractedArray> extractArrays(const QString &text, const QString &sourceFile, const QString &typeName, bool preserveEscapes)
{
    QString ncAll = stripComments(text);
QRegularExpression reDecl(QString::fromLatin1(R"((?:(?:static|const)\s+)*(?:struct\s+)?%1(?:\s+\w+)*\s*(?:\*+)?\s+([A-Za-z_]\w*)\s*\[[^\]]*\]\s*=\s*\{(.*?)\};)" )
                                  .arg(QRegularExpression::escape(typeName)), QRegularExpression::DotMatchesEverythingOption);
    auto it = reDecl.globalMatch(ncAll);
QRegularExpression reElem(QString::fromLatin1(R"(\{([\s\S]*?)\})"));
QRegularExpression reStr(QString::fromLatin1(R"("(?:\.|[^"])*")"));
    QList<ExtractedArray> out;
    while (it.hasNext())
    {
        auto m = it.next();
        QString var = m.captured(1);
        QString body = m.captured(2);
        int startPos = m.capturedStart(0);
        int declLine = ncAll.left(startPos).count(QLatin1Char('\n')) + 1;
        QList<QStringList> elements;
        auto me = reElem.globalMatch(body);
        while (me.hasNext())
        {
            auto em = me.next();
            QString inner = em.captured(1);
            int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\s,])(NULL|nullptr)(?:[\s,]|$)")));
            QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
            QStringList strs;
            auto sit = reStr.globalMatch(innerFor);
            while (sit.hasNext())
            {
                auto sm = sit.next();
                QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                strs << decoded;
            }
            if (!strs.isEmpty())
                elements.append(strs);
        }
        out.append({var, sourceFile, declLine, elements});
    }
    if (!out.isEmpty())
        return out;

    // Fallback: line-based scan for robustness
    QStringList lines = text.split(QLatin1Char('\n'));
    QRegularExpression reStart(QString::fromLatin1(R"((?:(?:static|const)\s+)*(?:struct\s+)?%1(?:\s+\w+)*\s*(?:\*+)?\s+([A-Za-z_]\w*)\s*\[[^\]]*\]\s*=\s*)")
                                      .arg(QRegularExpression::escape(typeName)));
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i];
        auto m = reStart.match(line);
        if (!m.hasMatch()) continue;
        QString var = m.captured(1);
        int declLine = i + 1;
        QStringList buf; buf << line; int braceDepth = line.count(QLatin1Char('{')) - line.count(QLatin1Char('}'));
        bool started = line.contains(QLatin1Char('{'));
        i++;
        while (i < lines.size())
        {
            buf << lines[i];
            braceDepth += lines[i].count(QLatin1Char('{'));
            braceDepth -= lines[i].count(QLatin1Char('}'));
            if (started && braceDepth == 0 && lines[i].contains(QStringLiteral("};")))
            { i++; break; }
            i++;
        }
        QString block = buf.join(QLatin1Char('\n'));
        QString nc = stripComments(block);
        QRegularExpression reInner(QString::fromLatin1(R"(\{(.*)\};)"), QRegularExpression::DotMatchesEverythingOption);
        auto mi = reInner.match(nc);
        if (!mi.hasMatch()) continue;
        QString body = mi.captured(1);
        QList<QStringList> elements;
        auto me = reElem.globalMatch(body);
        while (me.hasNext())
        {
            auto em = me.next();
            QString inner = em.captured(1);
            int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\s,])(NULL|nullptr)(?:[\s,]|$)")));
            QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
            QStringList strs;
            auto sit = reStr.globalMatch(innerFor);
            while (sit.hasNext())
            {
                auto sm = sit.next();
                QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                strs << decoded;
            }
            if (!strs.isEmpty()) elements.append(strs);
        }
        out.append({var, sourceFile, declLine, elements});
    }
    return out;

    // Fallback 2: tolerate unknown alias (match any typedef name)
    // Useful when preprocess alters type alias rendering
    QList<ExtractedArray> out2;
    QRegularExpression reDeclAny(QString::fromLatin1(R"((?:(?:static|const)\s+)*(?:struct\s+)?\w+(?:\s+\w+)*\s*(?:\*+)?\s+([A-Za-z_]\w*)\s*\[[^\]]*\]\s*=\s*\{(.*?)\};)"), QRegularExpression::DotMatchesEverythingOption);
    auto it2 = reDeclAny.globalMatch(text);
    while (it2.hasNext())
    {
        auto m = it2.next();
        QString var = m.captured(1);
        QString body = m.captured(2);
        int startPos = m.capturedStart(0);
        int declLine = text.left(startPos).count(QLatin1Char('\n')) + 1;
        QList<QStringList> elements;
        auto me = reElem.globalMatch(body);
        while (me.hasNext())
        {
            auto em = me.next();
            QString inner = em.captured(1);
            int sentinelPos = inner.indexOf(QRegularExpression(QStringLiteral("(?:^|[\s,])(NULL|nullptr)(?:[\s,]|$)")));
            QString innerFor = sentinelPos > 0 ? inner.left(sentinelPos) : inner;
            QStringList strs;
            auto sit = reStr.globalMatch(innerFor);
            while (sit.hasNext())
            {
                auto sm = sit.next();
                QString decoded = decodeCEscapedString(sm.captured(0), preserveEscapes);
                strs << decoded;
            }
            if (!strs.isEmpty()) elements.append(strs);
        }
        if (!elements.isEmpty()) out2.append({var, sourceFile, declLine, elements});
    }
    return out2;
}

QList<ExtractedArray> scanDirectoryArrays(const QString &root, const QStringList &extensions, const QString &mode, const QMap<QString, QString> &defines, const QString &typeName, bool preserveEscapes)
{
    QList<ExtractedArray> all;
    QDir dir(root);
    QStringList exts = extensions.isEmpty() ? QStringList{QStringLiteral(".h"), QStringLiteral(".hpp"), QStringLiteral(".c"), QStringLiteral(".cpp")} : extensions;
    auto matchExt = [&](const QString &fn)
    { for (const QString &e : exts) if (fn.toLower().endsWith(e.toLower())) return true; return false; };
    QList<QString> stack; stack << dir.absolutePath();
    while (!stack.isEmpty())
    {
        QString path = stack.takeLast();
        QDir d(path);
        QFileInfoList infos = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &fi : infos)
        {
            if (fi.isDir()) { stack << fi.absoluteFilePath(); continue; }
            if (!matchExt(fi.fileName())) continue;
            QString text = readTextFile(fi.absoluteFilePath());
            QString t = (mode == QLatin1String("effective")) ? preprocess(text, defines) : text;
            auto arrays = extractArrays(t, fi.absoluteFilePath(), typeName, preserveEscapes);
            if (arrays.isEmpty() && mode == QLatin1String("effective"))
            {
                arrays = extractArrays(text, fi.absoluteFilePath(), typeName, preserveEscapes);
            }
            all.append(arrays);
        }
    }
    return all;
}

static QString hexEscapeIfNeeded(const QString &s, bool literal, bool replaceComma)
{
    if (literal) return s;
    QByteArray b = s.toUtf8();
    QString out;
    for (int i = 0; i < b.size(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(b[i]);
        if (ch < 0x20 || ch >= 0x80)
        {
            out += QStringLiteral("\\x%1").arg(QString::number(ch, 16).toUpper()).rightJustified(4, QLatin1Char('0'));
        }
        else
        {
            QChar c = QLatin1Char(ch);
            if (replaceComma && c == QLatin1Char(',')) out += QLatin1String("，"); else out += c;
        }
    }
    return out;
}

bool writeArraysCsv(const QString &outputPath,
                    const QList<ExtractedArray> &arrays,
                    const QStringList &langColumns,
                    const QStringList &literalColumns,
                    bool replaceAsciiCommaWithCn)
{
    QFile f(outputPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream ts(&f); ts.setCodec("UTF-8"); ts << QChar(0xFEFF);
    // header
    ts << "source_path,line_number,array_variable";
    for (const QString &c : langColumns) ts << "," << c;
    ts << "\n";
    auto isLiteral = [&](int idx){ return literalColumns.contains(langColumns.value(idx)); };
    for (const auto &arr : arrays)
    {
        for (int i = 0; i < arr.elements.size(); ++i)
        {
            const QStringList &vals = arr.elements.at(i);
            if (i == 0)
            {
                ts << '"' << arr.sourceFile << '"' << "," << '"' << QString::number(arr.lineNumber) << '"' << "," << '"' << arr.arrayName << "[]" << '"';
            }
            else
            {
                ts << ",," << '"' << arr.arrayName << "[]" << '"';
            }
            int colCount = langColumns.size();
            for (int c = 0; c < colCount; ++c)
            {
                QString v = c < vals.size() ? vals.at(c) : QString();
                QString w = v;
                w.replace("\"", "\"\"");
                ts << "," << '"' << w << '"';
            }
            ts << "\n";
        }
    }
    return true;
}

    QList<ExtractedBlock> scanDirectory(const QString &root, const QStringList &extensions, const QString &mode, const QMap<QString, QString> &defines, const QString &typeName, bool preserveEscapes)
    {
        QList<ExtractedBlock> all;
        QDir dir(root);
        QStringList exts = extensions;
        auto matchExt = [&](const QString &fn)
        { for (const QString &e : exts) if (fn.toLower().endsWith(e.toLower())) return true; return false; };
        QList<QString> stack;
        stack << dir.absolutePath();
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
                if (!matchExt(fi.fileName()))
                    continue;
                QString text = readTextFile(fi.absoluteFilePath());
                QString t = (mode == QLatin1String("effective")) ? preprocess(text, defines) : text;
                auto blocks = extractBlocks(t, fi.absoluteFilePath(), typeName, preserveEscapes);
                all.append(blocks);
            }
        }
        return all;
    }

    QStringList discoverLanguageColumns(const QString &root, const QStringList &extensions, const QString &typeAlias)
    {
        // 仅解析指定别名的结构体（例如 _Tr_TEXT），避免误采集其它结构体字段
        // 1) 首选 include/tr_text.h（约定路径）
        {
            QString headerPath = QDir(root).absoluteFilePath(QStringLiteral("include/tr_text.h"));
            QString text = readTextFile(headerPath);
            QString nc = stripComments(text);
            QRegularExpression reBody(QStringLiteral("typedef\\s+struct\\s*\\{(.*?)\\}\\s*(\\w+)\\s*;"), QRegularExpression::DotMatchesEverythingOption);
            auto it = reBody.globalMatch(nc);
            while (it.hasNext())
            {
                auto m = it.next();
                QString alias = m.captured(2);
                if (alias != typeAlias)
                    continue;
                QStringList cols = parsePointerFields(m.captured(1));
                if (!cols.isEmpty())
                {
                    QStringList norm;
                    for (const QString &c : cols)
                    {
                        norm << (c.startsWith(QLatin1String("text_")) ? c : (QStringLiteral("text_") + c));
                    }
                    // 如果最后一个是 text_other（常见NULL哨兵），移除显示
                    if (!norm.isEmpty() && norm.last().compare(QLatin1String("text_other"), Qt::CaseInsensitive) == 0)
                    {
                        norm.removeLast();
                    }
                    return norm;
                }
            }
        }
        // 2) 遍历项目中所有指定扩展的文件，仅提取目标别名的结构体
        QStringList exts = extensions;
        if (exts.isEmpty())
            exts << QStringLiteral(".h") << QStringLiteral(".hpp") << QStringLiteral(".c") << QStringLiteral(".cpp");
        auto matchExt = [&](const QString &fn)
        { for (const QString &e : exts) if (fn.toLower().endsWith(e.toLower())) return true; return false; };
        QStringList langs; // 保持顺序
        QList<QString> stack;
        stack << QDir(root).absolutePath();
        QRegularExpression reBody(QStringLiteral("typedef\\s+struct\\s*\\{(.*?)\\}\\s*(\\w+)\\s*;"), QRegularExpression::DotMatchesEverythingOption);
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
                if (!matchExt(fi.fileName()))
                    continue;
                QString text = readTextFile(fi.absoluteFilePath());
                QString nc = stripComments(text);
                auto it = reBody.globalMatch(nc);
                while (it.hasNext())
                {
                    auto m = it.next();
                    QString alias = m.captured(2);
                    if (alias != typeAlias)
                        continue;
                    QString body = m.captured(1);
                    QStringList cols = parsePointerFields(body);
                    for (const QString &c : cols)
                    {
                        QString norm = c.startsWith(QLatin1String("text_")) ? c : (QStringLiteral("text_") + c);
                        langs << norm;
                    }
                }
            }
        }
        if (!langs.isEmpty())
        {
            // 末尾 text_other 作为哨兵不显示
            if (langs.last().compare(QLatin1String("text_other"), Qt::CaseInsensitive) == 0)
                langs.removeLast();
            return langs;
        }
        // 3) 回退默认列名（text_* 形式）
        QStringList cols = defaultLanguageColumns();
        QStringList norm;
        for (const QString &c : cols)
            norm << (QStringLiteral("text_") + c);
        return norm;
    }

    static QString csvEscape(const QString &s)
    {
        QString v = s;
        // 防止 CSV 中出现实际换行：将 CRLF/LF/CR 转为字面 "\\n"
        v.replace(QStringLiteral("\r\n"), QStringLiteral("\\n"));
        v.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
        v.replace(QLatin1Char('\r'), QStringLiteral("\\n"));
        // 可选：将制表符转为字面 "\\t"，避免表格中出现控制字符
        v.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
        QString q = v;
        q.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QStringLiteral("\"") + q + QStringLiteral("\"");
    }

    static QString toUtf8Hex(const QString &s)
    {
        if (s.isEmpty())
            return QString();
        QByteArray b = s.toUtf8();
        QString out;
        out.reserve(b.size() * 4);
        for (unsigned char c : b)
        {
            QString hx = QStringLiteral("%1").arg(c, 2, 16, QLatin1Char('0')).toUpper();
            out += QStringLiteral("\\x") + hx; // 保持小写 x，十六进制数字大写
        }
        return out;
    }

bool writeCsv(const QString &outputPath,
              const QList<ExtractedBlock> &rows,
              const QStringList &langColumns,
              const QStringList &literalColumns,
              bool replaceAsciiCommaWithCn)
{
        auto isLiteralCol = [&](const QString &name) -> bool {
            QString n = name;
            QStringList cands;
            cands << n;
            if (n.startsWith(QLatin1String("text_")))
                cands << n.mid(5);
            else
                cands << (QStringLiteral("text_") + n);
            for (const QString &c : cands)
                if (literalColumns.contains(c))
                    return true;
            return false;
        };
        auto writeChunk = [&](const QString &path, int startIdx, int endIdx) -> bool {
            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return false;
            f.write("\xEF\xBB\xBF");
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            QStringList headers;
            headers << QStringLiteral("source_file") << QStringLiteral("line_number") << QStringLiteral("variable_name") << langColumns;
            ts << headers.join(QLatin1Char(',')) << QStringLiteral("\n");

            auto findEnglishIndex = [&]() -> int {
                for (int i = 0; i < langColumns.size(); ++i)
                {
                    const QString c = langColumns[i].toLower();
                    if (c == QLatin1String("text_en") || c == QLatin1String("en") || c.endsWith(QLatin1String("_en")) || c.contains(QLatin1String("english")))
                        return i;
                }
                return -1;
            };
            auto findChineseIndex = [&]() -> int {
                for (int i = 0; i < langColumns.size(); ++i)
                {
                    const QString c = langColumns[i].toLower();
                    if (c == QLatin1String("text_cn") || c == QLatin1String("cn") || c.endsWith(QLatin1String("_cn")) || c.endsWith(QLatin1String("_zh")) || c.endsWith(QLatin1String("_chs")) || c.endsWith(QLatin1String("_hans")))
                        return i;
                }
                return -1;
            };
            const int idxEn = findEnglishIndex();
            const int idxCn = findChineseIndex();

            for (int ri = startIdx; ri < endIdx; ++ri)
            {
                const auto &r = rows.at(ri);
                QStringList cols;
                cols << csvEscape(r.sourceFile) << csvEscape(QString::number(r.lineNumber)) << csvEscape(r.variableName);
                for (int i = 0; i < langColumns.size(); ++i)
                {
                    QString v = (i < r.strings.size()) ? r.strings[i] : QString();
                    if (v.isEmpty())
                    {
                        QString fill;
                        if (idxEn >= 0 && idxEn < r.strings.size())
                            fill = r.strings[idxEn];
                        if (fill.isEmpty() && idxCn >= 0 && idxCn < r.strings.size())
                            fill = r.strings[idxCn];
                        if (fill.isEmpty())
                        {
                            for (const QString &s : r.strings)
                            {
                                if (!s.isEmpty()) { fill = s; break; }
                            }
                        }
                        v = fill;
                    }
                    if (replaceAsciiCommaWithCn && !v.isEmpty())
                    {
                        v.replace(QLatin1Char(','), QChar(0xFF0C));
                    }
                    cols << csvEscape(v);
                }
                ts << cols.join(QLatin1Char(',')) << QStringLiteral("\n");
            }
            f.close();
            return true;
        };

        if (rows.size() > 5000)
        {
            int chunkSize = 2000;
            int parts = (rows.size() + chunkSize - 1) / chunkSize;
            QFileInfo fi(outputPath);
            QString dir = fi.dir().absolutePath();
            QString base = fi.completeBaseName();
            bool allOk = true;
            for (int p = 0; p < parts; ++p)
            {
                int startIdx = p * chunkSize;
                int endIdx = qMin(rows.size(), (p + 1) * chunkSize);
                QString partName = QStringLiteral("%1_part%2.csv").arg(base).arg(p + 1, 2, 10, QLatin1Char('0'));
                QString outPath = QDir(dir).absoluteFilePath(partName);
                allOk = allOk && writeChunk(outPath, startIdx, endIdx);
            }
            return allOk;
        }
        else
        {
            return writeChunk(outputPath, 0, rows.size());
        }
    }

    static QStringList parseCsvLine(const QString &line)
    {
        QStringList out;
        QString cur;
        bool inq = false;
        for (int i = 0; i < line.size(); ++i)
        {
            QChar c = line[i];
            if (inq)
            {
                if (c == QLatin1Char('"'))
                {
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                    {
                        cur.append(QLatin1Char('"'));
                        i++;
                    }
                    else
                    {
                        inq = false;
                    }
                }
                else
                {
                    cur.append(c);
                }
            }
            else
            {
                if (c == QLatin1Char('"'))
                {
                    inq = true;
                }
                else if (c == QLatin1Char(','))
                {
                    out << cur;
                    cur.clear();
                }
                else
                {
                    cur.append(c);
                }
            }
        }
        out << cur;
        return out;
    }

    static bool detectFileCodec(const QString &path, QString &codecName, bool &utf8Bom)
    {
        codecName.clear();
        utf8Bom = false;
        QFile f(path);
        if (!f.exists())
            return false;
        if (!f.open(QIODevice::ReadOnly))
            return false;
        QByteArray data = f.readAll();
        f.close();

        if (data.startsWith("\xEF\xBB\xBF"))
        {
            utf8Bom = true;
            codecName = QStringLiteral("UTF-8");
            return true;
        }
        if (data.startsWith("\xFF\xFE"))
        {
            codecName = QStringLiteral("UTF-16LE");
            return true;
        }
        if (data.startsWith("\xFE\xFF"))
        {
            codecName = QStringLiteral("UTF-16BE");
            return true;
        }

        auto countReplacement = [](const QString &s) {
            int cnt = 0;
            for (QChar ch : s)
                if (ch.unicode() == 0xFFFD)
                    cnt++;
            return cnt;
        };
        auto decodeWith = [&](const char *name) -> QString {
            QTextCodec *c = QTextCodec::codecForName(name);
            return c ? c->toUnicode(data) : QString();
        };

        struct Candidate { const char *name; QString text; int rep; };
        Candidate cands[] = {
            {"UTF-8", decodeWith("UTF-8"), 0},
            {"GB18030", decodeWith("GB18030"), 0},
            {"GBK", decodeWith("GBK"), 0},
            {"GB2312", decodeWith("GB2312"), 0},
            {"CP936", decodeWith("CP936"), 0},
        };
        for (auto &c : cands)
            c.rep = countReplacement(c.text);

        const char *best = cands[0].name;
        int minRep = cands[0].rep;
        for (int i = 1; i < int(sizeof(cands)/sizeof(cands[0])); ++i)
        {
            if (cands[i].rep < minRep)
            {
                minRep = cands[i].rep;
                best = cands[i].name;
            }
        }
        codecName = QString::fromLatin1(best);
        return true;
    }

    static bool writeTextPreserveCodec(const QString &path, const QString &text, const QString &fallbackCodec = QStringLiteral("UTF-8"))
    {
        QString codec;
        bool utf8Bom = false;
        detectFileCodec(path, codec, utf8Bom);

        // 行尾风格检测：优先沿用现有文件的 CRLF/LF
        bool preferCRLF = false;
        if (QFile::exists(path))
        {
            QFile rf(path);
            if (rf.open(QIODevice::ReadOnly))
            {
                QByteArray existing = rf.readAll();
                rf.close();
                preferCRLF = existing.contains("\r\n") || existing.indexOf('\r') >= 0;
            }
        }
        else
        {
#ifdef Q_OS_WIN
            preferCRLF = true;
#else
            preferCRLF = false;
#endif
        }

        // 规范化行尾：先归一为 LF，再按目标风格输出
        QString normalized = text;
        normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        if (preferCRLF)
            normalized.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        QTextStream ts(&f);
        const QString useCodec = codec.isEmpty() ? fallbackCodec : codec;
        ts.setCodec(useCodec.toUtf8().constData());
        // 仅在原文件为 UTF-8 BOM 且当前编码为 UTF-8 时保留 BOM
        if (utf8Bom && useCodec.compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) == 0)
            ts << QChar(0xFEFF);
        ts << normalized;
        f.close();
        return true;
    }

QString generateCFromCsv(const QString &csvPath,
                             const QString &typeName,
                             const QString &cOutputPath,
                             const QString &headerOutputPath,
                             bool noStatic,
                             bool registryEmit,
                             const QString &registryArrayName,
                             bool useUtf8Literal,
                             bool fillMissingWithEnglish,
                             bool nullSentinel,
                             bool verbatim,
                             const QStringList &literalColumns,
                             const QString &annotateMode,
                             int perLine,
                             const QMap<QString, QPair<QString, QString>> &sourceMap,
                             const QString &sourceRoot)
{
    Q_UNUSED(useUtf8Literal);
    Q_UNUSED(perLine);
    // 使用与源码一致的自动编码读取，确保非 UTF-8 CSV 的中文也准确
    QString content = readTextFile(csvPath);
    if (content.isEmpty())
        return QString();
    QStringList allLines = content.split(QLatin1Char('\n'));
    int lineIdx = 0;
    QString headerLine = (lineIdx < allLines.size()) ? allLines.at(lineIdx++) : QString();
    if (!headerLine.isEmpty() && headerLine[0].unicode() == 0xFEFF)
        headerLine.remove(0, 1);
    QStringList headers = parseCsvLine(headerLine);
        int idxSource = headers.indexOf(QStringLiteral("source_file"));
        int idxLine = headers.indexOf(QStringLiteral("line_number"));
        int idxVar = headers.indexOf(QStringLiteral("variable_name"));
        QList<int> csvLangIdx;
        QStringList csvLangHeaders;
        for (int i = 0; i < headers.size(); ++i)
        {
            if (i != idxSource && i != idxLine && i != idxVar)
            {
                csvLangIdx << i;
                csvLangHeaders << headers[i];
            }
        }
        // 项目结构体语言顺序（以 _Tr_TEXT 的字段顺序为准），自动移除 text_other 哨兵
        QString rootDir = QFileInfo(csvPath).dir().absolutePath();
        QStringList structLangs = discoverLanguageColumns(rootDir, QStringList{QLatin1String(".h"), QLatin1String(".hpp"), QLatin1String(".c"), QLatin1String(".cpp")}, typeName);
        QStringList outLangHeaders = structLangs; // 输出按结构体顺序
        // CSV 头到索引的映射（支持 text_xx 与 xx 互相匹配）
        auto indexForLang = [&](const QString &lang)
        {
            int idx = csvLangHeaders.indexOf(lang);
            if (idx < 0 && lang.startsWith(QLatin1String("text_")))
            {
                QString shortName = lang.mid(5);
                idx = csvLangHeaders.indexOf(shortName);
            }
            if (idx < 0 && !lang.startsWith(QLatin1String("text_")))
            {
                QString withText = QStringLiteral("text_") + lang;
                idx = csvLangHeaders.indexOf(withText);
            }
            return idx;
        };
        // 英文列索引（用于缺失填充）
        int enHeaderIdx = indexForLang(QStringLiteral("text_en"));
        if (enHeaderIdx < 0)
            enHeaderIdx = indexForLang(QLatin1String("en"));

        auto fmtUtf8 = [&](const QString &s)
        { QString e = s; e.replace(QLatin1Char('\\'), QLatin1String("\\\\")).replace(QLatin1Char('"'), QLatin1String("\\\"")); return QStringLiteral("\"") + e + QStringLiteral("\""); };
        auto fmtHex = [&](const QString &s)
        {
            if (s.isEmpty())
                return QStringLiteral("\"\"");
            QByteArray b = s.toUtf8();
            QString out = QStringLiteral("\"");
            for (unsigned char c : b)
            {
                QString hx = QStringLiteral("%1").arg(c, 2, 16, QLatin1Char('0')).toUpper(); // 大写十六进制数字
                out += QStringLiteral("\\x") + hx;                                     // 保持小写 x
            }
            out += QStringLiteral("\"");
            return out;
        };
        auto fmtVerb = [&](const QString &s)
        { QString e = s; e.replace(QLatin1Char('"'), QLatin1String("\\\"")); return QStringLiteral("\"") + e + QStringLiteral("\""); };

        auto decodeCsvEscapes = [&](const QString &s)
        {
            QByteArray bytes;
            const QString raw = s;
            for (int i = 0; i < raw.size(); ++i)
            {
                QChar c = raw[i];
                if (c == QLatin1Char('\\') && i + 1 < raw.size())
                {
                    QChar n = raw[i + 1];
                    if (n == QLatin1Char('x'))
                    {
                        if (i + 3 < raw.size())
                        {
                            QChar h1 = raw[i + 2];
                            QChar h2 = raw[i + 3];
                            QString hex = QString(h1) + QString(h2);
                            bool ok = false;
                            int val = hex.toInt(&ok, 16);
                            if (ok)
                            {
                                bytes.append(char(val));
                                i += 3;
                                continue;
                            }
                        }
                    }
                    else if (n == QLatin1Char('n'))
                    {
                        bytes.append('\n');
                        i++;
                        continue;
                    }
                    else if (n == QLatin1Char('r'))
                    {
                        bytes.append('\r');
                        i++;
                        continue;
                    }
                    else if (n == 't')
                    {
                        bytes.append('\t');
                        i++;
                        continue;
                    }
                    else if (n == '\\')
                    {
                        bytes.append('\\');
                        i++;
                        continue;
                    }
                    else if (n == '"')
                    {
                        bytes.append('"');
                        i++;
                        continue;
                    }
                    else if (n == '\'')
                    {
                        bytes.append('\'');
                        i++;
                        continue;
                    }
                }
                bytes.append(QString(c).toUtf8());
            }
            return QString::fromUtf8(bytes);
        };

    QStringList lines;
    lines << QStringLiteral("/* Generated from CSV by DirModeEx */")
              << QStringLiteral("#include \"include/tr_text.h\"")
              << QStringLiteral("#include <stddef.h>")
              << QString();
    QStringList decls;
    QStringList regs;
    // 记录已使用的变量名，避免集中生成时发生重定义
    QSet<QString> usedVarNames;
    while (lineIdx < allLines.size())
    {
        QString line = allLines.at(lineIdx++);
        if (!line.isEmpty() && line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.isEmpty())
            continue;
        QStringList cols = parseCsvLine(line);
            QString var = (idxVar >= 0 && idxVar < cols.size()) ? cols[idxVar] : QStringLiteral("var_%1").arg(regs.size());
            QString src = (idxSource >= 0 && idxSource < cols.size()) ? cols[idxSource] : QString();
            QString ln = (idxLine >= 0 && idxLine < cols.size()) ? cols[idxLine] : QString();
            if ((src.isEmpty() || ln.isEmpty()) && sourceMap.contains(var))
            {
                auto p = sourceMap.value(var);
                if (src.isEmpty())
                    src = p.first;
                if (ln.isEmpty())
                    ln = p.second;
            }
            if (!sourceRoot.isEmpty() && src.startsWith(QLatin1String("./")))
            {
                QString rel = src.mid(2).replace(QLatin1Char('/'), QLatin1Char('\\'));
                src = QDir(sourceRoot).absoluteFilePath(rel);
            }
            if (!src.isEmpty() || !ln.isEmpty())
                lines << QStringLiteral("/* source: %1 line: %2 */").arg(src, ln);
            // 生成阶段去重：若发现同名变量冲突，基于行号或递增后缀调整为唯一名
            QString emitVar = var;
            if (usedVarNames.contains(emitVar))
            {
                QString lnDigits;
                for (const QChar &ch : ln)
                {
                    if (ch.isDigit()) lnDigits.append(ch);
                }
                if (!lnDigits.isEmpty())
                {
                    QString candidate = emitVar + QLatin1Char('_') + lnDigits;
                    if (!usedVarNames.contains(candidate))
                        emitVar = candidate;
                }
                if (usedVarNames.contains(emitVar))
                {
                    int dupIdx = 2;
                    QString candidate;
                    do { candidate = emitVar + QStringLiteral("_dup%1").arg(dupIdx++); } while (usedVarNames.contains(candidate));
                    emitVar = candidate;
                }
            }
            usedVarNames.insert(emitVar);
            QString storage = noStatic ? QString() : QStringLiteral("static ");
            lines << QStringLiteral("%1const %2 %3 = {").arg(storage, typeName, emitVar);
            QStringList vals;
            for (int k = 0; k < outLangHeaders.size(); ++k)
            {
                int csvIdx = indexForLang(outLangHeaders[k]);
                int csvCol = (csvIdx >= 0 && csvIdx < csvLangIdx.size()) ? csvLangIdx[csvIdx] : -1;
                QString v = (csvCol >= 0 && csvCol < cols.size()) ? cols[csvCol] : QString();
                QString raw = v;
                QString v2 = v;
                if (!verbatim && v.contains(QStringLiteral("\\x")))
                    v2 = decodeCsvEscapes(v);
                // 如果启用填充，且当前值为空或为NULL，则用英文列值填充
                if (fillMissingWithEnglish && (v2.isEmpty() || raw.compare(QStringLiteral("NULL"), Qt::CaseInsensitive) == 0))
                {
                    if (enHeaderIdx >= 0)
                    {
                        // 注意：enHeaderIdx 是 CSV 头列表中的索引（相对于 csvLangHeaders），需转换到整行列索引
                        int encsvCol = csvLangIdx.value(enHeaderIdx, -1);
                        QString env = (encsvCol >= 0 && encsvCol < cols.size()) ? cols[encsvCol] : QString();
                        QString env2 = (!verbatim && env.contains(QStringLiteral("\\x"))) ? decodeCsvEscapes(env) : env;
                        if (!env2.isEmpty())
                            v2 = env2;
                    }
                }
                if (verbatim)
                {
                    // 全局“保留原样”：不改变表示，直接输出原文本
                    vals << fmtVerb(raw);
                }
                else if (literalColumns.contains(outLangHeaders.value(k)))
                {
                    // “直写列”：如果原文本已经是 \xNN 形式，按原样输出；否则用UTF-8直写
                    if (raw.contains(QStringLiteral("\\x")))
                        vals << fmtVerb(raw);
                    else
                        vals << fmtUtf8(v2);
                }
                else
                {
                    // 默认：统一输出为 UTF-8 字面字符串
                    vals << fmtUtf8(v2);
                }
            }
            // 生成格式：一种语言一行，每行前带索引或名称注释
            int langCount = vals.size();
            for (int i = 0; i < langCount; ++i)
            {
                if (annotateMode == QStringLiteral("indices"))
                {
                    lines << QStringLiteral("    // [%1]").arg(i + 1);
                }
                else if (annotateMode == QStringLiteral("names"))
                {
                    QString name = (i < outLangHeaders.size()) ? outLangHeaders[i] : QString();
                    if (!name.isEmpty())
                        lines << QStringLiteral("    // [%1] %2").arg(i + 1).arg(name);
                    else
                        lines << QStringLiteral("    // [%1]").arg(i + 1);
                }
                lines << QStringLiteral("    %1,").arg(vals[i]);
            }
            // 末尾NULL哨兵：单独一行注释 + 值（强制添加）
            int sentinelIndex = langCount + 1;
            if (annotateMode == QStringLiteral("indices"))
            {
                lines << QStringLiteral("    // [%1]").arg(sentinelIndex);
            }
            else if (annotateMode == QStringLiteral("names"))
            {
                lines << QStringLiteral("    // [%1] %2").arg(sentinelIndex).arg(QStringLiteral("NULL"));
            }
            lines << QStringLiteral("    %1,").arg(QStringLiteral("NULL"));
            // 去掉最后一行的逗号
            if (!vals.isEmpty())
            {
                QString last = lines.takeLast();
                lines << last.left(last.size() - 1);
            }
            lines << QStringLiteral("};") << QString();
            decls << QStringLiteral("extern const %1 %2;").arg(typeName, emitVar);
            regs << emitVar;
        }
        if (registryEmit && !registryArrayName.isEmpty())
        {
            lines << QStringLiteral("const %1* %2[] = {").arg(typeName, registryArrayName);
            int perReg = 6;
            for (int k = 0; k < regs.size(); k += perReg)
            {
                QString chunk;
                QStringList row;
                for (int j = k; j < qMin(k + perReg, regs.size()); ++j)
                    row << (QStringLiteral("&") + regs[j]);
                chunk = row.join(QLatin1String(", "));
                lines << QStringLiteral("    %1,").arg(chunk);
            }
            if (!regs.isEmpty())
            {
                QString last = lines.takeLast();
                lines << last.left(last.size() - 1);
            }
            lines << QStringLiteral("};") << QString();
        }
        QString code = lines.join(QLatin1Char('\n'));
        if (!cOutputPath.isEmpty())
        {
            writeTextPreserveCodec(cOutputPath, code);
        }
        if (!headerOutputPath.isEmpty())
        {
            QString headerText = QStringLiteral("/* Declarations generated by DirModeEx */\n#pragma once\n#include \"include/tr_text.h\"\n\n");
            for (const auto &d : decls)
                headerText += d + QStringLiteral("\n");
            writeTextPreserveCodec(headerOutputPath, headerText);
        }
        return code;
    }

}
#if 0
/**
 * @file text_extractor.cpp
 * @brief 文本提取与 C 代码生成实现（Implementation for text extraction & C generation）
 *
 * 使用示例见 `text_extractor.h` 的文档块。
 * 关键代码处已添加注释（编码保留、换行风格保留、CSV 转义等）。
 */
#endif