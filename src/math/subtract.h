#ifndef SUBTRACT_H
#define SUBTRACT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int subtract(int a, int b);


// 产品结构体
typedef struct {
    char brand[50];
    char model[50];
    char color[30];
    int year;
} Car;
// Builder结构体
typedef struct {
    Car* car;

    // 函数指针，用于设置品牌
    void (*setBrand)(struct CarBuilder*, const char*);

    // 函数指针，用于设置型号
    void (*setModel)(struct CarBuilder*, const char*);

    // 函数指针，用于设置颜色
    void (*setColor)(struct CarBuilder*, const char*);

    // 函数指针，用于设置年份
    void (*setYear)(struct CarBuilder*, int);

    // 函数指针，用于获取构建好的Car
    Car* (*build)(struct CarBuilder*);

    // 函数指针，用于重置Builder
    void (*reset)(struct CarBuilder*);
} CarBuilder;

void setBrand(CarBuilder* this, const char* brand);
// 设置型号
void setModel(CarBuilder* this, const char* model);
void setColor(CarBuilder* this, const char* color);
// 设置年份
void setYear(CarBuilder* this, int year);
// 获取构建好的Car
Car* CarBuilder_build(CarBuilder* this);
// 重置Builder
void CarBuilder_reset(CarBuilder* this);

CarBuilder* createCarBuilder();
#endif // SUBTRACT_H

