#!/usr/bin/env python3
"""Иконки интерфейса (белые на прозрачном — окрашиваются в QML через filterColor)
и значок приложения. Рисуются в 4-кратном размере и уменьшаются для сглаживания."""
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
IMG = os.path.join(ROOT, "assets", "images")
os.makedirs(IMG, exist_ok=True)

S = 96          # итоговый размер иконки
K = 4           # множитель сглаживания
W = S * K
LW = int(9 * K)  # толщина линии


def canvas():
    im = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def save(im, name, size=S):
    im.resize((size, size), Image.LANCZOS).save(os.path.join(IMG, name))


def p(x, y):
    return (x * K, y * K)


def line(d, pts, width=LW):
    d.line([p(*q) for q in pts], fill="white", width=width, joint="curve")
    r = width / 2
    for q in (pts[0], pts[-1]):
        x, y = p(*q)
        d.ellipse([x - r, y - r, x + r, y + r], fill="white")


icons = {}


def icon(fn):
    icons[fn.__name__] = fn
    return fn


@icon
def play(d):
    d.polygon([p(30, 18), p(30, 78), p(80, 48)], fill="white")


@icon
def pause(d):
    d.rectangle([p(26, 18), p(40, 78)], fill="white")
    d.rectangle([p(56, 18), p(70, 78)], fill="white")


@icon
def stop(d):
    d.rectangle([p(22, 22), p(74, 74)], fill="white")


@icon
def grid(d):
    for i in range(3):
        for j in range(3):
            x, y = 14 + i * 24, 14 + j * 24
            d.rectangle([p(x, y), p(x + 18, y + 18)], fill="white")


@icon
def info(d):
    d.ellipse([p(10, 10), p(86, 86)], outline="white", width=LW)
    d.rectangle([p(43, 42), p(53, 70)], fill="white")
    d.ellipse([p(42, 24), p(54, 36)], fill="white")


@icon
def arrow_right(d):
    line(d, [(16, 48), (78, 48)])
    line(d, [(54, 24), (78, 48), (54, 72)])


@icon
def arrow_left(d):
    line(d, [(18, 48), (80, 48)])
    line(d, [(42, 24), (18, 48), (42, 72)])


@icon
def arrow_up(d):
    line(d, [(48, 18), (48, 80)])
    line(d, [(24, 42), (48, 18), (72, 42)])


@icon
def backspace(d):
    pts = [(30, 20), (86, 20), (86, 76), (30, 76), (8, 48), (30, 20)]
    d.line([p(*q) for q in pts], fill="white", width=LW, joint="curve")
    line(d, [(46, 36), (70, 60)])
    line(d, [(70, 36), (46, 60)])


@icon
def check(d):
    line(d, [(14, 50), (38, 74), (84, 24)], width=int(12 * K))


@icon
def close(d):
    line(d, [(22, 22), (74, 74)])
    line(d, [(74, 22), (22, 74)])


@icon
def folder(d):
    d.polygon([p(8, 22), p(36, 22), p(44, 30), p(88, 30), p(88, 78), p(8, 78)], fill="white")


@icon
def sheet(d):
    d.rectangle([p(16, 10), p(80, 86)], outline="white", width=LW)
    for y in (34, 52, 70):
        line(d, [(28, y), (68, y)], width=int(6 * K))


@icon
def sdcard(d):
    d.polygon([p(30, 8), p(80, 8), p(80, 88), p(16, 88), p(16, 22)], outline="white", width=LW)
    for x in (34, 48, 62):
        d.rectangle([p(x, 18), p(x + 7, 36)], fill="white")


@icon
def dot(d):
    d.ellipse([p(24, 24), p(72, 72)], fill="white")


@icon
def square(d):
    d.rectangle([p(8, 8), p(88, 88)], fill="white")


@icon
def frame(d):
    d.rectangle([p(10, 10), p(86, 86)], outline="white", width=int(10 * K))


@icon
def eye(d):
    d.ellipse([p(6, 26), p(90, 70)], outline="white", width=LW)
    d.ellipse([p(36, 36), p(60, 60)], fill="white")


@icon
def eye_off(d):
    eye(d)
    line(d, [(16, 84), (80, 12)])


for name, fn in icons.items():
    im, d = canvas()
    fn(d)
    save(im, name + ".png")


def app_icon(size, name):
    """Значок: клавиатура-«пульт» 3×3, правый нижний блок — янтарный с галочкой."""
    n = 512
    im = Image.new("RGBA", (n, n), (10, 11, 10, 255))
    d = ImageDraw.Draw(im)
    pad, gap = 70, 22
    cell = (n - 2 * pad - 2 * gap) / 3
    amber = (255, 176, 0, 255)
    for i in range(3):
        for j in range(3):
            if i >= 1 and j >= 1:
                continue
            x, y = pad + i * (cell + gap), pad + j * (cell + gap)
            d.rectangle([x, y, x + cell, y + cell], fill=(34, 37, 33, 255), outline=(70, 74, 68, 255), width=4)
    x0, y0 = pad + cell + gap, pad + cell + gap
    x1, y1 = n - pad, n - pad
    d.rectangle([x0, y0, x1, y1], fill=amber)
    cx, cy, s = (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2
    d.line([(cx - s * 0.52, cy + s * 0.02), (cx - s * 0.14, cy + s * 0.40), (cx + s * 0.55, cy - s * 0.38)],
           fill=(10, 11, 10, 255), width=34, joint="curve")
    im.resize((size, size), Image.LANCZOS).save(os.path.join(ROOT, name))


app_icon(144, "icon_144.png")
app_icon(110, "icon_110.png")
app_icon(96, "icon_96.png")
print("icons:", len(icons), "->", IMG)
