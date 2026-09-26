#!/usr/bin/env python3
"""Сверяет вывод dump_book с ожидаемыми значениями для всех тестовых файлов."""
import json
import os
import subprocess
import sys

DUMP, FIX = sys.argv[1], sys.argv[2]
exp = json.load(open(os.path.join(FIX, "expected.json"), encoding="utf-8"))


def dump(name):
    out = subprocess.run([DUMP, os.path.join(FIX, name)], capture_output=True).stdout.decode("utf-8")
    lines = out.rstrip("\n").split("\n")
    head = lines[0].split("\t")
    return head, [l.split("\t") for l in lines[1:]]


def norm(rows):
    """Хвостовые пустые ячейки не важны."""
    out = []
    for r in rows:
        r = list(r)
        while r and r[-1] == "":
            r.pop()
        out.append(r)
    return out


fails = 0


def check(name, key, sheet=None, skip_first=False):
    global fails
    head, rows = dump(name)
    want = exp[key][1:] if skip_first else exp[key]
    got = norm(rows)
    want = norm(want)
    ok = head[0] == "SHEET" and got == want and (sheet is None or head[1] == sheet)
    if not ok:
        fails += 1
        print("FAIL", name, head)
        for i, (a, b) in enumerate(zip(got, want)):
            if a != b:
                print("  row", i, "\n   got ", a, "\n   want", b)
                break
        if len(got) != len(want):
            print("  rows got", len(got), "want", len(want))
    else:
        print("ok  ", name, "rows", len(got))


check("check.xlsx", "check", "Проверка")
check("check_lo.xlsx", "check", "Проверка")
check("check_lo97.xls", "check", "Проверка")
check("check_xlwt.xls", "check", "Проверка")
check("check_2003.xls", "check", "Проверка")
check("big.xlsx", "big", "Большой")
check("big_lo97.xls", "big", "Большой")
check("check_biff5.xls", "biff5", "Лист1")
check("check_biff5_mini.xls", "biff5", "Лист1")

# CSV и HTML: проверяем по сути
for name in ("act_1251.csv", "act_utf8.csv"):
    head, rows = dump(name)
    rows = norm(rows)
    ok = (rows[0] == ["Акт списания № 1245 от 24.09.2026"] and rows[1] == ["№ п/п", "№ КНТ", "Наименование"]
          and rows[2][1] == "30012000" and rows[2][2] == "Перфоратор SDS-plus 800 Вт, в кейсе"
          and rows[-1][2] == "дубль; с точкой с запятой" and len(rows) == 15)
    print("ok  " if ok else "FAIL", name, rows[:3], rows[-1])
    fails += 0 if ok else 1

head, rows = dump("act_html.xls")
rows = norm(rows)
ok = rows[0] == ["№ КНТ", "Наименование"] and rows[1] == ["30012000", "Перфоратор SDS-plus 800 Вт, в кейсе & ко"] and len(rows) == 6
print("ok  " if ok else "FAIL", "act_html.xls", rows[:2])
fails += 0 if ok else 1

print("FAILS:", fails)
sys.exit(1 if fails else 0)
