/*
 * @Author: fenxue1 1803651830@qq.com
 * @Date: 2025-11-17 19:59:22
 * @LastEditors: fenxue1 1803651830@qq.com
 * @LastEditTime: 2025-11-18 21:49:20
 * @FilePath: \DirModeEx\tests\demo_proj\include\tr_text.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef TR_TEXT_H
#define TR_TEXT_H
/**
 * @file tr_text.h
 * @brief 演示用的多语言文本结构体（Translation text struct for demos）
 *
 * 说明：
 * - 字段按语言顺序排列，示例项目中用于初始化字符串常量；
 * - 最后一个字段 `text_other` 常作为哨兵位（数组初始化时写入 NULL）；
 * - 生成或导入流程会根据该结构体的字段顺序推断 CSV 语言列。
 */
typedef struct {
    const char *text_cn;   // 中文（Chinese）
    const char *text_en;   // 英文（English）
    const char *text_vn;   // 越南语（Vietnamese）
    const char *text_ko;   // 韩语（Korean）
    const char *text_tr;   // 土耳其语（Turkish）
    const char *text_ling; // 示例额外语言，默认标记“待翻译”
    const char *text_other; // 末尾哨兵字段（通常在初始化中写入 NULL）
} _Tr_TEXT;
#endif