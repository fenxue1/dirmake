/**
 * @file diff_utils.cpp
 * @brief 差异工具实现（Unified diff implementation）
 *
 * 生成统一 diff 文本，保留文件标头，按增删改行输出 hunk；
 * 算法：双列表遍历，遇到相同行时尽量合并上下文；其余以 -/+ 记录。
 */
#include "diff_utils.h"
#include <QStringList>

namespace DiffUtils
{

    static QStringList splitLines(const QString &s)
    {
        return s.split(QLatin1String("\n"), Qt::KeepEmptyParts);
    }

    QString unifiedDiff(const QString &filePath, const QString &original, const QString &modified)
    {
        QStringList a = splitLines(original);
        QStringList b = splitLines(modified);
        QString out;
        out += QStringLiteral("--- %1\n").arg(filePath);
        out += QStringLiteral("+++ %1\n").arg(filePath);
        int i = 0, j = 0;
        int hunkStartA = 0, hunkStartB = 0;
        QStringList hunk;
        auto flush = [&]()
        {
            if (hunk.isEmpty())
                return;
            int lenA = i - hunkStartA;
            int lenB = j - hunkStartB;
            out += QStringLiteral("@@ -%1,%2 +%3,%4 @@\n").arg(hunkStartA + 1).arg(lenA).arg(hunkStartB + 1).arg(lenB);
            out += hunk.join(QLatin1String("\n")) + QStringLiteral("\n");
            hunk.clear();
        };
        while (i < a.size() || j < b.size())
        {
            if (i < a.size() && j < b.size() && a[i] == b[j])
            {
                if (!hunk.isEmpty() && hunk.size() < 3)
                    hunk << QStringLiteral(" ") + a[i];
                else
                    flush();
                hunkStartA = i + 1;
                hunkStartB = j + 1;
                i++;
                j++;
            }
            else
            {
                if (i < a.size())
                {
                    hunk << QStringLiteral("-") + a[i];
                    i++;
                }
                if (j < b.size())
                {
                    hunk << QStringLiteral("+") + b[j];
                    j++;
                }
            }
        }
        flush();
        return out;
    }

}