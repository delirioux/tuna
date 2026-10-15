#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Per-composer statistics from a corpus of public-domain MIDI (PDMX), to tune the composers from their music.

    python3 tools/tuna_corpus.py index  PDMX_DIR            the scores per composer the filters keep
    python3 tools/tuna_corpus.py stats  PDMX_DIR [-o FILE]  measure them -> tools/corpus/composers.json
    python3 tools/tuna_corpus.py report [FILE]              the measurements as a table
    python3 tools/tuna_corpus.py tune [--dry]               move the composers toward them: tuna_style.c, tools/manners

PDMX (Long et al. 2024, https://zenodo.org/records/14648209) is not part of Tuna: unpack it somewhere and pass
the directory holding PDMX.csv and mid/. Only the numbers derived from it are kept here. The standard library
is all this needs. A score counts when PDMX has it in its deduplicated subset with no licence conflict, and,
where it was rated at all, with a rating of at least MIN_RATING."""
import csv
import re
import sys
import unicodedata

MIN_RATING = 4.0

# Who is who in PDMX's free-text composer field (lower case, accents dropped). (wanted, not wanted)
COMPOSERS = {
    "BACH": (r"\b(j\.? ?s\.? ?bach|johann sebastian bach|bach,? j(ohann|\.)? ?s)", r""),
    "MOZART": (r"\bmozart\b", r"leopold|franz xaver"),
    "BEETHOVEN": (r"\bbeethoven\b", r""),
    "BRAHMS": (r"\bbrahms\b", r""),
    "MAHLER": (r"\bmahler\b", r"\balma\b"),
    "CHOPIN": (r"\bchopin\b", r""),
    "SATIE": (r"\bsatie\b", r""),
    "DEBUSSY": (r"\bdebussy\b", r""),
    "DVORAK": (r"\bdvorak\b", r""),
    "SAINT-SAENS": (r"\bsaint[- ]saens\b", r""),
    "HILDEGARD": (r"\bhildegard\b", r""),
    "TALLIS": (r"\btallis\b", r""),
    "PALESTRINA": (r"\bpalestrina\b", r""),
    "ALLEGRI": (r"\ballegri\b", r"\blorenzo\b|\bdomenico\b"),
    "MONTEVERDI": (r"\bmonteverdi\b", r""),
    "SWEELINCK": (r"\bsweelinck\b", r""),
    "BUXTEHUDE": (r"\bbuxtehude\b", r""),
    "PACHELBEL": (r"\bpachelbel\b", r""),
    "PURCELL": (r"\bpurcell\b", r"\bdaniel\b"),
    "VIVALDI": (r"\bvivaldi\b", r""),
    "ALBINONI": (r"\balbinoni\b", r""),
    "HANDEL": (r"\bh(a|ae)ndel\b", r""),
    "SCARLATTI": (r"\b(d\.?|domenico) scarlatti\b|\bscarlatti,? d", r""),
    "RAMEAU": (r"\brameau\b", r""),
    "PERGOLESI": (r"\bpergolesi\b", r""),
    "GLUCK": (r"\bgluck\b", r""),
    "HAYDN": (r"\bhaydn\b", r"\bmichael\b|\bj\.? ?m\.? haydn"),
    "SCHUBERT": (r"\bschubert\b", r""),
    "ROSSINI": (r"\brossini\b", r""),
    "BERLIOZ": (r"\bberlioz\b", r""),
    "FELIX MENDELSSOHN": (r"\bmendelssohn\b", r"\bfanny\b|\bhensel\b"),
    "FANNY MENDELSSOHN": (r"\bfanny (mendelssohn|hensel)\b|\bhensel\b", r""),
    "ROBERT SCHUMANN": (r"\bschumann\b", r"\bclara\b|\bgeorg\b|\bcamillo\b"),
    "CLARA SCHUMANN": (r"\bclara (wieck[- ])?schumann\b|\bclara wieck\b", r""),
    "LISZT": (r"\bliszt\b", r""),
    "VERDI": (r"\bverdi\b", r""),
    "WAGNER": (r"\b(r\.?|richard) wagner\b|\bwagner,? r", r""),
    "GOUNOD": (r"\bgounod\b", r""),
    "JOHANN STRAUSS": (r"\bjoh(ann|\.)? strauss\b|\bj\.? strauss\b|\bstrauss,? j", r"\bjosef\b"),
    "BRUCKNER": (r"\bbruckner\b", r""),
    "BIZET": (r"\bbizet\b", r""),
    "FRANCK": (r"\b(c\.?|cesar) franck\b|\bfranck,? c", r""),
    "BRUCH": (r"\bbruch\b", r""),
    "MUSSORGSKY": (r"\bm[uo]+ss?[uo]rgsk[yi]", r""),
    "TCHAIKOVSKY": (r"\b(tch|ch|tsch|c)aikovsk[yi]|\btschaikowsk[yi]", r""),
    "SMETANA": (r"\bsmetana\b", r""),
    "GRIEG": (r"\bgrieg\b", r""),
    "MASSENET": (r"\bmassenet\b", r""),
    "RIMSKY-KORSAKOV": (r"\brimsk[yi][- ]korsakov", r""),
    "WIDOR": (r"\bwidor\b", r""),
    "FAURE": (r"\bfaure\b", r"\bjean[- ]baptiste\b"),
    "ELGAR": (r"\belgar\b", r""),
    "PUCCINI": (r"\bpuccini\b", r""),
    "RICHARD STRAUSS": (r"\b(r\.?|richard) strauss\b|\bstrauss,? r", r""),
    "SIBELIUS": (r"\bsibelius\b", r""),
    "RACHMANINOFF": (r"\brachmanino(ff|v)\b|\brakhmaninov\b", r""),
    "HOLST": (r"\bholst\b", r""),
    "VAUGHAN WILLIAMS": (r"\bvaughan[- ]williams\b", r""),
    "RAVEL": (r"\bravel\b", r""),
    "BARTOK": (r"\bbartok\b", r""),
    "STRAVINSKY": (r"\bstravinsk[yi]\b", r""),
    "PROKOFIEV": (r"\bprokofie(v|ff)\b", r""),
    "POULENC": (r"\bpoulenc\b", r""),
    "GERSHWIN": (r"\bgershwin\b", r""),
    "ORFF": (r"\borff\b", r""),
    "RODRIGO": (r"\bjoaquin rodrigo\b|\bj\. rodrigo\b", r""),
    "COPLAND": (r"\bcopland\b", r""),
    "KHACHATURIAN": (r"\bk?hacha?turi?[ay]n\b", r""),
    "KORNGOLD": (r"\bkorngold\b", r""),
    "DURUFLE": (r"\bdurufle\b", r""),
    "SHOSTAKOVICH": (r"\bs(c)?hostakovi(t)?ch\b", r""),
    "MESSIAEN": (r"\bmessiaen\b", r""),
    "BRITTEN": (r"\bbritten\b", r""),
    "BARBER": (r"\bsamuel barber\b|\bs\. barber\b", r""),
    "PIAZZOLLA": (r"\bpiazzolla\b", r""),
    "TEN HOLT": (r"\bten holt\b", r""),
    "ARPEGGIO": (r"\b(philip|phillip|p\.) glass\b", r""),
    "PHASING": (r"\bsteve reich\b", r""),
    "TINTINNABULI": (r"\barvo part\b", r""),
    "GORECKI": (r"\bgorecki\b", r""),
    "TAVENER": (r"\btavener\b", r""),
    "BALTIC": (r"\bvasks\b", r""),
    "SHIMMER": (r"\bwhitacre\b", r""),
    "ANTHEM": (r"\brutter\b", r""),
    "MORRICONE": (r"\bmorricone\b", r""),
    "FANFARE": (r"\bjohn (t\.? )?williams\b", r"\bthomas\b|18\d\d"),
    "SAGA": (r"\bhoward shore\b", r""),
    "MELANCHOLY": (r"\bpreisner\b", r""),
    "SHOWTUNE": (r"\blloyd[- ]webber\b", r"\bjulian\b|\bwilliam\b"),
}


def plain(s):
    s = unicodedata.normalize("NFKD", s or "")
    return "".join(c for c in s if not unicodedata.combining(c)).lower()


def who(name):
    """the Tuna composers a composer field names (several: an arrangement, a medley; then it counts for none)"""
    p = plain(name)
    return [k for k, (yes, no) in COMPOSERS.items() if re.search(yes, p) and not (no and re.search(no, p))]


def keep(row):
    if row["subset:deduplicated"] != "True" or row["subset:no_license_conflict"] != "True":
        return False
    if row["is_rated"] == "True" and float(row["rating"] or 0) < MIN_RATING:
        return False
    return True


def index(pdmx):
    """{composer: [(mid path, row)]} of the scores the filters keep"""
    csv.field_size_limit(1 << 30)
    out = {k: [] for k in COMPOSERS}
    with open(f"{pdmx}/PDMX.csv", newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            name = row["composer_name"]
            if not name or name == "NA":
                continue
            w = who(name)
            if len(w) != 1 or not keep(row):
                continue
            out[w[0]].append((row["mid"], row))
    return out


# ---- a manner's scores, by title: the words a title uses for it ----------------------------------------------
TITLES = {
    "FUGUE": r"\bfug(ue|a|e|hetta)\b", "CHORALE": r"\bchoral", "CHORAL": r"\bchoral", "GIGUE": r"\bgigue\b|\bgiga\b",
    "JIG": r"\bjig\b|\bgigue\b", "PRELUDE": r"\bprelud|\bpraelud", "CIACONA": r"chacon|ciacon|passacagl",
    "PASSACAG": r"passacagl|chacon", "CANTATA": r"\bcantat|\bkantate", "CANON": r"\b[ck]anon\b", "LAMENT": r"\blament",
    "LAMENTO": r"\blament", "ODE": r"\bode\b", "HORNPIPE": r"hornpipe", "CONCERTO": r"\bconcert", "LARGO": r"\blargo\b",
    "GLORIA": r"\bgloria\b", "ADAGIO": r"\badagio\b", "SONATA": r"\bsonat", "SUITE": r"\bsuite\b",
    "ARIA": r"\barias?\b", "MINUET": r"\bmenuet|\bminuet", "SYMPHONY": r"\bsymphon|\bsinfon", "HYMN": r"\bhymn",
    "MASS": r"\bmass\b|\bmissa\b|\bmesse\b|\bkyrie\b|\bsanctus\b|\bagnus dei\b|\bcredo\b", "SERENADE": r"\bserenad|nachtmusik",
    "SCHERZO": r"\bscherzo", "BAGATELL": r"\bbagatell|fur elise", "LIED": r"\blied(er)?\b",
    "MARCH": r"\bmarch(e)?\b|\bmarsch\b|\bmarcia\b", "LANDLER": r"\bla(e)?ndler", "OVERTURE": r"\bo(u)?verture",
    "WALTZ": r"\bwaltz|\bvalse|\bwalzer|\bvals\b", "REQUIEM": r"\brequiem", "NOCTURNE": r"\bnocturn|\bnotturno",
    "MAZURKA": r"\bmazur", "POLONAIS": r"\bpolonaise", "ETUDE": r"\betude|\bstudy\b", "RHAPSODY": r"\brhapsod",
    "PAVANE": r"\bpavan", "SICILIEN": r"\bsicilien", "TOCCATA": r"\btoccat", "ARABESQ": r"\barabesque",
    "CAKEWALK": r"cake ?walk", "GYMNOPED": r"gymnopedi", "GNOSSIEN": r"gnossien",
    "FANTASIA": r"\bfantasi|\bfantasy|\bfantaisie", "FANTASIE": r"\bfantasi|\bfantasy|\bfantaisie",
    "ROMANCE": r"\bromance|\bromanze", "MOTET": r"\bmotet", "MADRIGAL": r"\bmadrigal", "ANTHEM": r"\banthem",
    "PSALM": r"\bpsalm", "VESPERS": r"\bvesper|\bvespro", "POLKA": r"\bpolka", "TANGO": r"\btango", "MILONGA": r"\bmilonga",
    "CAROL": r"\bcarol", "DANCE": r"\bdance|\bdanse|\btanz", "CSARDAS": r"csardas|hungarian dance|ungarische",
    "HABANERA": r"habanera", "FARANDOL": r"farandole", "PROMENAD": r"promenade", "VOCALISE": r"vocalise",
    "BOLERO": r"\bbolero", "CHORUS": r"\bchorus|\bcoro\b|\bchoeur", "FOLKSONG": r"folk ?song", "VARIATIO": r"\bvariation",
    "STABAT": r"\bstabat", "QUARTET": r"\bquartet", "DUMKA": r"\bdumk", "FURIANT": r"\bfuriant", "HALLING": r"\bhalling",
    "ELEGY": r"\beleg", "ELEGIE": r"\beleg", "MELODIE": r"\bmelodie", "BALLET": r"\bballet", "MISERERE": r"\bmiserere",
    "PRAYER": r"\bave maria\b|\bpriere\b|\bprayer\b", "SONG": r"\bsongs?\b",
}


def titled(row, manner):
    pat = TITLES.get(manner)
    return bool(pat and re.search(pat, plain(" ".join(row[k] for k in ("title", "song_name", "subtitle")))))


# ---- reading a MIDI file --------------------------------------------------------------------------------------
def read_midi(data):
    """(ppq, {voice: [(on, off, pitch)]}, [(tick, numerator, denominator)], [(tick, microseconds a quarter)])
    A voice is a track (a channel in a single-track file); the drum channel is left out."""
    if data[:4] != b"MThd":
        raise ValueError("not a MIDI file")
    fmt, ntrk, ppq = int.from_bytes(data[8:10], "big"), int.from_bytes(data[10:12], "big"), int.from_bytes(data[12:14], "big")
    if ppq & 0x8000 or not ppq:
        raise ValueError("SMPTE time")
    pos = 8 + int.from_bytes(data[4:8], "big")
    voices, meters, tempi = {}, [], []
    for trk in range(ntrk):
        if data[pos:pos + 4] != b"MTrk":
            break
        end = pos + 8 + int.from_bytes(data[pos + 4:pos + 8], "big")
        i, t, status, held = pos + 8, 0, 0, {}
        while i < end:
            d = 0
            while True:
                b = data[i]
                i += 1
                d = (d << 7) | (b & 0x7F)
                if not b & 0x80:
                    break
            t += d
            b = data[i]
            if b == 0xFF:
                kind = data[i + 1]
                i += 2
                n = 0
                while True:
                    c = data[i]
                    i += 1
                    n = (n << 7) | (c & 0x7F)
                    if not c & 0x80:
                        break
                if kind == 0x58 and n >= 2:
                    meters.append((t, data[i], 1 << data[i + 1]))
                elif kind == 0x51 and n == 3:
                    tempi.append((t, int.from_bytes(data[i:i + 3], "big")))
                i += n
                continue
            if b in (0xF0, 0xF7):
                i += 1
                n = 0
                while True:
                    c = data[i]
                    i += 1
                    n = (n << 7) | (c & 0x7F)
                    if not c & 0x80:
                        break
                i += n
                continue
            if b & 0x80:
                status = b
                i += 1
            hi, ch = status & 0xF0, status & 0x0F
            if hi in (0x80, 0x90):
                pitch, vel = data[i], data[i + 1]
                i += 2
                if ch == 9:
                    continue
                key = (trk if fmt else ch, pitch)
                if hi == 0x90 and vel:
                    held.setdefault(key, []).append(t)
                elif held.get(key):
                    on = held[key].pop()
                    if t > on:
                        voices.setdefault(key[0], []).append((on, t, pitch))
            elif hi in (0xA0, 0xB0, 0xE0):
                i += 2
            elif hi in (0xC0, 0xD0):
                i += 1
            else:
                raise ValueError("bad status")
        pos = end
    return ppq, voices, sorted(meters), sorted(tempi)


def top_voice(voices, ppq):
    """the melody: of the voices that play a fair share of the notes the highest, and of that its top line"""
    if not voices:
        return []
    most = max(len(v) for v in voices.values())
    fair = [v for v in voices.values() if len(v) >= max(20, most // 4)]
    if not fair:
        return []
    v = sorted(max(fair, key=lambda v: sum(n[2] for n in v) / len(v)), key=lambda n: (n[0], -n[2]))
    tol = max(1, ppq // 24)
    out = []
    for on, off, p in v:
        if out:
            pon, poff, pp = out[-1]
            if on - pon <= tol:                           # struck together: the highest is the line
                continue
            if pp > p and poff > on + ppq // 8:           # under a note still sounding: an inner voice
                continue
        out.append((on, off, p))
    return out


def beat_map(meters, ppq):
    """a function tick -> beats, with the beat the time signature counts (6/8: two, 2/2: two)"""
    segs = []                                              # (tick, beats at it, ticks a beat)
    for t, num, den in ([(0, 4, 4)] if not meters or meters[0][0] > 0 else []) + meters:
        q = ppq * 4 / den
        tb = q * 3 if num in (6, 9, 12) and den >= 8 else q
        if segs and segs[-1][0] == t:
            segs.pop()
        b0 = segs[-1][1] + (t - segs[-1][0]) / segs[-1][2] if segs else 0.0
        segs.append((t, b0, tb))

    def at(t):
        lo, hi = 0, len(segs) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if segs[mid][0] <= t:
                lo = mid
            else:
                hi = mid - 1
        s = segs[lo]
        return s[1] + (t - s[0]) / s[2]
    return at


# ---- what is measured: the same of a score's melody and of Tuna's own ---------------------------------------
MIN_NOTES = 40
TOL = 0.07                                                 # of a beat
LEAP_STEPS = {7: 4, 8: 5, 9: 5, 10: 6, 11: 6, 12: 7}     # semitones -> scale steps


def measure(mel):
    """mel: [(on, off, pitch)] in beats, in time order. The numbers the tuning works with, or None (too short):
    iv        shares of repeat, step, third, fourth, wider (by semitones: 0, 1-2, 3-4, 5-6, 7 and up)
    leap      mean width of the wider leaps, in scale steps
    octave    share of the moves that are an octave
    up        share of the moves that rise
    range     semitones between the 2nd and 98th percentile of its notes
    npb       notes a beat (rests over 8 beats left out)
    onbeat    share of its notes that start on a beat
    sync      share that start off the beat and sound through the next one
    (onbeat and sync are reported only: a sparse legato line holds notes over beats whatever its syncopation)"""
    n = len(mel)
    if n < MIN_NOTES:
        return None
    iv, leaps, octs, up, down, moves = [0] * 5, [], 0, 0, 0, 0
    span, onbeat, sync = 0.0, 0, 0
    for k in range(n):
        on, off, p = mel[k]
        nxt = mel[k + 1][0] if k + 1 < n else off
        frac = on - int(on) if on >= 0 else 0.0
        on_b = frac < TOL or frac > 1.0 - TOL
        onbeat += on_b
        if not on_b:
            edge = int(on) + 1 + TOL
            sync += nxt > edge and off > edge
        if k + 1 == n:
            break
        span += min(nxt - on, 8.0)
        if nxt - off > 4.0:                                # a new start after a long rest is not a move
            continue
        d = mel[k + 1][2] - p
        s = abs(d)
        iv[0 if s == 0 else 1 if s <= 2 else 2 if s <= 4 else 3 if s <= 6 else 4] += 1
        moves += 1
        if s >= 7:
            leaps.append(LEAP_STEPS.get(s, 8))
            octs += s == 12
        up += d > 0
        down += d < 0
    if moves < MIN_NOTES // 2 or span <= 0 or not up + down:
        return None
    ps = sorted(m[2] for m in mel)
    return {"iv": [x / moves for x in iv], "leap": sum(leaps) / len(leaps) if len(leaps) >= 3 else None,
            "octave": octs / moves, "up": up / (up + down), "range": ps[int(0.98 * (n - 1))] - ps[int(0.02 * (n - 1))],
            "npb": (n - 1) / span, "onbeat": onbeat / n, "sync": sync / n, "notes": n}


def mean(ms):
    """the mean of measurements, each score (or seed) counting once"""
    ms = [m for m in ms if m]
    if not ms:
        return None
    out = {"n": len(ms), "notes": sum(m["notes"] for m in ms)}
    for k in ("leap", "octave", "up", "range", "npb", "onbeat", "sync"):
        xs = [m[k] for m in ms if m[k] is not None]
        out[k] = round(sum(xs) / len(xs), 4) if xs else None
    out["iv"] = [round(sum(m["iv"][i] for m in ms) / len(ms), 4) for i in range(5)]
    return out


def score(path):
    """a MIDI file's measurement, with its meter and tempo; None when there is no melody to measure"""
    try:
        with open(path, "rb") as f:
            ppq, voices, meters, tempi = read_midi(f.read())
    except (OSError, ValueError, IndexError):
        return None
    at = beat_map(meters, ppq)
    mel = [(at(on), at(off), p) for on, off, p in top_voice(voices, ppq)]
    if mel:
        # an upbeat bar shorter than its signature moves every beat: lay the beat where most notes start
        bins = [0] * 12
        for on, off, p in mel:
            bins[round(on * 12) % 12] += 1
        shift = bins.index(max(bins)) / 12
        if shift:
            mel = [(on - shift + 1, off - shift + 1, p) for on, off, p in mel]
    m = measure(mel)
    if m:
        num, den = (meters[0][1], meters[0][2]) if meters else (4, 4)
        m["meter"] = "compound" if num in (6, 9, 12) and den >= 8 else "triple" if num == 3 else "duple" if num in (2, 4) else "other"
        m["bpm"] = round(6e7 / tempi[0][1]) if tempi else None
    return m


# ---- the corpus, composer by composer ---------------------------------------------------------------------------
import collections
import importlib.util
import json
import pathlib
import statistics

ROOT = pathlib.Path(__file__).resolve().parent.parent
STATS = ROOT / "tools" / "corpus" / "composers.json"
MIN_SCORES = 15                                            # a composer is measured from at least this many scores
MIN_TITLED = 8                                             # a manner from at least this many of its own


def manners():
    """{composer: [manner names]} from tools/manners"""
    out = {}
    for f in sorted((ROOT / "tools" / "manners").glob("*.py")):
        spec = importlib.util.spec_from_file_location(f.stem, f)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for k, v in mod.MANNERS.items():
            out[k] = [m["name"] for m in v]
    return out


def stats(pdmx):
    man = manners()
    out = {}
    for name, rows in index(pdmx).items():
        got = [(row, m) for row, m in ((row, score(f"{pdmx}/{mid}")) for mid, row in rows) if m]
        c = {"scores": len(rows), "all": mean([m for _, m in got])}
        if got:
            c["meter"] = dict(collections.Counter(m["meter"] for _, m in got).most_common())
            bpm = [m["bpm"] for _, m in got if m["bpm"] and m["bpm"] != 120]      # (120: no tempo was written)
            c["bpm"] = round(statistics.median(bpm)) if len(bpm) >= 5 else None
            c["manners"] = {}
            for mn in man.get(name, []):
                sub = mean([m for row, m in got if titled(row, mn)])
                if sub:
                    c["manners"][mn] = sub
        out[name] = c
    return {"source": "PDMX (https://zenodo.org/records/14648209): deduplicated, no licence conflict, "
                      f"rated scores at {MIN_RATING} or better; the top line of each score",
            "composers": out}


def report(data):
    L = [f"{'':24}{'scores':>6} {'rep':>5} {'step':>5} {'3rd':>5} {'4th':>5} {'wide':>5} {'leap':>5} {'oct':>5} "
         f"{'up':>5} {'range':>5} {'n/beat':>6} {'onbeat':>6} {'sync':>5}"]

    def line(label, m):
        f = lambda x, w=5, d=2: f"{x:>{w}.{d}f}" if x is not None else " " * (w - 1) + "-"
        return (f"{label:<24}{m['n']:>6} " + " ".join(f(x) for x in m["iv"]) + f" {f(m['leap'], 5, 1)} {f(m['octave'])} "
                f"{f(m['up'])} {f(m['range'], 5, 1)} {f(m['npb'], 6)} {f(m['onbeat'], 6)} {f(m['sync'])}")
    for name, c in data["composers"].items():
        if not c.get("all"):
            L.append(f"{name:<24}{0:>6}")
            continue
        L.append(line(name, c["all"]))
        for mn, m in c.get("manners", {}).items():
            L.append(line("  " + mn, m))
    return "\n".join(L)


# ---- Tuna's own melodies, measured the same way (tests/tuna_corpus_dump.c) ------------------------------------
import concurrent.futures
import os
import subprocess

SRC = ROOT / "firmware" / "src"
DUMP = ROOT / "build" / "host" / "tuna_corpus_dump"
SEEDS = [s * 7919 + 17 for s in range(1, 7)]
SECONDS = 240
KEYS = ("p0", "p1", "p2", "p3", "max_leap", "octave", "melody_density", "syncopation", "direction", "melody_range")


def build_dump():
    DUMP.parent.mkdir(parents=True, exist_ok=True)
    srcs = ["tests/tuna_corpus_dump.c"] + [f"firmware/src/tuna_{m}.c" for m in ("gen", "style", "marbles", "marbles_tables", "theory")]
    subprocess.run(["cc", "-O2", "-Ifirmware/src", "-o", str(DUMP)] + srcs + ["-lm"], cwd=ROOT, check=True)


def pieces():
    """[(style id, manner index, composer, manner name)] as the firmware holds them"""
    out = []
    for line in subprocess.run([str(DUMP), "list"], capture_output=True, text=True, check=True).stdout.splitlines():
        ids, mn = line.split("\t")
        sid, k, name = ids.split(" ", 2)
        out.append((int(sid), int(k), name, mn))
    return out


def play(jobs):
    """jobs: [(style, manner, settings or None)] -> [(settings as played, mel_mode, division, measurement)]"""
    def text(job):
        sid, k, v = job
        vals = " ".join(str(v[key]) if v else "-9" for key in KEYS)
        return "".join(f"{sid} {k} {seed} {SECONDS} {vals}\n" for seed in SEEDS)

    def run(job):
        out = subprocess.run([str(DUMP)], input=text(job), capture_output=True, text=True, check=True).stdout
        ms, mel, head = [], [], None
        for line in out.splitlines():
            if line[0] == "N":
                a = line.split()
                mel.append((float(a[1]), float(a[2]), int(a[3])))
            elif line[0] == "J":
                head, mel = line.split(), []
            else:
                ms.append(measure(mel))
        played = {key: float(x) for key, x in zip(KEYS, head[5:])}
        return played, int(head[3]), int(head[4]), mean(ms)
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 2) as ex:
        return list(ex.map(run, jobs))


# ---- tuning: move what a manner plays toward what its composer wrote -----------------------------------------
ROUNDS = 6
PITCH = ("p0", "p1", "p2", "p3", "max_leap", "octave", "direction", "melody_range")
RHYTHM = ("melody_density",)
# how far a setting may move from the manner's own
SHARE = 2.0                                                # an interval's chance: at most doubled or halved
OCTAVE = 0.05
DIRECTION = 0.2
WIDER, NARROWER = 6, 4                                     # semitones of range
DENSITY = 1.3                                              # times, or divided by
WORTH = 0.9                                                # a change must bring the manner this much closer


def trust(n, own):
    """how far to go toward the corpus: further with more scores; half as far when they are the composer's
    whole work instead of the manner's own"""
    return n / (n + 8.0) * (1.0 if own else 0.5)


def goal_of(g0, t, s):
    goal = {k: g0[k] + s * (t[k] - g0[k]) for k in ("octave", "up", "range", "npb")}
    goal["leap"] = g0["leap"] + s * (t["leap"] - g0["leap"]) if g0["leap"] and t["leap"] else None
    goal["iv"] = [a + s * (b - a) for a, b in zip(g0["iv"], t["iv"])]
    return goal


def distance(g, t, rhythm):
    """how far a measurement is from another (the interval shares, plus the rest on scales of their own)"""
    d = sum(abs(a - b) for a, b in zip(g["iv"], t["iv"])) + abs(g["octave"] - t["octave"]) + abs(g["up"] - t["up"])
    d += abs(g["range"] - t["range"]) / 24.0
    if g["leap"] and t["leap"]:
        d += abs(g["leap"] - t["leap"]) / 6.0
    if rhythm:
        d += abs(g["npb"] - t["npb"]) / 2.0
    return d


def step(v, v0, g, goal, rhythm, top):
    """the settings for the next round, from what the present ones played; never far from the manner's own (v0):
    a manner is leaned toward the scores, not rewritten by them"""
    v = dict(v)
    raw = lambda x: [x["p0"], x["p1"] - x["p0"], x["p2"] - x["p1"], x["p3"] - x["p2"], 1.0 - x["p3"]]
    q0 = [max(a, 0.01) for a in raw(v0)]
    q = [max(a, 0.004) * (max(b, 0.002) / max(c, 0.002)) ** 0.8 for a, b, c in zip(raw(v), goal["iv"], g["iv"])]
    q = [min(max(a, b / SHARE), b * SHARE) for a, b in zip(q, q0)]
    acc = 0.0
    for i in range(4):
        acc += q[i] / sum(q)
        v[f"p{i}"] = min(max(acc, 0.01 * (i + 1)), 0.96 + 0.01 * i)
    near = lambda x, lo, hi: min(max(x, lo), hi)
    if goal["leap"] and g["leap"]:
        v["max_leap"] = near(v["max_leap"] + round(goal["leap"] - g["leap"]), max(v0["max_leap"] - 1, 4), min(v0["max_leap"] + 1, 7))
    v["octave"] = near(v["octave"] + 1.5 * (goal["octave"] - g["octave"]), 0.0, v0["octave"] + OCTAVE)
    v["direction"] = near(v["direction"] + 1.5 * (goal["up"] - g["up"]), v0["direction"] - DIRECTION, v0["direction"] + DIRECTION)
    v["melody_range"] = near(v["melody_range"] + round(0.8 * (goal["range"] - g["range"])),
                             max(v0["melody_range"] - NARROWER, 7), max(min(v0["melody_range"] + WIDER, top), v0["melody_range"]))
    if rhythm:
        v["melody_density"] = near(v["melody_density"] * (goal["npb"] / g["npb"]) ** 0.9,
                                   max(v0["melody_density"] / DENSITY, 0.05), min(v0["melody_density"] * DENSITY, 1.0))
    for k in v:                                           # as the sources hold them: hundredths, whole numbers
        v[k] = int(round(v[k])) if k in ("max_leap", "melody_range") else round(v[k], 2)
    for i in range(1, 4):
        v[f"p{i}"] = max(v[f"p{i}"], round(v[f"p{i - 1}"] + 0.01, 2))
    return v


def tune(data):
    """[(composer, manner index, manner name, changes, note)]: the settings to write, with what they did"""
    build_dump()
    bases = melody_bases()
    work = []
    for sid, k, name, mn in pieces():
        c = data["composers"].get(name)
        if not c or not c.get("all"):
            continue
        own = c["manners"].get(mn)
        own = own if own and own["n"] >= MIN_TITLED else None
        if not own and c["all"]["n"] < MIN_SCORES:
            continue
        work.append({"sid": sid, "k": k, "name": name, "mn": mn, "t": own or c["all"], "own": bool(own)})
    first = play([(w["sid"], w["k"], None) for w in work])
    live = []
    for w, (v, mel_mode, division, g) in zip(work, first):
        if mel_mode > 1 or not g or v["p3"] <= 0:         # a pattern, tintinnabuli, birdsong: not a free line
            continue
        w.update(v0=v, v=v, g0=g, g=g, s=trust(w["t"]["n"], w["own"]), division=division, best=None)
        w["goal"] = goal_of(g, w["t"], w["s"])
        w["top"] = max(v["melody_range"], min(31, 96 - bases[w["name"]][w["k"]]))   # (not above C7)
        w["best"] = (distance(g, w["goal"], w["own"]), v, g)
        live.append(w)
    for _ in range(ROUNDS):
        for w in live:
            w["v"] = step(w["v"], w["v0"], w["g"], w["goal"], w["own"], w["top"])
        for w, (v, mel_mode, division, g) in zip(live, play([(w["sid"], w["k"], w["v"]) for w in live])):
            if not g:
                w["v"], w["g"] = w["best"][1], w["best"][2]
                continue
            w["g"] = g
            d = distance(g, w["goal"], w["own"])
            if d < w["best"][0]:
                w["best"] = (d, w["v"], g)
    out = []
    for w in live:
        d, v, g = w["best"]
        if (d > WORTH * distance(w["g0"], w["goal"], w["own"])
                or distance(g, w["t"], w["own"]) >= distance(w["g0"], w["t"], w["own"])):
            v, g = w["v0"], w["g0"]
        keys = PITCH + (RHYTHM if w["own"] else ())
        ch = {k: v[k] for k in keys if abs(v[k] - w["v0"][k]) > 1e-6}
        if all(abs(v[f"p{i}"] - w["v0"][f"p{i}"]) < 0.015 for i in range(4)):       # (not worth a change)
            for i in range(4):
                ch.pop(f"p{i}", None)
        elif any(f"p{i}" in ch for i in range(4)):
            ch.update({f"p{i}": v[f"p{i}"] for i in range(4)})
        w.update(changes=ch, g1=g)
        out.append(w)
    return out


def tune_report(res):
    L = [f"{'':30}{'from':>5} {'trust':>5} {'before':>6} {'after':>6}   changes (distance to the scores: before, after)"]
    for w in res:
        b, a = distance(w["g0"], w["t"], w["own"]), distance(w["g1"], w["t"], w["own"])
        v0, ch = w["v0"], dict(w["changes"])
        parts = []
        if "p0" in ch:
            parts.append("profile " + " ".join(f"{v0[f'p{i}']:.2f}" for i in range(4)) + " -> "
                         + " ".join(f"{ch.pop(f'p{i}'):.2f}" for i in range(4)))
        parts += [f"{k} {v0[k]:g} -> {x:g}" for k, x in ch.items()]
        L.append(f"{w['name'] + ' ' + w['mn']:<30}{w['t']['n']:>4}{'*' if w['own'] else ' '} {w['s']:>5.2f} {b:>6.2f} {a:>6.2f}   "
                 + "; ".join(parts))
    L.append("(* the manner's own scores, by title: pitch and rhythm; else the composer's whole work: pitch only)")
    return "\n".join(L)


# ---- writing the settings into the sources -----------------------------------------------------------------------
def melody_bases():
    """{composer: [melody_base of each manner]}: where each manner's melody range begins"""
    c = (SRC / "tuna_style.c").read_text()
    out = {}
    man = manner_dicts()
    for m in re.finditer(r'\.name = "([^"]+)"', c):
        blk = c[m.start():c.index("\n    },", m.start())]
        b = re.search(r"\.melody_base = (\d+)", blk)
        base = int(b.group(1)) if b else 60
        out[m.group(1)] = [d.get("melody_base", base) for d in man.get(m.group(1), [{}])]
    return out


def manner_dicts():
    out = {}
    for f in sorted((ROOT / "tools" / "manners").glob("*.py")):
        spec = importlib.util.spec_from_file_location(f.stem, f)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        out.update(mod.MANNERS)
    return out


def fmt(k, x, c):
    if k in ("max_leap", "melody_range"):
        return str(int(x))
    return f"{x:.2f}f" if c else f"{x:g}"


def write_profile(text, name, ch):
    """a composer's profile in tuna_style.c with the changed settings"""
    a = text.index(f'.name = "{name}"')
    b = text.index("\n    },", a)
    blk = text[a:b]
    ch = dict(ch)
    if "p0" in ch:
        new = ".profile = {" + ", ".join(fmt("p", ch.pop(f"p{i}"), True) for i in range(4)) + "}"
        if not re.search(r"\.profile = \{[^}]*\}", blk):
            raise ValueError(f"{name}: no profile")
        blk = re.sub(r"\.profile = \{[^}]*\}", new, blk, count=1)
    add = []
    for k, x in ch.items():
        pat = rf"\.{k} = -?[0-9.]+f?"
        if re.search(pat, blk):
            blk = re.sub(pat, f".{k} = {fmt(k, x, True)}", blk, count=1)
        else:
            add.append(f".{k} = {fmt(k, x, True)}")
    if add:                                                # a line of its own after the profile's
        at = blk.index("\n", blk.index(".profile = "))
        blk = blk[:at] + "\n        " + ", ".join(add) + "," + blk[at:]
    return text[:a] + blk + text[b:]


def write_manner(text, name, k, ch):
    """a manner's dict(...) in a tools/manners file with the changed settings"""
    a = text.index(f'"{name}": [')
    at = a
    for _ in range(k + 1):
        at = text.index("dict(", at + 1)
    depth, end = 0, at + 4
    while True:
        depth += text[end] in "([{"
        depth -= text[end] in ")]}"
        if depth == 0:
            break
        end += 1
    d = text[at:end]                                       # dict( ... without its closing bracket
    ch = dict(ch)
    add = []
    if "p0" in ch:
        new = "profile=[" + ", ".join(fmt("p", ch.pop(f"p{i}"), False) for i in range(4)) + "]"
        if re.search(r"\bprofile=\[[^\]]*\]", d):
            d = re.sub(r"\bprofile=\[[^\]]*\]", new, d, count=1)
        else:
            add.append(new)
    for key, x in ch.items():
        pat = rf"(?<![\w.]){key}=-?[0-9.]+"
        if re.search(pat, d):
            d = re.sub(pat, f"{key}={fmt(key, x, False)}", d, count=1)
        else:
            add.append(f"{key}={fmt(key, x, False)}")
    if add:
        d = d.rstrip() + ",\n             " + ", ".join(add)
    return text[:at] + d + text[end:]


def write(res):
    c_path = SRC / "tuna_style.c"
    c = c_path.read_text()
    files = {f: f.read_text() for f in sorted((ROOT / "tools" / "manners").glob("*.py"))}
    for w in res:
        if not w["changes"]:
            continue
        if w["k"] == 0:
            c = write_profile(c, w["name"], w["changes"])
            continue
        for f, text in files.items():
            if f'"{w["name"]}": [' in text:
                files[f] = write_manner(text, w["name"], w["k"], w["changes"])
    # a manner that leaves a setting to its profile would follow the profile's change: it keeps what it had
    for w in res:
        if w["k"] == 0:
            continue
        base = next((b for b in res if b["name"] == w["name"] and b["k"] == 0), None)
        if not base:
            continue
        keep = {k: w["v0"][k] for k in base["changes"] if k not in w["changes"]}
        if any(f"p{i}" in keep for i in range(4)):
            keep.update({f"p{i}": w["v0"][f"p{i}"] for i in range(4)})
        for f, text in files.items():
            if keep and f'"{w["name"]}": [' in text:
                files[f] = write_manner(text, w["name"], w["k"], keep)
    c_path.write_text(c)
    for f, text in files.items():
        f.write_text(text)


def main():
    a = sys.argv[1:]
    if a[:1] == ["index"] and len(a) == 2:
        for name, rows in index(a[1]).items():
            print(f"{name:<20}{len(rows):>5}")
    elif a[:1] == ["stats"] and len(a) in (2, 4):
        out = pathlib.Path(a[3]) if len(a) == 4 and a[2] == "-o" else STATS
        out.parent.mkdir(parents=True, exist_ok=True)
        data = stats(a[1])
        out.write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n")
        print(report(data))
    elif a[:1] == ["tune"]:
        res = tune(json.loads(STATS.read_text()))
        print(tune_report(res), flush=True)
        if "--dry" not in a:
            write(res)
            subprocess.run([sys.executable, str(ROOT / "tools" / "tuna_manners.py")], check=True)
    elif a[:1] == ["report"]:
        print(report(json.loads((pathlib.Path(a[1]) if len(a) > 1 else STATS).read_text())))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
