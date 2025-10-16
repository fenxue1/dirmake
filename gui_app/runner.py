'''
Author: fenxue1 99110925+fenxue1@users.noreply.github.com
Date: 2025-10-08 23:44:55
LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
LastEditTime: 2025-10-08 23:47:42
FilePath: \test_mooc-clin\gui_app\runner.py
Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
'''
import subprocess
import sys
from typing import List, Optional
import os
import sys


def _get_program_and_base_args() -> tuple[str, List[str]]:
    # 在打包后的可执行环境中，没有Python解释器；改为调用同目录的 ty_text_extractor.exe
    if getattr(sys, 'frozen', False):
        exe_dir = os.path.dirname(sys.executable)
        cli_exe = os.path.join(exe_dir, 'ty_text_extractor.exe')
        return cli_exe, []
    # 普通环境：使用Python解释器运行脚本
    return sys.executable, ["text/ty_text_extractor.py"]


def build_extract_command(
    root: str,
    csv_out: str,
    type_name: str,
    preserve_escapes: bool,
    extensions: Optional[str] = None,
    mode: Optional[str] = None,
    defines: Optional[List[str]] = None,
) -> List[str]:
    program, base_args = _get_program_and_base_args()
    cmd = [program] + base_args + ["--root", root, "--output", csv_out, "--type", type_name]
    if preserve_escapes:
        cmd.append("--preserve-escapes")
    if extensions:
        cmd.extend(["--extensions", extensions])
    if mode:
        cmd.extend(["--mode", mode])
    if defines:
        for d in defines:
            cmd.extend(["--define", d])
    return cmd


def build_emit_command(
    csv_in: str,
    c_out: str,
    type_name: str,
    verbatim: bool,
    utf8_literal: bool,
    null_sentinel: bool,
    no_static: bool,
    root: Optional[str],
    csv_out_intermediate: Optional[str],
    header_output: Optional[str] = None,
    registry_name: Optional[str] = None,
    registry_emit: bool = False,
) -> List[str]:
    program, base_args = _get_program_and_base_args()
    cmd = [program] + base_args + ["--emit-from-csv", csv_in, "--emit-c-output", c_out, "--type", type_name]
    if verbatim:
        cmd.append("--emit-verbatim")
    if utf8_literal:
        cmd.append("--emit-utf8-literal")
    if null_sentinel:
        cmd.append("--emit-null-sentinel")
    if no_static:
        cmd.append("--emit-no-static")
    if header_output:
        cmd.extend(["--emit-header-output", header_output])
    if registry_name:
        cmd.extend(["--emit-registry-name", registry_name])
    if registry_emit:
        cmd.append("--emit-registry")
    if root:
        cmd.extend(["--root", root])
    if csv_out_intermediate:
        cmd.extend(["--output", csv_out_intermediate])
    return cmd


def run_command(cmd: List[str]) -> subprocess.Popen:
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)