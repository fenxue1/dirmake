/**
 * @file text_extractor.h
 * @brief 文本提取与生成模块接口（Text Extractor Module APIs）
 *
 * 功能名称（Module Name）：文本提取与代码生成（Text Extraction & C Code Generation）
 * 主要用途（Purpose）：
 * - 扫描 C/C++ 源文件，解析特定结构体的字符串初始化块；
 * - 生成 CSV；从 CSV 生成 C 代码；支持编码与转义策略；
 *
 * 使用示例（Usage Example）：
 *  - 提取：
 *    QStringList exts = {".c", ".cpp"};
 *    auto rows = TextExtractor::scanDirectory(root, exts, "raw", {}, "_Tr_TEXT", false);
 *    TextExtractor::writeCsv(outCsv, rows, TextExtractor::defaultLanguageColumns(), {"text_cn", "text_en"}, true);
 *  - 生成：
 *    TextExtractor::generateCFromCsv(csvPath, "_Tr_TEXT", cOut, hOut, false, true, "g_all_texts",
 *                                    false, true, true, false, {"text_cn", "text_en"}, "names", 0, {}, root);
 *
 * 注：中英文双语注释，中文优先；参数与返回说明见各函数文档。
 */
#ifndef TEXT_EXTRACTOR_H
#define TEXT_EXTRACTOR_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

struct ExtractedBlock {
    QString variableName;
    QStringList strings;
    QString sourceFile;
    int lineNumber{0};
};

struct ExtractedArray {
    QString arrayName;
    QString sourceFile;
    int lineNumber{0};
    QList<QStringList> elements; // each element values in language order (without NULL)
};

namespace TextExtractor {

// 语言列默认顺序
/**
 * @brief 获取默认语言列顺序（Get default language column ordering）
 * @return QStringList 默认列：cn,en,vn,ko,...（含other哨兵）
 */
QStringList defaultLanguageColumns();

// 读取文本（utf-8/gbk回退）
/**
 * @brief 自动编码读取文本，优先检测 BOM，再在 UTF-8/GB18030/GBK/GB2312/CP936 间选择替换符最少解码。
 * @param path 文件绝对路径（File absolute path）
 * @return QString 读取的 Unicode 文本（Decoded Unicode text）
 */
QString readTextFile(const QString &path);

// 去注释
/**
 * @brief 去除块/行注释，保留代码结构（Strip C/C++ style comments）
 * @param text 源文本
 * @return 去注释后的文本
 */
QString stripComments(const QString &text);

// 预处理 #if/#ifdef/#ifndef/#elif/#else/#endif，仅保留生效分支
/**
 * @brief 轻量预处理条件编译，仅保留命中分支（Lightweight preprocessor for conditional compilation）
 * @param text 源文本
 * @param defines 预定义宏与取值
 * @return 预处理后文本
 */
QString preprocess(const QString &text, const QMap<QString, QString> &defines);

// 从文本中提取初始化块
/**
 * @brief 按类型名解析结构体初始化语句，提取字符串数组（Extract initializer string arrays by type name）
 * @param text 预处理后的文本
 * @param sourceFile 源文件路径（用于溯源标注）
 * @param typeName 结构体别名或类型名（如"_Tr_TEXT"）
 * @param preserveEscapes 是否保留 \xNN 等转义（true 不解码）
 * @return ExtractedBlock 列表，含变量名/字符串/位置
 */
QList<ExtractedBlock> extractBlocks(const QString &text, const QString &sourceFile, const QString &typeName, bool preserveEscapes);

// 递归扫描目录
/**
 * @brief 递归扫描目录并提取符合扩展名的源文件中的初始化文本块
 * @param root 项目根目录
 * @param extensions 目标扩展名列表（如 .c/.cpp）
 * @param mode 提取模式：raw 或 effective（是否预处理）
 * @param defines 预处理宏
 * @param typeName 结构体类型名
 * @param preserveEscapes 是否保留转义
 * @return 提取结果集合
 */
QList<ExtractedBlock> scanDirectory(const QString &root, const QStringList &extensions, const QString &mode, const QMap<QString, QString> &defines, const QString &typeName, bool preserveEscapes);

// 提取结构体数组（按类型名），返回每个数组的元素值集合
QList<ExtractedArray> extractArrays(const QString &text, const QString &sourceFile, const QString &typeName, bool preserveEscapes);
QList<ExtractedArray> scanDirectoryArrays(const QString &root, const QStringList &extensions, const QString &mode, const QMap<QString, QString> &defines, const QString &typeName, bool preserveEscapes);

// 写数组到独立CSV：首行写 header：source_path,line_number,array_variable,<lang columns>
bool writeArraysCsv(const QString &outputPath,
                    const QList<ExtractedArray> &arrays,
                    const QStringList &langColumns,
                    const QStringList &literalColumns,
                    bool replaceAsciiCommaWithCn);

// 发现语言列（通过结构体字段名后缀）
/**
 * @brief 根据结构体的指针字段名 text_xx 推断语言列顺序（Discover language columns from struct fields）
 * @param root 项目根目录
 * @param extensions 要扫描的后缀
 * @param typeAlias 结构体别名
 * @return 语言列（不包含 text_other）
 */
QStringList discoverLanguageColumns(const QString &root, const QStringList &extensions, const QString &typeAlias);

// 写CSV（utf-8-sig），支持对非指定语言列进行UTF-8十六进制转义写出
/**
 * @brief 将提取结果写入 CSV（默认 UTF-8 BOM），支持保留指定语言列原文，其余以 UTF-8 十六进制转义。
 * @param outputPath 输出路径
 * @param rows 提取的块列表
 * @param langColumns 语言列顺序
 * @param literalColumns 需要直写原文的列
 * @param replaceAsciiCommaWithCn 是否将英文逗号替换为中文逗号
 * @return 是否写入成功
 */
bool writeCsv(const QString &outputPath,
              const QList<ExtractedBlock> &rows,
              const QStringList &langColumns,
              const QStringList &literalColumns,
              bool replaceAsciiCommaWithCn);

// CSV -> C 代码生成，返回代码字符串并可选写入文件及头文件
/**
 * @brief 根据 CSV 生成 C 源代码与头文件，支持直写/十六进制转义、注释模式、填充缺失项等。
 * @param csvPath 输入 CSV 路径
 * @param typeName 结构体类型名
 * @param cOutputPath C 源输出路径（可空）
 * @param headerOutputPath 头文件输出路径（可空）
 * @param noStatic 是否去除 static 修饰
 * @param registryEmit 是否生成注册表数组
 * @param registryArrayName 注册表数组变量名
 * @param useUtf8Literal 是否全局使用 UTF-8 直写（已在实现中禁用）
 * @param fillMissingWithEnglish 是否用英文列填充缺失项
 * @param nullSentinel 是否在末尾增加 NULL 哨兵
 * @param verbatim 是否保留原样（不转换）
 * @param literalColumns 直写列集合
 * @param annotateMode 注释模式：indices/names
 * @param perLine 每行元素个数（保留参数）
 * @param sourceMap 变量到源位置映射
 * @param sourceRoot 溯源根目录
 * @return 生成的 C 代码字符串
 *
 * @note 写入时保留原文件编码与 BOM，并保持换行风格（CRLF/LF）。
 */
QString generateCFromCsv(const QString &csvPath,
                         const QString &typeName,
                         const QString &cOutputPath,
                         const QString &headerOutputPath,
                         bool noStatic,
                         bool registryEmit,
                         const QString &registryArrayName,
                         bool useUtf8Literal,
                         bool fillMissingWithEnglish,
                         bool nullSentinel,
                         bool verbatim,
                         const QStringList &literalColumns,
                         const QString &annotateMode,
                         int perLine,
                         const QMap<QString, QPair<QString, QString>> &sourceMap,
                         const QString &sourceRoot);

}

#endif // TEXT_EXTRACTOR_H