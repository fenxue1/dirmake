/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-10-08 21:44:02
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-10-08 22:21:38
 * @FilePath: \test_mooc-clin\include\tr_text.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef TR_TEXT_H
#define TR_TEXT_H

// 多语言文本结构，用于在各模块内定义本地化字符串
typedef struct {
    const char *p_text_cn;      // 中文
    const char *p_text_en;      // 英文
    const char *p_text_vn;      // 越南语
    const char *p_text_ko;      // 韩语
    const char *p_text_turkish; // 土耳其语
    const char *p_text_russian; // 俄语
    const char *p_text_spanish; // 西班牙语
    const char *p_text_pt;      // 葡萄牙语
    const char *p_text_fa;      // 波斯语
    const char *p_text_jp;      // 日语
    const char *p_text_ar;      // 阿拉伯语
    const char *p_text_other;   // 其他语言
} TY_TEXT;

// 兼容命名，提供 _Tr_TEXT 别名
typedef TY_TEXT _Tr_TEXT;

#endif // TR_TEXT_H