#!/usr/bin/env python3
"""Тестовые книги для проверки чтения таблиц на хосте.

Пишет в каталог fixtures/ одинаковые по смыслу данные в разных форматах
и файл expected.json с ожидаемым текстом ячеек (как cellText в app.js).
"""
import datetime
import json
import os
import random
import shutil
import subprocess
import sys

import openpyxl
import xlwt

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "fixtures")
os.makedirs(OUT, exist_ok=True)

NAMES = [
    "Перфоратор SDS-plus 800 Вт, в кейсе", "Дрель ударная 750 Вт", "Болгарка УШМ 125 мм",
    "Чайник электрический 1,7 л", "Утюг с парогенератором", "Ёлочная гирлянда 10 м",
    "Лобзик электрический 650 Вт", "Фен строительный 2000 Вт", "Кофеварка капельная",
    "Мультиварка 5 л", "Шуруповёрт аккумуляторный 18 В", "Набор бит 32 предмета",
]
DECISIONS = ["ДОПУЩЕН", "ДОПУЩЕН", "допущен к утилизации", "НЕ ДОПУЩЕН", "", "Недопущен", "ДОПУЩЕН"]

random.seed(7)
HEAD = ["№ КНТ", "КНТ", "Наименование товара", "Решение", "Код товара", "Торговая марка",
        "Цена по каталогу КИС", "Дата обнаружения", "Состояние НТ"]
rows = []
for i in range(40):
    knt = 30012000 + i * 37
    rows.append([
        knt,                                   # число
        str(knt)[-4:],                          # короткий как строка
        NAMES[i % len(NAMES)],
        DECISIONS[i % len(DECISIONS)],
        145000 + i,
        ["ЗУБР", "Bosch", "Makita", "Tefal"][i % 4],
        [6490, 1234.5, 999.999, 15.25][i % 4],
        datetime.date(2026, 9, 1 + i % 28),
        ["повреждена упаковка", "некомплект", "брак"][i % 3],
    ])


def text(v):
    """cellText из app.js."""
    if v is None or v == "":
        return ""
    if isinstance(v, datetime.date):
        return v.strftime("%d.%m.%Y")
    if isinstance(v, (int, float)):
        if float(v).is_integer():
            return str(int(v))
        r = int(v * 100 + 0.5) / 100.0 if v >= 0 else -int(-v * 100 + 0.5) / 100.0
        return str(int(r)) if float(r).is_integer() else repr(r)
    return str(v).replace(" ", " ").strip()


expected_rows = [["Результат проверки КНТ"], HEAD] + rows
expected = [[text(c) for c in r] for r in expected_rows]
width = max(len(r) for r in expected)
expected = [r + [""] * (width - len(r)) for r in expected]

# --- xlsx (openpyxl) — с заголовком над таблицей и пустой строкой между
wb = openpyxl.Workbook()
ws = wb.active
ws.title = "Проверка"
ws.append(["Результат проверки КНТ"])
ws.append([])
ws.append(HEAD)
for r in rows:
    ws.append(r)
for row in ws.iter_rows(min_row=4, min_col=8, max_col=8):
    for c in row:
        c.number_format = "DD.MM.YYYY"
ws2 = wb.create_sheet("Второй лист")
ws2.append(["не должен читаться"])
wb.save(os.path.join(OUT, "check.xlsx"))

# --- xls BIFF8 (xlwt)
book = xlwt.Workbook(encoding="utf-8")
sh = book.add_sheet("Проверка")
datefmt = xlwt.easyxf(num_format_str="DD.MM.YYYY")
sh.write(0, 0, "Результат проверки КНТ")
for c, h in enumerate(HEAD):
    sh.write(2, c, h)
for r, row in enumerate(rows):
    for c, v in enumerate(row):
        if isinstance(v, datetime.date):
            sh.write(r + 3, c, v, datefmt)
        else:
            sh.write(r + 3, c, v)
book.add_sheet("Второй лист").write(0, 0, "не должен читаться")
book.save(os.path.join(OUT, "check_xlwt.xls"))

# --- LibreOffice: BIFF8, BIFF5 (Excel 95), XML 2003, CSV
def lo(src, filt, dst_ext, dst_name):
    tmp = os.path.join(OUT, "_lo")
    os.makedirs(tmp, exist_ok=True)
    subprocess.run(["soffice", "--headless", "--convert-to", f"{dst_ext}:{filt}", "--outdir", tmp, src],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    base = os.path.splitext(os.path.basename(src))[0]
    produced = os.path.join(tmp, base + "." + dst_ext)
    shutil.move(produced, os.path.join(OUT, dst_name))


src = os.path.join(OUT, "check.xlsx")
lo(src, "MS Excel 97", "xls", "check_lo97.xls")
lo(src, "Calc MS Excel 2007 XML", "xlsx", "check_lo.xlsx")

# --- SpreadsheetML (XML 2003), сохранённый с расширением .xls
from xml.sax.saxutils import escape


def xml_cell(v):
    if isinstance(v, datetime.date):
        return '<Cell ss:StyleID="d"><Data ss:Type="DateTime">%sT00:00:00.000</Data></Cell>' % v.isoformat()
    if isinstance(v, (int, float)):
        return '<Cell><Data ss:Type="Number">%s</Data></Cell>' % v
    return '<Cell><Data ss:Type="String">%s</Data></Cell>' % escape(str(v))


x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<?mso-application progid="Excel.Sheet"?>',
     '<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet" '
     'xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">',
     '<Worksheet ss:Name="Проверка"><Table>',
     '<Row>' + xml_cell("Результат проверки КНТ") + '</Row>',
     '<Row ss:Index="3">' + "".join(xml_cell(h) for h in HEAD) + '</Row>']
for r in rows:
    x.append('<Row>' + "".join(xml_cell(v) for v in r) + '</Row>')
x += ['</Table></Worksheet>', '<Worksheet ss:Name="Второй"><Table><Row>' + xml_cell("нет") +
      '</Row></Table></Worksheet>', '</Workbook>']
with open(os.path.join(OUT, "check_2003.xls"), "w", encoding="utf-8") as f:
    f.write("\n".join(x))

# --- большая книга: SST разрывается записями CONTINUE
wb = openpyxl.Workbook()
ws = wb.active
ws.title = "Большой"
ws.append(["№ КНТ", "Решение", "Наименование товара"])
big_expected = [["№ КНТ", "Решение", "Наименование товара"]]
for i in range(3000):
    name = "Позиция номер %d — длинное наименование товара с кириллицей ёЁ и символами «»" % i
    if i % 7 == 0:
        name += " / latin only part to mix compressed strings"
    ws.append([40000000 + i, "ДОПУЩЕН" if i % 5 else "НЕ ДОПУЩЕН", name])
    big_expected.append([str(40000000 + i), "ДОПУЩЕН" if i % 5 else "НЕ ДОПУЩЕН", name])
wb.save(os.path.join(OUT, "big.xlsx"))
lo(os.path.join(OUT, "big.xlsx"), "MS Excel 97", "xls", "big_lo97.xls")

# --- акт: CSV в Windows-1251 с шапкой не в первой строке
act = ["Акт списания № 1245 от 24.09.2026", "", "№ п/п;№ КНТ;Наименование"]
for i, r in enumerate(rows[:12]):
    act.append(f"{i + 1};{r[0]};\"{r[2]}\"")
act.append(f"13;{rows[0][0]};\"дубль; с точкой с запятой\"")
with open(os.path.join(OUT, "act_1251.csv"), "wb") as f:
    f.write("\r\n".join(act).encode("cp1251"))
with open(os.path.join(OUT, "act_utf8.csv"), "wb") as f:
    f.write(("﻿" + "\n".join(act)).encode("utf-8"))

# --- HTML, сохранённый как .xls (так выгружают некоторые учётные системы)
html = ["<html><head><meta http-equiv=Content-Type content=\"text/html; charset=windows-1251\"></head><body><table>",
        "<tr><th>№ КНТ</th><th>Наименование</th></tr>"]
for r in rows[:5]:
    html.append(f"<tr><td>{r[0]}</td><td>{r[2]} &amp; ко</td></tr>")
html.append("</table></body></html>")
with open(os.path.join(OUT, "act_html.xls"), "wb") as f:
    f.write("\n".join(html).encode("cp1251"))

# --- BIFF5 (Excel 5/95) вручную: LibreOffice больше не пишет этот формат
import struct


def rec(t, data):
    return struct.pack("<HH", t, len(data)) + data


def cfb(stream_name, stream):
    """Минимальный Compound File v3: FAT, каталог, поток (обычный или мини)."""
    SEC = 512
    mini = len(stream) < 4096
    sectors = []  # содержимое секторов по порядку

    def alloc(data):
        start = len(sectors)
        for i in range(0, max(len(data), 1), SEC):
            sectors.append(data[i:i + SEC].ljust(SEC, b"\0"))
        return start, (len(data) + SEC - 1) // SEC

    chains = []
    if mini:
        mdata = stream.ljust(((len(stream) + 63) // 64) * 64, b"\0")
        nmini = len(mdata) // 64
        minifat = b"".join(struct.pack("<I", i + 1 if i + 1 < nmini else 0xFFFFFFFE) for i in range(nmini))
        mf_start, mf_n = alloc(minifat.ljust(SEC, b"\xff"))
        chains.append((mf_start, mf_n))
        ms_start, ms_n = alloc(mdata)
        chains.append((ms_start, ms_n))
        root_start, root_size = ms_start, len(mdata)
        st_start, st_size = 0, len(stream)
    else:
        st_start, st_n = alloc(stream)
        chains.append((st_start, st_n))
        root_start, root_size = 0xFFFFFFFE, 0
        st_size = len(stream)
        mf_start, mf_n = 0xFFFFFFFE, 0

    def direntry(name, typ, start, size, child=0xFFFFFFFF):
        n = (name + "\0").encode("utf-16-le") if name else b""
        e = n.ljust(64, b"\0") + struct.pack("<HBB", len(n), typ, 1)
        e += struct.pack("<III", 0xFFFFFFFF, 0xFFFFFFFF, child)
        e += b"\0" * 16 + b"\0" * 4 + b"\0" * 16
        e += struct.pack("<III", start, size, 0)
        return e

    dirdata = direntry("Root Entry", 5, root_start, root_size, 1) + direntry(stream_name, 2, st_start, st_size)
    dirdata += direntry("", 0, 0, 0) * 2
    dir_start, dir_n = alloc(dirdata)
    chains.append((dir_start, dir_n))

    fat_index = len(sectors)
    sectors.append(b"")  # место под FAT
    fat = [0xFFFFFFFF] * (SEC // 4)
    for start, n in chains:
        for i in range(n):
            fat[start + i] = start + i + 1 if i + 1 < n else 0xFFFFFFFE
    fat[fat_index] = 0xFFFFFFFD
    sectors[fat_index] = b"".join(struct.pack("<I", x) for x in fat)

    hdr = b"\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1" + b"\0" * 16
    hdr += struct.pack("<HHHHH", 0x3E, 3, 0xFFFE, 9, 6) + b"\0" * 6
    hdr += struct.pack("<IIIIIIIII", 0, 1, dir_start, 0, 4096, mf_start, mf_n, 0xFFFFFFFE, 0)
    difat = [fat_index] + [0xFFFFFFFF] * 108
    hdr += b"".join(struct.pack("<I", x) for x in difat)
    return hdr + b"".join(sectors)


def biff5(rows_, sheet="Лист1", pad=0):
    cp = "cp1251"
    xfs = b"".join(rec(0x00E0, struct.pack("<HH", 0, fmt) + b"\0" * 12) for fmt in [0] * 16 + [0, 14])
    glob = rec(0x0809, struct.pack("<HHHH", 0x0500, 0x0005, 0, 0))
    glob += rec(0x0042, struct.pack("<H", 1251))
    glob += rec(0x041E, struct.pack("<HB", 14, 10) + b"dd.mm.yyyy")
    glob += xfs
    name = sheet.encode(cp)
    bs_pos = len(glob)
    glob += rec(0x0085, struct.pack("<IBB", 0, 0, 0) + bytes([len(name)]) + name)
    glob += rec(0x000A, b"")
    body = rec(0x0809, struct.pack("<HHHH", 0x0500, 0x0010, 0, 0))
    body += rec(0x0200, struct.pack("<HHHHH", 0, len(rows_), 0, 3, 0))
    for r, row in enumerate(rows_):
        for c, v in enumerate(row):
            if isinstance(v, str):
                b = v.encode(cp)
                body += rec(0x0204, struct.pack("<HHHH", r, c, 15, len(b)) + b)
            elif isinstance(v, datetime.date):
                serial = (v - datetime.date(1899, 12, 30)).days
                body += rec(0x027E, struct.pack("<HHHI", r, c, 17, (serial << 2) | 2))
            elif isinstance(v, int) and abs(v) < (1 << 29):
                body += rec(0x027E, struct.pack("<HHHI", r, c, 15, (v << 2) | 2))
            else:
                body += rec(0x0203, struct.pack("<HHHd", r, c, 15, float(v)))
    body += rec(0x000A, b"")
    # смещение листа в BOUNDSHEET
    glob = glob[:bs_pos + 4] + struct.pack("<I", len(glob)) + glob[bs_pos + 8:]
    return cfb("Book", glob + body + b"\0" * pad)


b5rows = [["№ КНТ", "Наименование товара", "Решение", "Дата"]]
b5exp = [["№ КНТ", "Наименование товара", "Решение", "Дата"]]
for i in range(6):
    b5rows.append([30012000 + i * 37, NAMES[i], DECISIONS[i] or "ДОПУЩЕН", datetime.date(2026, 9, 3 + i)])
    b5exp.append([str(30012000 + i * 37), NAMES[i], DECISIONS[i] or "ДОПУЩЕН", "%02d.09.2026" % (3 + i)])
with open(os.path.join(OUT, "check_biff5_mini.xls"), "wb") as f:
    f.write(biff5(b5rows))
with open(os.path.join(OUT, "check_biff5.xls"), "wb") as f:
    f.write(biff5(b5rows, pad=5000))

exp = {
    "biff5": b5exp,
    "check": expected,
    "big": big_expected,
    "act": [[c for c in line.split(";")] for line in []],
}
with open(os.path.join(OUT, "expected.json"), "w", encoding="utf-8") as f:
    json.dump(exp, f, ensure_ascii=False, indent=0)
shutil.rmtree(os.path.join(OUT, "_lo"), ignore_errors=True)
print("fixtures ->", OUT)
