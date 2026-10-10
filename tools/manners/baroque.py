# SPDX-License-Identifier: GPL-3.0-only
# Manners of the Baroque composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

MANNERS = {
    "BACH": [
        dict(name="FUGUE"),
        # the four-part chorales: homophonic, a chord on every beat, stepping bass, phrases of two bars that halt
        # on a fermata (rubato into the cadence), choir and organ
        dict(name="CHORALE", reverb="CATHEDRAL", bpm=64, division=2, chord_beats=1, period_bars=2, melody_density=0.39,
             melody_range=16, profile=[0.24, 0.85, 0.9, 0.98], max_leap=3, mel_min_ticks=2, suspension=0.2,
             bass_mode="WALKING", pad_mode="BLOCK", pad_tones=4, pad_base=55, fifth_bias=3.0, open=0.25,
             rubato=0.6, motif=0.1, motif_use=0.3, arrival_bars=1,
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "CHOIR", "STRINGS"),
                   ("BASSOON", "OBOE", "PIPEORGAN")], orch_mode="RANDOM",
             octave=0.05, direction=-0.16),
        # the suites' gigues: 12/8, leaping lines over a running bass, two voices
        dict(name="GIGUE", bpm=96, division=3, beats_per_bar=4, chord_beats=2, period_bars=4, melody_density=1,
             profile=[0.02, 0.37, 0.67, 0.86], max_leap=5, suspension=0.1, bass_mode="WALKING",
             pad_mode="COUNTER", motif=0.5, motif_use=0.9,
             orch=[("CELLO", "VIOLIN", "HARPSICHORD"), ("OUD", "OUD", "OUD"),
                   ("HARPSICHORD", "HARPSICHORD", "HARPSICHORD")], orch_mode="RANDOM",
             octave=0.02, direction=-0.04, melody_range=23),
    ],
    "BUXTEHUDE": [
        # the organ praeludia (stylus phantasticus): free toccata periods and fugal periods over a pedal, pauses
        dict(name="PRELUDE"),
        # the chorale preludes (Nun komm der Heiden Heiland BuxWV 211): the hymn tune richly ornamented in the
        # soprano over a quiet three-part accompaniment, slow, a halt at each line
        dict(name="CHORALE", bpm=56, chord_beats=2, period_bars=2, melody_density=0.5, melody_range=14,
             profile=[0.06, 0.8, 0.94, 0.99], max_leap=4, appoggiatura=0.3, suspension=0.35, turn=0.3,
             bass_mode="WALKING", pad_mode="BLOCK", pad_tones=3, pad_base=55, alt_every=0, gp=0, motif=0.3,
             motif_use=0.4, rubato=0.35, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "REEDORGAN", "PIPEORGAN"), ("PIPEORGAN", "OBOE", "PIPEORGAN"),
                   ("PIPEORGAN", "FLUTE", "PIPEORGAN")], orch_mode="RANDOM"),
        # the ciaconas and the passacaglia (BuxWV 159-161): 3/4 over a four-bar ground, the variations
        # growing in figuration
        dict(name="CIACONA", bpm=72, beats_per_bar=3, chord_beats=3, period_bars=4, harmony="CYCLE",
             cycle=["i", "VI", "iv", "V"], bass_mode="WALKING", pad_mode="COUNTER", melody_density=0.6,
             motif=0.5, motif_use=0.8, alt_every=0, gp=0, evolve_periods=2, evolve_depth=0.7,
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("PIPEORGAN", "REEDORGAN", "PIPEORGAN"),
                   ("HARPSICHORD", "HARPSICHORD", "HARPSICHORD")], orch_mode="RANDOM"),
        # the vocal concertos and cantatas (Membra Jesu nostri): voices over continuo, two violins weaving
        # above, triple-time arias
        dict(name="CANTATA", bpm=80, division=2, beats_per_bar=3, chord_beats=3, period_bars=4,
             melody_density=0.45, melody_range=14, profile=[0.06, 0.8, 0.93, 0.98], max_leap=4,
             suspension=0.35, trill=False, bass_mode="WALKING", pad_mode="COUNTER", pad_base=57,
             alt_every=0, gp=0, motif_beats=3, rubato=0.2, reverb="CATHEDRAL",
             orch=[("CELLO", "VOICE", "VIOLIN"), ("PIPEORGAN", "CHOIR", "VIOLIN"),
                   ("CONTRABASS", "VOICE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "PACHELBEL": [
        # the Canon: eight chords over a ground, violins in canon two bars apart, the figuration growing
        dict(name="CANON"),
        # the organ chorale preludes (Vom Himmel hoch, Wie schon leuchtet der Morgenstern): the chorale in long
        # notes over a running fugal accompaniment
        dict(name="CHORALE", harmony="PLANNED",
             w_major={"I": 10, "vi": 6, "iii": 3, "IV": 7, "ii": 5, "ii7": 4, "V": 8, "V7": 7, "vii°": 3,
                      "V7/V": 4, "V7/vi": 2},
             w_minor={"i": 10, "VI": 6, "III": 5, "iv": 7, "ii°": 3, "iiø7": 4, "V": 8, "V7": 7, "vii°": 3,
                      "V7/V": 3, "VII": 4},
             bpm=66, chord_beats=2, period_bars=2, melody_density=0.25, mel_min_ticks=4, melody_range=17,
             profile=[0.09, 0.81, 0.92, 0.98], max_leap=3, suspension=0.3, bass_mode="WALKING",
             pad_mode="COUNTER", pad_base=52, motif=0.3, motif_use=0.4, evolve_periods=0, rubato=0.3,
             arrival_bars=1, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("PIPEORGAN", "REEDORGAN", "PIPEORGAN"),
                   ("PIPEORGAN", "OBOE", "PIPEORGAN")], orch_mode="RANDOM",
             octave=0.03, direction=-0.06),
        # the chaconnes for keyboard (F minor): 3/4 over the falling tetrachord F Eb Db C, variations
        dict(name="CIACONA", root="F", scale="AEOLIAN", bpm=80, beats_per_bar=3, chord_beats=3, period_bars=4,
             cycle=["i", "VII", "VI", "V"], bass_mode="LINE", bass_notes=[0, -2, -4, -5], pad_mode="COUNTER",
             pad_base=53, melody_density=0.6, melody_base=65, motif=0.5, motif_use=0.8, evolve_periods=2,
             evolve_depth=0.8, suspension=0.3, trill=True,
             orch=[("HARPSICHORD", "HARPSICHORD", "HARPSICHORD"), ("PIPEORGAN", "PIPEORGAN", "PIPEORGAN")],
             orch_mode="RANDOM",
             profile=[0.1, 0.64, 0.86, 0.96], octave=0.05, direction=-0.08, melody_range=20),
    ],
    "PURCELL": [
        # the laments over a ground (Dido's lament, O let me weep): the chromatic descent in 3/2, sighs
        dict(name="LAMENT"),
        # the court odes and welcome songs (Come ye sons of art, Sound the trumpet): D major, trumpets and
        # drums, a running bass, the strings in repeated chords
        dict(name="ODE", root="D", scale="IONIAN", vocab="CLASSIC", harmony="PLANNED",
             w_major={"I": 10, "vi": 5, "iii": 2, "IV": 7, "ii": 5, "ii7": 3, "V": 9, "V7": 8, "vii°": 2,
                      "V7/V": 5, "V7/vi": 2},
             w_minor={"i": 10, "VI": 5, "III": 5, "iv": 7, "ii°": 2, "iiø7": 4, "V": 8, "V7": 8, "vii°": 3,
                      "V7/V": 3, "VII": 3},
             bpm=96, division=4, beats_per_bar=4, chord_beats=2, period_bars=4, melody_density=0.55,
             melody_range=18, profile=[0.18, 0.55, 0.75, 0.9], appoggiatura=0.1, chromatic=False, trill=True,
             direction=-0.04, bass_mode="WALKING", pad_mode="REPEAT", pad_base=52, perc_mode="PERIOD",
             perc=[7, 0, 0, 0, 0, 0], swell=0.3, rubato=0, reverb="HALL",
             orch=[("CONTRABASS", "TRUMPET", "STRINGS"), ("BASSOON", "TRUMPET", "OBOE"),
                   ("CELLO", "VOICE", "STRINGS")], orch_mode="CONTRAST",
             octave=0.01),
        # the theatre dances (Abdelazer, The Fairy Queen hornpipes): quick 3/2, syncopated, hemiola at the
        # cadence, four-part strings
        dict(name="HORNPIPE", vocab="CLASSIC", harmony="PLANNED",
             w_major={"I": 10, "vi": 5, "iii": 2, "IV": 6, "ii": 5, "V": 9, "V7": 7, "vii°": 2, "V7/V": 4},
             w_minor={"i": 10, "VI": 5, "III": 6, "iv": 7, "ii°": 2, "V": 8, "V7": 7, "vii°": 3, "V7/V": 3,
                      "VII": 5, "V7/III": 3},
             bpm=100, division=2, beats_per_bar=3, chord_beats=3, period_bars=4, melody_density=0.65,
             profile=[0.13, 0.5, 0.69, 0.9], syncopation=0.35, staccato=0.3, appoggiatura=0.1,
             suspension=0.1, chromatic=False, hemiola=True, direction=0.01, bass_mode="WALKING", pad_mode="BLOCK",
             motif=0.6, motif_use=0.8, rubato=0, swell=0.2, reverb="ROOM",
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("HARPSICHORD", "VIOLIN", "HARPSICHORD"),
                   ("BASSOON", "OBOE", "STRINGS")], orch_mode="RANDOM",
             max_leap=4, octave=0.02, melody_range=16),
        # the Music for the Funeral of Queen Mary: C minor, a slow homophonic march, flat trumpets and sackbuts,
        # muffled drums, a chord on every beat
        dict(name="FUNERAL", root="C", vocab="CLASSIC", harmony="PLANNED",
             w_major={"I": 10, "vi": 5, "IV": 7, "ii": 5, "V": 9, "V7": 6, "vii°": 2},
             w_minor={"i": 10, "VI": 6, "III": 4, "iv": 8, "ii°": 3, "iiø7": 3, "V": 9, "V7": 6, "vii°": 3,
                      "VII": 3},
             bpm=52, division=2, beats_per_bar=4, chord_beats=1, period_bars=4, melody_density=0.35,
             mel_min_ticks=2, melody_range=14, profile=[0.19, 0.83, 0.93, 0.98], max_leap=4, appoggiatura=0.15,
             suspension=0, chromatic=False, direction=-0.14, bass_mode="ROOT", pad_mode="BLOCK", pad_tones=4,
             perc_mode="PERIOD", perc=[4, 6, 0, 0, 0, 0], arrival_bars=2, gp=0.3, swell=0.5, rubato=0.3,
             reverb="CATHEDRAL",
             orch=[("TUBA", "TRUMPET", "BRASS"), ("PIPEORGAN", "CHOIR", "PIPEORGAN"),
                   ("BASSOON", "TRUMPET", "BRASS")], orch_mode="RANDOM",
             octave=0.02),
    ],
    "VIVALDI": [
        # the fast movements of the concertos: motor rhythm, drum bass, sequences, tutti and solo in turn
        dict(name="CONCERTO"),
        # the slow movements (Winter's and Spring's Largo): a long violin cantilena over pizzicato eighths
        dict(name="LARGO", root="EB", scale="IONIAN", bpm=58, chord_beats=2, melody_density=0.59,
             melody_range=21, profile=[0.08, 0.64, 0.82, 0.94], appoggiatura=0.3, suspension=0.2,
             bass_mode="ROOT", pad_mode="REPEAT", pad_base=55, alt_every=0, echo=0, rubato=0.2, swell=0.3,
             reverb="HALL",
             orch=[("CELLO", "VIOLIN", "PIZZ"), ("CONTRABASS", "VIOLIN", "PIZZ"), ("CELLO", "FLUTE", "PIZZ")],
             orch_mode="RANDOM",
             max_leap=4, octave=0.03, direction=-0.14),
        # the sacred choral works (Gloria RV 589): D major, choir over driving octave-leaping strings, trumpet
        dict(name="GLORIA", root="D", scale="IONIAN", bpm=104, melody_density=0.5, melody_range=21,
             profile=[0.35, 0.58, 0.75, 0.88], octave=0.14, alt_every=0, echo=0, swell=0.4, reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("CELLO", "TRUMPET", "STRINGS"),
                   ("PIPEORGAN", "CHOIR", "OBOE")], orch_mode="RANDOM",
             direction=-0.2,
             max_leap=5),
        # the storm concertos (Summer's Presto, La tempesta di mare): G minor, tremolo strings, rushing scales
        dict(name="TEMPESTA", root="G", bpm=138, melody_density=0.9, melody_range=25,
             profile=[0.08, 0.63, 0.77, 0.94], bass_mode="REPEAT", pad_mode="TREMOLO", sforzando=0.15,
             swell=0.4, echo=0, alt_every=2,
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CONTRABASS", "VIOLIN", "STRINGS")], orch_mode="RANDOM",
             octave=0.01, direction=-0.03,
             max_leap=5),
    ],
    "ALBINONI": [
        # the Adagio: slow pizzicato walking bass, organ chords, a long sighing violin line
        dict(name="ADAGIO"),
        # the oboe concertos (Op. 7, Op. 9): allegros, the oboe over driving strings, terraced echoes
        dict(name="CONCERTO", root="D", bpm=108, division=4, chord_beats=2, melody_density=0.6,
             profile=[0.1, 0.6, 0.85, 0.95], appoggiatura=0.1, suspension=0.1, arrival_bars=1,
             bass_mode="REPEAT", pad_mode="REPEAT", pad_base=52, echo=0.25, swell=0.2, rubato=0,
             orch=[("CELLO", "OBOE", "STRINGS"), ("HARPSICHORD", "OBOE", "STRINGS"),
                   ("CONTRABASS", "VIOLIN", "STRINGS")], orch_mode="CONTRAST"),
        # the trio sonatas (Op. 1, sonate da chiesa): two violins in imitation over a walking continuo
        dict(name="SONATA", bpm=76, division=4, melody_density=0.5, suspension=0.4, appoggiatura=0.2,
             pad_mode="COUNTER", pad_base=55, swell=0.3, rubato=0.1, reverb="ROOM",
             orch=[("CELLO", "VIOLIN", "VIOLIN"), ("HARPSICHORD", "VIOLIN", "VIOLIN"),
                   ("CELLO", "OBOE", "VIOLIN")], orch_mode="RANDOM"),
    ],
    "HANDEL": [
        # the orchestral suites (Water Music, Music for the Royal Fireworks): trumpets and drums, marching eighths
        dict(name="SUITE"),
        # the oratorio choruses (Messiah's Hallelujah, Zadok the Priest): choir in homophonic acclamations,
        # repeated notes, the great pause, trumpets and drums
        dict(name="ORATORIO", bpm=76, chord_beats=2, melody_density=0.45, profile=[0.37, 0.63, 0.76, 0.91],
             pad_mode="BLOCK", pad_tones=4, gp=0.3, swell=0.5, sforzando=0.1, reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("PIPEORGAN", "CHOIR", "TRUMPET"),
                   ("BASSOON", "TRUMPET", "STRINGS")], orch_mode="RANDOM",
             octave=0.02, direction=-0.13, melody_range=20,
             max_leap=5),
        # the opera arias (Ombra mai fu, Lascia ch'io pianga): a slow sarabande in F, a voice over strings
        dict(name="ARIA", root="F", bpm=50, division=2, beats_per_bar=3, chord_beats=3, period_bars=4,
             melody_density=0.35, melody_range=13, profile=[0.05, 0.75, 0.9, 0.97], appoggiatura=0.3,
             suspension=0.3, hemiola=True, bass_mode="ROOT", pad_mode="BLOCK", perc_mode="OFF", gp=0.1,
             rubato=0.3, swell=0.5, sforzando=0, reverb="HALL",
             orch=[("CELLO", "VOICE", "STRINGS"), ("CONTRABASS", "VOICE", "STRINGS"),
                   ("HARPSICHORD", "OBOE", "STRINGS")], orch_mode="RANDOM",
             max_leap=5, octave=0, direction=0),
        # the keyboard suites' passacaglia (HWV 432): G minor, the ground i iv VII III VI ii° V i, variations
        dict(name="PASSACAG", root="G", scale="AEOLIAN", harmony="CYCLE",
             cycle=["i", "iv", "VII", "III", "VI", "ii°", "V", "i"], bpm=76, chord_beats=2, period_bars=4,
             melody_density=0.65, bass_mode="ROOT", pad_mode="COUNTER", perc_mode="OFF", gp=0, evolve_periods=2,
             evolve_depth=0.8, sforzando=0, swell=0.2,
             orch=[("HARPSICHORD", "HARPSICHORD", "HARPSICHORD"), ("PIANO", "PIANO", "PIANO")],
             orch_mode="RANDOM",
             profile=[0.17, 0.49, 0.69, 0.9], octave=0.02, direction=-0.1, melody_range=20,
             max_leap=5),
    ],
    "SCARLATTI": [
        # the keyboard sonatas: fast 3/8, leaps across the keyboard, repeated notes, strummed Spanish chords
        dict(name="SONATA"),
        # the slow cantabile sonatas (K. 208, K. 32 "Aria"): a singing line over broken chords
        dict(name="ARIA", root="A", bpm=60, division=4, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.45, melody_range=20, profile=[0.06, 0.64, 0.81, 0.94], max_leap=5, repeat=0.5,
             appoggiatura=0.3, octave=0.04, sforzando=0, bass_mode="ROOT", pad_mode="BROKEN", color=0,
             mixture=0.1, strum_ms=0, rubato=0.2,
             orch=[("HARPSICHORD", "HARPSICHORD", "HARPSICHORD"), ("PIANO", "PIANO", "PIANO"),
                   ("GUITAR", "HARPSICHORD", "GUITAR")], orch_mode="RANDOM",
             direction=-0.04),
        # the Stabat Mater a 10: C minor, ten voices in polyphony with organ, suspensions
        dict(name="STABAT", root="C", scale="AEOLIAN", bpm=60, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_density=0.4, melody_range=14, profile=[0.04, 0.82, 0.94, 0.98], max_leap=4,
             suspension=0.5, repeat=0.3, sforzando=0, octave=0, color=0, mixture=0, strum_ms=0, trill=False,
             bass_mode="ROOT", pad_mode="COUNTER", rubato=0.2, swell=0.3, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "VOICE"), ("CELLO", "VOICE", "VOICE")], orch_mode="RANDOM",
             direction=0),
    ],
    "RAMEAU": [
        # the opera-ballet dances (Les Indes galantes, Les fetes d'Hebe): tambourins over a drone and a tabor,
        # gavottes between
        dict(name="BALLET"),
        # the Pieces de clavecin (Les cyclopes, Le rappel des oiseaux): D minor, batteries of broken chords,
        # ornaments on every other note
        dict(name="CLAVECIN", scale="AEOLIAN", bpm=100, period_bars=8, melody_density=0.65,
             bass_mode="ROOT", pad_mode="BROKEN", turn=0.4, appoggiatura=0.3, perc_mode="OFF", alt_every=0,
             reverb="ROOM",
             orch=[("HARPSICHORD", "HARPSICHORD", "HARPSICHORD")], orch_mode="FIXED",
             profile=[0.16, 0.63, 0.84, 0.92], octave=0.03, direction=-0.02, melody_range=18),
        # the tragedies lyriques' airs (Tristes apprets from Castor et Pollux): F minor, slow triple time,
        # chromatic sighs, bassoons
        dict(name="TRAGEDIE", root="F", scale="AEOLIAN", bpm=54, division=2, beats_per_bar=3, chord_beats=3,
             period_bars=4, melody_density=0.35, melody_range=14, appoggiatura=0.4, suspension=0.4,
             chromatic=True, turn=0.2, direction=-0.3, bass_mode="WALKING", pad_mode="BLOCK", perc_mode="OFF",
             alt_every=0, rubato=0.3, swell=0.5, reverb="HALL",
             orch=[("BASSOON", "VOICE", "STRINGS"), ("CELLO", "VOICE", "STRINGS"),
                   ("BASSOON", "OBOE", "STRINGS")], orch_mode="RANDOM",
             profile=[0.1, 0.68, 0.88, 0.96], octave=0),
    ],
    "PERGOLESI": [
        # the Stabat Mater (and Salve Regina): two high voices in thirds, chains of suspensions, sighs
        dict(name="STABAT"),
        # the comic intermezzi (La serva padrona): B flat major, quick 2/4 patter in repeated notes, staccato,
        # strings driving
        dict(name="BUFFA", root="BB", scale="IONIAN", bpm=132, beats_per_bar=2, chord_beats=2, period_bars=8,
             melody_density=0.7, profile=[0.3, 0.7, 0.88, 0.96], max_leap=5, staccato=0.5, appoggiatura=0.1,
             suspension=0, chromatic=False, direction=0, bass_mode="BEAT", pad_mode="REPEAT", pad_base=53,
             sforzando=0.1, swell=0.2, rubato=0, reverb="ROOM",
             orch=[("CELLO", "VOICE", "STRINGS"), ("HARPSICHORD", "VOICE", "STRINGS"),
                   ("CONTRABASS", "VIOLIN", "STRINGS")], orch_mode="RANDOM"),
    ],
}
