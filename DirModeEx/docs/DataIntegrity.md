<!--
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2025-11-17 19:28:25
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-11-17 19:49:24
 * @FilePath: \DirModeEx\docs\DataIntegrity.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# 数据完整性机制

本模块对 CSV 读取与处理阶段新增了数据完整性校验与报告输出，确保完整保留原始数据，不进行任何去重操作。

## 设计目标
- 保留 CSV 中的所有行（包括重复键与重复变量名），按原始顺序处理。
- 提供读取前后记录数对比与键频次统计，用于验证数据完整性。
- 在不改变现有业务逻辑的前提下，输出独立的完整性报告。

## 读取模块（Csv::parseFileWithReport）

### 解析特性
- 标准 CSV 引号处理：支持 RFC4180 双引号转义（`""` 表示一个 `"`）。
- 兼容常见的反斜杠转义（`\"`）用于引号文本的兼容读取。
- 自动跳过标题行：
  - 首个非空行若第二列不是整数（行号），视为标题；
  - 或命中典型列名（`source`/`line`/`variable`）≥2 项，视为标题。
- 保留原始顺序与重复项，不做任何去重、合并或排序。

### 错误与报告
- 若出现未闭合引号、字段不足（少于 3 列），解析将停止并返回错误。
- 报告字段：`totalLines`、`nonEmptyLines`、`parsedRows`、`duplicateKeyCount`、`keyHistogram`。

### 兼容性建议
- 若字段内包含引号，优先使用 RFC4180 的 `""` 转义；已有 `\"` 也可被兼容解析。
- 如需在第一行写表头，建议使用以下列名之一以提高识别准确率：`source_file`、`line_number`、`variable_name`。
- 新增 `CsvParseReport`：
  - `totalLines`：文件总行数（包含空行）。
  - `nonEmptyLines`：非空行数。
  - `parsedRows`：成功解析的行数。
  - `duplicateKeyCount`：重复键行数（按 `source_file|line_number|variable_name` 统计）。
  - `keyHistogram`：键频次分布。
- 不做任何去重：重复行保留并可被后续阶段逐行处理。

## 处理流程（CsvLangPlugin::applyTranslations）
- 调用 `parseFileWithReport` 获取行列表与报告。
- 记录统计并生成 `logs/csv_integrity_report.log`：包含计数与键频次分布。
- 处理阶段仍逐行应用至目标 `.c` 文件；不会丢弃或合并重复行。

## 使用说明
1. 在 UI 中选择项目根目录与 CSV 文件，运行“导入 CSV”。
2. 完成后查看：
   - `logs/csv_lang_plugin.log`：处理日志；
   - `logs/csv_lang_plugin.diff`：差异输出；
   - `logs/csv_integrity_report.log`：完整性报告（新增）。

## 注意事项
- 若同一键重复（`source+line+var`），后到的行会覆盖前一次修改，这是预期行为；报告中可查看重复情况。
- 若需改变重复行的处理策略（例如只处理首个或全部合并），请在配置层面提出明确规则后再扩展。
## 匹配策略配置（导入阶段）
- `strict_line_only`：布尔，默认 false。为 true 时仅按精确行号匹配，不再在±窗口内回退；适用于同名变量很多、行号准确的场景。
- `line_window`：整数，默认 50。非严格模式下的行号回退窗口大小。
- `ignore_variable_name`：布尔，默认 false。为 true 时忽略 CSV 的 `variable_name`，仅以 `source_file + line_number` 定位目标初始化体；适用于变量名不稳定或发生重命名的场景。
- 声明匹配支持：`static/const/struct` 限定词、附加限定词（如 `PROGMEM`）、指针 `*`、数组声明（`[]` 或 `[N]`）。