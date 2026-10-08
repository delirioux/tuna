# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 1950s (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings are
# its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop.

B16 = "x.x.x.x.x.x.x.x."              # eighths in a bar of sixteenths

GROOVES = {
    "ROCKNROLL": dict(kit="STD", kick="ROUND", tune=66, decay=58, snap=80, lanes=[
        ("KICK", 100, "x.......x.......x.......x......."),
        ("SNARE", 96, "....X.......X.......X.......X..."),
        ("HAT", 72, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 80, "x..............................."),
    ]),
    "SHUFFLE_TRIP": dict(kit="STD", kick="ROUND", tune=62, decay=60, lanes=[    # 4/4 in triplets (12 a bar)
        ("KICK", 96, "x.....x.....x.....x....."),
        ("SNARE", 92, "...X.....X.....X.....X.."),
        ("RIDE", 70, "x.xx.xx.xx.xx.xx.xx.xx.x"),
        ("PHAT", 60, "...x.....x.....x.....x.."),
    ]),
    "DOOWOP": dict(kit="STD", kick="ROUND", tune=60, tone=50, decay=50, fill=0, lanes=[   # 12/8 ballad
        ("KICK", 80, "x.....x.....x.....x....."),
        ("SNARE", 78, "...x.....x.....x.....x.."),
        ("HAT", 58, "xxxxxxxxxxxxxxxxxxxxxxxx"),
    ]),
    "ROCKABILLY": dict(kit="STD", kick="ROUND", tune=70, decay=40, snap=90, lanes=[
        ("KICK", 92, "x.......x.......x.......x......."),
        ("SNARE", 94, "....X.......X.......X.......X.x."),
        ("SNARE", 50, "..g...g...g...g...g...g...g...g."),
        ("HAT", 64, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
    "SOUL50": dict(kit="STD", kick="ROUND", tune=60, decay=55, lanes=[
        ("KICK", 96, "x.....x.x.......x.....x.x......."),
        ("SNARE", 94, "....X.......X.......X.......X..."),
        ("TAMB", 70, "....x.......x.......x.......x..."),
        ("HAT", 66, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
    "BOOMCHICK": dict(kit="STD", kick="ROUND", tune=62, tone=48, decay=45, snap=40, fill=0, lanes=[
        ("KICK", 84, "x.......x.......x.......x......."),
        ("SNARE", 80, "....x.......x.......x.......x..."),
        ("HAT", 56, "..x...x...x...x...x...x...x...x."),
    ]),
    "JAZZRIDE": dict(kit="STD", kick="ROUND", tune=58, tone=56, decay=70, fill=0, lanes=[   # swung, in triplets
        ("RIDE", 76, "x..x.xx..x.xx..x.xx..x.x"),
        ("PHAT", 66, "...x.....x.....x.....x.."),
        ("KICK", 50, "g.....g.....g.....g....."),
        ("SNARE", 50, ".....g..........g..g...."),
    ]),
}

TWELVE_BAR = ["I7", "I7", "I7", "I7", "IV7", "IV7", "I7", "I7", "V7", "IV7", "I7", "V7"]

GENRES = [
    dict(id="ROCK_AND_ROLL", name="ROCK & ROLL", decade=1950, year=1954,
         comment="Chuck Berry, Little Richard: a twelve-bar blues at full speed, boogie bass, pounding piano eighths, "
                 "the backbeat",
         key="A", scale="MIXOLYDIAN", bpm=168, div=4, chord_beats=4, period_bars=12, swing=0.3,
         vocab="BLUES", harmony="CYCLE", cycle=TWELVE_BAR,
         melody=dict(density=0.6, base=64, range=14, sync=0.35, add=("b3",), staccato=0.2, motif=0.7),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0, 4, 7, 9, 10, 9, 7, 4), semis=True),
         pad=dict(mode="RHYTHM", rhythm=B16 * 2, gate=1, base=57, tones=3),
         groove="ROCKNROLL", orch=[("EBASS", "SAX", "PIANO"), ("EBASS", "EGUITAR", "PIANO")]),
    dict(id="RHYTHM_AND_BLUES", name="R&B 50S", decade=1950, year=1949,
         comment="Fats Domino, Ruth Brown: a triplet shuffle, piano triplets, a walking boogie bass, the sax",
         key="BB", scale="MIXOLYDIAN", bpm=112, div=3, chord_beats=4, period_bars=12,
         vocab="BLUES", harmony="CYCLE", cycle=TWELVE_BAR,
         melody=dict(density=0.5, base=62, range=14, sync=0.3, add=("b3", "b5"), motif=0.7),
         bass=dict(mode="OSTINATO", rhythm="x..x..x..x..", notes=(0, 4, 7, 9), semis=True),
         pad=dict(mode="RHYTHM", rhythm="xxx" * 8, gate=1, base=58, tones=3),
         groove="SHUFFLE_TRIP", orch=[("EBASS", "SAX", "PIANO"), ("CONTRABASS", "TRUMPET", "PIANO")]),
    dict(id="DOO_WOP", name="DOO-WOP", decade=1950, year=1954, reverb="HALL",
         comment="the Penguins, the Platters: the I vi IV V of the fifties in a slow 12/8, piano triplets under "
                 "'ooh' harmonies, a bass singer walking",
         key="EB", scale="IONIAN", bpm=68, div=3, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["I", "vi", "IV", "V"],
         melody=dict(density=0.4, base=62, range=12, sync=0.1, profile=(0.2, 0.75, 0.9, 0.97), motif=0.8,
                     appoggiatura=0.2),
         bass=dict(mode="OSTINATO", rhythm="x..x..x..x..", notes=(0, 7, 12, 7), semis=True),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="DOOWOP", orch=[("EBASS", "VOICE", "CHOIR"), ("EBASS", "SAX", "CHOIR")]),
    dict(id="ROCKABILLY", name="ROCKABILLY", decade=1950, year=1955, reverb="SPRING",
         comment="Elvis at Sun, Carl Perkins: slapped upright bass on root and fifth, twangy guitar licks, "
                 "a light shuffle, brushes",
         key="E", scale="MIXOLYDIAN", bpm=176, div=4, chord_beats=4, period_bars=12, swing=0.45,
         vocab="BLUES", harmony="CYCLE", cycle=TWELVE_BAR,
         melody=dict(density=0.65, base=64, range=12, sync=0.4, add=("b3",), staccato=0.3,
                     profile=(0.25, 0.7, 0.88, 0.96)),
         bass=dict(mode="BEAT"),
         pad=dict(mode="RHYTHM", rhythm="..x...x...x...x...x...x...x...x.", gate=1, base=52, tones=3),
         groove="ROCKABILLY", orch=[("PIZZ", "EGUITAR", "EGUITAR"), ("CONTRABASS", "EGUITAR", "PIANO")]),
    dict(id="EARLY_SOUL", name="EARLY SOUL", decade=1950, year=1954,
         comment="Ray Charles, Sam Cooke: gospel brought to the dance hall, organ and piano, call and response, "
                 "the tambourine on the backbeat",
         key="BB", scale="IONIAN", bpm=126, div=4, chord_beats=4, period_bars=8, swing=0.25,
         vocab="BLUES", weights={"I": 10, "I7": 6, "IV": 8, "IV7": 5, "V": 5, "V7": 7, "vi": 4, "ii": 3, "I6": 3},
         melody=dict(density=0.5, base=63, range=14, sync=0.35, add=("b3", "b7"), turn=0.15, motif=0.7),
         bass=dict(mode="WALKING"),
         pad=dict(mode="BLOCK", base=55, tones=4),
         groove="SOUL50", orch=[("EBASS", "VOICE", "GOSPEL"), ("EBASS", "SAX", "PIANO")]),
    dict(id="COUNTRY", name="COUNTRY", decade=1950, year=1949,
         comment="Hank Williams' honky-tonk, a parent in the background: boom-chick bass and guitar, the fiddle, "
                 "I IV V",
         key="G", scale="IONIAN", bpm=116, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["I", "I", "IV", "IV", "V", "V", "I", "I"],
         melody=dict(density=0.5, base=64, range=12, sync=0.15, profile=(0.25, 0.75, 0.9, 0.97), turn=0.1),
         bass=dict(mode="BEAT"),
         pad=dict(mode="RHYTHM", rhythm="....x.......x.......x.......x...", gate=2, base=55, tones=3, strum_ms=8),
         groove="BOOMCHICK", orch=[("CONTRABASS", "VIOLIN", "GUITAR"), ("CONTRABASS", "EGUITAR", "GUITAR")]),
    dict(id="JAZZ_50S", name="JAZZ 50S", decade=1950, year=1955,
         comment="hard bop and cool, the other parent: the ride's swing, a walking bass, ii V I, sparse comping",
         key="F", scale="IONIAN", bpm=168, div=3, chord_beats=4, period_bars=8,
         vocab="JAZZ", fifth_bias=3.0, repeat=0.3,
         weights={"Imaj7": 10, "vi7": 6, "iii7": 4, "IVmaj7": 5, "ii7": 9, "ii9": 4, "V9": 6, "V13": 5, "V7b9": 4,
                  "II9": 4, "VI7b9": 3, "I6": 4, "bII7": 2},
         weights_minor={"i6": 10, "iiø7": 8, "V7b9": 8, "iv7": 5, "VImaj7": 4, "im9": 3},
         melody=dict(density=0.7, base=62, range=17, sync=0.45, chromatic=True, appoggiatura=0.2,
                     profile=(0.1, 0.62, 0.84, 0.95), max_leap=5, add=("b3", "b7", "b5")),
         bass=dict(mode="WALKING"),
         pad=dict(mode="RHYTHM", rhythm="..x.........x.x.........", gate=2, base=55, tones=4),
         groove="JAZZRIDE", orch=[("CONTRABASS", "SAX", "PIANO"), ("CONTRABASS", "TRUMPET", "PIANO")]),
]
