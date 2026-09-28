#!/usr/bin/env python3
"""Статическая проверка QML-файлов приложения по описаниям типов Cascades из NDK.

Запустить устройство здесь нельзя, поэтому ловим опечатки заранее:
неизвестные типы, свойства, сигналы (onXxx), значения перечислений, методы у id,
обращения к контроллеру (app.*) и палитре (T.*), несбалансированные скобки.

QML написан в строгом стиле «один член — одна строка», на это и рассчитан разбор.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qmltypes import Types  # noqa: E402

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
ASSETS = os.path.join(ROOT, "assets")
MODULES = ["bb.cascades", "bb.system"]

types = Types()
errors = []

# глобальные объекты QML/JS, которые не являются типами из qmltypes
GLOBALS = {"Color", "Application", "Math", "Qt", "ListItemData", "ListItem", "JSON", "parseInt", "String",
           "Number", "console", "true", "false", "undefined", "null", "event", "indexPath", "value",
           "var", "if", "else", "for", "return", "function", "this"}


def err(path, lineno, msg):
    errors.append("%s:%d: %s" % (os.path.relpath(path, ROOT), lineno, msg))


# ---------------------------------------------------------------- API контроллера
def controller_api():
    src = open(os.path.join(ROOT, "src", "Controller.hpp"), encoding="utf-8").read()
    api = set(re.findall(r"Q_PROPERTY\([^)]*?\b(\w+)\s+READ", src))
    api |= set(re.findall(r"Q_INVOKABLE\s+[\w:<>]+\s+(\w+)\(", src))
    for section in re.findall(r"(?:public slots|signals):(.*?)(?:\n\s*(?:public|private|protected)[\w ]*:|\n};)", src, re.S):
        api |= set(re.findall(r"void\s+(\w+)\(", section))
    return api


def theme_keys():
    src = open(os.path.join(ROOT, "src", "main.cpp"), encoding="utf-8").read()
    return set(re.findall(r't\.insert\("(\w+)"', src))


APP = controller_api()
THEME = theme_keys()


# ---------------------------------------------------------------- свои компоненты
def strip_comment(line):
    out, q = [], None
    i = 0
    while i < len(line):
        ch = line[i]
        if q:
            out.append(ch)
            if ch == "\\":
                out.append(line[i + 1] if i + 1 < len(line) else "")
                i += 2
                continue
            if ch == q:
                q = None
        else:
            if ch in "\"'":
                q = ch
            elif line.startswith("//", i):
                break
            out.append(ch)
        i += 1
    return "".join(out).rstrip()


def custom_components():
    comps = {}
    for fn in os.listdir(ASSETS):
        if not fn.endswith(".qml") or not fn[0].isupper():
            continue
        name = fn[:-4]
        lines = [strip_comment(l) for l in open(os.path.join(ASSETS, fn), encoding="utf-8")]
        root = None
        depth = 0
        props, signals, funcs = {}, set(), set()
        for l in lines:
            s = l.strip()
            if root is None:
                m = re.match(r"^([A-Z]\w*)\s*\{$", s)
                if m:
                    root = m.group(1)
                    depth = 1
                continue
            if depth == 1:
                m = re.match(r"^property\s+(\w+)\s+(\w+)", s)
                if m:
                    props[m.group(2)] = m.group(1)
                m = re.match(r"^signal\s+(\w+)", s)
                if m:
                    signals.add(m.group(1))
                m = re.match(r"^function\s+(\w+)", s)
                if m:
                    funcs.add(m.group(1))
            depth += s.count("{") - s.count("}")
        comps[name] = {"root": root, "props": props, "signals": signals, "funcs": funcs}
    return comps


CUSTOM = custom_components()


class T:
    """Тип объекта: C++ имя из qmltypes плюс свои свойства/сигналы/функции."""

    def __init__(self, short):
        self.short = short
        self.props, self.signals, self.funcs = {}, set(), set()
        self.cpp = None
        seen = set()
        while short in CUSTOM and short not in seen:
            seen.add(short)
            c = CUSTOM[short]
            self.props.update(c["props"])
            self.signals |= c["signals"]
            self.funcs |= c["funcs"]
            short = c["root"]
        self.cpp = types.resolve(short, MODULES)

    @property
    def known(self):
        return self.cpp is not None

    def prop(self, name):
        if name in self.props:
            return self.props[name]
        return types.prop(self.cpp, name) if self.cpp else None

    def has_signal(self, name):
        if name in self.signals:
            return True
        if name.endswith("Changed") and self.prop(name[:-7]) is not None:
            return True
        return bool(self.cpp) and types.has_signal(self.cpp, name)

    def has_method(self, name):
        return name in self.funcs or name in self.signals or (bool(self.cpp) and types.has_method(self.cpp, name))


def cpp_type_of(prop_type):
    """Тип свойства из qmltypes → C++ имя компонента (для групповых свойств)."""
    t = prop_type.rstrip("*").strip()
    return t if t in types.components else None


# ---------------------------------------------------------------- проверка выражений
ENUM_HOLDERS = {}
for (mod, short), cpp in types.exports.items():
    if types.components[cpp]["enums"]:
        ENUM_HOLDERS.setdefault(short, cpp)


def check_expr(path, lineno, expr, ids, frame_type):
    code = re.sub(r'"(?:[^"\\]|\\.)*"', '""', expr)
    code = re.sub(r"'(?:[^'\\]|\\.)*'", "''", code)
    for m in re.finditer(r"\bapp\.(\w+)", code):
        if m.group(1) not in APP:
            err(path, lineno, "у контроллера нет app.%s" % m.group(1))
    for m in re.finditer(r"\bT\.(\w+)", code):
        if m.group(1) not in THEME:
            err(path, lineno, "в палитре нет T.%s" % m.group(1))
    for m in re.finditer(r"(?<![\w.])([A-Z]\w*)\.([A-Z]\w*)\b", code):
        holder, val = m.groups()
        if holder in ("Color", "Application", "Qt", "Math", "ListItem", "ListItemData"):
            continue
        cpp = ENUM_HOLDERS.get(holder)
        if not cpp:
            err(path, lineno, "неизвестное перечисление %s.%s" % (holder, val))
            continue
        vals = set()
        for e in types.components[cpp]["enums"].values():
            vals |= set(e.keys()) if isinstance(e, dict) else set()
        if val not in vals:
            err(path, lineno, "в %s нет значения %s" % (holder, val))
    for m in re.finditer(r"(?<![\w.])([a-z]\w*)\.(\w+)(\s*\()?", code):
        ident, member, call = m.group(1), m.group(2), m.group(3)
        if ident in ("app", "event", "value", "indexPath", "ui", "Math", "d", "p", "items", "list", "rows",
                     "text", "kind", "action", "title", "body", "yes"):
            continue
        if ident not in ids:
            continue
        t = ids[ident]
        if call:
            if not t.has_method(member):
                err(path, lineno, "у %s (%s) нет метода %s()" % (ident, t.short, member))
        else:
            if t.prop(member) is None and not t.has_method(member):
                err(path, lineno, "у %s (%s) нет свойства %s" % (ident, t.short, member))
    for m in re.finditer(r"\bColor\.(\w+)", code):
        if m.group(1) not in ("create", "White", "Black", "Red", "Green", "Blue", "Gray", "DarkGray", "LightGray",
                              "Transparent", "Yellow", "Cyan", "Magenta"):
            err(path, lineno, "Color.%s не существует" % m.group(1))
    for m in re.finditer(r"\bui\.(\w+)", code):
        if m.group(1) not in ("du", "sdu", "px", "ddu", "sddu"):
            err(path, lineno, "ui.%s не существует" % m.group(1))
    if code.count("(") != code.count(")"):
        err(path, lineno, "несбалансированные круглые скобки")


# ---------------------------------------------------------------- разбор файла
def check_file(path):
    raw = open(path, encoding="utf-8").read().split("\n")
    lines = [strip_comment(l) for l in raw]
    objects = {}   # номер строки открытия → T (общий для обоих проходов)
    ids = {}
    for validate in (False, True):
        walk(path, lines, objects, ids, validate)


def walk(path, lines, objects, ids, validate):
    report = err if validate else (lambda *a: None)
    check = check_expr if validate else (lambda *a: None)
    cprop = check_prop if validate else (lambda *a: None)

    stack = []           # элементы: ("obj", T, lineno) | ("list", prop, lineno)
    js_depth = 0
    js_start = 0
    js_text = []
    for i, l in enumerate(lines, 1):
        s = l.strip()
        if not s:
            continue
        if js_depth > 0:
            js_depth += s.count("{") - s.count("}")
            js_text.append((i, s))
            if js_depth <= 0:
                for ln, t in js_text:
                    check(path, ln, t, ids, None)
                js_text = []
            continue
        if s.startswith("import "):
            continue
        if s in ("}", "},"):
            if not stack or stack[-1][0] != "obj":
                report(path, i, "лишняя закрывающая скобка }")
            else:
                stack.pop()
            continue
        if s in ("]", "],"):
            if not stack or stack[-1][0] != "list":
                report(path, i, "лишняя закрывающая скобка ]")
            else:
                stack.pop()
            continue
        cur = stack[-1] if stack else None

        # новый объект: «Тип {» или «свойство: Тип {»
        m = re.match(r"^(?:(\w+(?:\.\w+)*)\s*:\s*)?([A-Z]\w*)\s*\{$", s)
        if m:
            prop, tname = m.groups()
            if i not in objects:
                objects[i] = T(tname)
            t = objects[i]
            if not t.known:
                report(path, i, "неизвестный тип %s" % tname)
            if prop and cur and cur[0] == "obj":
                cprop(path, i, cur[1], prop, ids)
            if cur and cur[0] == "list" and cur[1] == "listItemComponents" and tname != "ListItemComponent":
                report(path, i, "в listItemComponents ожидается ListItemComponent")
            stack.append(("obj", t, i))
            continue
        if cur is None:
            report(path, i, "строка вне объекта: %s" % s)
            continue
        if cur[0] == "list":
            report(path, i, "в списке ожидается объект: %s" % s)
            continue
        obj = cur[1]

        m = re.match(r"^property\s+(\w+)\s+(\w+)(?:\s*:\s*(.+))?$", s)
        if m:
            obj.props[m.group(2)] = m.group(1)
            if m.group(3):
                check(path, i, m.group(3), ids, obj)
            continue
        m = re.match(r"^signal\s+(\w+)\s*\(.*\)$", s)
        if m:
            obj.signals.add(m.group(1))
            continue
        m = re.match(r"^function\s+(\w+)\s*\(.*\)\s*\{$", s)
        if m:
            obj.funcs.add(m.group(1))
            js_depth = 1
            js_start = i
            continue
        m = re.match(r"^id:\s*(\w+)$", s)
        if m:
            ids[m.group(1)] = obj
            continue
        m = re.match(r"^(\w+(?:\.\w+)*)\s*:\s*\[$", s)
        if m:
            cprop(path, i, obj, m.group(1), ids)
            stack.append(("list", m.group(1), i))
            continue
        m = re.match(r"^(on[A-Z]\w*)\s*:\s*(.*)$", s)
        if m:
            handler, rest = m.groups()
            sig = handler[2].lower() + handler[3:]
            if validate and not obj.has_signal(sig):
                report(path, i, "у %s нет сигнала %s (обработчик %s)" % (obj.short, sig, handler))
            if rest == "{":
                js_depth = 1
                js_start = i
            else:
                check(path, i, rest, ids, obj)
            continue
        m = re.match(r"^(\w+(?:\.\w+)*)\s*:\s*(.+)$", s)
        if m:
            prop, val = m.groups()
            cprop(path, i, obj, prop, ids)
            if val.endswith("{") and not re.match(r"^[A-Z]", val):
                report(path, i, "многострочное значение не поддерживается: %s" % s)
            check(path, i, val, ids, obj)
            continue
        report(path, i, "не разобрано: %s" % s)

    if stack:
        report(path, stack[-1][2], "не закрыт объект/список, открытый здесь")
    if js_depth:
        report(path, js_start, "не закрыт блок JavaScript")


def check_prop(path, lineno, obj, prop, ids):
    parts = prop.split(".")
    if parts[0][0].isupper():
        # присоединённые свойства (ListItem.*, ActionBar.* …) — только известные
        if parts[0] not in ("ListItem", "ActionBar", "Page", "MultiCover", "InputRoute"):
            err(path, lineno, "неизвестное присоединённое свойство %s" % prop)
        return
    t = obj.prop(parts[0])
    if t is None:
        err(path, lineno, "у %s нет свойства %s" % (obj.short, parts[0]))
        return
    for part in parts[1:]:
        cpp = cpp_type_of(t)
        if not cpp:
            err(path, lineno, "%s: нельзя обратиться к .%s у типа %s" % (prop, part, t))
            return
        t2 = types.prop(cpp, part)
        if t2 is None:
            err(path, lineno, "у %s нет свойства %s" % (cpp, part))
            return
        t = t2


files = sorted(f for f in os.listdir(ASSETS) if f.endswith(".qml"))
for f in files:
    check_file(os.path.join(ASSETS, f))

# все используемые картинки существуют
for f in files:
    for m in re.finditer(r'"asset:///([^"]+)"', open(os.path.join(ASSETS, f), encoding="utf-8").read()):
        if not os.path.exists(os.path.join(ASSETS, m.group(1))):
            errors.append("%s: нет файла assets/%s" % (f, m.group(1)))
    for m in re.finditer(r'icon:\s*"(\w+)"', open(os.path.join(ASSETS, f), encoding="utf-8").read()):
        if not os.path.exists(os.path.join(ASSETS, "images", m.group(1) + ".png")):
            errors.append("%s: нет иконки images/%s.png" % (f, m.group(1)))

print("QML-файлов:", len(files), "· свои компоненты:", ", ".join(sorted(CUSTOM)))
print("API контроллера:", len(APP), "· палитра:", len(THEME))
for e in errors:
    print("ОШИБКА", e)
print("ошибок:", len(errors))
sys.exit(1 if errors else 0)
