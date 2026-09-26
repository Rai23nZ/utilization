#!/usr/bin/env python3
"""Загрузка описаний типов Cascades (plugins.qmltypes) из BlackBerry 10 NDK.

Используется проверкой QML (check_qml.py) и как справочник:
    qmltypes.py Container Label        — свойства, сигналы и методы с учётом наследования
"""
import os
import re
import sys

IMPORTS = os.environ.get("BB_IMPORTS",
                         "/home/user/bbsdk/target_10_3_1_995/qnx6/armle-v7/usr/lib/qt4/imports")
MODULES = {
    "bb.cascades": "bb/cascades/plugins.qmltypes",
    "bb.system": "bb/system.1/plugins.qmltypes",
    "bb.device": "bb/device.1/plugins.qmltypes",
}


def _tokens(text):
    text = re.sub(r"//[^\n]*", "", text)
    for m in re.finditer(r'"(?:[^"\\]|\\.)*"|[A-Za-z_][A-Za-z0-9_.]*|-?\d+|[{}\[\]:;,]', text):
        yield m.group(0)


def _parse(tokens):
    """Возвращает дерево: (Имя, {поле: значение}, [дети])."""
    toks = list(tokens)
    pos = 0

    def value():
        nonlocal pos
        t = toks[pos]
        if t == "{":
            pos += 1
            out = {}
            while toks[pos] != "}":
                if toks[pos] == ",":
                    pos += 1
                    continue
                k = toks[pos][1:-1] if toks[pos].startswith('"') else toks[pos]
                pos += 2
                out[k] = value()
            pos += 1
            return out
        if t == "[":
            pos += 1
            out = []
            while toks[pos] != "]":
                if toks[pos] == ",":
                    pos += 1
                    continue
                out.append(value())
            pos += 1
            return out
        pos += 1
        if t.startswith('"'):
            return t[1:-1]
        return t

    def obj():
        nonlocal pos
        name = toks[pos]
        pos += 1
        assert toks[pos] == "{", (name, toks[pos])
        pos += 1
        fields, children = {}, []
        while toks[pos] != "}":
            if toks[pos] == ";":
                pos += 1
                continue
            if toks[pos + 1] == "{":
                children.append(obj())
            else:
                key = toks[pos]
                assert toks[pos + 1] == ":", (key, toks[pos + 1])
                pos += 2
                fields[key] = value()
        pos += 1
        return name, fields, children

    while toks[pos] != "Module":
        pos += 1
    return obj()


class Types:
    def __init__(self):
        self.components = {}   # C++ имя → dict
        self.exports = {}      # "модуль/Имя" → C++ имя
        for module, rel in MODULES.items():
            path = os.path.join(IMPORTS, rel)
            if not os.path.exists(path):
                continue
            _, _, comps = _parse(_tokens(open(path, encoding="utf-8").read()))
            for name, fields, children in comps:
                c = {
                    "name": fields.get("name"),
                    "prototype": fields.get("prototype"),
                    "attached": fields.get("attachedType"),
                    "props": {},
                    "signals": set(),
                    "methods": set(),
                    "enums": {},
                }
                for kind, f, ch in children:
                    if kind == "Property":
                        c["props"][f["name"]] = f.get("type", "")
                    elif kind == "Signal":
                        c["signals"].add(f["name"])
                    elif kind == "Method":
                        c["methods"].add(f["name"])
                    elif kind == "Enum":
                        vals = set()
                        for vk, vf, _ in ch:
                            pass
                        c["enums"][f["name"]] = f.get("values", {})
                self.components[c["name"]] = c
                for e in fields.get("exports", []):
                    mod, rest = e.split("/", 1)
                    short = rest.split(" ")[0]
                    self.exports[(mod, short)] = c["name"]

    def resolve(self, short, modules):
        for m in modules:
            if (m, short) in self.exports:
                return self.exports[(m, short)]
        return None

    def chain(self, cname):
        seen = []
        while cname and cname in self.components and cname not in seen:
            seen.append(cname)
            cname = self.components[cname]["prototype"]
        return seen

    def prop(self, cname, name):
        for c in self.chain(cname):
            if name in self.components[c]["props"]:
                return self.components[c]["props"][name]
        return None

    def has_signal(self, cname, name):
        return any(name in self.components[c]["signals"] for c in self.chain(cname))

    def has_method(self, cname, name):
        return any(name in self.components[c]["methods"] or name in self.components[c]["signals"]
                   for c in self.chain(cname))

    def all_props(self, cname):
        out = {}
        for c in reversed(self.chain(cname)):
            out.update(self.components[c]["props"])
        return out


if __name__ == "__main__":
    t = Types()
    for short in sys.argv[1:]:
        c = t.resolve(short, ["bb.cascades", "bb.system", "bb.device"]) or short
        print("==", short, "→", c, "chain:", " < ".join(t.chain(c)))
        for k, v in sorted(t.all_props(c).items()):
            print("   ", k, ":", v)
        sig = set()
        meth = set()
        for cc in t.chain(c):
            sig |= t.components[cc]["signals"]
            meth |= t.components[cc]["methods"]
        print("   signals:", " ".join(sorted(sig)))
        print("   methods:", " ".join(sorted(meth)))
