#include "ValueRange.h"
#include "tr_text.h"


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

static const _Tr_TEXT txt_input_points = {
    "输入点",
    "Input Points",
    "Điểm nhập vào",
    "\x4e\xc3\xba\x6d\x65\x72\x6f\x20\x64\x65\x20\x63\xc3\xb3\x64\x69\x67\x6f\x73\x3a",
    "\x4e\xc3\xba\x6d\x65\x72\x6f\x20\x64\x65\x20\x63\xc3\xb3\x64\x69\x67\x6f\x73\x3a",
    "Точки ввода",
    "Puntos de entrada",
    "Pontos de entrada",
    "نقاط ورودی",
    "入力ポイント",
    "نقاط الإدخال",
    "其它"
};

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

