/*
 * @Author: fenxue1 1803651830@qq.com
 * @Date: 2025-11-17 20:00:40
 * @LastEditors: fenxue1 1803651830@qq.com
 * @LastEditTime: 2025-11-23 22:29:40
 * @FilePath: \DirModeEx\tests\demo_proj\src\demo2.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE


 */
#define test_def_error (1)

#include "../include/tr_text.h"

/**
 * @file demo2.c
 * @brief `_Tr_TEXT` 初始化的转义/编码示例（Escapes & encoding demo）
 *
 * 说明：
 * - 第五列包含十六进制转义（"\x45\x78..."），用于验证转义解析；
 * - 末尾 `NULL` 作为哨兵；部分变量带 `static` 以测试存储类修饰。
 */

const _Tr_TEXT var_simple = {
    "示例6",
    "Example6",
    "Example7",
    "Example7",
    "\x45\x78\x61\x6D\x70\x6C\x65\x32",
    "Example2",
 NULL};

static const _Tr_TEXT var_simple2 = {
    "示例8",
    "Example8",
    "Example9",
    "Example9",
    "\x45\x78\x61\x6D\x70\x6C\x65",
    "Example",
 NULL};
const _Tr_TEXT var_simple3 = {
    "示例9",
    "Example9",
    "Example10",
    "Example10",
    "\x45\x78\x61\x6D\x70\x6C\x65",
    "Example",
 NULL};
const _Tr_TEXT var_simple4 = {
    "示例9",
    "Example9",
    "Example10",
    "Example10",
    "\x45\x78\x61\x6D\x70\x6C\x65",
    "Example",
 NULL};


 static const _Tr_TEXT var_maxtx1[22321] = {
    { "示例1", "示例4", "Exam、、nple8", "Exam4", "Exam4", "Exam4", NULL },
    { "示例2", "示例4", "Exam、、nple8", "Exam4", "Exam4", "Exam4", NULL },
    { "示例3", "示例4", "Exam、、nple8", "Exam4", "Exam4", "Exam4", NULL },
    { "示例4", "示例4", "Exam、、nple8", "Exam4", "Exam4", "Exam4", NULL }
};


int id_evnet = 1;

 static const DispMessageInfo _disp_message_info1 = {
    
    id_evnet,
    {
#if(test_def_error == 1)   
        "22222测试",
        "Exam4",
        "Exam、、nple8",
        "Exam4",
        "Exam4",
        "Exam4",
#else      
        "标题1111",
        "Exam4",
        "Exam、、nple8",
        "Exam4",
        "Exam4",
        "Exam4",
#endif        
        NULL
    },
    {
        "显示的信息",
        "示例4",
        "Exam、、nple8",
        "Exam4",
        "Exam4",
        "Exam4",
        NULL
    }

 };


 static const DispMessageInfo _disp_message_info2 = {
    id_evnet,
    {
        "标题1111",
        "Exam4",
        "Exam、、nple8",
        "Exam4",
        "Exam4",
        "Exam4",
        NULL
    },
    {
        "测试111",
        "测试112",
        "测试113",
        "测试114",
        "测试112",
        "测试112",
        "测试112",
        "测试117",
        "测试119",
        "测试120",
        "测试121",
        "测试122",
        NULL
}

 };



  static const DispMessageInfo _disp_message_info3 = {
    id_evnet,
    {
        "标题1111",
        "Exam4",
        "Exam、、nple8",
        "Exam4",
        "Exam4",
        "Exam4",
        NULL
    },
    {
        "显示的信息",
        "示例4",
        "string1",
        "string2",
        "示例4",
        "示例4",
        "示例4",
        "示例4",
        "string7",
        "string8",
        "string9",
        "string10",
        NULL
}

 };