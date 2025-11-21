/**
 * @file diff_utils.h
 * @brief 差异工具接口（Unified diff generation APIs）
 *
 * 功能名称：生成统一 diff（Generate unified diffs）
 * 主要用途：
 * - 对单文件的原始与修改内容生成统一格式差异文本；
 *
 * 使用示例：
 *  QString d = DiffUtils::unifiedDiff(path, orig, mod);
 */
#ifndef DIFF_UTILS_H
#define DIFF_UTILS_H

#include <QString>

namespace DiffUtils {

/**
 * @brief 生成单文件统一差异（Generate unified diff for a single file）
 * @param filePath 文件路径（用于标头显示）
 * @param original 原始文本
 * @param modified 修改后文本
 * @return 统一 diff 文本
 */
QString unifiedDiff(const QString &filePath, const QString &original, const QString &modified);

}

#endif // DIFF_UTILS_H