#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
TY_TEXT/_Tr_TEXT 多语言提取工具

功能概述：
- 扫描工程目录，匹配形如 `static const <类型名> <变量名> = { ... };` 的初始化块（类型名可配置，默认 _Tr_TEXT）
- 支持简易预处理：根据命令行传入的宏值保留生效的条件编译分支（#if/#ifdef/#ifndef/#elif/#else/#endif）
- 从初始化块中提取 12 个语言字符串（中文、英文、越南语、韩语、土耳其语、俄语、西班牙语、葡萄牙语、波斯语、日语、阿拉伯语、其他）
- 自动进行CSV标准转义，采用 `utf-8-sig` 编码，确保Excel可直接打开且不乱码、不列错位

使用示例：
1) 仅保留生效分支（默认模式 effective）：
   python ty_text_extractor.py --root D:\YourProject --output D:\out\ty_text.csv --define LASER_MODE_SELECT=1

2) 保留所有分支（模式 all，不做预处理，可能产生同名变量的多行）：
   python ty_text_extractor.py --root D:\YourProject --output D:\out\ty_text.csv --mode all

参数说明：
- --root          要扫描的工程根目录
- --output        输出CSV路径（建议以.csv结尾）
- --extensions    扫描的文件扩展名，逗号分隔（默认：.h,.hpp,.c,.cpp）
- --define        宏定义，格式 NAME=VALUE，可重复传入（例如：--define LASER_MODE_SELECT=1）
- --mode          处理模式：effective（默认，仅保留生效分支）或 all（不过滤条件编译）

CSV列：
- source_file, line_number, variable_name, cn, en, vn, ko, turkish, russian, spanish, pt, fa, jp, ar, other

注意：
- 字符串中大量存在十六进制转义（如 \xE1...）；脚本会尝试将其解码为UTF-8实际字符，以便在Excel中正确显示。
- 若解码失败，保留原始字符串字面值。
"""

import os
import re
import csv
import sys
import codecs
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass


# 语言列顺序与字段名
LANG_COLUMNS = [
    "cn", "en", "vn", "ko", "turkish", "russian",
    "spanish", "pt", "fa", "jp", "ar", "other",
]


@dataclass
class ExtractedBlock:
    variable_name: str
    strings: List[str]  # 长度<=12
    source_file: str
    line_number: int


def read_text_file(path: str) -> str:
    """以尽可能鲁棒的方式读取文本文件，优先utf-8，其次gbk。"""
    for enc in ("utf-8", "utf-8-sig", "gbk", "cp936"):
        try:
            with open(path, "r", encoding=enc, errors="strict") as f:
                return f.read()
        except Exception:
            continue
    # 退回二进制再解码
    with open(path, "rb") as f:
        data = f.read()
    try:
        return data.decode("utf-8", errors="replace")
    except Exception:
        return data.decode("latin-1", errors="replace")


_re_if = re.compile(r"^\s*#\s*if\b(.*)$")
_re_ifdef = re.compile(r"^\s*#\s*ifdef\b\s*(\w+).*$")
_re_ifndef = re.compile(r"^\s*#\s*ifndef\b\s*(\w+).*$")
_re_elif = re.compile(r"^\s*#\s*elif\b(.*)$")
_re_else = re.compile(r"^\s*#\s*else\b.*$")
_re_endif = re.compile(r"^\s*#\s*endif\b.*$")


def eval_condition(expr: str, defines: Dict[str, str]) -> bool:
    """非常简化的条件表达式求值：仅支持 `MACRO == value`、`defined(MACRO)`。
    复杂表达式不支持（将返回False）。
    """
    expr = expr.strip()
    if not expr:
        return False
    # 支持 defined(MACRO)
    m = re.match(r"defined\s*\(\s*(\w+)\s*\)", expr)
    if m:
        name = m.group(1)
        return name in defines
    # 支持 MACRO == value / MACRO != value
    m = re.match(r"(\w+)\s*(==|!=)\s*(\S+)", expr)
    if m:
        name, op, val = m.group(1), m.group(2), m.group(3)
        defval = defines.get(name)
        # 去掉可能的引号
        val = val.strip()
        val = val.strip('"\'')
        if defval is None:
            return False if op == "==" else True
        return (defval == val) if op == "==" else (defval != val)
    # 仅宏存在性
    m = re.match(r"^(\w+)$", expr)
    if m:
        return m.group(1) in defines
    return False


def preprocess(text: str, defines: Dict[str, str]) -> str:
    """简易预处理器：处理 #if/#ifdef/#ifndef/#elif/#else/#endif，仅保留生效代码。"""
    lines = text.splitlines()
    out_lines: List[str] = []
    # 栈元素： (parent_active, current_active, branch_taken)
    stack: List[Tuple[bool, bool, bool]] = []
    parent_active = True
    current_active = True
    branch_taken = False
    for line in lines:
        if _re_if.match(line):
            expr = _re_if.match(line).group(1)
            cond = eval_condition(expr, defines)
            stack.append((parent_active, current_active, branch_taken))
            parent_active = parent_active and current_active
            current_active = parent_active and cond
            branch_taken = cond
            continue
        if _re_ifdef.match(line):
            name = _re_ifdef.match(line).group(1)
            cond = name in defines
            stack.append((parent_active, current_active, branch_taken))
            parent_active = parent_active and current_active
            current_active = parent_active and cond
            branch_taken = cond
            continue
        if _re_ifndef.match(line):
            name = _re_ifndef.match(line).group(1)
            cond = name not in defines
            stack.append((parent_active, current_active, branch_taken))
            parent_active = parent_active and current_active
            current_active = parent_active and cond
            branch_taken = cond
            continue
        if _re_elif.match(line):
            if not stack:
                continue
            expr = _re_elif.match(line).group(1)
            cond = eval_condition(expr, defines)
            # 取出上一层状态，但不弹出
            # 依据C预处理规则：同一个#if链只允许一个分支为真
            # 如果之前已经命中分支，后续elif都不生效
            parent_active, prev_current, prev_taken = parent_active, current_active, branch_taken
            if stack:
                pa, _, bt = stack[-1]
                parent_active = pa
                if bt:
                    current_active = False
                else:
                    current_active = parent_active and cond
                branch_taken = bt or cond
            continue
        if _re_else.match(line):
            if not stack:
                continue
            pa, _, bt = stack[-1]
            parent_active = pa
            current_active = parent_active and (not bt)
            branch_taken = True
            continue
        if _re_endif.match(line):
            if not stack:
                continue
            pa, ca, bt = stack.pop()
            parent_active, current_active, branch_taken = pa, ca, bt
            continue
        # 普通行
        if parent_active and current_active:
            out_lines.append(line)
    return "\n".join(out_lines)


# 根据类型名构造初始化块起始匹配正则
def make_block_start_regex(type_name: str) -> re.Pattern:
    # 匹配可选的 static/const 以及花括号可能在下一行的情况
    # 形式示例：
    #   static const TYPE name = { ... };
    #   const static TYPE name =
    #   {
    #       ...
    #   };
    #   const TYPE name = { ... };
    #   TYPE name = { ... };
    return re.compile(
        rf"(?:(?:static|const)\s+)*"                 # 可选修饰符
        rf"(?:struct\s+)?"                           # 可选struct前缀
        rf"{re.escape(type_name)}"                   # 类型名
        rf"\s*(?:\*+)?"                             # 可选指针
        rf"\s+([A-Za-z_]\w*)"                       # 变量名
        rf"\s*(?:\[[^\]]*\])?"                    # 可选数组声明
        rf"(?:\s+\w+)*"                             # 可选宏/属性（如PROGMEM）
        rf"\s*=\s*"                                 # 赋值号
    )
_re_string_literal = re.compile(r'"(?:\\.|[^"\\])*"')


def strip_comments(text: str) -> str:
    """去除C/C++中的 // 和 /* */ 注释。"""
    # 去除 /* ... */，跨行匹配
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    # 去除 // ... 到行末
    text = re.sub(r"//.*", "", text)
    return text


def decode_c_escaped_string(s: str, preserve_escapes: bool = False) -> str:
    """获取字符串字面量内容。
    - preserve_escapes=True: 不解码，按原始转义保留，返回去除外层引号后的文本（保留 \\xNN 等转义）
    - preserve_escapes=False: 尝试解码C风格转义为真实字符（Excel友好）
    """
    raw = s[1:-1]  # 去除外层引号
    if preserve_escapes:
        return raw
    try:
        tmp = codecs.decode(raw, 'unicode_escape')
        # 将包含的字节值映射为真实utf-8
        data = tmp.encode('latin-1')
        return data.decode('utf-8')
    except Exception:
        return raw


def extract_blocks(text: str, source_file: str, type_name: str, preserve_escapes: bool = False) -> List[ExtractedBlock]:
    """从文本中提取指定类型初始化块。"""
    results: List[ExtractedBlock] = []
    lines = text.splitlines()
    re_block_start = make_block_start_regex(type_name)
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re_block_start.search(line)
        if not m:
            i += 1
            continue
        var_name = m.group(1)
        start_line_no = i + 1
        # 收集直到匹配到 '};'（考虑花括号可能在下一行，使用括号深度跟踪）
        buf = [line]
        i += 1
        brace_depth = 0
        started = False
        while i < len(lines):
            buf.append(lines[i])
            # 检测首次出现 '{'
            if not started and '{' in lines[i]:
                started = True
            if '{' in lines[i]:
                brace_depth += lines[i].count('{')
            if '}' in lines[i]:
                brace_depth -= lines[i].count('}')
            # 当已开始并且括号归零且当前行包含 '};'（允许空格）时认为块结束
            if started and brace_depth == 0 and re.search(r"}\s*;", lines[i]):
                i += 1
                break
            i += 1
        block_text = "\n".join(buf)
        block_text_nc = strip_comments(block_text)
        # 在大括号内提取字符串字面值
        brace_content_match = re.search(r"\{(.*)\};", block_text_nc, flags=re.S)
        strings: List[str] = []
        if brace_content_match:
            inner = brace_content_match.group(1)
            # 如果存在尾部哨兵（NULL 或 nullptr），在其之前截止提取，避免误提取后续其他字段中的字符串
            m_sentinel = re.search(r"(?:^|[\s,])(NULL|nullptr)(?:[\s,]|$)", inner, flags=re.S)
            inner_for_strings = inner[:m_sentinel.start()] if m_sentinel else inner
            for sm in _re_string_literal.finditer(inner_for_strings):
                decoded = decode_c_escaped_string(sm.group(0), preserve_escapes=preserve_escapes)
                strings.append(decoded)
        results.append(ExtractedBlock(variable_name=var_name, strings=strings, source_file=source_file, line_number=start_line_no))
    return results


def find_alias_base(root: str, exts: List[str], alias: str) -> Optional[str]:
    """查找 typedef 别名：例如 typedef TY_TEXT _Tr_TEXT; 返回 TY_TEXT。
    也处理 typedef struct TagName Alias; 返回 TagName（作为结构体标签名）。"""
    alias_re_1 = re.compile(rf"\btypedef\s+(\w+)\s+{re.escape(alias)}\s*;")
    alias_re_2 = re.compile(rf"\btypedef\s+struct\s+(\w+)\s+{re.escape(alias)}\s*;")
    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if not any(fname.lower().endswith(ext.lower()) for ext in exts):
                continue
            fpath = os.path.join(dirpath, fname)
            try:
                text = read_text_file(fpath)
            except Exception:
                continue
            text_nc = strip_comments(text)
            m = alias_re_1.search(text_nc)
            if m:
                return m.group(1)
            m = alias_re_2.search(text_nc)
            if m:
                return m.group(1)
    return None


def find_struct_body(root: str, exts: List[str], type_name: str) -> Optional[str]:
    """查找结构体主体：支持两种形式
    1) typedef struct { ... } TYPE_NAME;
    2) struct TYPE_NAME { ... };
    返回大括号内的主体文本。"""
    pat_typedef = re.compile(rf"typedef\s+struct\s*\{{(.*?)\}}\s*{re.escape(type_name)}\s*;", flags=re.S)
    pat_struct = re.compile(rf"struct\s+{re.escape(type_name)}\s*\{{(.*?)\}}\s*;", flags=re.S)
    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if not any(fname.lower().endswith(ext.lower()) for ext in exts):
                continue
            fpath = os.path.join(dirpath, fname)
            try:
                text = read_text_file(fpath)
            except Exception:
                continue
            text_nc = strip_comments(text)
            m = pat_typedef.search(text_nc)
            if m:
                return m.group(1)
            m = pat_struct.search(text_nc)
            if m:
                return m.group(1)
    return None


def parse_language_columns_from_body(body: str) -> List[str]:
    """从结构体主体中解析语言字段名（char* 指针）。生成列名，优先从字段后缀提取。
    支持 const char* / char const* / char*。"""
    cols: List[str] = []
    # 指针字段匹配
    re_ptr_field = re.compile(r"\b(?:const\s+char|char\s+const|char)\s*\*\s*(\w+)\s*;")
    for name in re_ptr_field.findall(body):
        # 提取后缀作为语言名，例如 p_text_cn -> cn
        m = re.search(r"_(\w+)$", name)
        lang = m.group(1) if m else name
        cols.append(lang)
    return cols


def discover_language_columns(root: str, exts: List[str], type_alias: str) -> List[str]:
    """综合发现语言列：解析别名指向的基础类型，然后查找结构体主体并解析字段。
    未找到则回退到默认 LANG_COLUMNS。"""
    # 解析别名链，最多5层
    seen = set()
    current = type_alias
    for _ in range(5):
        base = find_alias_base(root, exts, current)
        if not base or base == current or base in seen:
            break
        seen.add(current)
        current = base
    # 尝试在基础类型或别名自身上查找结构体
    for candidate in (current, type_alias):
        body = find_struct_body(root, exts, candidate)
        if body:
            cols = parse_language_columns_from_body(body)
            if cols:
                return cols
    return LANG_COLUMNS


def scan_directory(root: str, exts: List[str], mode: str, defines: Dict[str, str], type_name: str, preserve_escapes: bool = False) -> List[ExtractedBlock]:
    """递归扫描目录，提取所有匹配块。"""
    all_blocks: List[ExtractedBlock] = []
    for dirpath, dirnames, filenames in os.walk(root):
        for fname in filenames:
            if not any(fname.lower().endswith(ext.lower()) for ext in exts):
                continue
            fpath = os.path.join(dirpath, fname)
            try:
                text = read_text_file(fpath)
            except Exception:
                continue
            if mode == 'effective':
                text_pp = preprocess(text, defines)
            else:
                text_pp = text
            blocks = extract_blocks(text_pp, fpath, type_name, preserve_escapes=preserve_escapes)
            all_blocks.extend(blocks)
    return all_blocks


def write_csv(output_path: str, rows: List[ExtractedBlock], lang_columns: List[str]) -> None:
    """写出CSV，使用utf-8-sig编码，Excel友好。列按动态语言列生成，并包含源位置。"""
    headers = ["source_file", "line_number", "variable_name"] + lang_columns
    with open(output_path, "w", encoding="utf-8-sig", newline="") as f:
        writer = csv.writer(f, quoting=csv.QUOTE_ALL)
        writer.writerow(headers)
        for r in rows:
            n = len(lang_columns)
            values = r.strings[:n] + [""] * (n - len(r.strings))
            row = [r.source_file, r.line_number, r.variable_name] + values
            writer.writerow(row)


# =========================
#  CSV -> C 代码生成部分
# =========================

def encode_c_string(s: str) -> str:
    """将Unicode字符串编码为C安全的UTF-8字节字面量（十六进制转义）。"""
    if s is None:
        return 'NULL'
    if s == "":
        return '""'
    b = s.encode('utf-8')
    return '"' + ''.join(f"\\x{byte:02X}" for byte in b) + '"'


def format_c_string(s: str, use_utf8_literal: bool) -> str:
    """根据选项返回C字符串字面量。
    - use_utf8_literal=True: 直接输出UTF-8字面量（对 `\` 与 `"` 做必要转义）
    - use_utf8_literal=False: 使用十六进制转义的安全形式
    """
    if s is None:
        return 'NULL'
    if s == "":
        return '""'
    if use_utf8_literal:
        # 仅转义必要的反斜杠和双引号，其他保持原样（UTF-8文件编码）
        escaped = s.replace('\\', '\\\\').replace('"', '\\"')
        return f'"{escaped}"'
    return encode_c_string(s)


def format_c_string_verbatim(s: str) -> str:
    """逐字输出CSV中的内容为C字符串字面量：
    - 保留反斜杠和转义序列（如 \\xNN）原样；
    - 仅转义内部双引号；
    - 空字符串输出 \"\"；
    """
    if s is None:
        return 'NULL'
    if s == "":
        return '""'
    # 保留反斜杠原样，不做额外转义；仅转义双引号
    escaped = s.replace('"', '\\"')
    return f'"{escaped}"'


def generate_c_from_csv(
    csv_path: str,
    type_name: str,
    output_path: Optional[str] = None,
    no_static: bool = False,
    header_output: Optional[str] = None,
    registry_array_name: Optional[str] = None,
    registry_emit: bool = False,
    use_utf8_literal: bool = False,
    null_sentinel: bool = False,
    verbatim: bool = False,
    literal_columns: Optional[List[str]] = None,
    annotate_mode: Optional[str] = None,
    per_line: int = 4,
    source_map: Optional[Dict[str, Tuple[str, str]]] = None,
    source_root: Optional[str] = None,
) -> str:
    """读取CSV并生成C结构体变量初始化代码。
    如果提供output_path则写入文件，否则返回字符串。"""
    rows: List[Dict[str, str]] = []
    with open(csv_path, 'r', encoding='utf-8-sig', newline='') as f:
        reader = csv.reader(f)
        headers = next(reader)
        # 识别列
        idx_source = headers.index('source_file') if 'source_file' in headers else None
        idx_line = headers.index('line_number') if 'line_number' in headers else None
        idx_var = headers.index('variable_name') if 'variable_name' in headers else None
        lang_indices = [i for i, h in enumerate(headers) if h not in ('source_file', 'line_number', 'variable_name')]
        lang_headers = [h for h in headers if h not in ('source_file', 'line_number', 'variable_name')]
        for row in reader:
            var_name = row[idx_var] if idx_var is not None and idx_var < len(row) else f"var_{len(rows)}"
            src_val = row[idx_source] if idx_source is not None and idx_source < len(row) else ''
            line_val = row[idx_line] if idx_line is not None and idx_line < len(row) else ''
            # 若CSV未提供来源信息且给定了映射，则按变量名回填
            if (not src_val or not line_val) and source_map and var_name in source_map:
                mapped_src, mapped_line = source_map[var_name]
                src_val = src_val or (mapped_src or '')
                line_val = line_val or (mapped_line or '')
            # 若提供了来源根路径且当前为相对路径，转换为期望的绝对形式
            if source_root and src_val:
                if src_val.startswith('.\\'):
                    # 拼接为: <root> + '\' + 去掉前缀的相对部分
                    rel = src_val[2:]
                    src_val = f"{source_root}\\{rel}"
                elif src_val.startswith('./'):
                    rel = src_val[2:]
                    # 将相对路径中的正斜杠替换为反斜杠以保持示例风格
                    rel = rel.replace('/', '\\')
                    src_val = f"{source_root}\\{rel}"
            rows.append({
                'source_file': src_val,
                'line_number': line_val,
                'variable_name': var_name,
                'values': row,
                'lang_indices': lang_indices,
                'lang_headers': lang_headers,
                'annotate_mode': annotate_mode,
            })

    lines: List[str] = []
    lines.append("/* Generated from CSV by ty_text_extractor.py */")
    lines.append("#include \"include/tr_text.h\"")
    if null_sentinel:
        lines.append("#include <stddef.h>")
    lines.append("")
    decls: List[str] = []
    registry_items: List[str] = []
    for entry in rows:
        src = entry['source_file']
        ln = entry['line_number']
        var = entry['variable_name']
        lang_vals = []
        for i_idx, i in enumerate(entry['lang_indices']):
            v = entry['values'][i] if i < len(entry['values']) else ''
            col_name = entry['lang_headers'][i_idx] if i_idx < len(entry['lang_headers']) else ''
            if verbatim:
                # 逐字输出（保留反斜杠与转义序列），仅转义双引号
                lang_vals.append(format_c_string_verbatim(v))
            else:
                # 若配置了按列字面量输出，则这些列使用UTF-8字面量；其余使用安全十六进制形式
                if literal_columns and col_name in set(literal_columns):
                    lang_vals.append(format_c_string(v, use_utf8_literal=True))
                else:
                    lang_vals.append(format_c_string(v, use_utf8_literal=False))
        # 若启用NULL哨兵，则将最后一个语言字段（通常为other）置为NULL，避免额外多一个初始值
        if null_sentinel and lang_vals:
            lang_vals[-1] = 'NULL'
        # 注释包含来源位置
        if src or ln:
            lines.append(f"/* source: {src} line: {ln} */")
        # 生成结构体初始化
        storage = "" if no_static else "static "
        lines.append(f"{storage}const {type_name} {var} = {{")
        # 多行排版，每行放置的字段数量（默认4，可配置）
        for j in range(0, len(lang_vals), per_line):
            # 若需要标注，先输出注释行
            if 'lang_headers' in entry:
                headers_slice = entry['lang_headers'][j:j+per_line]
            else:
                headers_slice = []
            if headers_slice:
                # 生成 names 或 indices 标注
                # 判断是否由CLI传入 --emit-annotate 选项（经由闭包不可达，这里用环境变量开关不合适），
                # 因此采用约定：当 headers_slice 非空时，默认输出 names 标注；如需 indices 则在调用处控制。
                # 为了可控，这里检测一个约定变量 annotate_mode 附加在 entry 上（向后兼容不影响现有调用）。
                annotate_mode = entry.get('annotate_mode', None)
                if annotate_mode == 'indices':
                    idxs = [f"[{k+1}]" for k in range(j, min(j+per_line, len(lang_vals)))]
                    lines.append("    // " + ", ".join(idxs))
                elif annotate_mode == 'names':
                    pairs = [f"[{j+i+1}] {name}" for i, name in enumerate(headers_slice)]
                    lines.append("    // " + ", ".join(pairs))
            chunk = ", ".join(lang_vals[j:j+per_line])
            lines.append(f"    {chunk},")
        # 去掉最后一行多余逗号：直接重写最后一行
        if lang_vals:
            last = lines.pop()
            lines.append(last.rstrip(',') )
        lines.append("};")
        lines.append("")
        # 头文件声明
        decls.append(f"extern const {type_name} {var};")
        # 注册表收集
        registry_items.append(var)

    # 若需要生成注册表数组，将其追加到.c内容末尾
    if registry_emit and registry_array_name:
        lines.append(f"const {type_name}* {registry_array_name}[] = {{")
        per_line_reg = 6
        for k in range(0, len(registry_items), per_line_reg):
            chunk = ", ".join(f"&{name}" for name in registry_items[k:k+per_line_reg])
            lines.append(f"    {chunk},")
        if registry_items:
            last = lines.pop()
            lines.append(last.rstrip(','))
        lines.append("};")
        lines.append("")

    code = "\n".join(lines)
    # 若需要生成头文件
    if header_output:
        header_lines: List[str] = []
        header_lines.append("/* Declarations generated by ty_text_extractor.py */")
        header_lines.append("#pragma once")
        header_lines.append("#include \"include/tr_text.h\"")
        header_lines.append("")
        header_lines.extend(decls)
        header_code = "\n".join(header_lines) + "\n"
        with open(header_output, 'w', encoding='utf-8') as hf:
            hf.write(header_code)
    if output_path:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(code)
    return code


def parse_defines(define_args: List[str]) -> Dict[str, str]:
    """解析 --define NAME=VALUE 列表为字典。"""
    result: Dict[str, str] = {}
    for item in define_args or []:
        if '=' in item:
            k, v = item.split('=', 1)
            result[k.strip()] = v.strip()
        else:
            result[item.strip()] = "1"
    return result


def main(argv: Optional[List[str]] = None) -> int:
    import argparse
    parser = argparse.ArgumentParser(description="提取TY_TEXT/_Tr_TEXT等结构体的多语言字符串并输出CSV")
    parser.add_argument('--root', required=True, help='工程根目录')
    parser.add_argument('--output', required=True, help='输出CSV路径')
    parser.add_argument('--extensions', default='.h,.hpp,.c,.cpp', help='扫描扩展名，逗号分隔')
    parser.add_argument('--define', action='append', default=[], help='宏定义 NAME=VALUE，可重复传入')
    parser.add_argument('--mode', choices=['effective', 'all'], default='effective', help='effective: 保留生效分支；all: 不处理条件编译')
    parser.add_argument('--type', default='_Tr_TEXT', help='结构体类型名，默认 _Tr_TEXT')
    parser.add_argument('--emit-from-csv', help='从CSV生成C代码的输入CSV路径（跳过扫描流程）')
    parser.add_argument('--emit-c-output', help='生成的C代码输出路径（可选，默认打印到stdout）')
    parser.add_argument('--emit-header-output', help='生成头文件声明的输出路径（可选）')
    parser.add_argument('--emit-no-static', action='store_true', help='生成C代码时移除static关键字')
    parser.add_argument('--emit-registry-name', help='生成注册表数组的名称（可选）')
    parser.add_argument('--emit-registry', action='store_true', help='是否在.c中生成指针注册表数组')
    parser.add_argument('--emit-utf8-literal', action='store_true', help='字符串以UTF-8字面量输出（不使用\\x转义）')
    parser.add_argument('--emit-null-sentinel', action='store_true', help='在结构体末尾追加NULL哨兵')
    parser.add_argument('--preserve-escapes', action='store_true', help='提取时保留原始转义，不解码')
    parser.add_argument('--emit-verbatim', action='store_true', help='CSV->C 按原始内容逐字输出（仅转义双引号）')
    parser.add_argument('--emit-literal-cols', help='逗号分隔列名（如 text_cn,text_en），这些列以UTF-8字面量输出，其它使用\\xNN安全形式')
    parser.add_argument('--emit-annotate', choices=['names', 'indices'], help='在结构体初始化每行值前生成标注注释（语言名或索引）')
    parser.add_argument('--emit-per-line', type=int, help='每行输出的语言字段数量，设置为1则一行一种语言（默认4）')
    parser.add_argument('--emit-source-map', help='可选：提供一个CSV（含 variable_name,source_file,line_number）用于按变量名回填来源注释')
    parser.add_argument('--emit-source-root', help='可选：当来源路径为相对路径(./ 或 .\\)时，拼接为以此为前缀的绝对形式')

    args = parser.parse_args(argv)
    # 若请求CSV->C生成，则执行生成后返回
    if args.emit_from_csv:
        literal_cols = None
        if args.emit_literal_cols:
            literal_cols = [c.strip() for c in args.emit_literal_cols.split(',') if c.strip()]
        per_line = args.emit_per_line if args.emit_per_line and args.emit_per_line > 0 else 4
        # 加载来源映射（如提供）
        source_map: Optional[Dict[str, Tuple[str, str]]] = None
        if args.emit_source_map:
            source_map = {}
            with open(args.emit_source_map, 'r', encoding='utf-8-sig', newline='') as mf:
                mreader = csv.reader(mf)
                mheaders = next(mreader)
                midx_var = mheaders.index('variable_name') if 'variable_name' in mheaders else None
                midx_src = mheaders.index('source_file') if 'source_file' in mheaders else None
                midx_line = mheaders.index('line_number') if 'line_number' in mheaders else None
                for mrow in mreader:
                    if midx_var is None or midx_var >= len(mrow):
                        continue
                    mv = mrow[midx_var]
                    ms = mrow[midx_src] if midx_src is not None and midx_src < len(mrow) else ''
                    ml = mrow[midx_line] if midx_line is not None and midx_line < len(mrow) else ''
                    if mv:
                        source_map[mv] = (ms, ml)
        code = generate_c_from_csv(
            args.emit_from_csv,
            args.type,
            args.emit_c_output,
            no_static=args.emit_no_static,
            header_output=args.emit_header_output,
            registry_array_name=args.emit_registry_name,
            registry_emit=args.emit_registry,
            use_utf8_literal=args.emit_utf8_literal,
            null_sentinel=args.emit_null_sentinel,
            verbatim=args.emit_verbatim,
            literal_columns=literal_cols,
            annotate_mode=args.emit_annotate,
            per_line=per_line,
            source_map=source_map,
            source_root=args.emit_source_root,
        )
        if not args.emit_c_output:
            print(code)
        else:
            print(f"已生成C代码: {args.emit_c_output}")
        return 0

    root = args.root
    output = args.output
    exts = [e.strip() for e in args.extensions.split(',') if e.strip()]
    defines = parse_defines(args.define)

    print(f"扫描目录: {root}")
    print(f"扩展名: {exts}")
    print(f"模式: {args.mode}")
    print(f"类型名: {args.type}")
    if defines:
        print(f"宏定义: {defines}")
    else:
        print("宏定义: 无")

    # 动态发现语言列
    lang_columns = discover_language_columns(root, exts, args.type)
    print(f"检测到类型 {args.type} 的语言列: {lang_columns}")
    blocks = scan_directory(root, exts, args.mode, defines, args.type, preserve_escapes=args.preserve_escapes)
    print(f"共提取到 {len(blocks)} 个初始化块")
    write_csv(output, blocks, lang_columns)
    print(f"已写出: {output}")
    return 0


if __name__ == '__main__':
    sys.exit(main())