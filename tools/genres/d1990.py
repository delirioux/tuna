# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 1990s (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings are
# its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop.

B16 = "x.x.x.x.x.x.x.x."              # eighths in a bar of sixteenths
OFF = "..x...x...x...x."              # the off-beat eighths of a bar
FOUR = "x...x...x...x..."             # four on the floor, one bar

GROOVES = {
    # Nirvana, Soundgarden: a heavy rock beat, the kick pushing the 'and' of 3, open hats washing, crash on the one
    "G90_GRUNGE": dict(kit="STD", kick="PUNCH", tune=56, tone=58, decay=66, snap=90, drive=40, lanes=[
        ("KICK", 104, "x.....x...x.....x.....x...x.x..."),
        ("SNARE", 110, "....X.......X.......X.......X..."),
        ("OHAT", 64, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 84, "x..............................."),
    ]),
    # Oasis, Blur: a straight mid-tempo rock beat, the tambourine in sixteenths, the open hat on the 'and' of 4
    "G90_BRITPOP": dict(kit="STD", kick="ROUND", tune=62, decay=56, snap=70, lanes=[
        ("KICK", 98, "x.......x.x.....x.......x.x....."),
        ("SNARE", 100, "....X.......X.......X.......X..."),
        ("HAT", 66, "x.x.x.x.x.x.x...x.x.x.x.x.x.x..."),
        ("OHAT", 70, "..............x...............x."),
        ("TAMB", 58, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
    ]),
    # N.W.A to Mobb Deep: a hard sampled boom-bap break, the kick late in the bar, a cracking snare, dusty hats
    "G90_GANGSTA": dict(kit="STD", kick="PUNCH", tune=54, tone=52, decay=70, snap=100, drive=50, lanes=[
        ("KICK", 110, "x.........x..x....x.......x....."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("SNARE", 46, ".......g.........g........g....g"),
        ("HAT", 62, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.xx"),
    ]),
    # Dr. Dre, Warren G: a laid-back 808, the clap and snare on 2 and 4, swung sixteenth hats, an open hat
    "G90_GFUNK": dict(kit="80", tune=58, decay=80, snap=70, lanes=[
        ("KICK", 104, "x......x..x.....x.....x...x....."),
        ("SNARE", 96, "....X.......X.......X.......X..."),
        ("CLAP", 84, "....x.......x.......x.......x..."),
        ("HAT", 58, "xxxxxxxxxxxxxx.xxxxxxxxxxxxxxx.x"),
        ("OHAT", 62, "..............x...............x."),
        ("SHAKER", 40, "..x...x...x...x...x...x...x...x."),
    ]),
    # Jodeci, TLC, Aaliyah: a syncopated 808 kick, a snappy snare with a rim, skittering hats with rolls
    "G90_RNB": dict(kit="80", tune=62, decay=70, snap=90, roll=3, lanes=[
        ("KICK", 100, "x..x......x.....x..x...x..x....."),
        ("SNARE", 98, "....X.......X.......X.......X..."),
        ("RIM", 60, "......x..x........x.....x..x...."),
        ("HAT", 56, "x.xxx.x.x.xxx.x.x.xxx.x.x.xxx.xx"),
        ("OHAT", 50, "..........x...............x....."),
    ]),
    # Portishead, Massive Attack: a slow dusty break, heavy kick, a big lazy snare, little else
    "G90_TRIPHOP": dict(kit="STD", kick="ROUND", tune=46, tone=44, decay=78, snap=56, drive=64, fill=0, lanes=[
        ("KICK", 108, "x.......x.x.....x......x..x....."),
        ("SNARE", 104, "....X.......X.......X.......X..."),
        ("SNARE", 40, "...........g..........g........g"),
        ("HAT", 50, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("SHAKER", 34, "..x...x...x...x...x...x...x...x."),
    ]),
    # Goldie, LTJ Bukem, ragga jungle: the Amen break chopped at 165, ghosted snares, the ride riding through
    "G90_JUNGLE": dict(kit="STD", kick="PUNCH", tune=72, tone=70, decay=46, snap=110, lanes=[
        ("KICK", 104, "x.x.......xx....x.x.......x....."),
        ("SNARE", 108, "....X.....X..X......X.......X..x"),
        ("SNARE", 48, ".......g.g.....g.......g.g.g.g.."),
        ("RIDE", 60, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("HAT", 48, ".x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x"),
        ("CRASH", 70, "x..............................."),
    ]),
    # Roni Size, Ed Rush & Optical: the clean two-step at 174, the kick on 1 and the 'and' of 3, hard snare 2 and 4
    "G90_DNB": dict(kit="10", tune=66, decay=52, snap=96, drive=30, lanes=[
        ("KICK", 110, "x.........x.......x.......x....."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("SNARE", 40, ".......g.......g..............g."),
        ("HAT", 58, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("SHAKER", 44, ".x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x"),
    ]),
    # Paul van Dyk, ATB: four on the floor, the open hat on every off-beat, a clap on 2 and 4, sixteenth hats
    "G90_TRANCE": dict(kit="10", tune=64, decay=60, snap=70, lanes=[
        ("KICK", 112, FOUR * 2),
        ("OHAT", 74, OFF * 2),
        ("CLAP", 86, "....x.......x.......x.......x..."),
        ("HAT", 48, ".x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x"),
        ("CRASH", 70, "x..............................."),
    ]),
    # 2 Unlimited, Culture Beat, Corona: four on the floor, a clap and snare on 2 and 4, the off-beat hat
    "G90_EURODANCE": dict(kit="10", tune=68, decay=48, snap=84, lanes=[
        ("KICK", 110, FOUR * 2),
        ("SNARE", 90, "....x.......x.......x.......x..."),
        ("CLAP", 92, "....x.......x.......x.......x..."),
        ("OHAT", 70, OFF * 2),
        ("HAT", 52, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("TAMB", 46, "..x...x...x...x...x...x...x...x."),
    ]),
    # Korn, Limp Bizkit: a hip-hop-bounce kick under a cracking snare, the china on the accents
    "G90_NUMETAL": dict(kit="STD", kick="PUNCH", tune=52, tone=60, decay=58, snap=100, drive=60, lanes=[
        ("KICK", 112, "x.x...x..x.x....x.x...x...x.x..."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("HAT", 64, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CHINA", 76, "x...............x..............."),
    ]),
    # Green Day, Blink-182: fast and straight, kick on 1 and 3 and the 'and', snare 2 and 4, a crashing ride
    "G90_POPPUNK": dict(kit="STD", kick="PUNCH", tune=70, decay=44, snap=96, lanes=[
        ("KICK", 104, "x.....x.x.......x.....x.x......."),
        ("SNARE", 104, "....X.......X.......X.......X..."),
        ("OHAT", 70, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("CRASH", 80, "x...............x..............."),
    ]),
    # American Football, Cap'n Jazz: 5/4, one bar loop, a busy jazzy kit on the ride, the snare on 2 and 4.5
    "G90_EMO": dict(kit="STD", kick="ROUND", tune=64, tone=66, decay=50, snap=70, lanes=[
        ("KICK", 92, "x.....x...x.....x..."),
        ("SNARE", 94, "....X.......X.....X."),
        ("SNARE", 40, "..g......g.....g...."),
        ("RIDE", 64, "x.x.x.x.x.x.x.x.x.x."),
        ("BELL", 56, "x.......x.......x..."),
    ]),
    # DJ Playero, El General: the dembow, kick on every beat, the snare on 3 3 2 (the 'and' of 1, of 2...)
    "G90_DEMBOW": dict(kit="80", tune=64, decay=56, snap=88, lanes=[
        ("KICK", 108, FOUR * 2),
        ("SNARE", 98, "...x..x....x..x....x..x....x..x."),
        ("HAT", 52, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("TIMBALE", 54, "..............x.x.............xx"),
    ]),
}

GENRES = [
    dict(id="GRUNGE", name="GRUNGE", decade=1990, year=1991,
         comment="Nirvana, Soundgarden, Pearl Jam: four power chords in a loop (i iv III VI, Teen Spirit), "
                 "quiet clean verses against fuzz-wall choruses, a sullen low melody",
         key="F", scale="AEOLIAN", bpm=116, div=4, chord_beats=2, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "iv5", "III5", "VI5"],
         melody=dict(density=0.45, base=57, range=12, sync=0.35, add=("b5",), profile=(0.3, 0.7, 0.88, 0.96),
                     motif=0.8),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0,), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="x..x..x.x..x..x.x..x..x.x..x..x.", gate=1, base=48, tones=3, power=True),
         groove="G90_GRUNGE", orch_mode="CONTRAST",
         orch=[("EBASS", "EGUITAR", "EGUITAR"), ("WAVEBASS", "DISTGUITAR", "DISTGUITAR")]),
    dict(id="BRITPOP", name="BRITPOP", decade=1990, year=1994,
         comment="Oasis, Blur, Pulp: strummed acoustic sevenths and sus chords (Wonderwall's cycle), a Beatles-ish "
                 "tune, tambourine and a straight mid-tempo beat",
         key="F#", scale="IONIAN", bpm=88, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["vi7", "I", "Vsus4", "V7sus"],
         cycle2=["IV", "V", "vi", "vi", "IV", "V", "Iadd9", "Vsus4"], cycle_switch=2,
         melody=dict(density=0.5, base=62, range=12, sync=0.3, profile=(0.3, 0.75, 0.9, 0.97), motif=0.75),
         bass=dict(mode="OSTINATO", rhythm="x.x.x.x.x.x.x.x.", notes=(0, 0, 0, 7), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="x...x.x..xx.x.x.x...x.x..xx.x.x.", gate=2, base=55, tones=3, strum_ms=9),
         groove="G90_BRITPOP",
         orch=[("EBASS", "EGUITAR", "GUITAR"), ("EBASS", "PIANO", "GUITAR"), ("WAVEBASS", "SAX", "GUITAR")]),
    dict(id="GANGSTA_RAP", name="GANGSTA RAP", decade=1990, year=1988,
         comment="N.W.A, Mobb Deep, Wu-Tang: a hard dusty boom-bap break, a deep bass on the kicks, a dark looped "
                 "two-note piano hook a minor second apart, menace in Phrygian",
         key="EB", scale="PHRYGIAN", bpm=92, div=4, chord_beats=8, period_bars=8, swing=0.35, swing16=True,
         vocab="BLUES", harmony="CYCLE", cycle=["i", "bII", "i", "iv"],
         melody=dict(mode="PATTERN", base=58, pattern=(4, None, None, 5, None, None, 4, None, None, None, 3, None,
                                                        0, None, None, None)),
         bass=dict(mode="808", gate=6),
         pad=dict(mode="RHYTHM", rhythm="x...........................x...", gate=3, base=52, tones=3),
         groove="G90_GANGSTA",
         orch=[("808", "PIANO", "STRINGS"), ("808", "BELLS", "PIANO"), ("WAVEBASS", "PIANO", "STRINGS")]),
    dict(id="G_FUNK", name="G-FUNK", decade=1990, year=1992,
         comment="Dr. Dre's The Chronic, Warren G, Snoop: Parliament slowed down, a whining portamento synth lead "
                 "up high, a fat Moog bass line, Rhodes ninths, the laid-back swing",
         key="A", scale="DORIAN", bpm=94, div=4, chord_beats=8, period_bars=8, swing=0.45, swing16=True,
         vocab="BLUES", harmony="CYCLE", cycle=["i9", "iv9"],
         melody=dict(density=0.38, base=72, range=12, sync=0.45, profile=(0.15, 0.6, 0.85, 0.95), min_ticks=4,
                     add=("b5",), motif=0.85, motif_beats=4),
         bass=dict(mode="OSTINATO", rhythm="x......x..x...x.", notes=(0, 12, 10, 7), semis=True, gate=0),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G90_GFUNK",
         orch=[("WAVEBASS", "SAWLEAD", "EPIANO"), ("WAVEBASS", "TALKBASS", "EPIANO"), ("SYNTHBASS", "SAWLEAD", "SINEKEY")]),
    dict(id="RNB_90S", name="R&B 90S", decade=1990, year=1992,
         comment="Jodeci, TLC, Aaliyah, Babyface: lush major sevenths and minor ninths on electric piano, a gliding "
                 "sub bass, a skittering 808 beat, a melismatic lead",
         key="DB", scale="IONIAN", bpm=88, div=4, chord_beats=4, period_bars=8, swing=0.3, swing16=True,
         vocab="JAZZ", harmony="CYCLE", cycle=["IVmaj7", "iii7", "ii9", "Imaj7"],
         melody=dict(density=0.55, base=63, range=14, sync=0.5, turn=0.3, appoggiatura=0.2, add=("b3", "b7"),
                     profile=(0.15, 0.7, 0.88, 0.96), motif=0.65),
         bass=dict(mode="808", gate=0),
         pad=dict(mode="BLOCK", base=57, tones=3),
         groove="G90_RNB",
         orch=[("808", "SINEKEY", "EPIANO"), ("WAVEBASS", "VOICE", "EPIANO"), ("808", "SAX", "PAD")]),
    dict(id="TRIP_HOP", name="TRIP HOP", decade=1990, year=1994,
         comment="Portishead's Dummy, Massive Attack, Tricky: a slow heavy break, a deep sub, minor ninths and a "
                 "falling lament, a theremin-like lead, cinematic strings",
         key="G", scale="HARM_MINOR", bpm=80, div=4, chord_beats=4, period_bars=8, swing=0.2, swing16=True,
         vocab="JAZZ", harmony="CYCLE", cycle=["im9", "im9", "VImaj7", "V7b9"],
         melody=dict(density=0.3, base=64, range=12, sync=0.3, chromatic=True, appoggiatura=0.3, min_ticks=4,
                     profile=(0.15, 0.7, 0.88, 0.96), motif=0.7, motif_beats=4, direction=-0.3),
         bass=dict(mode="OSTINATO", rhythm="x.......x.x.....", notes=(0, 0, 7), semis=True, gate=0),
         pad=dict(mode="BLOCK", base=52, tones=3),
         groove="G90_TRIPHOP",
         orch=[("WAVEBASS", "SINEKEY", "STRINGS"), ("WAVEBASS", "SAWLEAD", "EPIANO"), ("808", "FLUTE", "PAD")]),
    dict(id="JUNGLE", name="JUNGLE", decade=1990, year=1993,
         comment="Goldie, M-Beat, Shy FX: chopped Amen breaks at 165, a booming sub at half speed, ragga skank "
                 "stabs and a dub-siren melody over a minor vamp",
         key="G", scale="AEOLIAN", bpm=165, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "i", "VI", "VII"],
         melody=dict(density=0.3, base=67, range=12, sync=0.5, min_ticks=4, profile=(0.35, 0.7, 0.9, 0.97),
                     motif=0.85, motif_beats=4),
         bass=dict(mode="808", rhythm="x.........x.....", notes=(0, 0, 7, 0), semis=True),
         pad=dict(mode="RHYTHM", rhythm="........x...............x.......", gate=2, base=57, tones=3),
         groove="G90_JUNGLE",
         orch=[("808", "SAWLEAD", "ORGAN"), ("WAVEBASS", "PLUCKSYNTH", "ORGAN"), ("808", "SINEKEY", "PIANO")]),
    dict(id="DRUM_AND_BASS", name="DRUM & BASS", decade=1990, year=1996,
         comment="Roni Size, Ed Rush & Optical, Bad Company: the clean two-step at 174, a growling detuned reese bass, "
                 "dark atmospheric pads and a sparse pluck",
         key="F", scale="AEOLIAN", bpm=174, div=4, chord_beats=16, period_bars=16,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "i", "iv"],
         melody=dict(density=0.25, base=65, range=12, sync=0.55, min_ticks=2, profile=(0.3, 0.7, 0.9, 0.97),
                     motif=0.85, motif_beats=4),
         bass=dict(mode="OSTINATO", rhythm="x.........x.....", notes=(0, 0, 1, 0), semis=True, gate=0),
         pad=dict(mode="BLOCK", base=53, tones=3),
         groove="G90_DNB",
         orch=[("FATBASS", "PLUCKSYNTH", "SINEKEY"), ("FATBASS", "SINEKEY", "SHIMMER"), ("FATBASS", "GLITCH", "SINEKEY")]),
    dict(id="TRANCE_90S", name="TRANCE", decade=1990, year=1993, reverb="HALL",
         comment="Paul van Dyk, Robert Miles, ATB, Ferry Corsten: 138 four on the floor, the rolling off-beat bass, "
                 "sixteenth arpeggios over i VI III VII, supersaw chords, long builds",
         key="A", scale="AEOLIAN", bpm=138, div=4, chord_beats=8, period_bars=16, swell=0.6,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "III", "VII"],
         melody=dict(mode="PATTERN", base=69, pattern_chord=True,
                     pattern=(0, 1, 2, 3, 2, 1, 0, 2, 3, 4, 2, 1, 0, 1, 2, 4)),
         bass=dict(mode="OSTINATO", rhythm=OFF, notes=(0,), semis=True, gate=2),
         pad=dict(mode="BLOCK", base=57, tones=3, spread=True),
         groove="G90_TRANCE", orch_mode="ENERGY",
         orch=[("WAVEBASS", "PLUCKSYNTH", "PAD"), ("WAVEBASS", "PLUCKSYNTH", "SUPERSAW"), ("WAVEBASS", "CHIP", "SUPERSAW")]),
    dict(id="EURODANCE", name="EURODANCE", decade=1990, year=1992,
         comment="2 Unlimited, Haddaway, Culture Beat, Corona: four on the floor at 130, the octave-jumping bass, "
                 "house-piano stabs, a big minor synth hook",
         key="C", scale="AEOLIAN", bpm=130, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "III", "VII"],
         melody=dict(density=0.6, base=67, range=12, sync=0.45, staccato=0.3, profile=(0.3, 0.7, 0.9, 0.97),
                     motif=0.85),
         bass=dict(mode="OSTINATO", rhythm=B16, notes=(0, 12), semis=True, gate=1),
         pad=dict(mode="RHYTHM", rhythm="x..x..x...x..x..x..x..x...x..x..", gate=2, base=57, tones=3),
         groove="G90_EURODANCE",
         orch=[("WAVEBASS", "SYNCLEAD", "PIANO"), ("WAVEBASS", "SAWLEAD", "PIANO"), ("808", "SYNCLEAD", "PLUCKSYNTH")]),
    dict(id="NU_METAL", name="NU METAL", decade=1990, year=1997,
         comment="Korn, Limp Bizkit, Deftones: drop-tuned chugs on the low string and the flat second, a "
                 "hip-hop-bounce beat, eerie sparse lead notes and scratches",
         key="D", scale="PHRYGIAN", bpm=100, div=4, chord_beats=8, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["i5", "i5", "bII5", "VI5"],
         melody=dict(density=0.3, base=62, range=12, sync=0.5, chromatic=True, add=("b5",),
                     profile=(0.35, 0.75, 0.9, 0.97), motif=0.8),
         bass=dict(mode="OSTINATO", rhythm="x.x...x..x.x....", notes=(0, 0, 0, 1), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="x.x...x..x.x....x.x...x...x.x...", gate=1, base=38, tones=3, power=True),
         groove="G90_NUMETAL",
         orch=[("EBASS", "GLITCH", "DISTGUITAR"), ("EBASS", "EGUITAR", "DISTGUITAR")]),
    dict(id="POP_PUNK_90S", name="POP PUNK", decade=1990, year=1994,
         comment="Green Day's Dookie, the Offspring, Blink-182: downpicked power-chord eighths on I V vi IV at full "
                 "speed, a driving eighth bass, a catchy major tune",
         key="E", scale="IONIAN", bpm=176, div=4, chord_beats=4, period_bars=8,
         vocab="BLUES", harmony="CYCLE", cycle=["I5", "V5", "vi", "IV5"],
         melody=dict(density=0.55, base=64, range=12, sync=0.25, profile=(0.3, 0.75, 0.9, 0.97), motif=0.8),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm=B16 * 2, gate=1, base=52, tones=3, power=True),
         groove="G90_POPPUNK",
         orch=[("WAVEBASS", "DISTGUITAR", "DISTGUITAR"), ("EBASS", "EGUITAR", "DISTGUITAR")]),
    dict(id="MIDWEST_EMO", name="EMO", decade=1990, year=1994,
         comment="American Football, Cap'n Jazz, Mineral: 5/4, twinkly clean guitars tapping open major sevenths "
                 "and add9s, a second guitar's wandering line, a busy jazzy kit",
         key="F", scale="IONIAN", bpm=126, div=4, beats=5, chord_beats=5, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["IVmaj7", "Iadd9", "vi7", "IVadd9"],
         melody=dict(density=0.45, base=67, range=14, sync=0.4, profile=(0.15, 0.6, 0.82, 0.94), motif=0.7,
                     motif_beats=5),
         bass=dict(mode="OSTINATO", rhythm="x.....x...x.....x...", notes=(0, 0, 2, 1), gate=0),
         pad=dict(mode="FIGURE", base=55, figure=(0, 2, 4, 3, 1, 3, 5, 4, 2, 4)),
         groove="G90_EMO",
         orch=[("EBASS", "EGUITAR", "GUITAR"), ("EBASS", "GUITAR", "EGUITAR"), ("EBASS", "TRUMPET", "GUITAR")]),
    dict(id="REGGAETON_90S", name="REGGAETON 90S", decade=1990, year=1994,
         comment="DJ Playero, El General, Tego's beginnings: the dembow riddim from Jamaica via Panama and Puerto "
                 "Rico, a deep bass on the three-three-two, synth stabs and a minor hook",
         key="G", scale="AEOLIAN", bpm=96, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "VII", "i"],
         melody=dict(density=0.5, base=67, range=12, sync=0.5, staccato=0.3, profile=(0.35, 0.7, 0.9, 0.97),
                     motif=0.85),
         bass=dict(mode="OSTINATO", rhythm="x..x..x.x..x..x.", notes=(0, 0, 7, 0), semis=True, gate=2),
         pad=dict(mode="RHYTHM", rhythm="...x..x....x..x....x..x....x..x.", gate=1, base=57, tones=3),
         groove="G90_DEMBOW",
         orch=[("808", "SYNCLEAD", "PLUCKSYNTH"), ("WAVEBASS", "CHIP", "PIANO"), ("808", "SAWLEAD", "MARIMBA")]),
]
