#include "ValueRange.h"


// 函数实现
void init_value_range(ValueRange *vr, double min, double max, double initial, double step) {
    vr->min_value = min;
    vr->max_value = max;
    vr->step_value = step;

    // 确保当前值在范围内
    if (initial < min || initial > max) {
        vr->current_value = min; // 如果初始值不在范围内，设置为最小值
    } else {
        vr->current_value = initial;
    }
}

void display_value_range(const ValueRange *vr) {
    printf("ValueRange: [min: %.2f, max: %.2f, current: %.2f, step: %.2f]\n",
           vr->min_value, vr->max_value, vr->current_value, vr->step_value);
}

void increment_value(ValueRange *vr) {
    if (vr->current_value + vr->step_value <= vr->max_value) {
        vr->current_value += vr->step_value;
    } else {
        printf("Cannot increment: value exceeds max limit.\n");
    }
}

void decrement_value(ValueRange *vr) {
    if (vr->current_value - vr->step_value >= vr->min_value) {
        vr->current_value -= vr->step_value;
    } else {
        printf("Cannot decrement: value exceeds min limit.\n");
    }
}

double get_current_value(const ValueRange *vr) {
    return vr->current_value;
}

// 初始化整个结构体数组
void init_value_range_array(ValueRange *vr_array, size_t count, 
                             double min[], double max[], 
                             double initial[], double step[]) {
    for (size_t i = 0; i < count; i++) {
        vr_array[i].init = init_value_range;
        vr_array[i].display = display_value_range;
        vr_array[i].increment = increment_value;
        vr_array[i].decrement = decrement_value;

        vr_array[i].init(&vr_array[i], min[i], max[i], initial[i], step[i]);
    }
}

