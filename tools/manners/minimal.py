# SPDX-License-Identifier: GPL-3.0-only
# Manners of the minimalists and the holy minimalists (tools/tuna_manners.py): the first is the profile in
# tuna_style.c as it is.

MANNERS = {
    "TEN HOLT": [
        # Canto Ostinato: the 3+3+2+2 piano figure over a slow cycle, fixed bass ostinato
        dict(name="CANTO"),
        # Horizon, Lemniscaat, Incantatie: the same machinery turned darker and wilder: minor, a denser figure in
        # a longer additive bar, accents and off-beat melody, harmonies with sevenths and added notes
        dict(name="HORIZON", root="E", scale="AEOLIAN", bpm=160, meter=[3, 2, 3, 2, 2], melody_density=0.35,
             syncopation=0.3, sforzando=0.2, swell=0.6,
             cycle=["iadd9", "VImaj7", "iv7", "V"], cycle2=["i7", "IIIadd9", "VIadd9", "V"],
             pad_figure=[0, 2, 4, 1, 3, 5, 2, 4, 3, 1, 2, 0], figure_len=12,
             orch=[("PIANO", "PIANO", "PIANO"), ("PIANO", "MARIMBA", "PIANO"), ("PIANO", "PIANO", "MARIMBA")],
             orch_mode="RANDOM"),
    ],
    "GLASS": [
        # Einstein on the Beach, Music in Twelve Parts, Glassworks: the additive figure on organs and winds
        dict(name="ENSEMBLE"),
        # Metamorphosis, the Piano Etudes, Mad Rush: solo piano, slower, rocking triplet broken chords under a
        # plain singing line, one chord a bar
        dict(name="ETUDE", root="E", bpm=84, division=3, chord_beats=4, period_bars=4, melody_density=0.35,
             mel_min_ticks=3, pad_mode="FIGURE", pad_figure=[0, 1, 2, 1, 2, 1], figure_len=6, pad_base=52,
             rubato=0.15, swell=0.4, reverb="HALL",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # Koyaanisqatsi, The Hours: running string arpeggios in sixteenths, a slow chant above, low choir
        dict(name="SCORE", root="F", bpm=72, division=4, chord_beats=4, period_bars=4, melody_density=0.25,
             mel_min_ticks=8, melody_base=60, pad_mode="ARPEGGIO", swell=0.8, reverb="HALL",
             cycle=["i", "VI", "iv", "V"],
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("CELLO", "VIOLIN", "PIANO"), ("ORGAN", "CHOIR", "ORGAN")],
             orch_mode="RANDOM"),
    ],
    "REICH": [
        # Piano Phase, Drumming, Six Marimbas: one pattern against itself, drifting
        dict(name="PHASE"),
        # Music for 18 Musicians, Octet: pulsing chords in repeated eighths over a slow cycle of chords, mallets,
        # clarinets and voices playing the pattern
        dict(name="PULSE", root="D", scale="IONIAN", vocab="POP", harmony="CYCLE", bpm=100, division=4,
             beats_per_bar=4, chord_beats=8, period_bars=4,
             cycle=["Iadd9", "vi7", "IVmaj7", "Vsus4"], cycle_len=4,
             bass_mode="REPEAT", pad_mode="REPEAT", pad_tones=4, pad_base=57, swell=0.7,
             orch=[("PIANO", "MARIMBA", "PIANO"), ("CLARINET", "VOICE", "PIANO"), ("CELLO", "XYLOPHONE", "PIANO"),
                   ("CLARINET", "CLARINET", "MARIMBA")], orch_mode="RANDOM"),
        # Tehillim: the psalm sung in a canon of voices in changing bar lengths, over hand drums and tambourines
        dict(name="PSALM", bpm=132, division=2, beats_per_bar=6, chord_beats=6, meter=[2, 3, 2, 2, 3],
             mel_mode="PHRASE", melody_density=0.6, mel_min_ticks=1, max_leap=3,
             pad_mode="CANON", canon_ticks=4, canon_steps=-2, motif=0.7, motif_use=0.8,
             perc_mode="PERIOD", perc=[0, 0, 4, 0, 0, 10], reverb="ROOM",
             orch=[("CONTRABASS", "VOICE", "VOICE"), ("ORGAN", "VOICE", "CLARINET"), ("CONTRABASS", "VOICE", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "PART": [
        # Spiegel im Spiegel, Fur Alina, Fratres: tintinnabuli for one or two instruments
        dict(name="TINTINAB"),
        # Missa syllabica, Berliner Messe, Magnificat, Passio: choir and organ, syllabic, a held pedal, phrases
        # parted by silences in a large stone space
        dict(name="MASS", root="D", scale="AEOLIAN", bpm=60, melody_density=0.6, mel_min_ticks=2,
             bass_mode="PEDAL", gp=0.35, swell=0.2, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "CHOIR", "STRINGS"),
                   ("PIPEORGAN", "VOICE", "REEDORGAN")], orch_mode="RANDOM"),
        # Cantus in Memory of Benjamin Britten: falling A minor scales in canon through the strings, the low
        # toll, growing louder all the way
        dict(name="CANTUS", root="A", scale="AEOLIAN", bpm=100, beats_per_bar=6, chord_beats=6, melody_density=0.6,
             melody_base=64, melody_range=19, mel_mode="PHRASE", direction=-1.0, profile=[0.02, 0.92, 0.99, 1.0], max_leap=3,
             pad_mode="CANON", canon_ticks=12, canon_steps=-7, swell=1.0, evolve_periods=8, evolve_depth=1.0,
             reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "STRINGS", "STRINGS"),
                   ("CONTRABASS", "STRINGS", "CELLO")], orch_mode="FIXED"),
    ],
    "GORECKI": [
        # Symphony of Sorrowful Songs: the slow canon and the lament over strings
        dict(name="SYMPHONY"),
        # Totus Tuus, Amen, Euntes ibant: a cappella choir, plain diatonic chords, the same words repeated,
        # held fermatas and pauses
        dict(name="MOTET", root="G", scale="IONIAN", vocab="CLASSIC", harmony="PLANNED", bpm=50, chord_beats=2,
             period_bars=4, melody_density=0.35, mel_min_ticks=2, pad_mode="BLOCK", pad_tones=4, pad_base=55,
             repeat=0.8, plagal=True, gp=0.3, rubato=0.4, swell=0.6, evolve_periods=0, reverb="CATHEDRAL",
             w_major={"I": 10, "IV": 7, "V": 5, "vi": 5, "ii": 4, "iii": 2, "IVmaj7": 2},
             w_minor={"i": 10, "iv": 7, "V": 5, "VI": 5, "III": 3},
             orch=[("CELLO", "CHOIR", "STRINGS"), ("PIPEORGAN", "CHOIR", "PIPEORGAN"),
                   ("CONTRABASS", "VOICE", "STRINGS")], orch_mode="RANDOM"),
        # Harpsichord Concerto: relentless driving repeated chords on a drone-like D minor, hammered accents
        dict(name="CONCERTO", root="D", bpm=152, division=2, chord_beats=4, period_bars=4, melody_density=0.9,
             mel_min_ticks=1, staccato=0.5, sforzando=0.3, max_leap=4, bass_mode="REPEAT", pad_mode="REPEAT",
             swell=0.3, rubato=0.0, evolve_periods=0, cycle=["i", "VI", "i", "V"], cycle_len=4, reverb="ROOM",
             orch=[("CELLO", "HARPSICHORD", "STRINGS"), ("CONTRABASS", "STRINGS", "HARPSICHORD"),
                   ("HARPSICHORD", "HARPSICHORD", "STRINGS")], orch_mode="RANDOM"),
    ],
    "TAVENER": [
        # Song for Athene, the Orthodox liturgical works: chant over the ison
        dict(name="LITURGY"),
        # The Protecting Veil, Svyati: the cello's long cantilena over held strings and the ison
        dict(name="LAMENT", bpm=56, melody_base=57, melody_range=17, mel_mode="PHRASE", melody_density=0.35,
             mel_min_ticks=2, appoggiatura=0.2, swell=0.8, rubato=0.4, alt_every=0, reverb="HALL",
             orch=[("CONTRABASS", "CELLO", "STRINGS"), ("DRONE", "CELLO", "STRINGS"), ("CELLO", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
        # The Lamb, The Tyger: short homophonic carols for choir, G major, a chord on each half bar
        dict(name="CAROL", root="G", scale="IONIAN", tuning="EQUAL", harmony="PLANNED", bpm=60, chord_beats=2,
             period_bars=2, melody_density=0.5, mel_min_ticks=2, mel_mode="PHRASE", bass_mode="ROOT",
             pad_mode="BLOCK", pad_tones=4, pad_base=55, alt_every=0, gp=0.2, reverb="CATHEDRAL",
             w_major={"i": 10, "iv": 5, "v": 4, "vi": 5, "ii": 3, "iii": 2, "Vsus4": 2, "isus2": 3},
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CELLO", "CHOIR", "STRINGS"),
                   ("CONTRABASS", "VOICE", "REEDORGAN")], orch_mode="RANDOM"),
    ],
    "VASKS": [
        # Musica dolorosa, Cantabile: strings over a pedal, birdsong breaking in
        dict(name="ELEGY"),
        # Dona nobis pacem, Pater noster: choir and organ or strings, homophonic, folk-like, no birds
        dict(name="CHORAL", bpm=58, melody_mask=0, melody_range=10, profile=[0.08, 0.85, 0.96, 0.99], max_leap=3,
             mel_min_ticks=2, bass_mode="ROOT", alt_every=0, reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("PIPEORGAN", "CHOIR", "PIPEORGAN"),
                   ("CELLO", "CHOIR", "STRINGS")], orch_mode="RANDOM"),
        # Distant Light, the violin concerto: an agitated solo violin, wide leaps, trembling strings, birdsong
        dict(name="CONCERTO", bpm=88, melody_density=0.55, melody_mask=0, melody_range=19, max_leap=6,
             profile=[0.04, 0.6, 0.82, 0.93], syncopation=0.2, sforzando=0.2, bass_mode="BAR", pad_mode="TREMOLO",
             reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "VIOLIN", "STRINGS")], orch_mode="RANDOM"),
    ],
    "WHITACRE": [
        # Lux Aurumque, Sleep, Water Night: the cluster choir
        dict(name="CHORAL"),
        # October, Sleep and Lux for winds: the lyrical wind band, warmer and moving more, swelling to climaxes
        # with timpani and cymbals
        dict(name="WINDBAND", root="D#", bpm=66, melody_density=0.45, mel_min_ticks=2, melody_range=14,
             max_leap=5, swell=0.9, rubato=0.2, perc_mode="PERIOD", perc=[5, 0, 2, 4, 0, 0], reverb="HALL",
             orch=[("TUBA", "HORN", "BRASS"), ("BASSOON", "FLUTE", "CLARINET"), ("TUBA", "CLARINET", "BRASS"),
                   ("TUBA", "TRUMPET", "BRASS")], orch_mode="ENERGY"),
    ],
    "RUTTER": [
        # the anthems (For the Beauty of the Earth, The Lord Bless You and Keep You): lilting 6/8 with harp
        dict(name="ANTHEM"),
        # the carols (Shepherd's Pipe Carol, Star Carol, Nativity Carol): brisk, bright, crisp, piping flute
        dict(name="CAROL", bpm=132, division=2, beats_per_bar=4, chord_beats=2, period_bars=4, melody_density=0.65,
             mel_min_ticks=1, staccato=0.4, rubato=0.0, swell=0.3, bass_mode="BAR", pad_mode="AFTERBEAT",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("BASSOON", "FLUTE", "PIZZ"), ("CONTRABASS", "OBOE", "STRINGS")],
             orch_mode="RANDOM"),
        # the Requiem (Pie Jesu, Lux aeterna): slow, soprano and oboe over organ, plagal amens, suspensions
        dict(name="REQUIEM", bpm=58, division=2, beats_per_bar=4, chord_beats=4, period_bars=4, melody_density=0.35,
             mel_min_ticks=2, melody_range=12, suspension=0.4, plagal=True, rubato=0.3, pad_mode="BLOCK",
             pad_tones=4, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "VOICE", "PIPEORGAN"), ("CONTRABASS", "OBOE", "STRINGS"),
                   ("CELLO", "CHOIR", "STRINGS")], orch_mode="RANDOM"),
    ],
}
