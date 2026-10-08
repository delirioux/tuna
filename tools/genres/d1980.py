# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 1980s (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings are
# its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop.

E8 = "x.x.x.x.x.x.x.x."               # eighths in a bar of sixteenths
S16 = "xxxxxxxxxxxxxxxx"              # sixteenths in a bar
OFF8 = "..x...x...x...x."             # the off-eighths in a bar

GROOVES = {
    # new wave: tight, dry, straight eighths on the hat, the kick pushing the 'and' of three, an open hat lift
    "G80_NEWWAVE": dict(kit="STD", kick="PUNCH", tune=70, tone=72, decay=40, snap=90, lanes=[
        ("KICK", 104, "x.......x.x.....x.......x.x....."),
        ("SNARE", 100, "....X.......X.......X.......X..."),
        ("HAT", 74, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x..."),
        ("OHAT", 70, "..............................x."),
        ("CRASH", 80, "x..............................."),
    ]),
    # post-punk: Stephen Morris' tom beat: floor tom driving sixteenths, kick and snare under it, no ride
    "G80_POSTPUNK": dict(kit="STD", kick="ROUND", tune=60, tone=46, decay=52, snap=70, lanes=[
        ("KICK", 100, "x.......x.......x.......x......."),
        ("SNARE", 96, "....X.......X.......X.......X..."),
        ("FLOOR_TOM", 84, "x.xx..xxx.xx..xxx.xx..xxx.xx..x."),
        ("TOM_MID", 70, "..............................x."),
        ("HAT", 58, "..x...x...x...x...x...x...x...x."),
    ]),
    # synth-pop: a drum machine (707-like), kick on the beats, claps on 2 and 4, machine sixteenths
    "G80_SYNTHPOP": dict(kit="10", tune=62, tone=60, decay=50, snap=70, lanes=[
        ("KICK", 104, "x.......x.....x.x.......x......."),
        ("SNARE", 96, "....X.......X.......X.......X..."),
        ("CLAP", 86, "....x.......x.......x.......x..."),
        ("HAT", 66, "xgxgxgxgxgxgxgxgxgxgxgxgxgxgxgxg"),
        ("OHAT", 60, "..............x..............x.."),
    ]),
    # hardcore punk: the skank beat, kick on the beats, snare on every off-eighth, crash-ride bashed
    "G80_HARDCORE": dict(kit="STD", kick="PUNCH", tune=70, tone=80, decay=34, snap=96, drive=50, lanes=[
        ("KICK", 104, "x...x...x...x...x...x...x...x..."),
        ("SNARE", 100, "..X...X...X...X...X...X...X...X."),
        ("RIDE", 76, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 86, "x.......x.......x..............."),
    ]),
    # thrash: double-kick gallops, the snare on 2 and 4, the china on the beats
    "G80_THRASH": dict(kit="STD", kick="PUNCH", tune=72, tone=88, decay=30, snap=100, drive=70, lanes=[
        ("KICK", 108, "x.xxx.xxx.xxx.xxx.xxx.xxx.xxx.xx"),
        ("SNARE", 104, "....X.......X.......X.......X..."),
        ("HAT", 70, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CHINA", 84, "x.......x.......x.......x......."),
        ("CRASH", 92, "x..............................."),
    ]),
    # glam metal: the big gated snare, a solid kick, eighth hats, crashes on the downbeats
    "G80_GLAM": dict(kit="STD", kick="PUNCH", tune=58, tone=70, decay=58, snap=110, drive=30, lanes=[
        ("KICK", 106, "x.......x.x.....x.......x.x....."),
        ("SNARE", 110, "....X.......X.......X.......X..."),
        ("HAT", 72, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 92, "x...............x..............."),
        ("TOM_HI", 76, "............................xx.."),
        ("FLOOR_TOM", 80, "..............................xx"),
    ]),
    # gothic rock: Doktor Avalanche, a drum machine with a pounding floor tom, sixteenth hats, the open hat on the 'ands'
    "G80_GOTH": dict(kit="10", tune=56, tone=50, decay=62, snap=80, lanes=[
        ("KICK", 104, "x...x...x...x...x...x...x...x..."),
        ("SNARE", 100, "....X.......X.......X.......X..."),
        ("HAT", 62, "xxgxxxgxxxgxxxgxxxgxxxgxxxgxxxgx"),
        ("OHAT", 64, "..x.......x.......x.......x....."),
        ("FLOOR_TOM", 84, "x.....x.......x.x.....x.......x."),
    ]),
    # alternative / indie: live, light and loose, the tambourine on the backbeat, sixteenth hats with ghosts
    "G80_INDIE": dict(kit="STD", kick="ROUND", tune=64, tone=60, decay=48, snap=74, lanes=[
        ("KICK", 96, "x.....x.x.......x.....x.x......."),
        ("SNARE", 94, "....X.......X.......X.......X..."),
        ("SNARE", 44, "..........g..g............g....g"),
        ("HAT", 64, "xgxgxgxgxgxgxgxgxgxgxgxgxgxgxgxg"),
        ("TAMB", 72, "....x.......x.......x.......x..."),
    ]),
    # Chicago house: the 909 four-on-the-floor, the clap on 2 and 4, the open hat on every off-beat
    "G80_HOUSE": dict(kit="10", tune=62, tone=64, decay=60, snap=64, fill=0, lanes=[
        ("KICK", 110, "x...x...x...x...x...x...x...x..."),
        ("CLAP", 94, "....X.......X.......X.......X..."),
        ("OHAT", 76, "..x...x...x...x...x...x...x...x."),
        ("HAT", 56, "g.g.g.g.g.g.g.g.g.g.g.g.g.g.g.g."),
        ("TAMB", 54, ".x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x"),
    ]),
    # Detroit techno: the 909 four-on-the-floor, machine-funk rims and claps off the grid, sixteenth hats
    "G80_TECHNO": dict(kit="10", tune=66, tone=70, decay=48, snap=72, fill=0, lanes=[
        ("KICK", 110, "x...x...x...x...x...x...x...x..."),
        ("CLAP", 90, "....X.......X.......X.......X..."),
        ("RIM", 74, "...x..x....x.x.....x..x...x..x.."),
        ("HAT", 62, "gxgxgxgxgxgxgxgxgxgxgxgxgxgxgxgx"),
        ("OHAT", 70, "..x...x...x...x...x...x...x...x."),
        ("COWBELL", 56, "......x.........x.....x........."),
    ]),
    # industrial / EBM: a distorted machine, kick on the beats, a crushing snare, metal (china) on the off-beats
    "G80_INDUSTRIAL": dict(kit="10", tune=54, tone=82, decay=46, snap=110, drive=110, lanes=[
        ("KICK", 112, "x...x...x...x...x...x...x...x..x"),
        ("SNARE", 108, "....X.......X.......X.......X..."),
        ("CLAP", 90, "....x.......x.......x.......x..."),
        ("CHINA", 74, "..x...x...x...x...x...x...x...x."),
        ("HAT", 60, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
    ]),
    # new jack swing: swung sixteenths, a syncopated kick, the hard snare-clap on 2 and 4, rimshot pickups
    "G80_NJS": dict(kit="80", tune=64, tone=72, decay=56, snap=100, fill=0, lanes=[
        ("KICK", 108, "x..x..x...x..x..x..x..x....x.x.."),
        ("SNARE", 104, "....X.......X.......X.......X..."),
        ("CLAP", 96, "....X.......X.......X.......X..."),
        ("HAT", 70, "xxxgxxxgxxxgxxxgxxxgxxxgxxxgxxxg"),
        ("RIM", 70, ".......x..........x....x.......x"),
    ]),
    # golden-age hip hop: the 808 boom on a broken kick, the snare-clap hard on 2 and 4, sparse eighth hats
    "G80_HIPHOP": dict(kit="80", tune=56, tone=58, decay=100, snap=90, fill=0, lanes=[
        ("KICK", 112, "x......x..x.....x.....x...x....."),
        ("SNARE", 106, "....X.......X.......X.......X..."),
        ("CLAP", 84, "....x.......x.......x.......x..."),
        ("HAT", 66, "x.x.x.x.x.x.x.xxx.x.x.x.x.x.x.x."),
        ("COWBELL", 58, "..........x.................x..."),
    ]),
}

GENRES = [
    dict(id="NEW_WAVE_80S", name="NEW WAVE", decade=1980, year=1978,
         comment="the Cars, Blondie, the Police: tight driving eighths on the bass, choppy off-beat guitar, a "
                 "bright synth hook over I V vi IV",
         key="E", scale="IONIAN", bpm=148, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["I", "V", "vi", "IV"],
         melody=dict(density=0.55, base=67, range=12, sync=0.35, staccato=0.35, motif=0.75,
                     profile=(0.25, 0.7, 0.88, 0.96)),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm=OFF8 * 2, gate=1, base=57, tones=3),
         groove="G80_NEWWAVE", orch=[("EBASS", "SYNTH", "EGUITAR"), ("EBASS", "SINEKEY", "EGUITAR")]),
    dict(id="POST_PUNK_80S", name="POST-PUNK", decade=1980, year=1978,
         comment="Joy Division, early Cure, Gang of Four: the bass high and melodic as the lead (Peter Hook), the "
                 "floor-tom beat, stark minor chords stabbed by a thin guitar",
         key="A", scale="AEOLIAN", bpm=150, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "i", "VII"],
         melody=dict(mode="PATTERN", base=57, range=12, min_ticks=1,
                     pattern=(4, "-", 4, 3, 2, "-", 0, "-", 4, "-", 4, 5, 4, "-", 2, "-")),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm="x.....x.......x.x.....x.......x.", gate=2, base=57, tones=3),
         groove="G80_POSTPUNK", orch=[("EBASS", "EBASS", "EGUITAR"), ("WAVEBASS", "EBASS", "EGUITAR")]),
    dict(id="SYNTH_POP_80S", name="SYNTH-POP", decade=1980, year=1980,
         comment="Depeche Mode, the Human League, OMD: drum machine, a sequenced octave synth bass, a bright "
                 "arpeggio and a synth hook, minor i VI III VII",
         key="C", scale="AEOLIAN", bpm=122, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "III", "VII"],
         melody=dict(density=0.5, base=67, range=12, sync=0.3, motif=0.8, profile=(0.25, 0.72, 0.9, 0.97)),
         bass=dict(mode="OSTINATO", rhythm=E8, notes=(0, 12), semis=True, gate=1),
         pad=dict(mode="ARPEGGIO", base=60, tones=3),
         groove="G80_SYNTHPOP", orch=[("WAVEBASS", "SYNTH", "PLUCKSYNTH"), ("WAVEBASS", "SINEKEY", "CHIP")]),
    dict(id="HARDCORE_PUNK_80S", name="HARDCORE PUNK", decade=1980, year=1981,
         comment="Black Flag, Minor Threat, Dead Kennedys: the skank beat flat out, distorted power chords in "
                 "eighths, the bass doubling the roots, a chord every two beats",
         key="E", scale="AEOLIAN", bpm=200, div=4, chord_beats=2, period_bars=4,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "VI5", "VII5", "i5", "iv5", "VI5", "VII5", "VII5"],
         melody=dict(density=0.3, base=64, range=10, sync=0.1, staccato=0.3, motif=0.8, min_ticks=4),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm=E8 * 2, gate=1, base=52, tones=3, power=True),
         groove="G80_HARDCORE", orch=[("WAVEBASS", "EGUITAR", "DISTGUITAR")]),
    dict(id="THRASH_METAL_80S", name="THRASH METAL", decade=1980, year=1983,
         comment="Metallica, Slayer, Megadeth: palm-muted sixteenth riffs on the low E with Phrygian half-steps, "
                 "double-kick gallops, power-chord stabs",
         key="E", scale="PHRYGIAN", bpm=176, div=4, chord_beats=8, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "i5", "bII5", "i5"],
         melody=dict(mode="PATTERN", base=40, range=12, min_ticks=1,
                     pattern=(0, 0, 0, 1, 0, 0, 3, None, 0, 0, 4, 3, 0, 0, 1, 2)),
         bass=dict(mode="OSTINATO", rhythm=E8, notes=(0,), semis=True, gate=1),
         pad=dict(mode="RHYTHM", rhythm="x.......x.......x.......x.....x.", gate=2, base=52, tones=3, power=True),
         groove="G80_THRASH", orch=[("WAVEBASS", "DISTGUITAR", "DISTGUITAR")]),
    dict(id="GLAM_METAL_80S", name="GLAM METAL", decade=1980, year=1983,
         comment="Def Leppard, Bon Jovi, Motley Crue: the huge gated snare mid-tempo, chugging power chords, an "
                 "anthem in major with bVII, a shredding guitar lead",
         key="E", scale="MIXOLYDIAN", bpm=126, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["I5", "bVII5", "IV5", "I5", "I5", "bVII5", "IV5", "V5"],
         melody=dict(density=0.65, base=64, range=17, sync=0.35, add=("b3",), motif=0.6,
                     profile=(0.15, 0.65, 0.85, 0.95)),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm=E8 * 2, gate=1, base=52, tones=3, power=True),
         groove="G80_GLAM", orch=[("EBASS", "DISTGUITAR", "DISTGUITAR"), ("EBASS", "DISTGUITAR", "SYNTH")]),
    dict(id="GOTHIC_ROCK_80S", name="GOTHIC ROCK", decade=1980, year=1981,
         comment="Sisters of Mercy, Siouxsie, Bauhaus: a machine beat with pounding toms, a deep driving bass, a "
                 "flanged guitar arpeggio circling the minor chord, choir in the background",
         key="D", scale="HARM_MINOR", bpm=132, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "iv", "V"],
         melody=dict(mode="PATTERN", base=62, range=14, pattern_chord=True, min_ticks=1,
                     pattern=(0, "-", 1, "-", 2, "-", 3, "-", 4, "-", 3, "-", 2, "-", 1, "-")),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G80_GOTH", orch=[("WAVEBASS", "EGUITAR", "CHOIR"), ("WAVEBASS", "SHIMMER", "CHOIR")]),
    dict(id="ALT_INDIE_80S", name="ALTERNATIVE", decade=1980, year=1983,
         comment="R.E.M., the Smiths, the Pixies: a jangling arpeggiated guitar in eighths, a melodic bass, "
                 "vi IV I V, a tambourine on the backbeat; louder sets bring the distortion",
         key="G", scale="IONIAN", bpm=134, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["vi", "IV", "I", "V"], cycle2=["I", "iii", "IV", "IV"],
         cycle_switch=2,
         melody=dict(density=0.5, base=64, range=12, sync=0.3, motif=0.7, profile=(0.2, 0.72, 0.9, 0.97)),
         bass=dict(mode="OSTINATO", rhythm="x.x.x.xxx.x.x.x.", notes=(0, 0, 2, 3, 0, 0, 2, 1)),
         pad=dict(mode="FIGURE", base=55, tones=3, figure=(0, 2, 3, 1, 2, 3, 1, 2), strum_ms=0),
         groove="G80_INDIE", orch_mode="ENERGY",
         orch=[("EBASS", "EGUITAR", "GUITAR"), ("EBASS", "EGUITAR", "EGUITAR"), ("EBASS", "DISTGUITAR", "EGUITAR")]),
    dict(id="CHICAGO_HOUSE", name="CHICAGO HOUSE", decade=1980, year=1985,
         comment="Frankie Knuckles, Marshall Jefferson, Larry Heard: the 909 four-on-the-floor, a jacking "
                 "syncopated bass, piano stabs on i7 IV in Dorian, an organ riff",
         key="A", scale="DORIAN", bpm=122, div=4, chord_beats=4, period_bars=8, swing=0.25, swing16=True,
         vocab="POP", harmony="CYCLE", cycle=["i7", "IV", "i7", "IV"],
         melody=dict(mode="PATTERN", base=64, range=12, min_ticks=1,
                     pattern=(4, None, None, 4, None, None, 6, None, None, 4, None, 3, None, 2, None, None)),
         bass=dict(mode="OSTINATO", rhythm="x..x..x...x..x.x", notes=(0, 0, 12, 0, 10, 0), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="..x..x....x...x...x..x....x..x..", gate=2, base=57, tones=4),
         groove="G80_HOUSE", orch=[("WAVEBASS", "ORGAN", "PIANO"), ("WAVEBASS", "SINEKEY", "PIANO")]),
    dict(id="DETROIT_TECHNO", name="DETROIT TECHNO", decade=1980, year=1987,
         comment="Juan Atkins, Derrick May, Model 500, Rhythim Is Rhythim: machine funk on the 909, a sequenced synth riff, "
                 "sweeping strings over minor sevenths, a syncopated sequencer bass",
         key="F", scale="AEOLIAN", bpm=128, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i7", "VImaj7", "iv7", "v7"],
         melody=dict(mode="PATTERN", base=55, range=12, pattern_chord=True, min_ticks=1,
                     pattern=(3, None, 2, 3, None, 2, None, 1, 3, None, 2, 3, None, 4, None, 2)),
         bass=dict(mode="OSTINATO", rhythm="x.x..x.x..x..x.x", notes=(0, 0, 0, 12, 0, 7, 0), semis=True, gate=1),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G80_TECHNO", orch=[("WAVEBASS", "PLUCKSYNTH", "STRINGS"), ("WAVEBASS", "CHIP", "PAD")]),
    dict(id="INDUSTRIAL_80S", name="INDUSTRIAL", decade=1980, year=1984,
         comment="Front 242, Nitzer Ebb, Skinny Puppy, early Ministry: a distorted machine, the EBM bass "
                 "sequencing sixteenths around the kick, Phrygian menace, metal hits and noise stabs",
         key="E", scale="PHRYGIAN", bpm=120, div=4, chord_beats=8, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "i5", "bII5", "i5"],
         melody=dict(mode="PATTERN", base=64, range=12, min_ticks=1,
                     pattern=(None, None, None, None, None, None, 1, 0, None, None, None, None, 4, None, 1, None)),
         bass=dict(mode="OSTINATO", rhythm=".xxx.xxx.xxx.xxx", notes=(0, 0, 12, 0, 0, 12, 1, 0), semis=True, gate=1),
         pad=dict(mode="RHYTHM", rhythm="x...............x.....x.........", gate=2, base=52, tones=3, power=True),
         groove="G80_INDUSTRIAL", orch=[("ACID", "GLITCH", "DISTGUITAR"), ("WAVEBASS", "GLITCH", "DISTGUITAR")]),
    dict(id="NEW_JACK_SWING", name="NEW JACK SWING", decade=1980, year=1987,
         comment="Teddy Riley, Guy, Bobby Brown, Keith Sweat: hard-swung sixteenths, a slamming snare, a "
                 "syncopated synth bass, brassy synth stabs, gospel minor sevenths",
         key="F#", scale="DORIAN", bpm=106, div=4, chord_beats=4, period_bars=8, swing=0.6, swing16=True,
         vocab="POP", harmony="CYCLE", cycle=["i7", "iv7", "i7", "v7", "i7", "iv7", "VII", "v7"],
         melody=dict(density=0.45, base=65, range=12, sync=0.5, staccato=0.3, add=("b5",), motif=0.75),
         bass=dict(mode="OSTINATO", rhythm="x..x..x...x..x..", notes=(0, 0, 10, 12, 7), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="......x..x....x.......x..x...x..", gate=1, base=58, tones=3),
         groove="G80_NJS", orch=[("WAVEBASS", "SINEKEY", "BRASS"), ("WAVEBASS", "SYNTH", "EPIANO")]),
    dict(id="GOLDEN_AGE_HIP_HOP", name="HIP HOP 80S", decade=1980, year=1986,
         comment="Run-DMC, LL Cool J, Eric B. & Rakim, Public Enemy: the 808 boom under a hard snare, a one-chord "
                 "loop, horn-stab samples and scratch-like chirps",
         key="G", scale="DORIAN", bpm=96, div=4, chord_beats=8, period_bars=8, swing=0.3, swing16=True,
         vocab="BLUES", harmony="CYCLE", cycle=["i7", "i7", "iv7", "i7"],
         melody=dict(mode="PATTERN", base=67, range=12, min_ticks=1,
                     pattern=(None, None, None, None, None, None, 4, 5, 4, None, None, None, 2, None, 0, None)),
         bass=dict(mode="808"),
         pad=dict(mode="RHYTHM", rhythm="..........x.........x.x.........", gate=2, base=57, tones=3),
         groove="G80_HIPHOP", orch=[("808", "GLITCH", "BRASS"), ("808", "CHIP", "PIANO")]),
]
