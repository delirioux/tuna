#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Composer manners -> firmware/src/tuna_manners.inc (and the style field table, tuna_style_fields.inc).

    python3 tools/tuna_manners.py              writes both
    python3 tools/tuna_manners.py --check      fails if they differ from what this writes
    python3 tools/tuna_manners.py --show NAME  a composer's base profile, in the manners' own syntax
    python3 tools/tuna_manners.py --fields     every field a manner may set

A composer wrote in more than one way (Bach's chorales, fugues and dances). Its profile in tuna_style.c is its
first manner; tools/manners/*.py add the others as changes to it:

    MANNERS = {
        "BACH": [
            dict(name="FUGUE"),                                       # the profile as it is: a name only
            dict(name="CHORALE", bpm=66, division=2, pad_mode="BLOCK", pad_tones=4, bass_mode="LINE", ...,
                 orch=[("CONTRABASS", "CHOIR", "PIPEORGAN")], orch_mode="FIXED"),
        ],
    }

A field takes the value as the profile holds it: numbers, enum names without their prefix (bass_mode="WALKING",
scale="DORIAN", vocab="ROMANTIC", tuning="MEANTONE"), root as a note name, lists for arrays (profile, meter,
pattern, bass_notes, pad_figure, perc, cycle), chord weights as {chord name: weight} (w_major, w_minor: the names
of the manner's vocabulary; unnamed chords 0), bass_rhythm / pad_rhythm as tick strings ('x' sounds), mel_add as
blue-note names ("b3", "b7"). Only what differs from the profile is stored (a few bytes a field)."""
import json
import pathlib
import re
import subprocess
import sys
import tempfile
import importlib.util

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "firmware" / "src"
OUT = SRC / "tuna_manners.inc"
FIELDS_OUT = SRC / "tuna_style_fields.inc"
MANNER_DIR = ROOT / "tools" / "manners"
MAX_MANNERS = 4

_spec = importlib.util.spec_from_file_location("tuna_genres", ROOT / "tools" / "tuna_genres.py")
G = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(G)


def die(msg):
    sys.exit(f"tuna_manners: {msg}")


# ---- the tg_style struct: its fields, read from the header -------------------------------------------------
CTYPES = {"float": "F32", "int": "I32", "uint8_t": "U8", "int8_t": "I8", "bool": "BOOL", "uint16_t": "U16",
          "int16_t": "I16", "uint32_t": "U32"}
SKIP = {"name", "orch", "orch_n", "orch_mode", "drum_pat"}     # (orch: a manner's own sets; drum_pat: not here)


def defines():
    d = {}
    for h in ("tuna_style.h", "tuna_gen.h", "tuna_theory.h"):
        for m in re.finditer(r"#define\s+(\w+)\s+(\d+)", (SRC / h).read_text()):
            d[m.group(1)] = int(m.group(2))
    return d


def struct_fields():
    """[(name, kind, count)] of tg_style, nested 'alt' flattened as alt.x"""
    h = (SRC / "tuna_style.h").read_text()
    end = h.index("} tg_style;")
    start = h.rindex("typedef struct {", 0, end)
    body = re.sub(r"/\*.*?\*/", "", h[start + len("typedef struct {"): end], flags=re.S)
    D = defines()
    out = []
    # nested structs: struct { ... } name; / name[n];
    for m in re.finditer(r"struct\s*\{(.*?)\}\s*(\w+)(\[\w+\])?\s*;", body, re.S):
        if m.group(2) in SKIP:
            continue
        for decl in m.group(1).split(";"):
            decl = decl.strip()
            if not decl:
                continue
            t, names = decl.split(None, 1)
            for n in names.split(","):
                out.append((m.group(2) + "." + n.strip(), CTYPES[t], 1))
    flat = re.sub(r"struct\s*\{.*?\}\s*\w+(\[\w+\])?\s*;", "", body, flags=re.S)
    for decl in flat.split(";"):
        decl = " ".join(decl.split())
        if not decl or decl.startswith("const char"):
            continue
        t, rest = decl.split(None, 1)
        if t not in CTYPES:
            die(f"tg_style: type {t} unknown ({decl})")
        for n in rest.split(","):
            n = n.strip()
            m = re.match(r"(\w+)(?:\[(\w+)\])?(?:\[(\w+)\])?$", n)
            name, c1, c2 = m.groups()
            if name in SKIP:
                continue
            if c2:
                die(f"tg_style: two-dimensional {name}")
            if not c1:
                cnt = 1
            elif c1.isdigit():
                cnt = int(c1)
            elif c1 in D:
                cnt = D[c1]
            elif c1.endswith("_COUNT"):                    # an enum's count (TG_PI_COUNT)
                cnt = enum_names(c1[:-5]).index("COUNT")
            else:
                die(f"tg_style: size {c1} unknown")
            out.append((name, CTYPES[t], cnt))
    return out


def enum_names(prefix):
    """the names of an enum whose members start with prefix, in order (TG_BASS_ -> ['MARBLES', ...])"""
    for h in ("tuna_style.h", "tuna_gen.h", "tuna_theory.h"):
        t = re.sub(r"/\*.*?\*/", "", (SRC / h).read_text(), flags=re.S)
        for m in re.finditer(r"enum\s*\{(.*?)\}", t, re.S):
            items = [x.strip() for x in m.group(1).split(",") if x.strip()]
            if items and items[0].startswith(prefix):
                names, v = [], -1
                for it in items:
                    if "=" in it:
                        die(f"enum {prefix}: explicit values not handled")
                    names.append(it[len(prefix):])
                return names
    die(f"enum {prefix} not found")


ENUM_FIELDS = {"bass_mode": "TG_BASS_", "pad_mode": "TG_PAD_", "mel_mode": "TG_MEL_", "scale": "TS_",
               "color_scale": "TS_", "harmony": "TG_HARM_", "vocab": "TG_VOC_", "perc_mode": "TG_PERC_",
               "tuning": "TT_TUNE_", "mod_kind": "TG_MOD_", "alt.mel_mode": "TG_MEL_", "alt.bass_mode": "TG_BASS_",
               "alt.pad_mode": "TG_PAD_", "alt.harmony": "TG_HARM_", "reverb": "TG_REV_"}
PLUS_ONE = {"alt.mel_mode", "alt.bass_mode", "alt.pad_mode", "alt.harmony", "reverb"}   # 0: unchanged / its own
LENS = {"pattern": "pattern_len", "bass_notes": "bass_len", "pad_figure": "figure_len", "cycle": "cycle_len",
        "cycle2": "cycle2_len"}
GENRE_ONLY = {"groove", "swing16", "bass_semis", "bass_gate", "pad_rhythm", "pad_gate", "power"}
NOTES = {v: k for k, v in G.NOTE.items() if len(k) == 1 or k[1] == "#"}
TG_REST, TG_HOLD = -128, -127


def field_table(fields):
    """the C field table: an id per field, its offset, kind and count"""
    L = ["/* SPDX-License-Identifier: GPL-3.0-only */",
         "/* generated by tools/tuna_manners.py from tg_style (tuna_style.h): every field a manner may change */",
         "enum {"]
    L += [f"    TGF_{n.replace('.', '_')}," for n, k, c in fields]
    L += ["    TGF_COUNT", "};",
          "static const tg_field TG_FIELDS[TGF_COUNT] = {"]
    L += [f"    {{offsetof(tg_style, {n}), TGK_{k}, {c}}}," for n, k, c in fields]
    L += ["};", "#ifdef TG_FIELD_NAMES", "static const char *const TG_FIELD_NAME[TGF_COUNT] = {"]
    L += [f'    "{n}",' for n, k, c in fields]
    L += ["};", "#endif"]
    return "\n".join(L) + "\n"


# ---- the base profiles, as the firmware holds them (tests/tuna_style_dump.c) --------------------------------
def base_profiles():
    exe = pathlib.Path(tempfile.gettempdir()) / "tuna_style_dump"
    srcs = ["tests/tuna_style_dump.c", "firmware/src/tuna_style.c", "firmware/src/tuna_theory.c"]
    r = subprocess.run(["cc", "-O0", "-w", "-Ifirmware/src", "-o", str(exe)] + srcs + ["-lm"], cwd=ROOT,
                       capture_output=True, text=True)
    if r.returncode:
        die("the profile dump does not build:\n" + r.stderr[-2000:])
    out = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    return json.loads(out)


# ---- a manner's settings -> {field: [values]} ----------------------------------------------------------------
class Ctx:
    def __init__(self):
        self.fields = struct_fields()
        self.kind = {n: (k, c) for n, k, c in self.fields}
        self.enums = {f: enum_names(p) for f, p in ENUM_FIELDS.items()}
        self.vn = G.vocab_names()
        self.vocab_list = enum_names("TG_VOC_")
        self.inst = G.INSTRUMENTS
        self.modes = G.ORCH_MODES


def ticks(s, where):
    if any(ch not in "x." for ch in s) or len(s) > 32:
        die(f"{where}: a rhythm is 'x' and '.', at most 32 ticks ({s!r})")
    return sum(1 << i for i, ch in enumerate(s) if ch == "x")


def values(ctx, name, v, base, settings, where):
    """the value list (one per element) a setting gives field `name`"""
    if name not in ctx.kind:
        die(f"{where}: no field {name} (python3 tools/tuna_manners.py --fields)")
    if name in GENRE_ONLY:
        die(f"{where}: {name} is the popular genres' (tools/genres)")
    kind, cnt = ctx.kind[name]
    if name in ENUM_FIELDS:
        names = ctx.enums[name]
        if v not in names:
            die(f"{where}: {name}={v!r} ({' '.join(names)})")
        x = names.index(v) + (1 if name in PLUS_ONE else 0)
        return [x]
    if name == "root":
        if v not in G.NOTE:
            die(f"{where}: root={v!r} (a note name: C, F#, BB ...)")
        return [G.NOTE[v]]
    if name in ("bass_rhythm", "pad_rhythm"):
        return [ticks(v, where)]
    if name == "mel_add":
        return [sum(1 << G.BLUE[n] for n in v)] if all(n in G.BLUE for n in v) else die(f"{where}: mel_add {v}")
    if name in ("w_major", "w_minor"):
        voc = settings.get("vocab", ctx.vocab_list[base["vocab"]])
        arrs = G.VOCABS[voc]
        names = ctx.vn[arrs[0] if name == "w_major" else arrs[1]]
        if not isinstance(v, dict):
            die(f"{where}: {name} is {{chord: weight}}")
        out = [0] * cnt
        for ch, w in v.items():
            if ch not in names:
                die(f"{where}: {name}: no chord {ch!r} in {voc} ({' '.join(names)})")
            out[names.index(ch)] = int(w)
        return out
    if name in ("cycle", "cycle2"):
        voc = settings.get("vocab", ctx.vocab_list[base["vocab"]])
        scale = settings.get("scale", G.SCALES[base["scale"]])
        names = ctx.vn[G.VOCABS[voc][1 if scale in G.MINOR_SCALES else 0]]
        bad = [c for c in v if c not in names]
        if bad:
            die(f"{where}: {name}: chords {bad} not in {voc} ({' '.join(names)})")
        return [names.index(c) for c in v] + [0] * (cnt - len(v))
    if name == "pattern":
        out = [TG_REST if x is None else TG_HOLD if x == "-" else int(x) for x in v]
        return out + [0] * (cnt - len(out))
    if cnt > 1:
        if not isinstance(v, (list, tuple)) or len(v) > cnt:
            die(f"{where}: {name} is a list of up to {cnt}")
        return [float(x) if kind == "F32" else int(x) for x in v] + [0] * (cnt - len(v))
    if kind == "BOOL":
        return [1 if v else 0]
    return [float(v) if kind == "F32" else int(v)]


def encode(kind, x, where, name):
    """a value as the delta stores it (int16): floats in hundredths"""
    if kind == "F32":
        q = round(x * 100)
    else:
        q = int(x)
    lim = {"U8": (0, 255), "I8": (-128, 127), "BOOL": (0, 1), "U16": (0, 65535), "I16": (-32768, 32767),
           "I32": (-32768, 32767), "F32": (-32768, 32767)}.get(kind)
    if lim and not lim[0] <= q <= lim[1]:
        die(f"{where}: {name}={x} out of range")
    return q


def deltas(ctx, sname, manner, base, where):
    """[(field id, index, int16)] for what the manner changes"""
    settings = {k: v for k, v in manner.items() if k not in ("name", "orch", "orch_mode", "why")}
    ids = {n: i for i, (n, k, c) in enumerate(ctx.fields)}
    out = []
    for name, v in settings.items():
        vals = values(ctx, name, v, base, settings, where)
        kind, cnt = ctx.kind[name]
        cur = base[name] if isinstance(base[name], list) else [base[name]]
        if name in LENS:                                   # an array sets its length too
            n = len([x for x in v]) if name != "pattern" else len(v)
            if n != base[LENS[name]]:
                out.append((ids[LENS[name]], 0, n))
        for i, x in enumerate(vals):
            if kind == "F32" and abs(x - cur[i]) < 0.005 or kind != "F32" and x == cur[i]:
                continue
            if kind == "U32":                              # two halves
                if (x & 0xFFFF) != (cur[i] & 0xFFFF):
                    out.append((ids[name], 0, x & 0xFFFF))
                if (x >> 16) != (cur[i] >> 16):
                    out.append((ids[name], 1, x >> 16))
                continue
            out.append((ids[name], i, encode(kind, x, where, name)))
    return out


def load_manners():
    files = sorted(MANNER_DIR.glob("*.py"))
    allm = {}
    for f in files:
        spec = importlib.util.spec_from_file_location(f.stem, f)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for k, v in getattr(mod, "MANNERS", {}).items():
            if k in allm:
                die(f"{k} in two files")
            allm[k] = (v, f.name)
    return allm


def render():
    ctx = Ctx()
    fields_text = field_table(ctx.fields)
    if "--check" not in sys.argv:
        FIELDS_OUT.write_text(fields_text)
        if not OUT.exists():                               # (the first time: the dump builds without manners)
            OUT.write_text("#define TG_MANNERS 0\nstatic const tg_manner TG_MANNER[1];\n"
                           "static const uint16_t TG_MANNER_FIRST[TG_STYLE_COUNT];\n"
                           "static const uint8_t TG_MANNER_N[TG_STYLE_COUNT];\n")
    base = base_profiles()
    by_name = {b["name"]: (i, b) for i, b in enumerate(base)}
    allm = load_manners()
    L = ["/* SPDX-License-Identifier: GPL-3.0-only */",
         "/* generated by tools/tuna_manners.py from tools/manners: the composers' manners, each the changes it",
         " * makes to the composer's profile (tuna_style.c); edit those, not this */"]
    table = []
    first = [0] * len(base)
    count = [0] * len(base)
    nd = 0
    for i, b in enumerate(base):
        if b["name"] not in allm:
            continue
        mans, fname = allm[b["name"]]
        where0 = f"{fname} {b['name']}"
        if not 1 <= len(mans) <= MAX_MANNERS:
            die(f"{where0}: 1..{MAX_MANNERS} manners")
        if set(mans[0]) - {"name", "why"}:
            die(f"{where0}: the first manner is the profile as it is (a name only)")
        first[i], count[i] = len(table), len(mans)
        names = set()
        for k, m in enumerate(mans):
            nm = m.get("name", "")
            where = f"{where0} {nm or k}"
            if not re.fullmatch(r"[A-Z0-9 &'.-]{1,10}", nm):
                die(f"{where}: name: capitals, at most 10 characters")
            if nm in names:
                die(f"{where}: two manners called {nm}")
            names.add(nm)
            d = deltas(ctx, b["name"], m, b, where) if k else []
            orch = m.get("orch")
            if orch:
                if len(orch) > 4 or any(len(s) != 3 for s in orch):
                    die(f"{where}: orch: up to 4 sets of (bass, melody, third)")
                for s in orch:
                    for x in s:
                        if x not in ctx.inst:
                            die(f"{where}: instrument {x}")
            mode = m.get("orch_mode", "RANDOM")
            if mode not in ctx.modes:
                die(f"{where}: orch_mode {mode} ({' '.join(ctx.modes)})")
            cname = f"TGM_{i}_{k}"
            if d:
                L.append(f"static const tg_delta {cname}[] = {{   /* {b['name']} {nm} */")
                items = [f"{{TGF_{ctx.fields[f][0].replace('.', '_')}, {ix}, {v}}}" for f, ix, v in d]
                line = "   "
                for it in items:
                    if len(line) + len(it) + 2 > 116:
                        L.append(line.rstrip())
                        line = "   "
                    line += " " + it + ","
                L.append(line.rstrip().rstrip(","))
                L.append("};")
                nd += len(d)
            table.append((i, nm, cname if d else "0", len(d), orch, mode))
    missing = [b["name"] for b in base[:89] if b["name"] not in allm]
    L.append(f"#define TG_MANNERS {len(table)}")
    L.append("static const tg_manner TG_MANNER[TG_MANNERS > 0 ? TG_MANNERS : 1] = {")
    for i, nm, cn, n, orch, mode in table:
        o = "{" + ", ".join("{" + ", ".join(f"TI_{x}" for x in s) + "}" for s in (orch or [])) + "}" if orch else "{{0}}"
        L.append(f'    {{"{nm}", {cn}, {n}, {o}, {len(orch or [])}, TG_ORCH_{mode}}},   /* {base[i]["name"]} */')
    if not table:
        L.append("    {0},")
    L.append("};")
    def arr(decl, vals):
        L.append(decl + " = {")
        for k in range(0, len(vals), 24):
            L.append("    " + ", ".join(map(str, vals[k:k + 24])) + ",")
        L.append("};")
    arr(f"static const uint16_t TG_MANNER_FIRST[{len(base)}]", first)
    arr(f"static const uint8_t TG_MANNER_N[{len(base)}]", count)
    return fields_text, "\n".join(L) + "\n", len(table), nd, missing


def show(name):
    ctx = Ctx()
    base = {b["name"]: b for b in base_profiles()}
    if name not in base:
        die(f"no composer {name}")
    b = base[name]
    out = []
    for n, kind, cnt in ctx.fields:
        v = b[n]
        if n in ENUM_FIELDS:
            x = v - (1 if n in PLUS_ONE else 0)
            v = "(unchanged)" if n in PLUS_ONE and v == 0 else ctx.enums[n][x] if 0 <= x < len(ctx.enums[n]) else v
        elif n == "root":
            v = NOTES.get(v, v)
        elif n in ("w_major", "w_minor"):
            arr = G.VOCABS[ctx.vocab_list[b["vocab"]]][0 if n == "w_major" else 1]
            v = {ch: w for ch, w in zip(ctx.vn[arr], v) if w}
        out.append(f"{n} = {v!r}")
    print(f"{name} (index {list(base).index(name)}):")
    print("\n".join("  " + x for x in out))


def main():
    if "--fields" in sys.argv:
        for n, k, c in Ctx().fields:
            print(f"{n:<16} {k:<4} {c if c > 1 else ''}")
        return
    if "--show" in sys.argv:
        show(sys.argv[sys.argv.index("--show") + 1])
        return
    fields_text, text, n, nd, missing = render()
    if "--check" in sys.argv:
        if OUT.read_text() != text or FIELDS_OUT.read_text() != fields_text:
            die("tuna_manners.inc or tuna_style_fields.inc is out of date (python3 tools/tuna_manners.py)")
        print(f"tuna_manners: {n} manners, up to date")
        return
    OUT.write_text(text)
    print(f"tuna_manners: {n} manners ({nd} changes) -> {OUT.relative_to(ROOT)}"
          + (f"; composers without manners: {len(missing)}" if missing else ""))


if __name__ == "__main__":
    main()
