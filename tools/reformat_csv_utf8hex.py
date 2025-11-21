import csv
import sys
import argparse


def decode_escapes(s: str) -> str:
    r"""Decode C/Python-style escapes like \xNN, \n, \r, \t, \", \\ ."""
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]
            if n == 'x' and i + 3 < len(s):
                h1 = s[i + 2]
                h2 = s[i + 3]
                try:
                    out.append(int(h1 + h2, 16))
                    i += 4
                    continue
                except ValueError:
                    pass
            elif n == 'n':
                out.append(ord('\n'))
                i += 2
                continue
            elif n == 'r':
                out.append(ord('\r'))
                i += 2
                continue
            elif n == 't':
                out.append(ord('\t'))
                i += 2
                continue
            elif n == '\\':
                out.append(ord('\\'))
                i += 2
                continue
            elif n == '"':
                out.append(ord('"'))
                i += 2
                continue
            elif n == "'":
                out.append(ord("'"))
                i += 2
                continue
        # default: write UTF-8 of the single char
        out.extend(s[i].encode('utf-8'))
        i += 1
    return out.decode('utf-8', errors='replace')


def to_utf8_hex(s: str) -> str:
    if not s:
        return ""
    b = s.encode('utf-8')
    return ''.join(f"\\x{c:02X}" for c in b)


def main():
    ap = argparse.ArgumentParser(description="Reformat CSV: keep text_cn/text_en literal; others to UTF-8 hex escapes")
    ap.add_argument('--input', required=True, help='Input CSV path (UTF-8 with BOM)')
    ap.add_argument('--output', required=True, help='Output CSV path (UTF-8 with BOM)')
    ap.add_argument('--literal-cols', default='text_cn,text_en', help='Comma-separated column names to keep literal (e.g. text_cn,text_en)')
    ap.add_argument('--decode-escapes', action='store_true', help='Decode existing \\xNN/\\n/\\t escapes before formatting')
    args = ap.parse_args()

    literal = set([x.strip() for x in args.literal_cols.split(',') if x.strip()])

    with open(args.input, 'r', encoding='utf-8-sig', newline='') as fin, \
         open(args.output, 'w', encoding='utf-8-sig', newline='') as fout:
        reader = csv.reader(fin)
        writer = csv.writer(fout)
        headers = next(reader)
        writer.writerow(headers)

        # locate language columns: all except first three meta columns
        try:
            idx_source = headers.index('source_file')
            idx_line = headers.index('line_number')
            idx_var = headers.index('variable_name')
        except ValueError:
            # assume first three are meta
            idx_source, idx_line, idx_var = 0, 1, 2

        lang_idx = [i for i in range(len(headers)) if i not in (idx_source, idx_line, idx_var)]
        lang_names = [headers[i] for i in lang_idx]

        for row in reader:
            if not row:
                continue
            out = row[:]  # copy
            for k, lang in zip(lang_idx, lang_names):
                v = row[k]
                v2 = v
                if args.decode_escapes and ('\\x' in v or '\\n' in v or '\\t' in v or '\\r' in v):
                    v2 = decode_escapes(v)
                if lang in literal:
                    out[k] = v2
                else:
                    out[k] = to_utf8_hex(v2)
            writer.writerow(out)


if __name__ == '__main__':
    main()