# SPDX-License-Identifier: GPL-3.0-only
# Manners of the Classical composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

MANNERS = {
    "GLUCK": [
        # the profile: Orfeo's ballet music (the Dance of the Blessed Spirits' D minor flute solo over a figure)
        dict(name="BALLET"),
        # the reform arias (Che faro senza Euridice, O del mio dolce ardor): a plain, syllabic tune in major,
        # four square bars, the strings repeating the chord in eighths, no ornament but the appoggiatura
        dict(name="ARIA", root="C", scale="IONIAN", bpm=76, division=2, chord_beats=2, period_bars=4,
             melody_density=0.4, melody_range=12, profile=[0.15, 0.78, 0.94, 0.99], max_leap=3, mel_min_ticks=1,
             appoggiatura=0.3, bass_mode="BAR", pad_mode="REPEAT", pad_base=53, motif=0.5, motif_use=0.7,
             swell=0.2, rubato=0.25, reverb="HALL",
             orch=[("CELLO", "VOICE", "STRINGS"), ("CONTRABASS", "VOICE", "STRINGS"),
                   ("BASSOON", "OBOE", "STRINGS")], orch_mode="RANDOM"),
        # the overtures and the Dance of the Furies: D minor allegro, tremolo strings, sforzandi, timpani
        dict(name="OVERTURE", bpm=132, division=4, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.6, profile=[0.2, 0.55, 0.78, 0.92], max_leap=5, appoggiatura=0.05, syncopation=0.2,
             bass_mode="REPEAT", pad_mode="TREMOLO", pad_base=55, sforzando=0.15, swell=0.7, rubato=0.0,
             motif=0.7, motif_beats=2, motif_use=0.85, perc_mode="PERIOD", perc=[8, 0, 0, 0, 0, 0],
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "OBOE", "STRINGS"),
                   ("CONTRABASS", "HORN", "STRINGS")], orch_mode="RANDOM"),
    ],
    "HAYDN": [
        # the profile: the symphonies' and quartets' witty 2/4 allegros (general pauses, echoes)
        dict(name="SYMPHONY"),
        # the minuets: 3/4, bass on the downbeat and the strings on two and three, horns and timpani, trills
        dict(name="MINUET", bpm=132, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.55, profile=[0.15, 0.6, 0.82, 0.94], max_leap=5, bass_mode="BAR",
             pad_mode="AFTERBEAT", staccato=0.3, sforzando=0.08, gp=0.1, echo=0.15, hemiola=True,
             perc_mode="PERIOD", perc=[5, 0, 0, 0, 0, 0],
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "OBOE", "HORN"),
                   ("CELLO", "FLUTE", "STRINGS")], orch_mode="RANDOM",
             octave=0, direction=0, melody_range=17),
        # the hymn (Gott erhalte, the Emperor quartet) and the Creation's chorales: slow, a chord on every beat
        dict(name="HYMN", bpm=66, division=2, beats_per_bar=4, chord_beats=1, period_bars=4,
             melody_density=0.3, melody_range=16, profile=[0.12, 0.64, 0.9, 0.98], max_leap=4, mel_min_ticks=2,
             trill=False, appoggiatura=0.15, bass_mode="WALKING", pad_mode="BLOCK", pad_tones=3, pad_base=55,
             fifth_bias=2.0, gp=0.0, echo=0.0, swell=0.4, rubato=0.3, motif=0.3, motif_use=0.4, arrival_bars=2,
             staccato=0.0, perc_mode="OFF", reverb="HALL",
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "CHOIR", "STRINGS"),
                   ("BASSOON", "CLARINET", "STRINGS")], orch_mode="RANDOM",
             octave=0.04, direction=-0.04),
    ],
    "MOZART": [
        # the profile: the piano sonatas' allegros over an Alberti bass (K. 545)
        dict(name="SONATA"),
        # the opera arias: a singing line over strings repeating the chord, appoggiaturas, held cadences
        dict(name="ARIA", bpm=76, division=2, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.35, melody_range=20, profile=[0.14, 0.65, 0.84, 0.94], appoggiatura=0.45,
             mel_min_ticks=1, bass_mode="BAR", pad_mode="REPEAT", pad_base=53, rubato=0.3, swell=0.4,
             arrival_bars=2, reverb="HALL",
             orch=[("CELLO", "VOICE", "STRINGS"), ("CONTRABASS", "VOICE", "STRINGS"),
                   ("BASSOON", "CLARINET", "STRINGS")], orch_mode="RANDOM",
             octave=0.02, direction=-0.04),
        # the serenades and divertimenti (Eine kleine Nachtmusik): G major, rockets, staccato, driving strings
        dict(name="SERENADE", root="G", bpm=132, division=2, beats_per_bar=4, chord_beats=4, period_bars=8,
             melody_density=0.72, profile=[0.26, 0.48, 0.71, 0.94], max_leap=5, octave=0.11, staccato=0.45,
             appoggiatura=0.15, bass_mode="REPEAT", pad_mode="REPEAT", pad_base=55, sforzando=0.05, echo=0.2,
             motif=0.7, motif_beats=2, motif_use=0.85,
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "VIOLIN", "STRINGS"),
                   ("BASSOON", "OBOE", "HORN"), ("CELLO", "CLARINET", "STRINGS")], orch_mode="RANDOM",
             direction=-0.02, melody_range=20),
        # the masses and the Requiem: D minor, slow, choir over organ and strings, suspensions, trombone-dark
        dict(name="MASS", root="D", scale="AEOLIAN", bpm=60, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_density=0.3, melody_range=12, profile=[0.15, 0.8, 0.94, 0.99], max_leap=4,
             mel_min_ticks=2, appoggiatura=0.15, suspension=0.35, trill=False, bass_mode="WALKING",
             pad_mode="BLOCK", pad_tones=4, pad_base=53, fifth_bias=1.8, swell=0.6, rubato=0.3, arrival_bars=2,
             perc_mode="PERIOD", perc=[6, 0, 0, 0, 0, 0], reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("PIPEORGAN", "CHOIR", "PIPEORGAN"),
                   ("BASSOON", "BRASS", "STRINGS")], orch_mode="RANDOM",
             octave=0, direction=0),
    ],
    "BEETHOVEN": [
        # the profile: the symphonies' C minor allegros (the Fifth): short motif, sforzandi, timpani
        dict(name="SYMPHONY"),
        # the slow sonata movements (Moonlight, Pathetique adagio): triplets rolling under a sparse, low tune
        dict(name="SONATA", root="C#", bpm=54, division=3, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.33, melody_base=60, melody_range=18, profile=[0.2, 0.54, 0.82, 0.95], max_leap=4,
             mel_min_ticks=2, syncopation=0.0, sforzando=0.0, bass_mode="ROOT", pad_mode="FIGURE",
             pad_figure=[0, 1, 2], pad_base=50, swell=0.5, rubato=0.25, motif=0.6, motif_use=0.7,
             perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED",
             octave=0.05, direction=0.04),
        # the scherzi: one in a bar at 3/4, staccato, sudden accents, hemiolas
        dict(name="SCHERZO", bpm=260, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.5, profile=[0.19, 0.51, 0.7, 0.84], staccato=0.55, sforzando=0.2, hemiola=True,
             bass_mode="BAR", pad_mode="AFTERBEAT", gp=0.15, swell=0.6, rubato=0.0,
             perc_mode="PERIOD", perc=[7, 0, 0, 0, 0, 0],
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "HORN", "STRINGS"),
                   ("PIANO", "PIANO", "PIANO")], orch_mode="RANDOM",
             octave=0.04, direction=-0.04, melody_range=23),
        # the bagatelles (Fur Elise): A minor 3/8, a winding tune over broken chords, piano alone
        dict(name="BAGATELL", root="A", bpm=132, division=2, beats_per_bar=3, chord_beats=3, period_bars=4,
             melody_density=0.6, melody_base=64, melody_range=20, profile=[0.2, 0.56, 0.8, 0.94], max_leap=4,
             appoggiatura=0.2, chromatic=True, sforzando=0.0, syncopation=0.0, bass_mode="ROOT",
             pad_mode="BROKEN", pad_base=45, swell=0.2, rubato=0.3, repeat=0.6, perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED",
             octave=0.05, direction=-0.13),
    ],
}
