# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 1960s (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings are
# its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop.

E8 = "x.x.x.x.x.x.x.x."              # eighths in a bar of sixteenths
OFF8 = "..x...x...x...x."             # the off-beat eighths of a bar


def two(a, b=None):
    """a loop of two bars"""
    return a + (a if b is None else b)


GROOVES = {
    # Stax, Al Jackson Jr.: a fat, slightly late backbeat, the kick on 1, the and of 2 and 3
    "G60_STAX": dict(kit="STD", kick="ROUND", tune=60, tone=58, decay=60, snap=92, lanes=[
        ("KICK", 100, two("x.....x.x.......", "x.....x.x.....x.")),
        ("SNARE", 100, two("....X.......X...")),
        ("HAT", 66, two(E8)),
        ("OHAT", 58, two("................", "..............x.")),
    ]),
    # Motown, Benny Benjamin: the snare on all four beats, the tambourine with it, eighth hats
    "G60_MOTOWN": dict(kit="STD", kick="ROUND", tune=64, decay=52, snap=84, lanes=[
        ("KICK", 94, two("x.......x.x.....", "x.......x.x...x.")),
        ("SNARE", 90, two("x...X...x...X...")),
        ("TAMB", 80, two("x...X...x...X...")),
        ("HAT", 56, two(E8)),
    ]),
    # surf: the pounding floor tom in eighths, a driving backbeat, tom sixteenths at the loop's end (Wipe Out)
    "G60_SURF": dict(kit="STD", kick="PUNCH", tune=70, decay=48, snap=80, lanes=[
        ("KICK", 96, two("x.......x.x.....")),
        ("SNARE", 98, two("....X.......X...")),
        ("FLOOR_TOM", 74, two("x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.....")),
        ("TOM_HI", 82, two("................", "............xxxx")),
        ("RIDE", 60, two("x...x...x...x...")),
    ]),
    # Merseybeat, Ringo: washy half-open hats in eighths, the backbeat with a pickup into the next phrase
    "G60_MERSEY": dict(kit="STD", kick="ROUND", tune=66, decay=50, snap=78, lanes=[
        ("KICK", 92, two("x.......x.......", "x.......x.x.....")),
        ("SNARE", 94, two("....X.......X...", "....X.......X.x.")),
        ("OHAT", 62, two(E8)),
        ("CRASH", 76, "x..............................."),
    ]),
    # folk rock (the Byrds): a light backbeat under a jangling tambourine in eighths
    "G60_FOLKROCK": dict(kit="STD", kick="ROUND", tune=60, tone=54, decay=50, snap=60, lanes=[
        ("KICK", 86, two("x.......x.x.....")),
        ("SNARE", 84, two("....X.......X...")),
        ("TAMB", 70, two("x.X.x.X.x.X.x.X.")),
        ("HAT", 48, two("x...x...x...x...")),
    ]),
    # garage: a pounding kick on every beat, trashy open hats, gritty
    "G60_GARAGE": dict(kit="STD", kick="PUNCH", tune=62, decay=56, snap=96, drive=30, lanes=[
        ("KICK", 100, two("x...x...x...x...", "x...x...x.x.x...")),
        ("SNARE", 104, two("....X.......X...")),
        ("OHAT", 70, two(E8)),
        ("CRASH", 84, "x..............................."),
    ]),
    # psychedelic (Tomorrow Never Knows): a hypnotic tom-laden loop under a ride
    "G60_PSYCH": dict(kit="STD", kick="ROUND", tune=56, tone=50, decay=70, snap=70, lanes=[
        ("KICK", 98, two("x.....x...x.....", "x.....x...x...x.")),
        ("SNARE", 92, two("....X.......X...")),
        ("TOM_MID", 76, two("..x.......x.....", "..x.......x.....")),
        ("FLOOR_TOM", 84, two("..............x.", "...........x..xx")),
        ("RIDE", 62, two(E8)),
        ("CRASH", 70, "x..............................."),
    ]),
    # ska: kick on 1 and 3, the rim on 2 and 4, open hats on every off-beat
    "G60_SKA": dict(kit="STD", kick="ROUND", tune=68, decay=44, snap=86, lanes=[
        ("KICK", 90, two("x.......x.......")),
        ("RIM", 92, two("....x.......x...")),
        ("OHAT", 70, two(OFF8)),
        ("HAT", 50, two("x...x...x...x...")),
        ("SNARE", 70, two("................", "............xxx.")),
    ]),
    # rocksteady: slower, beat 3 marked by the kick and the rim (the one drop to come), soft eighth hats
    "G60_ROCKSTEADY": dict(kit="STD", kick="ROUND", tune=58, tone=52, decay=58, snap=60, fill=0, lanes=[
        ("KICK", 88, two("x.......X.......")),
        ("RIM", 90, two("........X.......", "........X.....x.")),
        ("HAT", 58, two("x.x.x.x.x.x.x.x.")),
        ("OHAT", 52, two("......x.........", "......x.......x.")),
    ]),
    # James Brown (Cold Sweat, Clyde Stubblefield and Jabo Starks): the ONE, syncopated kicks, ghost snares
    "G60_FUNK": dict(kit="STD", kick="PUNCH", tune=66, tone=72, decay=46, snap=104, lanes=[
        ("KICK", 104, two("X.........x.....", "X......x..x.....")),
        ("SNARE", 100, two("....X.......X...", "....X.......X...")),
        ("SNARE", 74, two(".g.....g.g....g.", ".g.....g.g...g.g")),
        ("HAT", 70, two(E8)),
        ("OHAT", 60, two("................", "..............x.")),
    ]),
}

GENRES = [
    dict(id="SOUL_60S", name="SOUL", decade=1960, year=1962,
         comment="Stax and Memphis, Otis Redding, Sam & Dave: a horn section answering, the organ under it, "
                 "Duck Dunn's bass on the root and octave, Al Jackson's heavy backbeat",
         key="C", scale="MIXOLYDIAN", bpm=106, div=4, chord_beats=4, period_bars=8, swing=0.12,
         vocab="BLUES", fifth_bias=1.5,
         weights={"I": 10, "I7": 6, "IV": 8, "IV7": 5, "V7": 5, "bVII": 4, "vi": 3, "ii": 2, "I9": 2},
         melody=dict(density=0.45, base=60, range=14, sync=0.4, add=("b3", "b7"), staccato=0.25, motif=0.75,
                     profile=(0.25, 0.65, 0.85, 0.95)),
         bass=dict(mode="OSTINATO", rhythm="x.....x.x.....x.", notes=(0, 12, 0, 7), semis=True),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G60_STAX", orch=[("EBASS", "BRASS", "PIANO"), ("EBASS", "SAX", "ORGAN"),
                                  ("EBASS", "TRUMPET", "ORGAN")], orch_mode="CYCLE"),
    dict(id="MOTOWN", name="MOTOWN", decade=1960, year=1963,
         comment="Holland-Dozier-Holland, the Supremes, the Four Tops: the snare and tambourine on every beat, "
                 "Jamerson's busy melodic bass, piano pounding quarters, I vi ii V",
         key="C", scale="IONIAN", bpm=124, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["I", "vi", "ii7", "V", "I", "iii", "IV", "V"],
         melody=dict(density=0.55, base=64, range=12, sync=0.35, profile=(0.2, 0.7, 0.88, 0.96), motif=0.75,
                     staccato=0.1),
         bass=dict(mode="OSTINATO", rhythm="x..x..x.x.x..x.x", notes=(0, 12, 7, 5, 7, 12, 7, 0), semis=True),
         pad=dict(mode="RHYTHM", rhythm=two("x...x...x...x..."), gate=2, base=57, tones=3),
         groove="G60_MOTOWN", orch=[("EBASS", "SAX", "PIANO"), ("EBASS", "VOICE", "PIANO"),
                                    ("EBASS", "BELLS", "PIANO")]),
    dict(id="SURF_ROCK", name="SURF ROCK", decade=1960, year=1961, reverb="SPRING",
         comment="Dick Dale, the Ventures, the Chantays: a reverb guitar tremolo-picking down the chord, the "
                 "Andalusian i VII VI V, pounding floor toms",
         key="E", scale="AEOLIAN", bpm=160, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VII", "VI", "V"],
         melody=dict(mode="PATTERN", base=55, pattern_chord=True,
                     pattern=[3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2]),
         bass=dict(mode="OSTINATO", rhythm=E8, notes=(0, 0, 7, 7, 12, 12, 7, 7), semis=True),
         pad=dict(mode="RHYTHM", rhythm=two("x..x..x...x..x.."), gate=1, base=52, tones=3, strum_ms=4),
         groove="G60_SURF", orch=[("EBASS", "EGUITAR", "GUITAR"), ("EBASS", "EGUITAR", "ROCKORGAN")]),
    dict(id="MERSEYBEAT", name="MERSEYBEAT", decade=1960, year=1963,
         comment="the early Beatles, the Searchers, Gerry and the Pacemakers: chugging rhythm guitar eighths, a "
                 "melodic Hofner bass, washy hats, the vi and iii in a major-key pop song",
         key="G", scale="IONIAN", bpm=150, div=4, chord_beats=4, period_bars=8,
         vocab="POP", fifth_bias=1.4,
         weights={"I": 10, "vi": 7, "IV": 8, "V": 8, "ii": 4, "iii": 4, "bVII": 2, "bVI": 2, "V/vi": 2},
         melody=dict(density=0.55, base=64, range=12, sync=0.3, profile=(0.25, 0.7, 0.88, 0.96), motif=0.8),
         bass=dict(mode="OSTINATO", rhythm="x...x...x...x.x.", notes=(0, 7, 12, 7, 0), semis=True),
         pad=dict(mode="RHYTHM", rhythm=two(E8), gate=1, base=55, tones=3, strum_ms=5),
         groove="G60_MERSEY", orch=[("EBASS", "VOICE", "EGUITAR"), ("EBASS", "EGUITAR", "GUITAR")]),
    dict(id="FOLK_ROCK", name="FOLK ROCK", decade=1960, year=1965,
         comment="the Byrds, Dylan electric, the Mamas & the Papas: a jangling twelve-string arpeggio, sus chords, "
                 "the tambourine, close vocal harmony",
         key="D", scale="IONIAN", bpm=118, div=4, chord_beats=4, period_bars=8,
         vocab="POP", fifth_bias=1.2,
         weights={"I": 10, "IV": 9, "V": 6, "Vsus4": 3, "ii": 4, "vi": 4, "Isus2": 3, "Iadd9": 3, "bVII": 3},
         melody=dict(density=0.4, base=62, range=12, sync=0.2, profile=(0.3, 0.75, 0.9, 0.97), motif=0.7,
                     min_ticks=4),
         bass=dict(mode="OSTINATO", rhythm="x.......x...x...", notes=(0, 7, 12), semis=True),
         pad=dict(mode="FIGURE", figure=(0, 1, 2, 3, 4, 3, 2, 1), base=55, tones=3),
         groove="G60_FOLKROCK", orch=[("EBASS", "VOICE", "EGUITAR"), ("EBASS", "FLUTE", "EGUITAR")]),
    dict(id="GARAGE_ROCK", name="GARAGE ROCK", decade=1960, year=1965, reverb="SPRING",
         comment="the Kingsmen, the Sonics, ? and the Mysterians, Them: three chords (I bVII IV, Gloria), fuzz "
                 "power chords in the Louie Louie 1-2-3, 1-2, a cheap Farfisa organ hook",
         key="E", scale="MIXOLYDIAN", bpm=140, div=4, chord_beats=2, period_bars=4,
         vocab="BLUES", harmony="CYCLE", cycle=["I5", "bVII5", "IV5", "IV5"],
         melody=dict(density=0.6, base=64, range=10, sync=0.35, add=("b3",), staccato=0.4, motif=0.9,
                     motif_use=0.9, profile=(0.35, 0.75, 0.9, 0.97)),
         bass=dict(mode="OSTINATO", rhythm=E8, notes=(0,)),
         pad=dict(mode="RHYTHM", rhythm=two("x.x.x...x.x....."), gate=1, base=52, tones=3, power=True),
         groove="G60_GARAGE", orch=[("EBASS", "ROCKORGAN", "DISTGUITAR"), ("EBASS", "EGUITAR", "DISTGUITAR")]),
    dict(id="PSYCHEDELIC", name="PSYCHEDELIC", decade=1960, year=1966, reverb="SPRING",
         comment="Revolver to the Doors and Hendrix: a Dorian vamp i IV over a droning bass, swirling organ, a "
                 "long fuzz-guitar line, a hypnotic tom loop",
         key="A", scale="DORIAN", bpm=104, div=4, chord_beats=4, period_bars=8,
         vocab="MODAL", harmony="CYCLE", cycle=["i", "i", "IV", "IV"], cycle2=["i", "bVII", "IV", "i"],
         cycle_switch=2, echo=0.2,
         melody=dict(density=0.45, base=62, range=17, sync=0.3, add=("b5",), min_ticks=4,
                     profile=(0.15, 0.6, 0.82, 0.94), max_leap=5, motif=0.5),
         bass=dict(mode="OSTINATO", rhythm="x.....x.x.......", notes=(0, 0, 12, 10), semis=True),
         pad=dict(mode="BLOCK", base=55, tones=3, spread=True),
         groove="G60_PSYCH", orch=[("EBASS", "DISTGUITAR", "ORGAN"), ("EBASS", "FLUTE", "ORGAN"),
                                   ("EBASS", "DISTGUITAR", "STRINGS")], orch_mode="CYCLE"),
    dict(id="SKA", name="SKA", decade=1960, year=1961, reverb="SPRING",
         comment="the Skatalites, Prince Buster, Millie: the guitar and piano chopping every off-beat, a walking "
                 "upright bass, a horn section on the melody",
         key="BB", scale="IONIAN", bpm=136, div=4, chord_beats=4, period_bars=8, swing=0.25,
         vocab="BLUES", harmony="CYCLE", cycle=["I", "I", "IV", "V7", "I", "vi", "IV", "V7"],
         melody=dict(density=0.55, base=62, range=14, sync=0.3, staccato=0.2, motif=0.75,
                     profile=(0.2, 0.68, 0.86, 0.95)),
         bass=dict(mode="WALKING"),
         pad=dict(mode="RHYTHM", rhythm=two(OFF8), gate=1, base=58, tones=3),
         groove="G60_SKA", orch=[("CONTRABASS", "TRUMPET", "PIANO"), ("CONTRABASS", "SAX", "GUITAR"),
                                 ("EBASS", "TRUMPET", "GUITAR")]),
    dict(id="ROCKSTEADY", name="ROCKSTEADY", decade=1960, year=1966, reverb="SPRING",
         comment="Alton Ellis, the Paragons, Phyllis Dillon: ska slowed down, the guitar on 2 and 4 only, a "
                 "heavy melodic bass with gaps, beat three marked, sweet minor-key harmony",
         key="A", scale="AEOLIAN", bpm=82, div=4, chord_beats=4, period_bars=8, swing=0.15,
         vocab="POP", harmony="CYCLE", cycle=["i", "i", "iv", "VII", "i", "VI", "iv", "v"],
         melody=dict(density=0.4, base=62, range=12, sync=0.35, profile=(0.25, 0.72, 0.9, 0.97), motif=0.75),
         bass=dict(mode="OSTINATO", rhythm="x.....x.x.x.....", notes=(0, 0, 1, 2), gate=3),
         pad=dict(mode="RHYTHM", rhythm=two("....x.......x..."), gate=2, base=57, tones=3),
         groove="G60_ROCKSTEADY", orch=[("EBASS", "SAX", "GUITAR"), ("EBASS", "TRUMPET", "PIANO")]),
    dict(id="FUNK_60S", name="FUNK", decade=1960, year=1967,
         comment="James Brown, Cold Sweat and Papa's Got a Brand New Bag: everything on the ONE, a ninth-chord "
                 "vamp, chicken-scratch guitar sixteenths, horn stabs, a bridge to IV",
         key="E", scale="MIXOLYDIAN", bpm=112, div=4, chord_beats=4, period_bars=16, swing=0.08, swing16=True,
         vocab="BLUES", harmony="CYCLE", cycle=["I9"] * 12 + ["IV9", "IV9", "I9", "V9"],
         melody=dict(mode="PATTERN", base=64, pattern_chord=True,
                     pattern=[3, None, None, None, None, None, 2, None, None, None, 3, None, None, None, None, None]),
         bass=dict(mode="OSTINATO", rhythm="x......x..x.x...", notes=(0, 10, 12, 7), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm=two("..x.x.xx..x.x.xx", "..x.x.xx..x.xx.x"), gate=1, base=60, tones=3),
         groove="G60_FUNK", orch=[("EBASS", "BRASS", "EGUITAR"), ("EBASS", "TRUMPET", "EGUITAR"),
                                  ("EBASS", "SAX", "EGUITAR")]),
]
