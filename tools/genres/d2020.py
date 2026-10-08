# SPDX-License-Identifier: GPL-3.0-only
# TUNA genres of the 2020s, so far (tools/tuna_genres.py writes them out). Ticks: div ticks a beat; a groove's strings
# are its loop (two bars unless len): 'X' accent, 'x' hit, 'g' ghost, '.' rest. Bass rhythms are one bar, pad rhythms
# the groove's loop. Rap genres are their beats: the hook is played by an instrument.

R = None                                # a rest in a melody pattern
H = "-"                                 # a held note in a melody pattern
X = -128                                # a rest in a pad figure (TG_REST)

GROOVES = {
    # drift phonk: a distorted 808 kick, the clap on 2 and 4, Memphis sixteenth hats, the cowbell is the melody
    "G20_PHONK": dict(kit="80", tune=58, tone=70, decay=96, snap=90, drive=110, roll=3, lanes=[
        ("KICK", 112, "X.....x...x.....X.....x..x..x..."),
        ("CLAP", 108, "....X.......X.......X.......X..."),
        ("HAT", 70, "x.xxx.xxx.xxx.xxx.xxx.xxx.xxx.xx"),
        ("OHAT", 60, "..............x...............x."),
    ]),
    # Brooklyn drill: UK drill's half-time at a faster tempo, snare on 3 with the displaced late snare, skippy hats,
    # the kick under the sliding 808
    "G20_BKDRILL": dict(kit="80", tune=54, decay=100, snap=100, drive=30, roll=4, lanes=[
        ("KICK", 110, "X.........x.....X.....x...x....."),
        ("SNARE", 112, "........X...............X..X...."),
        ("SNARE", 46, "..............g..........g....g."),
        ("HAT", 76, "x..x..x...x..x..x..x..x.x...x.x."),
    ]),
    # rage: a punchy distorted kick, the clap on 3 (half time at 160), eighth hats with rolls, open hats
    "G20_RAGE": dict(kit="80", tune=66, tone=80, decay=100, snap=110, drive=90, roll=7, lanes=[
        ("KICK", 116, "X.....x.....x...X.....x.x......."),
        ("CLAP", 112, "........X...............X......."),
        ("SNARE", 90, "........X...............X.....x."),
        ("HAT", 74, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("OHAT", 64, "......x...............x........."),
    ]),
    # Jersey club: the kick in fives (1 2 3 3& 4), the bed-squeak on the off-beats, claps on 2 and 4 and a quick
    # clap run into the loop's end
    "G20_JERSEY": dict(kit="10", tune=70, decay=56, snap=90, lanes=[
        ("KICK", 116, "X...x...x.x.x...X...x...x.x.x..."),
        ("CLAP", 104, "....X.......X.......X.......XXXX"),
        ("WOODBLOCK", 80, "..x...x...x...x...x...x...x...x."),
        ("HAT", 62, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
    # sped up: a bright pop beat played fast, crisp claps, sixteenth hats
    "G20_SPEDUP": dict(kit="10", tune=88, tone=90, decay=46, snap=100, lanes=[
        ("KICK", 106, "x.....x.x.......x.....x.x...x..."),
        ("CLAP", 102, "....X.......X.......X.......X..."),
        ("HAT", 66, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
        ("TAMB", 56, "....x.......x.......x.......x..."),
    ]),
    # slowed + reverb: a slow trap-R&B beat pitched down, a long boom, a dull snare, sparse hats
    "G20_SLOWED": dict(kit="80", tune=36, tone=36, decay=110, snap=40, roll=2, fill=0, lanes=[
        ("KICK", 104, "x.........x.....x.....x........."),
        ("SNARE", 92, "........x...............x......."),
        ("HAT", 48, "x...x...x...x.x.x...x...x...x.x."),
    ]),
    # bedroom pop: a cheap drum machine, soft and swung, a shaker
    "G20_BEDROOM": dict(kit="66", tune=60, tone=50, decay=50, snap=50, fill=0, lanes=[
        ("KICK", 88, "x.......x.x.....x.......x......."),
        ("SNARE", 82, "....x.......x.......x.......x..."),
        ("HAT", 54, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
        ("SHAKER", 46, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
    ]),
    # country trap: a trap beat (snare on 3, rolling hats) under the banjo, a stomp-clap
    "G20_CTRYTRAP": dict(kit="80", tune=60, decay=104, snap=80, roll=5, lanes=[
        ("KICK", 108, "x.....x.......x.x.....x...x....."),
        ("SNARE", 104, "........X...............X......."),
        ("CLAP", 84, "........x...............x......."),
        ("HAT", 70, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
    # pop punk revival: the punk beat (kick-snare eighths, backbeat) with trap hat rolls on top (Travis Barker)
    "G20_POPPUNK": dict(kit="STD", kick="PUNCH", tune=70, decay=50, snap=96, drive=40, roll=6, lanes=[
        ("KICK", 110, "X.....x.x.x.....X.....x...x.x..."),
        ("SNARE", 112, "....X.......X.......X.......X..."),
        ("SNARE", 50, "..........g..............g....g."),
        ("HAT", 72, "x.x.x.xxx.x.x.x.x.x.x.xxx.x.xxxx"),
        ("CRASH", 86, "x..............................."),
        ("OHAT", 60, "..............x...............x."),
    ]),
    # Y2K pop revival: a glossy 2-step skip (kick skipping past 3), claps on 2 and 4, shuffled hats
    "G20_Y2K": dict(kit="10", tune=74, tone=80, decay=50, snap=96, lanes=[
        ("KICK", 108, "x.........x.....x.......x.x....."),
        ("CLAP", 100, "....X.......X.......X.......X..."),
        ("HAT", 62, "..x...x...x...x...x...x...x...x."),
        ("HAT", 44, "x..gx..gx..gx..gx..gx..gx..gx..g"),
        ("SHAKER", 50, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x."),
    ]),
}

GENRES = [
    dict(id="DRIFT_PHONK", name="DRIFT PHONK", decade=2020, year=2020,
         comment="Kordhell, DVRST, Kaito Shoma: Memphis phonk made for drifting: a cowbell riff in sixteenths over a "
                 "distorted 808, Phrygian gloom, the clap on 2 and 4",
         key="C#", scale="PHRYGIAN", bpm=126, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "i", "bII", "i", "i", "VI", "bII", "i"],
         melody=dict(mode="PATTERN", base=67,
                     pattern=(0, R, 7, R, 0, 7, R, 5, R, 4, R, 2, 1, R, 0, R)),
         bass=dict(mode="808"),
         pad=dict(mode="BLOCK", base=52, tones=3),
         groove="G20_PHONK", orch_mode="CYCLE",
         orch=[("808", "CHIP", "CHOIR"), ("808", "CHIP", "PAD"), ("808", "STEELDRUM", "CHOIR")]),
    dict(id="BROOKLYN_DRILL", name="BROOKLYN DRILL", decade=2020, year=2020,
         comment="Pop Smoke, Fivio Foreign, 808Melo: UK drill brought to New York, faster, the 808 sliding up and "
                 "down an octave, a dark harmonic-minor piano and strings, a vocal-like lead",
         key="F", scale="HARM_MINOR", bpm=144, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "iv", "V"],
         melody=dict(density=0.35, base=62, range=12, sync=0.45, profile=(0.2, 0.7, 0.88, 0.96), motif=0.8,
                     min_ticks=2),
         bass=dict(mode="808", rhythm="x.....x...x...x.", notes=(0, 12, 0, 10, 0, 12, 7, 3), semis=True),
         pad=dict(mode="BLOCK", base=55, tones=3),
         groove="G20_BKDRILL",
         orch=[("808", "PIANO", "PAD"), ("808", "VOICE", "STRINGS"), ("808", "PIANO", "SINEKEY")]),
    dict(id="RAGE_RAP", name="RAGE", decade=2020, year=2020,
         comment="Playboi Carti's Whole Lotta Red, Trippie Redd's Miss the Rage, Ken Carson: a blaring supersaw riff "
                 "looping over 808s and a distorted kick, at 160",
         key="C#", scale="AEOLIAN", bpm=160, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VI", "i", "VII"],
         melody=dict(mode="PATTERN", base=61, pattern_chord=True,
                     pattern=(3, H, 2, H, 3, H, 4, H, 3, H, 2, H, 1, H, 2, H)),
         bass=dict(mode="808"),
         pad=dict(mode="NONE"),
         groove="G20_RAGE",
         orch=[("808", "SUPERSAW", "PLUCKSYNTH"), ("808", "SAWLEAD", "PLUCKSYNTH")]),
    dict(id="JERSEY_CLUB", name="JERSEY CLUB", decade=2020, year=2022,
         comment="the 2020s Jersey club wave (Lil Uzi Vert's Just Wanna Rock, Ice Spice, Bandmanrill): the kick in "
                 "fives, the bed-squeak on the off-beats, a chopped vocal hook at 140",
         key="G", scale="DORIAN", bpm=140, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["i", "VII", "iv", "VII"],
         melody=dict(mode="PATTERN", base=67,
                     pattern=(4, R, 4, R, 2, R, R, 4, R, 6, 4, R, 2, R, 0, R)),
         bass=dict(mode="808", gate=3),
         pad=dict(mode="RHYTHM", rhythm="x.....x.....x...x.....x.....x...", gate=2, base=57, tones=3),
         groove="G20_JERSEY",
         orch=[("808", "VOICE", "PLUCKSYNTH"), ("808", "SYNCLEAD", "PLUCKSYNTH")]),
    dict(id="SPED_UP", name="SPED UP", decade=2020, year=2021,
         comment="TikTok's sped-up edits: a pop song pitched up and played fast, the chipmunk vocal hook bright and "
                 "high, I V vi IV",
         key="E", scale="IONIAN", bpm=168, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["I", "V", "vi", "IV"],
         melody=dict(density=0.65, base=76, range=12, sync=0.4, profile=(0.25, 0.7, 0.88, 0.96), motif=0.8,
                     min_ticks=2),
         bass=dict(mode="OSTINATO", rhythm="x.....x...x.....", notes=(0, 0, 2), gate=3),
         pad=dict(mode="RHYTHM", rhythm="x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.", gate=1, base=64, tones=3),
         groove="G20_SPEDUP",
         orch=[("WAVEBASS", "VOICE", "PLUCKSYNTH"), ("WAVEBASS", "SINEKEY", "PIANO")]),
    dict(id="SLOWED_REVERB", name="SLOWED+REVERB", decade=2020, year=2020,
         comment="the slowed + reverb edits of YouTube and TikTok: a song pulled down to a crawl, lower, washed in "
                 "reverb, dreamy major sevenths, the 808 booming",
         key="D", scale="AEOLIAN", bpm=68, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["VImaj7", "iv7", "i", "III"],
         melody=dict(density=0.25, base=57, range=12, sync=0.25, profile=(0.2, 0.75, 0.9, 0.97), motif=0.8,
                     min_ticks=4),
         bass=dict(mode="808"),
         pad=dict(mode="BLOCK", base=52, tones=3, spread=True),
         groove="G20_SLOWED", swing=0.3, swing16=True, orch_mode="CYCLE",
         orch=[("808", "VOICE", "SINEKEY"), ("808", "SINEKEY", "SHIMMER"), ("808", "FLUTE", "CHOIR")]),
    dict(id="BEDROOM_POP", name="BEDROOM POP", decade=2020, year=2020,
         comment="Clairo, beabadoobee, Rex Orange County, Steve Lacy: home-made pop, a jangly strummed guitar, "
                 "major sevenths, a soft bass and a cheap drum machine",
         key="A", scale="IONIAN", bpm=92, div=4, chord_beats=4, period_bars=8, swing=0.25, swing16=True,
         vocab="JAZZ", harmony="CYCLE", cycle=["Imaj7", "iii7", "IVmaj7", "IVmaj7"],
         melody=dict(density=0.45, base=64, range=12, sync=0.35, profile=(0.25, 0.72, 0.9, 0.97), motif=0.75),
         bass=dict(mode="OSTINATO", rhythm="x.....x.x.......", notes=(0, 0, 2)),
         pad=dict(mode="RHYTHM", rhythm="x..x..x.x..x..x.x..x..x.x..x..x.", gate=2, base=55, tones=3, strum_ms=12),
         groove="G20_BEDROOM",
         orch=[("WAVEBASS", "SINEKEY", "GUITAR"), ("EBASS", "SYNTH", "GUITAR"), ("WAVEBASS", "VOICE", "EGUITAR")]),
    dict(id="COUNTRY_TRAP", name="COUNTRY TRAP", decade=2020, year=2021,
         comment="Lil Nas X's Old Town Road to Morgan Wallen and Jelly Roll: a banjo roll and a twangy guitar over "
                 "trap drums and an 808",
         key="G", scale="IONIAN", bpm=140, div=4, chord_beats=8, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["I", "V", "vi", "IV"],
         melody=dict(density=0.4, base=62, range=12, sync=0.3, profile=(0.25, 0.72, 0.9, 0.97), motif=0.8,
                     min_ticks=2),
         bass=dict(mode="808"),
         pad=dict(mode="FIGURE", figure=(0, X, 1, X, 3, X, 0, X, 2, X, 4, X, 1, X, 3, X), base=55, tones=3),   # a banjo roll
         groove="G20_CTRYTRAP",
         orch=[("808", "EGUITAR", "GUITAR"), ("808", "FLUTE", "GUITAR"), ("808", "EGUITAR", "PIZZ")]),
    dict(id="POP_PUNK_REVIVAL", name="POP PUNK 2020S", decade=2020, year=2021,
         comment="Machine Gun Kelly's Tickets to My Downfall, Olivia Rodrigo's good 4 u, Travis Barker's drums: "
                 "pop punk power chords and bass eighths with trap hat rolls",
         key="A", scale="IONIAN", bpm=160, div=4, chord_beats=4, period_bars=8,
         vocab="POP", harmony="CYCLE", cycle=["IV", "I", "V", "vi"],
         melody=dict(density=0.55, base=64, range=12, sync=0.3, profile=(0.3, 0.72, 0.9, 0.97), motif=0.8,
                     min_ticks=2),
         bass=dict(mode="REPEAT"),
         pad=dict(mode="RHYTHM", rhythm="x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.", gate=1, base=50, tones=3, power=True),
         groove="G20_POPPUNK",
         orch=[("WAVEBASS", "EGUITAR", "DISTGUITAR"), ("808", "SYNTH", "DISTGUITAR")]),
    dict(id="Y2K_POP_REVIVAL", name="Y2K REVIVAL", decade=2020, year=2022,
         comment="PinkPantheress, NewJeans, Charli XCX: the glossy pop of 2000 revisited, a 2-step skip, bright "
                 "plucked arpeggios, a breathy hook over major sevenths",
         key="AB", scale="IONIAN", bpm=132, div=4, chord_beats=4, period_bars=8, swing=0.2, swing16=True,
         vocab="POP", harmony="CYCLE", cycle=["IVmaj7", "Iadd9", "vi7", "V"],
         melody=dict(density=0.5, base=68, range=12, sync=0.45, profile=(0.25, 0.7, 0.88, 0.96), motif=0.8,
                     min_ticks=2),
         bass=dict(mode="OSTINATO", rhythm="x.....x...x..x..", notes=(0, 0, 3, 2), gate=3),
         pad=dict(mode="ARPEGGIO", base=60, tones=3),
         groove="G20_Y2K",
         orch=[("SYNTHBASS", "VOICE", "PLUCKSYNTH"), ("WAVEBASS", "SINEKEY", "PLUCKSYNTH"),
               ("WAVEBASS", "VOICE", "MARIMBA")]),
]

