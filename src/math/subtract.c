#include "subtract.h"
#include "tr_text.h"

int subtract(int a, int b) {
    return a - b;
}

// 设置品牌
void setBrand(CarBuilder* this, const char* brand) {
    strncpy(this->car->brand, brand, sizeof(this->car->brand) - 1);
    this->car->brand[sizeof(this->car->brand) - 1] = '\0';
}

// 设置型号
void setModel(CarBuilder* this, const char* model) {
    strncpy(this->car->model, model, sizeof(this->car->model) - 1);
    this->car->model[sizeof(this->car->model) - 1] = '\0';
}

// 设置颜色
void setColor(CarBuilder* this, const char* color) {
    strncpy(this->car->color, color, sizeof(this->car->color) - 1);
    this->car->color[sizeof(this->car->color) - 1] = '\0';
}

// 设置年份
void setYear(CarBuilder* this, int year) {
    this->car->year = year;
}

CarBuilder* createCarBuilder() {
    CarBuilder* builder = (CarBuilder*)malloc(sizeof(CarBuilder));
    if (builder == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    builder->car = (Car*)malloc(sizeof(Car));
    if (builder->car == NULL) {
        perror("Memory allocation failed");
    }
    memset(builder->car, 0, sizeof(Car));
    builder->setBrand = setBrand;
    builder->setModel = setModel;
    builder->setColor = setColor;
    builder->setYear = setYear;
    builder->build = CarBuilder_build;
    builder->reset = CarBuilder_reset;
    return builder;
}

Car* CarBuilder_build(CarBuilder* this) {
    return this->car;
}

void CarBuilder_reset(CarBuilder* this) {
    memset(this->car, 0, sizeof(Car));
}

static const _Tr_TEXT txt_input_points = {
    "输入点",
    "Input Points",
    "Điểm nhập vào",
    "입력 포인트",
    "Giriş Noktaları",
    "Точки ввода",
    "Puntos de entrada",
    "Pontos de entrada",
    "نقاط ورودی",
    "入力ポイント",
    "نقاط الإدخال",
    "其它"
};