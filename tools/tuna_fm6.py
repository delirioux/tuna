#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Tuna's own FM6 patches -> firmware/src/tuna_fm6.inc (packed 128-byte voices).

    python3 tools/tuna_fm6.py            writes it
    python3 tools/tuna_fm6.py --check    fails if the file differs from what this writes

Patches made for Tuna (no factory ROM data of any instrument), written as Felucca's own are
(tools/gen_fm6_patches.py: op() and voice(), OP1..OP6). A family loads one onto FM6 as the track's own patch
(tuna_sounds.h TSN_FM6P). Each is checked by ear in the audition (tests/tuna_audition.sh).

The algorithms used (the generic 6-operator numbering):
   1  1<-2,  3<-4<-5<-6 (fb 6)             carriers 1 3
   2  1<-2 (fb 2),  3<-4<-5<-6             carriers 1 3
   5  1<-2,  3<-4,  5<-6 (fb 6)            carriers 1 3 5
  18  1<-(2, 3 fb, 4<-5<-6)                carrier 1
  32  six carriers (fb 6)                   additive
Envelope rates: higher is faster; levels 0..99. ol: output level; fc: coarse ratio (0 = 0.5); det: 7 = none.
"""
import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "firmware" / "src" / "tuna_fm6.inc"
_spec = importlib.util.spec_from_file_location("gen_fm6", ROOT / "tools" / "gen_fm6_patches.py")
_g = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_g)
op, voice, pack = _g.op, _g.voice, _g.pack
OFF = op(ol=0)

# (C name, voice)
PATCHES = [
    # a church organ: flue pipes at 16' 8' 4' 2 2/3' 2' 1', slower to speak than a tonewheel, a little chiff
    ("PIPEORGAN", voice("PIPE ORGAN", 32, [
        op(r=(62, 60, 60, 68), l=(99, 99, 99, 0), ol=94, det=7),
        op(r=(55, 60, 60, 66), l=(99, 99, 99, 0), ol=82, fc=0, det=6),
        op(r=(66, 60, 60, 68), l=(99, 99, 99, 0), ol=86, fc=2, det=8),
        op(r=(66, 60, 60, 70), l=(99, 99, 99, 0), ol=70, fc=3, det=7),
        op(r=(70, 60, 60, 70), l=(99, 99, 99, 0), ol=78, fc=4, det=6),
        op(r=(99, 55, 60, 72), l=(99, 70, 70, 0), ol=64, fc=8, det=8)],
        fb=1, lpms=0)),
    # a reed electric piano: a barking attack with the velocity, a slow decay, tremolo
    ("REEDEP", voice("REED EP", 5, [
        op(r=(96, 28, 22, 58), l=(99, 88, 0, 0), ol=99, kvs=2, ams=2, rs=2),
        op(r=(95, 62, 30, 60), l=(99, 55, 30, 0), ol=86, kvs=6, rs=2),
        op(r=(96, 30, 22, 58), l=(99, 85, 0, 0), ol=88, fc=2, kvs=2, ams=2, rs=2),
        op(r=(95, 70, 40, 60), l=(99, 40, 0, 0), ol=76, fc=3, kvs=7),
        op(r=(96, 32, 24, 58), l=(99, 80, 0, 0), ol=84, det=11, kvs=1, ams=1),
        op(r=(95, 50, 30, 60), l=(99, 60, 0, 0), ol=72, kvs=4)],
        fb=5, lfs=40, lamd=18, lfw=4, lpms=0)),
    # a slapped and popped bass: the thumb's click (ratio 3, gone at once), a growl on velocity
    ("SLAPBASS", voice("SLAP BASS", 5, [
        op(r=(99, 50, 30, 62), l=(99, 85, 70, 0), ol=99, kvs=2, rs=2),
        op(r=(99, 75, 40, 70), l=(99, 55, 35, 0), ol=92, kvs=7, rs=2),
        op(r=(99, 55, 35, 62), l=(99, 80, 60, 0), ol=80, fc=2, kvs=2),
        op(r=(99, 88, 60, 70), l=(99, 0, 0, 0), ol=90, fc=3, kvs=6),
        OFF, OFF],
        fb=0, lpms=0)),
    # an analogue-style brass section: a feedback saw, its brightness swelling in
    ("SYNBRASS", voice("SYN BRASS", 2, [
        op(r=(70, 45, 30, 60), l=(99, 92, 86, 0), ol=99, kvs=1),
        op(r=(52, 40, 30, 60), l=(99, 82, 74, 0), ol=82, kvs=3),
        op(r=(70, 45, 30, 60), l=(99, 92, 86, 0), ol=90, det=10, kvs=1),
        op(r=(52, 40, 30, 60), l=(99, 80, 70, 0), ol=80, det=4, kvs=3),
        op(r=(60, 40, 30, 60), l=(99, 80, 70, 0), ol=58),
        op(r=(60, 40, 30, 60), l=(99, 80, 70, 0), ol=46)],
        fb=7, pr=(80, 50, 50, 50), pl=(46, 50, 50, 50), lpms=0)),
    # a harmonica: reedy (odd partials through ratio 3), a breathy bend in, the hand's vibrato
    ("HARMONICA", voice("HARMONICA", 5, [
        op(r=(78, 50, 40, 70), l=(99, 92, 86, 0), ol=99, kvs=2, ams=1),
        op(r=(82, 40, 30, 70), l=(99, 86, 76, 0), ol=80, kvs=5),
        op(r=(78, 50, 40, 70), l=(99, 90, 84, 0), ol=72, fc=2, kvs=2),
        op(r=(82, 45, 35, 70), l=(99, 80, 70, 0), ol=72, fc=3, kvs=4),
        op(r=(78, 50, 40, 70), l=(99, 88, 80, 0), ol=0, det=6),
        op(r=(80, 50, 40, 70), l=(99, 80, 70, 0), ol=56)],
        fb=6, pr=(85, 50, 50, 50), pl=(47, 50, 50, 50), lfs=34, lfd=40, lpmd=8, lamd=12, lfw=4, lpms=3)),
    # pan pipes: almost a sine, a chiff at the start, a slow vibrato
    ("PANFLUTE", voice("PAN FLUTE", 5, [
        op(r=(66, 40, 40, 62), l=(99, 95, 92, 0), ol=99),
        op(r=(99, 72, 40, 60), l=(99, 30, 12, 0), ol=64, kvs=3),
        op(r=(66, 40, 40, 62), l=(99, 90, 86, 0), ol=58, fc=2, det=8),
        op(r=(99, 80, 50, 60), l=(99, 20, 0, 0), ol=60, fc=7),
        OFF, OFF],
        fb=0, lfs=30, lfd=55, lpmd=6, lfw=4, lpms=3)),
    # a shakuhachi: breath (a high, inharmonic modulator), the note bent up into place, wide vibrato
    ("SHAKUHACHI", voice("SHAKUHACHI", 5, [
        op(r=(60, 40, 40, 58), l=(99, 94, 90, 0), ol=99),
        op(r=(70, 50, 40, 58), l=(99, 70, 50, 0), ol=66, kvs=4),
        op(r=(60, 40, 40, 58), l=(99, 80, 70, 0), ol=62, fc=2),
        op(r=(80, 50, 40, 60), l=(99, 60, 40, 0), ol=84, fc=9, ff=40),
        OFF, OFF],
        fb=0, pr=(70, 60, 50, 50), pl=(44, 50, 50, 50), lfs=28, lfd=60, lpmd=12, lfw=4, lpms=4)),
    # an erhu: a bowed, nasal two-string fiddle, sliding into each note, deep vibrato
    ("ERHU", voice("ERHU", 1, [
        op(r=(58, 40, 40, 60), l=(99, 95, 90, 0), ol=99, kvs=2),
        op(r=(62, 40, 35, 60), l=(99, 88, 80, 0), ol=78, kvs=4),
        op(r=(58, 40, 40, 60), l=(99, 92, 86, 0), ol=72, fc=2, det=8),
        op(r=(62, 40, 35, 60), l=(99, 85, 76, 0), ol=72),
        op(r=(62, 40, 35, 60), l=(99, 80, 70, 0), ol=62, fc=3),
        op(r=(62, 40, 35, 60), l=(99, 80, 70, 0), ol=52)],
        fb=6, pr=(60, 50, 50, 50), pl=(46, 50, 50, 50), lfs=36, lfd=45, lpmd=14, lfw=4, lpms=4)),
    # a trumpet with a harmon mute: thin, nasal, buzzing
    ("MUTETRUMPET", voice("MUTE TPT", 18, [
        op(r=(84, 50, 40, 70), l=(99, 90, 84, 0), ol=94, kvs=2),
        op(r=(80, 50, 40, 70), l=(99, 84, 76, 0), ol=86, kvs=4),
        op(r=(80, 50, 40, 70), l=(99, 84, 76, 0), ol=70),
        op(r=(80, 50, 40, 70), l=(99, 80, 70, 0), ol=82, fc=3),
        op(r=(80, 50, 40, 70), l=(99, 80, 70, 0), ol=60),
        op(r=(80, 50, 40, 70), l=(99, 80, 70, 0), ol=50, fc=2)],
        fb=7, pr=(90, 50, 50, 50), pl=(47, 50, 50, 50), lfs=36, lfd=50, lpmd=5, lfw=4, lpms=3)),
    # a flugelhorn: mellow brass, the brightness coming slowly
    ("FLUGELHORN", voice("FLUGELHORN", 2, [
        op(r=(66, 45, 35, 62), l=(99, 92, 88, 0), ol=99, kvs=1),
        op(r=(55, 40, 30, 62), l=(99, 78, 70, 0), ol=72, kvs=3),
        op(r=(66, 45, 35, 62), l=(99, 90, 86, 0), ol=88, fc=2, kvs=1),
        op(r=(55, 40, 30, 62), l=(99, 74, 66, 0), ol=64, kvs=3),
        OFF, OFF],
        fb=3, lfs=33, lfd=55, lpmd=5, lfw=4, lpms=3)),
    # a toy piano: struck metal rods, inharmonic, short
    ("TOYPIANO", voice("TOY PIANO", 5, [
        op(r=(99, 45, 30, 62), l=(99, 55, 0, 0), ol=99, kvs=2, rs=3),
        op(r=(99, 70, 40, 62), l=(99, 40, 0, 0), ol=88, fc=3, ff=50, kvs=2, rs=3),
        op(r=(99, 45, 30, 62), l=(99, 60, 0, 0), ol=90, fc=6, ff=12, rs=3),
        op(r=(99, 70, 40, 62), l=(99, 30, 0, 0), ol=80, fc=5, kvs=2),
        OFF, OFF],
        fb=0, lpms=0)),
    # a fretless bass: round, a 'mwah' on velocity, glides (the family: legato, glide)
    ("FRETLESS", voice("FRETLESS", 5, [
        op(r=(80, 40, 25, 62), l=(99, 90, 76, 0), ol=99, kvs=2),
        op(r=(80, 48, 30, 62), l=(99, 70, 50, 0), ol=80, kvs=5),
        op(r=(80, 40, 25, 62), l=(99, 88, 72, 0), ol=82, fc=2, kvs=2),
        op(r=(80, 50, 30, 62), l=(99, 60, 40, 0), ol=55, fc=2, kvs=4),
        OFF, OFF],
        fb=0, lpms=0)),
    # an English horn (cor anglais): a dark double reed, a formant near the third partial
    ("ENGLISHHORN", voice("ENG HORN", 1, [
        op(r=(68, 45, 40, 64), l=(99, 92, 88, 0), ol=99, kvs=2),
        op(r=(70, 45, 35, 64), l=(99, 84, 78, 0), ol=84, kvs=4),
        op(r=(68, 45, 40, 64), l=(99, 88, 82, 0), ol=70, fc=3),
        op(r=(70, 45, 35, 64), l=(99, 80, 72, 0), ol=74),
        op(r=(70, 45, 35, 64), l=(99, 78, 70, 0), ol=64, fc=2),
        op(r=(70, 45, 35, 64), l=(99, 76, 66, 0), ol=48)],
        fb=5, lfs=32, lfd=60, lpmd=4, lfw=4, lpms=3)),
    # a concert flute (in place of Felucca's sampled one: its 31 KB freed for Tuna): almost a sine with its octave,
    # warmer as it is blown harder, breath (a feedback modulator gone to noise) stronger at the start, and a vibrato
    # that comes in late
    ("FLUTE", voice("FLUTE", 5, [
        op(r=(62, 40, 40, 62), l=(99, 96, 94, 0), ol=99, kvs=2, ams=1),
        op(r=(70, 45, 40, 62), l=(99, 70, 62, 0), ol=74, kvs=3),
        op(r=(62, 40, 40, 62), l=(99, 88, 84, 0), ol=84, fc=2),
        op(r=(70, 45, 40, 62), l=(99, 60, 50, 0), ol=40),
        op(r=(80, 52, 40, 62), l=(99, 72, 62, 0), ol=56, kvs=2),
        op(r=(99, 60, 40, 62), l=(99, 90, 86, 0), ol=88, fc=7, ff=37)],
        fb=7, lfs=33, lfd=60, lpmd=6, lamd=4, lfw=4, lpms=3)),
    # a tenor sax (in place of the sampled one, 31 KB): a buzzing reed (the feedback modulator, brighter with
    # the breath's force), a nasal stack with ratio 3, a scoop up into each note, a late vibrato
    ("SAX", voice("SAX", 2, [
        op(r=(72, 45, 40, 64), l=(99, 92, 88, 0), ol=99, kvs=2),
        op(r=(78, 40, 30, 64), l=(99, 84, 76, 0), ol=86, kvs=5),
        op(r=(72, 45, 40, 64), l=(99, 90, 86, 0), ol=78, det=9, kvs=2),
        op(r=(76, 42, 32, 64), l=(99, 82, 74, 0), ol=72, fc=3, kvs=4),
        op(r=(76, 42, 32, 64), l=(99, 80, 72, 0), ol=60),
        op(r=(76, 42, 32, 64), l=(99, 76, 66, 0), ol=50, fc=2)],
        fb=6, pr=(80, 50, 50, 50), pl=(45, 50, 50, 50), lfs=34, lfd=55, lpmd=7, lfw=4, lpms=3)),
]


def render():
    L = ["/* SPDX-License-Identifier: GPL-3.0-only */",
         "/* generated by tools/tuna_fm6.py: Tuna's own FM6 patches, packed (128 bytes); edit that, not this */"]
    for cname, v in PATCHES:
        name = "".join(chr(x) for x in v[145:155]).rstrip()
        L += [f"static const uint8_t TSN_FM6_{cname}[128] = {{   /* {name} */", _g.c_rows(pack(v)), "};"]
    return "\n".join(L) + "\n"


if __name__ == "__main__":
    text = render()
    if "--check" in sys.argv:
        if not OUT.exists() or OUT.read_text() != text:
            sys.exit("tuna_fm6: firmware/src/tuna_fm6.inc is out of date (python3 tools/tuna_fm6.py)")
        print(f"tuna_fm6: {len(PATCHES)} patches, up to date")
    else:
        OUT.write_text(text)
        print(f"tuna_fm6: {len(PATCHES)} patches -> {OUT.relative_to(ROOT)}")
