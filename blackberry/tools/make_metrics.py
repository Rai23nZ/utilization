#!/usr/bin/env python3
"""Таблица ширин символов IBM Plex Sans SemiBold → src/SansMetrics.hpp.

Нужна для переноса наименования на вторую строку заранее, в C++: в Cascades нет
измерения текста, а автоперенос зависит от того, успела ли раскладка узнать ширину.
"""
import os

from fontTools.ttLib import TTFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
font = TTFont(os.path.join(ROOT, "assets", "fonts", "PlexSans-SemiBold.ttf"))
cmap = font.getBestCmap()
hmtx = font["hmtx"]
rows = []
for code in sorted(cmap):
    if code < 0x20:
        continue
    rows.append((code, hmtx[cmap[code]][0]))
fallback = hmtx[cmap[ord("n")]][0]

out = ["/* Сгенерировано tools/make_metrics.py: ширина символов IBM Plex Sans SemiBold, единиц на 1000 em. */",
       "#ifndef SANSMETRICS_HPP", "#define SANSMETRICS_HPP", "",
       "static const unsigned short SANS_FALLBACK = %d;" % fallback,
       "static const unsigned short SANS_ADV[][2] = {"]
line = "   "
for code, adv in rows:
    item = " {0x%04X, %d}," % (code, adv)
    if len(line) + len(item) > 110:
        out.append(line)
        line = "   "
    line += item
out.append(line)
out += ["};", "static const int SANS_COUNT = sizeof(SANS_ADV) / sizeof(SANS_ADV[0]);", "", "#endif", ""]
open(os.path.join(ROOT, "src", "SansMetrics.hpp"), "w", encoding="utf-8").write("\n".join(out))
print("glyphs:", len(rows), "fallback:", fallback)
