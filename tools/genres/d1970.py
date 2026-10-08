# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 1970s (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings are
# its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop.

R = -128                              # a rest in a pad figure (TG_REST)
B16 = "x.x.x.x.x.x.x.x."              # eighths in a bar of sixteenths

GROOVES = {
    # Led Zeppelin, AC/DC: a heavy backbeat, the kick pushing the "and" of 3, eighths on a half-open hat
    "G70_HARDROCK": dict(kit="STD", kick="PUNCH", tune=58, decay=62, snap=78, drive=30, lanes=[
        ("KICK", 110, "X.......x.x.....X.......x.x..x.."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("HAT", 80, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 90, "x..............................."),
    ]),
    # Black Sabbath's "Paranoid", Judas Priest: driving kick eighths under the snare, the ride, crashes
    "G70_METAL": dict(kit="STD", kick="PUNCH", tune=54, decay=56, snap=84, drive=30, lanes=[
        ("KICK", 108, "X.x.x.x.X.x.x.x.X.x.x.x.X.x.x.x."),
        ("SNARE", 114, "....X.......X.......X.......X..."),
        ("RIDE", 76, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 96, "X...............X..............."),
    ]),
    # Genesis, Yes, Pink Floyd: 7/4 in eighths (4 + 3 beats), the snare's pickup on the "and" of 7
    "G70_PROG7": dict(kit="STD", kick="ROUND", tune=60, tone=58, decay=64, snap=60, lanes=[
        ("KICK", 100, "X...x...x...x.X...x...x...x."),
        ("SNARE", 98, "..X...X...X..x..X...X...X..x"),
        ("HAT", 70, "xxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
        ("CRASH", 84, "x..........................."),
        ("TOM_MID", 80, "..........................xx"),
    ]),
    # T. Rex, Slade, Sweet: the stomp: kick and floor tom on every beat, claps and snare on 2 and 4, tambourine
    "G70_GLAM": dict(kit="STD", kick="ROUND", tune=60, decay=70, snap=72, lanes=[
        ("KICK", 108, "X...x...X...x...X...x...X...x..."),
        ("FLOOR_TOM", 92, "x...x...x...x...x...x...x...x..."),
        ("SNARE", 104, "....X.......X.......X.......X..."),
        ("CLAP", 96, "....X.......X.......X.......X..."),
        ("TAMB", 70, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
    # the one drop: kick and cross-stick together on 3 only, nothing on 1; hats in eighths, a rim on the "and"
    "G70_ONEDROP": dict(kit="STD", kick="ROUND", tune=56, tone=48, decay=60, snap=50, fill=0, lanes=[
        ("KICK", 104, "........X...............X......."),
        ("RIM", 100, "........X...............X......."),
        ("HAT", 66, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("HAT", 46, "...g.......g...g...g.......g...."),
        ("OHAT", 58, "..............x..............x.."),
    ]),
    # Chic, Bee Gees, Donna Summer: four on the floor, the open hat on every offbeat, snare and clap on 2 and 4
    "G70_DISCO": dict(kit="STD", kick="PUNCH", tune=62, decay=58, snap=74, lanes=[
        ("KICK", 112, "X...X...X...X...X...X...X...X..."),
        ("SNARE", 98, "....X.......X.......X.......X..."),
        ("CLAP", 80, "....x.......x.......x.......x..."),
        ("HAT", 58, "x.gxx.gxx.gxx.gxx.gxx.gxx.gxx.gx"),
        ("OHAT", 88, "..x...x...x...x...x...x...x...x."),
        ("CONGA_HI", 60, "......x.......x.x.....x.......x."),
    ]),
    # the Ramones, the Sex Pistols: fast, flat out, kick-snare in eighths on the hats, crash on the one
    "G70_PUNK": dict(kit="STD", kick="PUNCH", tune=66, decay=44, snap=90, drive=40, lanes=[
        ("KICK", 110, "X.....x.x.x.....X.....x.x.x....."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("HAT", 86, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 96, "X...............X..............."),
    ]),
    # Fania: the 3-2 son clave (one bar a clave in sixteenths at half tempo), cowbell on the beats, the conga
    # tumbao (slap on 2, open tones on 4 and the "and"), bongo martillo, timbale cascara
    "G70_SALSA": dict(kit="STD", kick="ROUND", tune=60, tone=60, decay=40, snap=50, fill=0, lanes=[
        ("CLAVE", 96, "x..x..x...x.x...x..x..x...x.x..."),
        ("COWBELL", 84, "X.x.x.x.X.x.x.x.X.x.x.x.X.x.x.x."),
        ("CONGA_HI", 92, "g.g.X.g.g.g.x.x.g.g.X.g.g.g.x.x."),
        ("CONGA_LO", 96, "............x.x.............x.x."),
        ("BONGO_HI", 66, "xgxgxgxgxgxgxgxgxgxgxgxgxgxgxgxg"),
        ("TIMBALE", 74, "x.xx.x.xx.x.x.xxx.xx.x.xx.x.x.xx"),
    ]),
    # Kool Herc's merry-go-round: a funky-drummer-style break looped on two decks, ghost snares, an open hat
    "G70_BREAK": dict(kit="STD", kick="PUNCH", tune=60, tone=56, decay=56, snap=82, drive=20, fill=0, lanes=[
        ("KICK", 112, "X.x.......x..x..X.x.......x..x.."),
        ("SNARE", 108, "....X.......X.......X.......X..."),
        ("SNARE", 50, ".......g.g....g........g.g..g..g"),
        ("HAT", 72, "xxxxxxx.xxxxxxxxxxxxxxx.xxxxxxxx"),
        ("OHAT", 80, ".......x...............x........"),
    ]),
    # Neu!, Kraftwerk: Klaus Dinger's motorik: kick on 1, the "and" of 2 and 3, snare 2 and 4, steady eighths,
    # no fills, the same for ever
    "G70_MOTORIK": dict(kit="STD", kick="ROUND", tune=62, tone=52, decay=46, snap=58, fill=0, lanes=[
        ("KICK", 104, "X.....x.X.....x.X.....x.X.....x."),
        ("SNARE", 96, "....x.......x.......x.......x..."),
        ("HAT", 74, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("TAMB", 50, "....x.......x.......x.......x..."),
    ]),
}

GENRES = [
    dict(id="HARD_ROCK_70S", name="HARD ROCK", decade=1970, year=1969,
         comment="Led Zeppelin, Deep Purple, AC/DC: a blues-rock riff on distorted power chords, eighth-note bass, "
                 "a pentatonic guitar solo with blue notes, a heavy backbeat",
         key="A", scale="MIXOLYDIAN", bpm=126, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["I5", "I5", "bVII5", "IV5", "I5", "I5", "bIII5", "IV5"],
         melody=dict(density=0.6, base=64, range=15, sync=0.45, add=("b3", "b5", "b7"), staccato=0.15, motif=0.7,
                     profile=(0.2, 0.62, 0.84, 0.95)),
         bass=dict(mode="OSTINATO", rhythm="x.x.x.x.x.x.xxx.", notes=(0, 0, 0, 0, 0, 0, 10, 12), semis=True),
         pad=dict(mode="RHYTHM", rhythm="x..x..x.x.x..x..x..x..x.x..x.x..", gate=2, base=52, tones=3, power=True),
         groove="G70_HARDROCK", orch=[("WAVEBASS", "DISTGUITAR", "DISTGUITAR"), ("EBASS", "EGUITAR", "DISTGUITAR"),
                                      ("WAVEBASS", "ROCKORGAN", "DISTGUITAR")]),
    dict(id="HEAVY_METAL_70S", name="HEAVY METAL", decade=1970, year=1970,
         comment="Black Sabbath, Judas Priest: Phrygian power chords chugging in eighths with the bass, the "
                 "flat second, a screaming minor lead, ride and driving kick",
         key="E", scale="PHRYGIAN", bpm=160, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "i5", "bII5", "i5", "i5", "VI5", "VII5", "i5"],
         melody=dict(density=0.55, base=64, range=15, sync=0.3, add=("b5",), motif=0.65,
                     profile=(0.2, 0.68, 0.86, 0.95)),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0,), gate=1),
         pad=dict(mode="RHYTHM", rhythm=B16 * 2, gate=1, base=52, tones=3, power=True),
         groove="G70_METAL", orch=[("WAVEBASS", "DISTGUITAR", "DISTGUITAR"), ("EBASS", "EGUITAR", "DISTGUITAR")]),
    dict(id="PROG_ROCK_70S", name="PROG ROCK", decade=1970, year=1971, reverb="HALL",
         comment="Genesis, Yes, Pink Floyd, ELP: 7/4 riffs, a Moog lead over Mellotron and Hammond chords, "
                 "modal harmony wandering through Dorian, an arpeggiated bass riff",
         key="D", scale="DORIAN", bpm=120, div=2, beats=7, chord_beats=7, period_bars=8,
         vocab="POP", harmony="PLANNED", repeat=0.3, open=0.3,
         weights_minor={"i": 10, "iv": 6, "IV": 6, "VII": 7, "III": 5, "VI": 5, "v": 4, "isus2": 4, "VIIsus2": 3,
                        "IIIadd9": 3, "iadd9": 4},
         weights={"I": 10, "Iadd9": 6, "IV": 6, "vi": 5, "V": 5, "bVII": 4, "ii": 3},
         melody=dict(density=0.6, base=62, range=17, sync=0.25, profile=(0.15, 0.62, 0.84, 0.95), max_leap=4,
                     motif=0.7, turn=0.1),
         bass=dict(mode="OSTINATO", rhythm="x..xx.x.x..x.x", notes=(0, 7, 12, 7, 0, 12, 10), semis=True),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G70_PROG7", orch_mode="CONTRAST",
         orch=[("WAVEBASS", "SAWLEAD", "STRINGS"), ("WAVEBASS", "SAWLEAD", "ROCKORGAN"), ("EBASS", "FLUTE", "PIANO")]),
    dict(id="GLAM_ROCK", name="GLAM ROCK", decade=1970, year=1971,
         comment="T. Rex, Bowie's Ziggy, Slade, Sweet: a stomping shuffle with handclaps, boogie guitar on "
                 "the beats, a saxophone or guitar hook, a singalong I-bVII-IV",
         key="E", scale="MIXOLYDIAN", bpm=132, div=4, chord_beats=4, period_bars=8, swing=0.5,
         vocab="BLUES", harmony="CYCLE", cycle=["I", "I", "bVII", "IV", "I", "I", "IV", "V"],
         melody=dict(density=0.5, base=64, range=12, sync=0.3, add=("b3",), motif=0.8, staccato=0.2,
                     profile=(0.3, 0.72, 0.9, 0.97)),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0, 0, 7, 7, 9, 9, 7, 7), semis=True),
         pad=dict(mode="RHYTHM", rhythm="x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.", gate=1, base=52, tones=3, power=True),
         groove="G70_GLAM", orch=[("EBASS", "SAX", "DISTGUITAR"), ("EBASS", "EGUITAR", "PIANO")]),
    dict(id="ROOTS_REGGAE", name="REGGAE", decade=1970, year=1972, reverb="SPRING",
         comment="Bob Marley and the Wailers, Burning Spear: the one drop on 3, the guitar and organ skank on "
                 "2 and 4, a deep melodic bass that leaves holes, a melodica-like lead",
         key="A", scale="AEOLIAN", bpm=74, div=4, chord_beats=4, period_bars=8, swing=0.15,
         vocab="POP", harmony="CYCLE", cycle=["i", "i", "VII", "VII", "i", "i", "iv", "VII"],
         melody=dict(density=0.4, base=62, range=12, sync=0.45, motif=0.75, profile=(0.25, 0.72, 0.9, 0.97)),
         bass=dict(mode="OSTINATO", rhythm="x..x..x...x.x...", notes=(0, 0, 7, 12, 10), semis=True, gate=3),
         pad=dict(mode="RHYTHM", rhythm="....x.......x.......x.......x...", gate=2, base=60, tones=3, strum_ms=4),
         groove="G70_ONEDROP", orch=[("EBASS", "FLUTE", "GUITAR"), ("EBASS", "SAX", "GUITAR")]),
    dict(id="DISCO", name="DISCO", decade=1970, year=1974,
         comment="Chic, the Bee Gees, Gloria Gaynor: four on the floor, open hats on the offbeats, the octave "
                 "bass, a choppy guitar on the sixteenths, a string hook, a minor seventh vamp",
         key="A", scale="DORIAN", bpm=120, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i7", "i7", "iv7", "iv7", "i7", "i7", "VII", "v7"],
         melody=dict(density=0.5, base=67, range=12, sync=0.4, motif=0.85, staccato=0.25,
                     profile=(0.25, 0.65, 0.88, 0.96)),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0, 12), semis=True, gate=1),
         pad=dict(mode="RHYTHM", rhythm="..x.x.xx..x.x.xx..x.x.xx..x.x.xx", gate=1, base=60, tones=3),
         groove="G70_DISCO", orch=[("EBASS", "STRINGS", "GUITAR"), ("EBASS", "FLUTE", "GUITAR"),
                                   ("SYNTHBASS", "STRINGS", "GUITAR")]),
    dict(id="PUNK_70S", name="PUNK", decade=1970, year=1976, reverb="BATHROOM",
         comment="the Ramones, Sex Pistols, the Clash: three chords as fast as possible, downstroked power "
                 "chords and bass in eighths, a shouted two-note hook",
         key="E", scale="IONIAN", bpm=180, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["I5", "I5", "IV5", "V5", "I5", "IV5", "V5", "V5"],
         melody=dict(density=0.45, base=64, range=9, sync=0.15, motif=0.85, max_leap=3,
                     profile=(0.4, 0.8, 0.94, 0.99)),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0,), gate=2),
         pad=dict(mode="RHYTHM", rhythm=B16 * 2, gate=1, base=52, tones=3, power=True),
         groove="G70_PUNK", orch=[("WAVEBASS", "EGUITAR", "DISTGUITAR"), ("WAVEBASS", "DISTGUITAR", "DISTGUITAR")]),
    dict(id="SALSA", name="SALSA", decade=1970, year=1971,
         comment="Fania: Willie Colon, Hector Lavoe, Eddie Palmieri: the 3-2 clave, an anticipated tumbao bass "
                 "that skips the one, the piano montuno, trombones and trumpets, cowbell and congas",
         key="A", scale="HARM_MINOR", bpm=96, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i", "iv", "V7", "i", "i", "iv", "V7", "V7"],
         melody=dict(density=0.55, base=64, range=14, sync=0.55, motif=0.75, staccato=0.3,
                     profile=(0.2, 0.65, 0.86, 0.96)),
         bass=dict(mode="OSTINATO", rhythm="......x.....x...", notes=(0, 2)),
         pad=dict(mode="FIGURE", base=52, tones=3,
                  figure=(3, R, 2, 4, R, 3, R, 2, 4, R, 3, R, 1, 3, R, 2)),
         groove="G70_SALSA", orch=[("EBASS", "TRUMPET", "PIANO"), ("EBASS", "BRASS", "PIANO")]),
    dict(id="OLD_SCHOOL_HIP_HOP", name="BLOCK PARTY", decade=1970, year=1973,
         comment="Kool Herc, Grandmaster Flash in the Bronx: the break of a funk record looped on two turntables, "
                 "a funky drummer beat, a one-chord minor groove, chopped guitar and a horn stab hook cut back "
                 "and forth",
         key="E", scale="DORIAN", bpm=100, div=4, chord_beats=4, period_bars=4, swing=0.2, swing16=True,
         vocab="BLUES", harmony="CYCLE", cycle=["i7", "i7", "i7", "iv7"],
         melody=dict(mode="PATTERN", base=64,
                     pattern=(0, None, None, 0, None, 2, None, None, 3, 2, None, None, None, None, None, None)),
         bass=dict(mode="OSTINATO", rhythm="x.x...x..xx...x.", notes=(0, 0, 12, 10, 7, 12), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="..x..x....x..x..x.x..x....x..x.x", gate=1, base=58, tones=3),
         groove="G70_BREAK", orch=[("EBASS", "BRASS", "EGUITAR"), ("EBASS", "SAX", "CLAV")]),
    dict(id="KRAUTROCK", name="KRAUTROCK", decade=1970, year=1971,
         comment="Neu!, Can, Kraftwerk's Autobahn: the motorik beat forever, a hypnotic eighth-note bass on one "
                 "chord, a sequencer figure, drones that barely change",
         key="D", scale="MIXOLYDIAN", bpm=142, div=4, chord_beats=4, period_bars=16,
         vocab="MODAL", harmony="CYCLE", cycle=["I", "I", "I", "I", "I", "I", "bVII", "I"],
         cycle2=["I", "I", "IV", "I"], cycle_switch=3,
         melody=dict(mode="PATTERN", base=62,
                     pattern=(0, None, 4, None, 7, None, 4, None, 9, None, 7, None, 4, None, 2, None)),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G70_MOTORIK", orch_mode="CYCLE",
         orch=[("EBASS", "EGUITAR", "GUITAR"), ("WAVEBASS", "SYNTH", "SINEKEY"), ("WAVEBASS", "CHIP", "ORGAN")]),
]
