#!/usr/bin/env python3
"""
注释覆盖率报告生成器（Comment Coverage Reporter）

- 扫描项目源文件（.h/.hpp/.c/.cpp/.cc/.qml/.js 可扩展），统计：
  - 文件级文档块（/** ... @file ... */ 或开头处 /** ... */）是否存在；
  - 函数/方法声明/定义总数；
  - 具有紧邻文档注释（/** 或 ///）的函数数量；
  - 计算覆盖率百分比。

运行：
  python tools/comment_report.py --root . --out reports/comment_coverage.md
"""
import os
import re
import argparse
from typing import Tuple, List

SOURCE_EXTS = {'.h', '.hpp', '.c', '.cpp', '.cc'}

def has_file_docblock(text: str) -> bool:
    head = text[:2000]
    return ('@file' in head) or re.search(r"/\*\*.*?\*/", head, re.S) is not None

FUNC_REGEX = re.compile(r"^\s*(?:template\s*<[^>]+>\s*)?"
                        r"(?:[\w_:<>*&\s]+)\s+([A-Za-z_][\w:]*)\s*\([^\)]*\)\s*(?:const\s*)?(?:\{|;)$")

def count_functions_with_docs(lines: List[str]) -> Tuple[int, int]:
    total = 0
    documented = 0
    for i, line in enumerate(lines):
        if FUNC_REGEX.match(line):
            total += 1
            # Look up 3 lines for doc markers
            window = '\n'.join(lines[max(0, i-3):i])
            if (('/**' in window) or re.search(r"^\s*///", window, re.M)):
                documented += 1
    return total, documented

def process_file(path: str) -> dict:
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            text = f.read()
    except Exception:
        return { 'path': path, 'error': 'read_failed' }
    lines = text.splitlines()
    total, documented = count_functions_with_docs(lines)
    file_doc = has_file_docblock(text)
    coverage = (documented / total * 100.0) if total > 0 else (100.0 if file_doc else 0.0)
    return {
        'path': path,
        'file_doc': file_doc,
        'total_functions': total,
        'documented_functions': documented,
        'coverage_percent': round(coverage, 2)
    }

def walk(root: str) -> List[str]:
    files = []
    for dirpath, _, filenames in os.walk(root):
        for fn in filenames:
            ext = os.path.splitext(fn)[1].lower()
            if ext in SOURCE_EXTS:
                files.append(os.path.join(dirpath, fn))
    return files

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default='.')
    ap.add_argument('--out', default='reports/comment_coverage.md')
    args = ap.parse_args()
    files = walk(args.root)
    results = [process_file(p) for p in files]
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    total_files = len(results)
    avg_coverage = round(sum(r.get('coverage_percent', 0.0) for r in results) / total_files, 2) if total_files else 0.0
    with open(args.out, 'w', encoding='utf-8') as out:
        out.write('# 注释覆盖率报告 (Comment Coverage Report)\n\n')
        out.write(f'- 文件数: `{total_files}`\n')
        out.write(f'- 平均覆盖率: `{avg_coverage}%`\n\n')
        out.write('| 文件 | 文件头文档 | 函数总数 | 有文档函数 | 覆盖率 |\n')
        out.write('|---|---:|---:|---:|---:|\n')
        for r in sorted(results, key=lambda x: x['path']):
            out.write(f"| `{os.path.relpath(r['path'], args.root)}` | {'✔' if r['file_doc'] else '✘'} | {r['total_functions']} | {r['documented_functions']} | {r['coverage_percent']}% |\n")
    print(f'Written: {args.out}')

if __name__ == '__main__':
    main()