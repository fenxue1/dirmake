/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-11 02:56:33
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-11-12 22:41:50
 * @FilePath: \test_mooc-clin\DirModeEx\language_settings.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

namespace ProjectLang
{

    /**
     * @file language_settings.h
     * @brief 项目语言设置与初始化接口（Project language settings APIs）
     *
     * 功能名称：语言字段初始化与回滚（Language field init & undo）
     * 主要用途：
     * - 在结构体中插入新的语言字段并生成备份；
     * - 回滚最近一次初始化；
     * - 用英文填充缺失语言项；
     */

    struct InitResult
    {
        bool success{false};
        QStringList modifiedFiles; // absolute paths
        QString logPath;           // absolute path to log file
        QString outputDir;         // generated session folder to open
        QString message;           // user-facing message
    };

    /**
     * @brief 添加新语言字段（Add a new language field）
     * @param root 项目根目录
     * @param langCode 语言代码（不含前缀，例如 "fr"）
     * @return InitResult 结果细节（修改文件、日志路径、输出目录等）
     */
    InitResult addLanguageAndInitialize(const QString &root, const QString &langCode);

    /**
     * @brief 撤销最近一次语言初始化（Undo last initialization）
     * @param root 项目根目录
     * @return InitResult 结果细节
     */
    InitResult undoLastInitialization(const QString &root);

    /**
     * @brief 填充缺失语言项为英文（Fill missing entries with English）
     * @param root 项目根目录
     * @return InitResult 结果细节
     */
    InitResult fillMissingEntriesWithEnglish(const QString &root);

}