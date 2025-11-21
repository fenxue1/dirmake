#ifndef VALUERANGE_H
#define VALUERANGE_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// 定义 ValueRange 结构体
typedef struct ValueRange {
    double min_value;   // 最小值
    double max_value;   // 最大值
    double current_value; // 当前值
    double step_value;  // 步进值

    // 函数指针
    void (*init)(struct ValueRange *vr, double min, double max, double initial, double step);
    void (*display)(const struct ValueRange *vr);
    void (*increment)(struct ValueRange *vr);
    void (*decrement)(struct ValueRange *vr);
} ValueRange;
// 初始化整个结构体数组
void init_value_range_array(ValueRange *vr_array, size_t count, 
                             double min[], double max[], 
                             double initial[], double step[]);
// 显示当前值
void display_value_range_range(const ValueRange *vr);

// 减少当前值
void decrease_value(ValueRange *vr);
// 增加当前值
void increase_value(ValueRange *vr);
// 设定当前值
void set_value(ValueRange *vr, double value);
// 获取当前值
double get_current_value(const ValueRange *vr);
#endif
