/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-17 22:55:12
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-25 19:09:35
 * @FilePath: \test_cmake\src\utils\utils.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_TREE_SIZE 100

// 定义孩子节点结构体
typedef struct CTNode {
    int child; // 孩子节点的索引
    struct CTNode *next; // 指向下一个兄弟节点
} CTNode;

// 定义树节点结构体
typedef struct CTBox {
    int data; // 数据域
    CTNode *firstchild; // 指向第一个孩子节点
} CTBox;

// 定义树结构体
typedef struct CTree {
    CTBox nodes[MAX_TREE_SIZE]; // 存储节点
    int r; // 根节点位置
    int n; // 节点数量
} CTree;

// 创建新的孩子节点
CTNode* createChildNode(int childIndex);
// 添加孩子节点
void addChild(CTree* tree, int parentIndex, int childIndex);  
// 打印树的结构
void printTree(CTree* tree);
// 计算节点的度
int calculateDegree(CTree* tree, int index);

// 打印树的结构和每个节点的度
void printTreeAndDegrees(CTree* tree);

void print_message(const char *message);
int add(int a, int b);


#endif // UTILS_H




