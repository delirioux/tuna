# SPDX-License-Identifier: GPL-3.0-only
# Manners of the film and stage composers (tools/tuna_manners.py): the first is the profile in tuna_style.c
# as it is.

MANNERS = {
    "MORRICONE": [
        # the spaghetti westerns (Dollars trilogy, Once Upon a Time in the West): whistle, strummed guitar, beat
        dict(name="WESTERN"),
        # Gabriel's Oboe, Cinema Paradiso, Deborah's Theme: the slow major-key love theme, long leaning lines,
        # applied dominants, strings, no drums
        dict(name="ROMANZA", root="D", scale="IONIAN", harmony="PLANNED", bpm=64, period_bars=4, chord_beats=2,
             melody_density=0.35, mel_min_ticks=2, appoggiatura=0.25, suspension=0.3, rubato=0.4, swell=0.8,
             perc_mode="OFF", strum_ms=0, evolve_periods=0, pad_mode="BLOCK", reverb="HALL",
             w_major={"I": 10, "vi": 7, "iii": 3, "IV": 8, "ii": 5, "ii7": 4, "V": 7, "V7": 8, "V7/V": 4,
                      "V7/vi": 4, "V7/IV": 3, "iv": 3, "IVmaj7": 3, "vi7": 3},
             w_minor={"i": 10, "VI": 7, "III": 4, "iv": 7, "iiø7": 4, "V": 7, "V7": 8, "V7/V": 3, "VII": 3,
                      "iv7": 3, "VImaj7": 3},
             orch=[("CONTRABASS", "OBOE", "STRINGS"), ("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "STRINGS", "PIANO")],
             orch_mode="RANDOM"),
    ],
    "JOHN WILLIAMS": [
        # Star Wars, Raiders, Superman: the brass march
        dict(name="MARCH"),
        # Hedwig's Theme, Hook: the quick fairy-tale waltz-scherzo, celesta, chromatic minor
        dict(name="SCHERZO", root="E", scale="HARM_MINOR", bpm=168, division=2, beats_per_bar=3, chord_beats=3,
             period_bars=8, melody_density=0.6, mel_min_ticks=1, chromatic=True, appoggiatura=0.2, octave=0.15,
             bass_mode="BAR", pad_mode="AFTERBEAT", perc_mode="OFF", color=0.0,
             w_minor={"i": 10, "iv": 6, "iv6": 4, "V": 6, "V7": 7, "VI": 5, "bII": 3, "#iv°7": 3, "vii°7": 3,
                      "Ger+6": 3, "III": 3, "III+": 2, "iiø7": 3, "V7/V": 2},
             orch=[("CONTRABASS", "CELESTA", "STRINGS"), ("CELLO", "FLUTE", "STRINGS"), ("CONTRABASS", "GLOCKEN", "PIZZ")],
             orch_mode="RANDOM"),
        # Schindler's List, the Angela's Ashes / Memoirs themes: the slow minor elegy for solo violin and strings
        dict(name="ELEGY", root="D", scale="AEOLIAN", bpm=60, division=2, chord_beats=4, period_bars=4,
             melody_density=0.35, mel_min_ticks=2, melody_range=17, appoggiatura=0.3, rubato=0.4, swell=0.8,
             color=0.0, octave=0.1, bass_mode="ROOT", perc_mode="OFF", reverb="HALL",
             w_minor={"i": 10, "iv": 7, "VI": 6, "III": 4, "V7": 7, "V": 5, "iiø7": 5, "iv6": 3, "VII": 3,
                      "V7/iv": 3, "Ger+6": 2, "bII": 2},
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "VIOLIN", "PIANO"), ("CONTRABASS", "CLARINET", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "HOWARD SHORE": [
        # the Shire: pentatonic folk tune on whistle, lilting
        dict(name="PASTORAL"),
        # Rohan and Gondor: the noble minor-mode horn call, brass and choir, timpani, swelling
        dict(name="EPIC", scale="DORIAN", bpm=76, division=2, swing=0.0, melody_mask=0, melody_density=0.35,
             mel_min_ticks=2, max_leap=5, octave=0.15, swell=0.9, turn=0.0, perc_mode="PERIOD", perc=[8, 2, 3, 4, 0, 0],
             reverb="HALL",
             w_major={"i": 10, "bVII": 7, "bVI": 6, "iv": 5, "bIII": 5, "v": 4, "IV": 3, "isus4": 2, "5": 3},
             w_minor={"i": 10, "bVII": 7, "bVI": 6, "iv": 5, "bIII": 5, "v": 4, "IV": 3, "isus4": 2, "5": 3},
             orch=[("TUBA", "HORN", "BRASS"), ("CONTRABASS", "CHOIR", "STRINGS"), ("CONTRABASS", "VIOLIN", "STRINGS"),
                   ("TUBA", "TRUMPET", "STRINGS")], orch_mode="ENERGY"),
        # Isengard and Mordor: the 5/4 ostinato, low brass, driving strings and drums, Phrygian darkness
        dict(name="BATTLE", root="C", scale="PHRYGIAN", bpm=112, division=2, beats_per_bar=5, chord_beats=5,
             period_bars=4, swing=0.0, melody_mask=0, melody_base=55, melody_range=12, melody_density=0.4,
             staccato=0.3, sforzando=0.3, max_leap=4, turn=0.0, bass_mode="OSTINATO", bass_rhythm="x..x..x.x.",
             bass_notes=[0, 0, 2, 0], bass_len=4, pad_mode="REPEAT", perc_mode="PERIOD", perc=[9, 6, 6, 5, 0, 0],
             reverb="HALL",
             w_major={"i": 10, "II": 6, "bVI": 5, "iv": 4, "v": 3, "5": 5, "isus4": 3},
             w_minor={"i": 10, "II": 6, "bVI": 5, "iv": 4, "v": 3, "5": 5, "isus4": 3},
             orch=[("TUBA", "BRASS", "STRINGS"), ("CONTRABASS", "HORN", "STRINGS"), ("TUBA", "CHOIR", "BRASS")],
             orch_mode="RANDOM"),
    ],
    "LLOYD WEBBER": [
        # Jesus Christ Superstar, the Phantom's title song: the rock opera with kit and synths
        dict(name="ROCK"),
        # Memory, Music of the Night, I Don't Know How to Love Him: the 12/8 ballad, piano broken chords, the
        # key lifted a semitone for the last chorus
        dict(name="BALLAD", root="A#", scale="IONIAN", bpm=72, division=3, beats_per_bar=4, chord_beats=4,
             melody_density=0.4, mel_min_ticks=2, melody_range=15, octave=0.08, suspension=0.2, swell=0.9,
             bass_mode="ROOT", pad_mode="BROKEN", perc_mode="OFF", reverb="HALL",
             orch=[("PIANO", "VOICE", "PIANO"), ("CONTRABASS", "VOICE", "STRINGS"), ("CELLO", "SAX", "PIANO")],
             orch_mode="RANDOM"),
        # Evita: the tango numbers, habanera bass, bandoneon and violin, crisp accents
        dict(name="TANGO", bpm=116, division=2, beats_per_bar=4, chord_beats=4,
             melody_density=0.5, mel_min_ticks=1, staccato=0.4, sforzando=0.3, syncopation=0.3,
             bass_mode="OSTINATO", bass_rhythm="x..x..x.", bass_notes=[0, 2, 0], bass_len=3, pad_mode="AFTERBEAT",
             perc_mode="OFF", evolve_periods=0, reverb="ROOM",
             orch=[("CONTRABASS", "BANDONEON", "PIANO"), ("CONTRABASS", "VIOLIN", "BANDONEON"),
                   ("CONTRABASS", "VOICE", "PIZZ")], orch_mode="RANDOM"),
    ],
    "PREISNER": [
        # the Kieslowski waltzes (Van den Budenmayer, Decalogue, Veronique): 3/4, piano and flute
        dict(name="WALTZ"),
        # Requiem for My Friend (Lacrimosa): slow D minor, choir and voice over organ or strings
        dict(name="REQUIEM", root="D", bpm=60, beats_per_bar=4, chord_beats=4, period_bars=4, motif_beats=4,
             melody_density=0.3, mel_min_ticks=2, pad_tones=4, bass_mode="ROOT", rubato=0.3, swell=0.8,
             reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "VOICE", "STRINGS"),
                   ("CONTRABASS", "CHOIR", "STRINGS")], orch_mode="RANDOM"),
        # Song for the Unification of Europe (Blue): the great choral hymn with brass and timpani, building
        dict(name="HYMN", bpm=72, beats_per_bar=4, chord_beats=4, period_bars=4, motif_beats=4, melody_density=0.35,
             mel_min_ticks=2, pad_tones=4, bass_mode="ROOT", swell=1.0, evolve_periods=4, evolve_depth=1.0,
             perc_mode="PERIOD", perc=[8, 0, 3, 6, 0, 0], reverb="CATHEDRAL",
             orch=[("TUBA", "CHOIR", "BRASS"), ("CONTRABASS", "CHOIR", "STRINGS"), ("PIPEORGAN", "TRUMPET", "PIPEORGAN")],
             orch_mode="ENERGY"),
    ],
}
