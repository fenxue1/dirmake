#include "test_leetecode.h"
#include "tr_text.h"



int lengthOfLongestSubstring(char *s) {
    int count[256] = {0}; // 假设 ASCII 字符集
    int maxLength = 0;
    int j = 0;

    for (int i = 0; s[i] != '\0'; i++) {
        count[(unsigned char)s[i]]++; // 使用 unsigned char 作为索引
        while (count[(unsigned char)s[i]] > 1) {
            count[(unsigned char)s[j++]] -= 1; // 使用 unsigned char 作为索引
        }
        maxLength = maxLength > (i - j + 1) ? maxLength : (i - j + 1);
    }

    return maxLength;
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
    NULL
};


