import os
import sys

def ensure_dir(p):
    d = os.path.dirname(p)
    if d and not os.path.exists(d):
        os.makedirs(d, exist_ok=True)

def write_file(p, content):
    ensure_dir(p)
    with open(p, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join('tests', 'python_demo_proj')
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    inc = os.path.join(target, 'include', 'tr_text.h')
    src1 = os.path.join(target, 'src', 'demo.c')
    src2 = os.path.join(target, 'src', 'demo2.c')
    src3 = os.path.join(target, 'src', 'demo3.c')
    huge = os.path.join(target, 'src', 'huge.c')

    h = (
        '#ifndef TR_TEXT_H\n'
        '#define TR_TEXT_H\n'
        'typedef struct {\n'
        '    const char *text_cn;\n'
        '    const char *text_en;\n'
        '    const char *text_vn;\n'
        '    const char *text_ko;\n'
        '    const char *text_tr;\n'
        '    const char *text_ru;\n'
        '    const char *text_pt;\n'
        '    const char *text_es;\n'
        '    const char *text_fa;\n'
        '    const char *text_jp;\n'
        '    const char *text_ar;\n'
        '    const char *text_other;\n'
        '} _Tr_TEXT;\n'
        '#endif\n'
    )

    c1 = (
        '#include "../include/tr_text.h"\n\n'
        'static const struct _Tr_TEXT var_simple = {\n'
        '    "中文",\n'
        '    "Hello",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    NULL\n'
        '};\n\n'
        'const struct _Tr_TEXT var_plain = {\n'
        '    "再见",\n'
        '    "Goodbye",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    NULL\n'
        '};\n'
    )

    c2 = (
        '#include "../include/tr_text.h"\n\n'
        'const struct _Tr_TEXT var_simple = { "示例", "Example", "", "", "", "", "", "", "", "", "", NULL }; '
        'const struct _Tr_TEXT var_simple2 = { "示例2", "Example2", "", "", "", "", "", "", "", "", "", NULL };\n'
    )

    c3 = (
        '#include "../include/tr_text.h"\n\n'
        'static const struct _Tr_TEXT PROGMEM var_progmem = {\n'
        '    "测试",\n'
        '    "Test",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    NULL\n'
        '};\n\n'
        'static const struct _Tr_TEXT *var_ptr = &(const struct _Tr_TEXT){\n'
        '    "指针",\n'
        '    "Pointer",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    "",\n'
        '    NULL\n'
        '};\n'
    )

    write_file(inc, h)
    write_file(src1, c1)
    write_file(src2, c2)
    write_file(src3, c3)

    if count and count > 0:
        lines = ["#include \"../include/tr_text.h\"\n\n"]
        for i in range(1, count + 1):
            idx = str(i).zfill(5)
            cn = f"中文{idx}"
            en = f"EN{idx}"
            line = (
                f"const struct _Tr_TEXT var_item_{idx} = {{ \"{cn}\", \"{en}\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", NULL }};\n"
            )
            lines.append(line)
        write_file(huge, "".join(lines))

if __name__ == '__main__':
    main()