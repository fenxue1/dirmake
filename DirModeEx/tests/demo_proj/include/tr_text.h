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
typedef struct {
    const char *text_cn;
    const char *text_en;
    const char *text_vn;
    const char *text_ko;
    const char *text_tr;
    const char *text_ling;   // 待翻译
    const char *text_other;
} _Tr_TEXT;
#endif