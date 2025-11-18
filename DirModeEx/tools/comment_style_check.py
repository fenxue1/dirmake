#!/usr/bin/env python3
"""
注释风格一致性检查（Comment Style Consistency Check）

- 检测双语（中英文）是否同时出现于文档注释块；
- 检查文档注释是否使用标准格式（/** ... */ 或 ///）；
- 输出按文件的风格一致性报告。

运行：
  python tools/comment_style_check.py --root . --out reports/comment_style.md
"""
import os
import re
import argparse

SOURCE_EXTS = {'.h', '.hpp', '.c', '.cpp', '.cc'}

DOCBLOCK_REGEX = re.compile(r"/\*\*.*?\*/", re.S)
LINE_DOC_REGEX = re.compile(r"^\s*///.*$", re.M)

def is_bilingual(text: str) -> bool:
    has_cjk = re.search(r"[\u4e00-\u9fff]", text) is not None
    has_en = re.search(r"[A-Za-z]", text) is not None
    return has_cjk and has_en

def process_file(path: str) -> dict:
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            text = f.read()
    except Exception:
        return { 'path': path, 'error': 'read_failed' }
    blocks = DOCBLOCK_REGEX.findall(text)
    lines = LINE_DOC_REGEX.findall(text)
    docs = blocks + lines
    total_docs = len(docs)
    bilingual_docs = sum(1 for d in docs if is_bilingual(d))
    style_ok = total_docs > 0
    bilingual_ok = (bilingual_docs == total_docs) if total_docs > 0 else False
    return {
        'path': path,
        'doc_blocks': len(blocks),
        'line_docs': len(lines),
        'total_docs': total_docs,
        'bilingual_docs': bilingual_docs,
        'style_ok': style_ok,
        'bilingual_ok': bilingual_ok,
    }

def walk(root: str):
    for dirpath, _, filenames in os.walk(root):
        for fn in filenames:
            ext = os.path.splitext(fn)[1].lower()
            if ext in SOURCE_EXTS:
                yield os.path.join(dirpath, fn)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default='.')
    ap.add_argument('--out', default='reports/comment_style.md')
    args = ap.parse_args()
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    results = [process_file(p) for p in walk(args.root)]
    with open(args.out, 'w', encoding='utf-8') as out:
        out.write('# 注释风格一致性报告 (Comment Style Consistency)\n\n')
        out.write('| 文件 | 文档注释数 | 行内文档数 | 合计 | 双语合规 | 风格存在 |\n')
        out.write('|---|---:|---:|---:|---:|---:|\n')
        for r in sorted(results, key=lambda x: x['path']):
            out.write(f"| `{os.path.relpath(r['path'], args.root)}` | {r['doc_blocks']} | {r['line_docs']} | {r['total_docs']} | {'✔' if r['bilingual_ok'] else '✘'} | {'✔' if r['style_ok'] else '✘'} |\n")
    print(f'Written: {args.out}')

if __name__ == '__main__':
    main()