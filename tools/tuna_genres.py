#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""TUNA's popular genres, 1950s to now: readable data in tools/genres/d*.py, checked and written out as C.

    python3 tools/tuna_genres.py          writes firmware/src/tuna_genres.inc (styles, ids, eras, grooves)
    python3 tools/tuna_genres.py --check  fails if that file is not what the data gives (tests/tuna_tests.sh)

A genre is a dict (see GENRE_DEFAULTS and the modules in tools/genres/); its drums are a groove (GROOVE_DEFAULTS),
written as strings of ticks: 'X' accented, 'x' struck, 'g' a ghost note, '.' silent. Chords are named as in the
chord vocabulary the genre uses (firmware/src/tuna_style.c), read from there, so a misspelt chord fails here,
not on the device."""
import importlib.util
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "firmware" / "src"
OUT = SRC / "tuna_genres.inc"
GENRE_DIR = ROOT / "tools" / "genres"

REVERBS = ["ROOM", "SPRING", "BATHROOM", "HALL", "CATHEDRAL"]   # TG_REV_* (firmware/src/tuna_style.h)
DECADES = [1950, 1960, 1970, 1980, 1990, 2000, 2010, 2020]

NOTE = {"C": 0, "C#": 1, "DB": 1, "D": 2, "D#": 3, "EB": 3, "E": 4, "F": 5, "F#": 6, "GB": 6, "G": 7, "G#": 8,
        "AB": 8, "A": 9, "A#": 10, "BB": 10, "B": 11}
# scales (TS_*) and their notes, as firmware/src/tuna_theory.c has them (for major / minor: the minor third)
SCALES = ["IONIAN", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDIAN", "AEOLIAN", "LOCRIAN", "HARM_MINOR", "MEL_MINOR",
          "PENT_MAJOR", "PENT_MINOR", "BLUES", "WHOLE_TONE", "OCT_HW", "OCT_WH", "MESSIAEN3", "ACOUSTIC",
          "HUNG_MINOR", "DOUBLE_HARM", "PHRYG_DOM"]
MINOR_SCALES = {"DORIAN", "PHRYGIAN", "AEOLIAN", "LOCRIAN", "HARM_MINOR", "MEL_MINOR", "PENT_MINOR", "BLUES",
                "OCT_HW", "OCT_WH", "MESSIAEN3", "HUNG_MINOR"}   # (those with a minor third)
VOCABS = {"CLASSIC": ("vocab_major", "vocab_minor"), "ROMANTIC": ("rom_major", "rom_minor"),
          "JAZZ": ("jazz_major", "jazz_minor"), "MODAL": ("modal_both", "modal_both"),
          "MODERN": ("modern_both", "modern_both"), "POP": ("pop_major", "pop_minor"),
          "BLUES": ("blues_major", "blues_minor")}
BASS = ["MARBLES", "ROOT", "WALKING", "BAR", "MARCH", "PEDAL", "DRONE", "OSTINATO", "LINE", "REPEAT", "TOLL", "BEAT",
        "NONE", "808"]
PAD = ["BLOCK", "ALBERTI", "COUNTER", "BROKEN", "AFTERBEAT", "PARALLEL", "DOUBLE", "REPEAT", "ARPEGGIO", "OOMPAH",
       "TREMOLO", "ORGANUM", "TINTI", "CANON", "FIGURE", "ADDITIVE", "PHASE", "NONE", "RHYTHM"]
MEL = ["PHRASE", "CHANT", "TINTI", "PATTERN", "BIRDSONG"]
HARM = ["PLANNED", "CYCLE", "DRONE"]
ORCH_MODES = ["FIXED", "CYCLE", "RANDOM", "ENERGY", "CONTRAST"]
INSTRUMENTS = ["DEFAULT", "PIANO", "HARPSICHORD", "ORGAN", "STRINGS", "VIOLIN", "CELLO", "CONTRABASS", "PIZZ", "FLUTE",
               "OBOE", "CLARINET", "BASSOON", "HORN", "TRUMPET", "BRASS", "TUBA", "HARP", "GUITAR", "CHOIR", "VOICE",
               "BELLS", "CELESTA", "MARIMBA", "BANDONEON", "SAX", "SYNTH", "PAD", "WHISTLE", "DRONE", "EPIANO",
               "EGUITAR", "DISTGUITAR", "EBASS", "SYNTHBASS", "808", "ACID", "SUPERSAW", "FATBASS", "SYNCLEAD",
               "CHIP", "CLAV", "ROCKORGAN", "GOSPEL", "TALKBASS", "GLITCH", "SINEKEY", "SHIMMER", "KALIMBA",
               "LOGDRUM", "PLUCKSYNTH", "SAWLEAD", "WAVEBASS", "STEELDRUM"]
DRUMS = {"KICK": 36, "BDRUM": 35, "SNARE": 38, "SNARE2": 40, "RIM": 37, "CLAP": 39, "HAT": 42, "PHAT": 44, "OHAT": 46,
         "CRASH": 49, "RIDE": 51, "BELL": 53, "TAMB": 54, "COWBELL": 56, "CONGA_HI": 63, "CONGA_LO": 64,
         "BONGO_HI": 60, "BONGO_LO": 61, "TIMBALE": 65, "CLAVE": 75, "SHAKER": 70, "WOODBLOCK": 76, "TOM_LO": 45,
         "TOM_MID": 47, "TOM_HI": 50, "FLOOR_TOM": 41, "AGOGO": 67, "TRIANGLE": 81, "SPLASH": 55, "CHINA": 52}
KITS = {"STD": 0, "80": 4, "10": 5, "66": 6, "55": 7, "77": 8}
BLUE = {"b2": 1, "2": 2, "b3": 3, "3": 4, "4": 5, "b5": 6, "#4": 6, "5": 7, "b6": 8, "6": 9, "b7": 10, "7": 11}

GROOVE_DEFAULTS = dict(kit="STD", kick="PUNCH", tune=64, tone=64, decay=64, snap=64, drive=0, roll=0, fill=1, len=0)
MELODY_DEFAULTS = dict(mode="PHRASE", density=0.5, base=62, range=14, profile=(0.2, 0.65, 0.85, 0.95), max_leap=4,
                       sync=0.2, staccato=0.0, min_ticks=0, add=(), pattern=(), pattern_chord=False, direction=0.0,
                       octave=0.0, turn=0.0, motif=0.6, motif_beats=2, motif_use=0.7, var_invert=0.1,
                       var_alter=0.3, mask=None, appoggiatura=0.0, suspension=0.0, trill=False, chromatic=False)
BASS_DEFAULTS = dict(mode="ROOT", rhythm=None, notes=(), semis=False, gate=0, base=None)
PAD_DEFAULTS = dict(mode="BLOCK", rhythm=None, gate=0, base=55, tones=3, power=False, spread=False, strum_ms=0,
                    figure=(), interval=0)
GENRE_DEFAULTS = dict(comment="", key="C", scale="IONIAN", bpm=120, div=4, beats=4, chord_beats=4, period_bars=8,
                      swing=0.0, swing16=False, vocab="POP", harmony="PLANNED", cycle=(), cycle2=(), cycle_switch=0,
                      weights=None, weights_minor=None, arrival_bars=1, fifth_bias=1.0, repeat=0.5, open=0.0,
                      swell=0.0, sforzando=0.0, rubato=0.0, mixture=0.0, modulate=0.0, sideslip=0.0,
                      orch_mode="RANDOM", orch=(), echo=0.0, gp=0.0, evolve_periods=0, evolve_depth=0.0,
                      evolve_modulate=False, mod_kind="RELATED", groove=None, perc="GROOVE",
                      reverb=None)


def die(msg):
    sys.exit(f"tuna_genres: {msg}")


def vocab_names():
    """the chord names of every vocabulary, by index, read from tuna_style.c"""
    text = (SRC / "tuna_style.c").read_text()
    out = {}
    for arr in {a for pair in VOCABS.values() for a in pair}:
        m = re.search(r"static const tg_chord_def " + arr + r"\[TG_VOCAB\] = \{(.*?)\n\};", text, re.S)
        if not m:
            die(f"vocabulary {arr} not found in tuna_style.c")
        names = re.findall(r'"([^"]+)"\}', m.group(1))
        if len(names) != 24:
            die(f"vocabulary {arr}: {len(names)} chords, not 24")
        out[arr] = names
    return out


def ticks(s, n, what):
    """a tick string as masks (hits, accent, ghost)"""
    s = s.replace(" ", "").replace("|", "")
    if len(s) != n:
        die(f"{what}: {len(s)} ticks, the loop has {n}")
    hits = accent = ghost = 0
    for i, c in enumerate(s):
        if c in "xXg":
            hits |= 1 << i
        if c == "X":
            accent |= 1 << i
        if c == "g":
            ghost |= 1 << i
        if c not in "xXg.":
            die(f"{what}: '{c}' (x X g . only)")
    return hits, accent, ghost


def load_genres(only=None):
    genres, grooves = [], {}
    for path in sorted(GENRE_DIR.glob("d*.py")):
        if only and path.stem not in only:
            continue
        spec = importlib.util.spec_from_file_location(path.stem, path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for k, g in getattr(mod, "GROOVES", {}).items():
            if k in grooves:
                die(f"groove {k} twice ({path.name})")
            grooves[k] = g
        for g in mod.GENRES:
            g = dict(g)
            g["_file"] = path.name
            genres.append(g)
    return genres, grooves


def f(x):
    s = f"{float(x):.4g}"
    return (s if "." in s or "e" in s else s + ".0") + "f"


def c_ident(s):
    return re.sub(r"[^A-Z0-9]", "_", s.upper())


def emit(genres, grooves):
    vn = vocab_names()
    seen, lines_ids, lines_styles = set(), [], []
    groove_index = {}
    groove_lines = []
    for gname in sorted(grooves):
        gr = dict(GROOVE_DEFAULTS, **grooves[gname])
        groove_index[gname] = len(groove_index)
        lanes = gr.get("lanes", [])
        if not 1 <= len(lanes) <= 8:
            die(f"groove {gname}: {len(lanes)} lanes (1..8)")
        n = gr["len"] or len(lanes[0][2].replace(" ", "").replace("|", ""))
        if n > 32:
            die(f"groove {gname}: {n} ticks (32 at most)")
        if gr["kit"] not in KITS:
            die(f"groove {gname}: kit {gr['kit']} ({' '.join(KITS)})")
        ls = []
        for drum, vel, pat in lanes:
            if drum not in DRUMS:
                die(f"groove {gname}: drum {drum} ({' '.join(DRUMS)})")
            h, a, gh = ticks(pat, n, f"groove {gname} {drum}")
            ls.append(f"{{{DRUMS[drum]}, {int(vel)}, 0x{h:08X}u, 0x{a:08X}u, 0x{gh:08X}u}}")
        groove_lines.append(
            f'    {{"{gname}", {n}, {KITS[gr["kit"]]}, {1 if gr["kick"] == "ROUND" else 0}, {gr["tune"]}, {gr["tone"]}, '
            f'{gr["decay"]}, {gr["snap"]}, {gr["drive"]}, {gr["roll"]}, {gr["fill"]},\n'
            f'     {{{", ".join(ls)}}}}},')
    used_grooves = set()
    by_decade = {d: [] for d in DECADES}
    for g0 in genres:
        g = dict(GENRE_DEFAULTS, **g0)
        gid = g.get("id") or die(f"a genre without id in {g0['_file']}")
        where = f"{gid} ({g['_file']})"
        if gid in seen:
            die(f"{where}: id twice")
        seen.add(gid)
        if not re.fullmatch(r"[A-Z0-9_]+", gid):
            die(f"{where}: id A-Z 0-9 _ only")
        name = g.get("name") or die(f"{where}: no name")
        if len(name) > 14 or name != name.upper():
            die(f"{where}: name '{name}' (capitals, 14 characters at most)")
        dec = g.get("decade")
        if dec not in DECADES:
            die(f"{where}: decade {dec}")
        year = int(g.get("year") or die(f"{where}: no year"))
        by_decade[dec].append((year, gid))
        key = g["key"].upper()
        if key not in NOTE:
            die(f"{where}: key {g['key']}")
        if g["scale"] not in SCALES:
            die(f"{where}: scale {g['scale']}")
        minor = g["scale"] in MINOR_SCALES
        if g["vocab"] not in VOCABS:
            die(f"{where}: vocab {g['vocab']}")
        names = vn[VOCABS[g["vocab"]][1 if minor else 0]]

        def chord(c):
            if c not in names:
                die(f"{where}: chord {c} not in the {g['vocab']} vocabulary ({'minor' if minor else 'major'}): "
                    f"{' '.join(names)}")
            return names.index(c)
        mel = dict(MELODY_DEFAULTS, **g.get("melody", {}))
        bass = dict(BASS_DEFAULTS, **g.get("bass", {}))
        pad = dict(PAD_DEFAULTS, **g.get("pad", {}))
        if mel["mode"] not in MEL or bass["mode"] not in BASS or pad["mode"] not in PAD:
            die(f"{where}: a mode: melody {mel['mode']}, bass {bass['mode']}, pad {pad['mode']}")
        if g["harmony"] not in HARM:
            die(f"{where}: harmony {g['harmony']}")
        bar = g["beats"] * g["div"]
        loop = 2 * bar
        gr = None
        if g["groove"]:
            if g["groove"] not in grooves:
                die(f"{where}: groove {g['groove']} not defined")
            used_grooves.add(g["groove"])
            gr = dict(GROOVE_DEFAULTS, **grooves[g["groove"]])
            loop = gr["len"] or len(gr["lanes"][0][2].replace(" ", "").replace("|", ""))
            if loop % bar and bar % loop:
                die(f"{where}: groove {g['groove']} of {loop} ticks on bars of {bar}")
        d = []
        d.append(f'.name = "{name}", .root = {NOTE[key]}, .scale = TS_{g["scale"]},')
        d.append(f'.bpm = {g["bpm"]}, .division = {g["div"]}, .beats_per_bar = {g["beats"]}, '
                 f'.chord_beats = {g["chord_beats"]}, .period_bars = {g["period_bars"]},')
        d.append(f'.melody_density = {f(mel["density"])}, .melody_base = {mel["base"]}, .melody_range = {mel["range"]},')
        d.append(f'.profile = {{{", ".join(f(x) for x in mel["profile"])}}}, .max_leap = {mel["max_leap"]},')
        d.append(f'.syncopation = {f(mel["sync"])}, .appoggiatura = {f(mel["appoggiatura"])}, '
                 f'.suspension = {f(mel["suspension"])}, .chromatic = {str(mel["chromatic"]).lower()}, '
                 f'.trill = {str(mel["trill"]).lower()}, .arrival_bars = {g["arrival_bars"]},')
        d.append(f'.bass_mode = TG_BASS_{bass["mode"]}, .pad_mode = TG_PAD_{pad["mode"]}, .pad_base = {pad["base"]}, '
                 f'.pad_tones = {pad["tones"]},')
        d.append(f'.fifth_bias = {f(g["fifth_bias"])}, .repeat = {f(g["repeat"])}, .swell = {f(g["swell"])}, '
                 f'.sforzando = {f(g["sforzando"])}, .rubato = {f(g["rubato"])},')
        d.append(f'.motif = {f(mel["motif"])}, .motif_beats = {mel["motif_beats"]}, .motif_use = {f(mel["motif_use"])}, '
                 f'.var_invert = {f(mel["var_invert"])}, .var_alter = {f(mel["var_alter"])},')
        if g["swing"]:
            d.append(f'.swing = {f(g["swing"])}, .swing16 = {str(g["swing16"]).lower()},')
        perc = g["perc"] if gr else ("OFF" if g["perc"] == "GROOVE" else g["perc"])
        d.append(f'.perc_mode = TG_PERC_{perc},')
        if g["reverb"]:                            # the genre's own reverb (else ROOM, as its era)
            if g["reverb"] not in REVERBS:
                die(f"{where}: reverb {g['reverb']} ({' '.join(REVERBS)})")
            d.append(f'.reverb = TG_REV_{g["reverb"]} + 1,')
        # chord weights for planned harmony: the named chords, the rest 0 (the tonic at least)
        for wkey, arrname in (("weights", VOCABS[g["vocab"]][0]), ("weights_minor", VOCABS[g["vocab"]][1])):
            w = [0] * 24
            src = g[wkey] if g[wkey] is not None else (g["weights"] if wkey == "weights_minor" and not minor and
                                                       g["weights_minor"] is None else None)
            want_minor = wkey == "weights_minor"
            arr_names = vn[arrname]
            if src:
                for c, v in src.items():
                    if c not in arr_names:
                        if want_minor == minor:
                            die(f"{where}: weight for {c}, not in {arrname}: {' '.join(arr_names)}")
                        continue
                    w[arr_names.index(c)] = int(v)
            if not any(w):
                w[0] = 10
                w[2 if len(arr_names) > 2 else 0] = max(w[2], 4)
            d.append(f'.{"w_major" if wkey == "weights" else "w_minor"} = {{{", ".join(str(x) for x in w)}}},')
        d.append(f'.vocab = TG_VOC_{g["vocab"]}, .harmony = TG_HARM_{g["harmony"]},')
        if g["cycle"]:
            cy = [chord(c) for c in g["cycle"]]
            if len(cy) > 16:
                die(f"{where}: cycle of {len(cy)} (16 at most)")
            d.append(f'.cycle = {{{", ".join(map(str, cy))}}}, .cycle_len = {len(cy)},')
        elif g["harmony"] != "PLANNED":
            die(f"{where}: harmony {g['harmony']} needs a cycle")
        if g["cycle2"]:
            cy = [chord(c) for c in g["cycle2"]]
            if len(cy) > 8:
                die(f"{where}: cycle2 of {len(cy)} (8 at most)")
            d.append(f'.cycle2 = {{{", ".join(map(str, cy))}}}, .cycle2_len = {len(cy)}, .cycle_switch = {g["cycle_switch"]},')
        if g["open"]:
            d.append(f'.open = {f(g["open"])},')
        for k in ("mixture", "modulate", "sideslip", "echo", "gp"):
            if g[k]:
                d.append(f".{k} = {f(g[k])},")
        if g["evolve_periods"]:
            d.append(f'.evolve_periods = {g["evolve_periods"]}, .evolve_depth = {f(g["evolve_depth"])}, '
                     f'.evolve_modulate = {str(g["evolve_modulate"]).lower()}, .mod_kind = TG_MOD_{g["mod_kind"]},')
        d.append(f'.mel_mode = TG_MEL_{mel["mode"]},')
        if mel["pattern"]:
            if len(mel["pattern"]) > 16:
                die(f"{where}: melody pattern of {len(mel['pattern'])} (16 at most)")
            pv = ["TG_REST" if p is None else "TG_HOLD" if p == "-" else str(int(p)) for p in mel["pattern"]]
            d.append(f'.pattern = {{{", ".join(pv)}}}, .pattern_len = {len(pv)}, '
                     f'.pattern_chord = {str(mel["pattern_chord"]).lower()},')
        elif mel["mode"] == "PATTERN":
            die(f"{where}: melody PATTERN without a pattern")
        # (a melody of sixteenths is rarely a genre's: eighths at the least unless the genre says)
        min_ticks = mel["min_ticks"] if mel["min_ticks"] else (g["div"] // 2 if g["div"] >= 4 else 0)
        if min_ticks:
            d.append(f'.mel_min_ticks = {min_ticks},')
        for k, fld in (("direction", "direction"), ("staccato", "staccato"), ("turn", "turn"), ("octave", "octave")):
            if mel[k]:
                d.append(f".{fld} = {f(mel[k])},")
        if mel["add"]:
            m = 0
            for a in mel["add"]:
                if a not in BLUE:
                    die(f"{where}: melody add {a} ({' '.join(BLUE)})")
                m |= 1 << BLUE[a]
            d.append(f".mel_add = 0x{m:03X},")
        if mel["mask"]:
            m = 0
            for a in mel["mask"]:
                m |= 1 << (0 if a == "1" else BLUE[a])
            d.append(f".melody_mask = 0x{m:03X},")
        # bass
        if bass["rhythm"]:
            h, _, _ = ticks(bass["rhythm"], bar, f"{where} bass rhythm")
            d.append(f".bass_rhythm = 0x{h:04X}u,")
        if bass["notes"]:
            if len(bass["notes"]) > 8:
                die(f"{where}: bass notes {len(bass['notes'])} (8 at most)")
            d.append(f'.bass_notes = {{{", ".join(str(int(x)) for x in bass["notes"])}}}, .bass_len = {len(bass["notes"])},')
        if bass["mode"] == "OSTINATO" and not bass["notes"]:
            die(f"{where}: OSTINATO bass without notes")
        if bass["semis"]:
            d.append(".bass_semis = true,")
        if bass["gate"]:
            d.append(f'.bass_gate = {bass["gate"]},')
        if bass["mode"] == "808" and not gr and not bass["rhythm"]:
            die(f"{where}: an 808 bass needs a groove (its kick) or a rhythm")
        # pad
        if pad["mode"] == "RHYTHM":
            if not pad["rhythm"]:
                die(f"{where}: pad RHYTHM without a rhythm")
            h, _, _ = ticks(pad["rhythm"], loop, f"{where} pad rhythm")
            d.append(f'.pad_rhythm = 0x{h:08X}u, .pad_gate = {pad["gate"]},')
        if pad["power"]:
            d.append(".power = true,")
        if pad["spread"]:
            d.append(".spread = true,")
        if pad["strum_ms"]:
            d.append(f'.strum_ms = {pad["strum_ms"]},')
        if pad["figure"]:
            d.append(f'.pad_figure = {{{", ".join(str(int(x)) for x in pad["figure"])}}}, .figure_len = {len(pad["figure"])},')
        if pad["interval"]:
            d.append(f'.pad_interval = {pad["interval"]},')
        # instruments
        orch = g["orch"] or die(f"{where}: no instruments (orch)")
        if len(orch) > 4:
            die(f"{where}: {len(orch)} instrument sets (4 at most)")
        sets = []
        for s in orch:
            if len(s) != 3:
                die(f"{where}: an instrument set is (bass, melody, third part)")
            for i in s:
                if i not in INSTRUMENTS:
                    die(f"{where}: instrument {i} ({' '.join(INSTRUMENTS)})")
            sets.append("{" + ", ".join(f"TI_{i}" for i in s) + "}")
        d.append(f'.orch = {{{", ".join(sets)}}}, .orch_n = {len(sets)}, .orch_mode = TG_ORCH_{g["orch_mode"]},')
        if gr:
            d.append(f'.groove = {groove_index[g["groove"]] + 1},')
        cid = "TG_STYLE_G_" + gid
        lines_ids.append(f"    {cid},")
        body = "\n".join("        " + x for x in d)
        comment = g["comment"].replace("*/", "* /")
        lines_styles.append(f"    [{cid}] = {{\n        /* {comment} */\n{body}\n    }},")
    unused = set(grooves) - used_grooves
    if unused:
        die(f"grooves never used: {' '.join(sorted(unused))}")
    eras = []
    for dec in DECADES:
        items = sorted(by_decade[dec])
        body = ", ".join(f"{{TG_STYLE_G_{gid}, {year}}}" for year, gid in items) or "{0, 0}"
        eras.append(f"static const tp_composer era_{dec}[] = {{{body}}};")
        eras.append(f"#define ERA_{dec}_N {len(items)}")
    out = ["/* SPDX-License-Identifier: GPL-3.0-only */",
           "/* TUNA's popular genres, written by tools/tuna_genres.py from the files in tools/genres: edit those, not this. */",
           "#if defined(TUNA_GENRE_IDS)"] + lines_ids + \
          ["#elif defined(TUNA_GENRE_STYLES)"] + lines_styles + \
          ["#elif defined(TUNA_GENRE_GROOVES)",
           f"static const tg_groove grooves[{len(groove_lines)}] = {{"] + groove_lines + ["};"] + \
          ["#elif defined(TUNA_GENRE_ERAS)"] + eras + ["#endif", ""]
    return "\n".join(out), len(genres), len(grooves)


def main():
    # --only d1960[,d1970]: just those files (the others' decades stay empty); --out PATH: write there instead
    only = out = None
    for a in sys.argv[1:]:
        if a.startswith("--only="):
            only = set(a.split("=", 1)[1].split(","))
        if a.startswith("--out="):
            out = pathlib.Path(a.split("=", 1)[1])
    genres, grooves = load_genres(only)
    text, ng, nr = emit(genres, grooves)
    if out:
        out.write_text(text)
        print(f"tuna_genres: {ng} genres, {nr} grooves -> {out}")
        return
    if "--check" in sys.argv:
        if not OUT.exists() or OUT.read_text() != text:
            die(f"{OUT.relative_to(ROOT)} is not what tools/genres gives: run python3 tools/tuna_genres.py")
        print(f"tuna_genres: {ng} genres, {nr} grooves, up to date")
        return
    OUT.write_text(text)
    print(f"tuna_genres: {ng} genres, {nr} grooves -> {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
