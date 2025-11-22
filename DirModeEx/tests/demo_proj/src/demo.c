/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-17 19:59:30
 * @LastEditors: fenxue1 1803651830@qq.com
 * @LastEditTime: 2025-11-22 13:17:50
 * @FilePath: \DirModeEx\tests\demo_proj\src\demo.c
 * @Description: 杩欐槸榛橈拷?锟斤拷?锟界疆,璇凤拷?锟界疆`customMade`, 鎵撳紑koroFileHeader鏌ョ湅閰嶇疆 杩涳拷?锟斤拷?锟界疆: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "../include/tr_text.h"

/**
 * @file demo.c
 * @brief `_Tr_TEXT` 结构体初始化示例（Demo of struct initializers）
 *
 * 说明：
 * - 每个变量表示一组按语言顺序的字符串；末尾以 NULL 作为哨兵；
 * - 用于测试文本提取、CSV 生成与导入等功能；
 * - 部分条目包含异常字符/转义，便于验证编码/转义策略。
 */
#define test_def (1)
// 示例变量：按 `_Tr_TEXT` 字段顺序提供文本，末尾 NULL 哨兵
#if (test_def == 1)
const _Tr_TEXT var_simple1 = {
#if (test_def == 1)
    "示例1-----",
#else
    "示例2-----",
#endif
    "Example1",
    "Example2",
    "Example1",
    "Example1",
    "Example1",
    NULL};
#else
const _Tr_TEXT var_simple1 = {
    "示例1",    // text_cn
    "Example1", // text_en
    "Example2", // text_vn
    "Example1", // text_ko
    "Example1", // text_tr
    "Example1", // text_ling
    NULL        // text_other
};
#endif
const _Tr_TEXT var_simple2 = {
    "示例1",
    "Example1",
    "Example2",
    "Example2",
    "Example",
    "Example",
    NULL};
const _Tr_TEXT var_simple3 = {
    "示例2",
    "Example2",
    "Example3",
    "Example3",
    "Example",
    "Example",
    NULL};

const _Tr_TEXT var_simple6 = {
    "示例3",
    "Example3",
    "Example4",
    "Example4",
    "Example2",
    "Example2",
    NULL};
const _Tr_TEXT var_simple7 = {
    "示例4",
    "Example4",
    "Example5",
    "Example5",
    "Example",
    "Example",
    NULL};
const _Tr_TEXT var_simple8 = {
    "示例5",
    "Example5",
    "Example6",
    "Example6",
    "Example",
    "Example",
    NULL};

const _Tr_TEXT var_simple9 = {
    "示例11",
    "Example3",
    "Example4",
    "Example4",
    "Example2",
    "Example2",
    NULL};
const _Tr_TEXT var_simple10 = {
    "示例12",
    "Example4",
    "Example5",
    "Example5",
    "Example",
    "Example",
    NULL};
const _Tr_TEXT var_simple11 = {
    "示例13",
    "Exam、、nple5",
    "Exam、、nple6",
    "Exam、、nple7",
    "Exam、、nple8",
    "Exam、、nple9",
    NULL};

// 结构体数组示例：用于测试数组提取与 CSV 导入
static const _Tr_TEXT var_maxtx[] = {};

static const _Tr_TEXT var_maxtx1[22321] = {
    { "Exam4", "Exam4", "Exam4", "Exam、、nple8", "Exam4", NULL }
};

static const _Tr_TEXT var_maxtx2[22322] = {
   // { "中文1", "Exam4", "Exam4", "Exam4", "Exam、、nple8", "Exam4", NULL },
{ "中文2", "Exam4", "Exam4", "Exam4", "Exam、、nple8", "Exam4", NULL },
{ "中文3", "Exam4", "Exam4", "Exam4", "Exam、、nple8", "Exam4", NULL }
};