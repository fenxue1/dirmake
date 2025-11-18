# 注释规范与交付说明 (Commenting Guide & Deliverables)

## 目标
- 为项目中所有功能模块添加中英文双语文档注释与关键代码行内注释；
- 使用标准文档注释格式（Doxygen 风格 `/** ... */` 或 `///`）；
- 生成“注释覆盖率报告”与“风格一致性检查报告”。

## 编写要求
- 文档注释应包含：功能名称、主要用途、输入参数、返回值、使用示例；
- 行内注释覆盖关键算法、业务规则、特殊处理、注意事项；
- 中文优先，附英文解释；确保与代码同步更新。

## 已注释的模块
- `text_extractor.h/.cpp`: 文本提取与 C 代码生成；
- `csv_parser.h/.cpp`: CSV 解析；
- `csv_lang_plugin.h`: CSV 翻译应用插件；
- `language_settings.h`: 语言初始化与回滚；
- `diff_utils.h`: 统一 diff 生成；
- `mainwindow.h`: 主窗口 UI 模块；
- `main.cpp`: 应用入口。

## 生成报告
```bash
python tools/comment_report.py --root . --out reports/comment_coverage.md
python tools/comment_style_check.py --root . --out reports/comment_style.md
```

## 报告说明
- 覆盖率：统计函数/方法的文档注释存在比例；文件头有 `@file` 或文档块视为文件级文档；
- 风格一致性：检查所有文档注释是否为双语（同时存在中文与英文字符），并统计数量。

## 维护建议
- 在新增/修改函数时，同步更新其上方的文档注释；
- 关键算法处添加简洁、直达的行内注释；
- 在 PR 校验阶段运行两份报告，保证质量门槛。