# SPDX-License-Identifier: GPL-3.0-only
# Manners of the early Romantic composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

MANNERS = {
    "SCHUBERT": [
        # the profile: the Lieder (Ave Maria, Standchen): a song over triplet chords, clarinet or voice and piano
        dict(name="LIED"),
        # the Unfinished: B minor 3/4, a cello/oboe song over pizzicato and syncopated strings, sudden outbursts
        dict(name="SYMPHONY", root="B", scale="AEOLIAN", bpm=120, division=2, beats_per_bar=3, chord_beats=3,
             period_bars=8, melody_density=0.4, melody_range=17, syncopation=0.3, bass_mode="BEAT",
             pad_mode="REPEAT", pad_base=55, sforzando=0.12, swell=0.7, rubato=0.1, gp=0.15,
             perc_mode="PERIOD", perc=[6, 0, 0, 0, 0, 0], reverb="HALL",
             orch=[("PIZZ", "CELLO", "STRINGS"), ("CONTRABASS", "OBOE", "STRINGS"),
                   ("CONTRABASS", "CLARINET", "STRINGS"), ("TUBA", "HORN", "STRINGS")], orch_mode="RANDOM"),
        # the marches (Marches militaires): D major 2/4, dotted and staccato, bass and chord in turn, piano duet
        dict(name="MARCH", root="D", bpm=112, division=4, beats_per_bar=2, chord_beats=2, period_bars=8,
             melody_density=0.55, profile=[0.3, 0.55, 0.82, 0.94], staccato=0.5, appoggiatura=0.05, turn=0.05,
             bass_mode="MARCH", pad_mode="OOMPAH", pad_base=55, mixture=0.15, sforzando=0.05, rubato=0.0,
             swell=0.4, arrival_bars=1,
             orch=[("PIANO", "PIANO", "PIANO"), ("TUBA", "TRUMPET", "BRASS"), ("CONTRABASS", "FLUTE", "STRINGS")],
             orch_mode="RANDOM"),
        # the Landler and German dances: 3/4, oom-pah-pah, a turning tune, piano
        dict(name="LANDLER", root="A#", bpm=150, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.55, profile=[0.12, 0.6, 0.85, 0.95], lilt=0.15, bass_mode="BAR", pad_mode="OOMPAH",
             pad_base=55, mixture=0.2, rubato=0.1, swell=0.2, repeat=0.6,
             orch=[("PIANO", "PIANO", "PIANO"), ("CELLO", "VIOLIN", "STRINGS")], orch_mode="RANDOM"),
    ],
    "ROSSINI": [
        # the profile: the overtures (the Rossini crescendo, staccato over oom-pah, rising orchestration)
        dict(name="OVERTURE"),
        # the buffa arias (Largo al factotum, Una voce poco fa): 6/8 patter, voice over strings, trills and turns
        dict(name="ARIA", root="C", bpm=132, division=3, beats_per_bar=2, chord_beats=2, period_bars=8,
             melody_density=0.7, melody_range=14, profile=[0.4, 0.6, 0.85, 0.95], staccato=0.4, turn=0.15,
             appoggiatura=0.1, bass_mode="BAR", pad_mode="AFTERBEAT", pad_base=55, swell=0.6, evolve_periods=0,
             perc_mode="OFF",
             orch=[("CELLO", "VOICE", "STRINGS"), ("CONTRABASS", "VOICE", "PIZZ"), ("BASSOON", "VOICE", "STRINGS")],
             orch_mode="RANDOM"),
        # the sacred works (Stabat Mater, Petite messe solennelle: choir, two pianos and a harmonium)
        dict(name="MASS", root="F", scale="AEOLIAN", bpm=72, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_density=0.35, melody_range=12, profile=[0.15, 0.75, 0.92, 0.98], max_leap=4,
             mel_min_ticks=1, trill=False, staccato=0.0, appoggiatura=0.2, suspension=0.25, bass_mode="ROOT",
             pad_mode="BLOCK", pad_tones=4, pad_base=53, fifth_bias=1.5, swell=0.5, rubato=0.25, arrival_bars=2,
             evolve_periods=0, perc_mode="OFF", reverb="CATHEDRAL",
             orch=[("PIANO", "CHOIR", "REEDORGAN"), ("PIANO", "CHOIR", "PIANO"), ("CONTRABASS", "CHOIR", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "BERLIOZ": [
        # the profile: the Symphonie fantastique (the idee fixe, tremolo contrasts, a big orchestra)
        dict(name="SYMPHONY"),
        # the waltzes (Un bal): A major 3/4, violins over harps, lilting
        dict(name="WALTZ", root="A", bpm=168, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.5, profile=[0.08, 0.62, 0.84, 0.94], lilt=0.2, bass_mode="BAR", pad_mode="AFTERBEAT",
             pad_base=55, alt_every=0, sforzando=0.0, swell=0.5, perc_mode="OFF", reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "HARP"), ("CELLO", "FLUTE", "STRINGS"),
                   ("CONTRABASS", "CLARINET", "STRINGS")], orch_mode="RANDOM"),
        # the marches (March to the Scaffold, Rakoczy): G minor, brass, dotted, snare, bass drum and cymbals
        dict(name="MARCH", root="G", scale="AEOLIAN", bpm=108, division=4, beats_per_bar=4, chord_beats=4,
             period_bars=8, melody_density=0.5, profile=[0.25, 0.5, 0.78, 0.92], staccato=0.4, sforzando=0.15,
             bass_mode="MARCH", pad_mode="REPEAT", alt_every=0, swell=0.9, rubato=0.0,
             perc_mode="PERIOD", perc=[9, 7, 6, 6, 0, 0],
             orch=[("TUBA", "BRASS", "STRINGS"), ("TUBA", "TRUMPET", "BRASS"), ("CONTRABASS", "BASSOON", "STRINGS")],
             orch_mode="RANDOM"),
        # the Requiem and Te Deum: slow, choir, brass choirs and timpani in a vast space
        dict(name="REQUIEM", root="G", scale="AEOLIAN", bpm=58, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_density=0.3, melody_range=12, profile=[0.15, 0.78, 0.93, 0.98], max_leap=5,
             mel_min_ticks=2, suspension=0.3, bass_mode="ROOT", pad_mode="BLOCK", pad_tones=4, alt_every=0,
             swell=0.8, rubato=0.3, gp=0.2, perc_mode="PERIOD", perc=[10, 0, 0, 4, 0, 0], reverb="CATHEDRAL",
             orch=[("TUBA", "CHOIR", "BRASS"), ("CONTRABASS", "CHOIR", "STRINGS"), ("TUBA", "CHOIR", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "FELIX MENDELSSOHN": [
        # the profile: the Italian symphony (A major 6/8, Allegro vivace)
        dict(name="SYMPHONY"),
        # the fairy scherzi (A Midsummer Night's Dream, the Octet): G minor, quick, pianissimo staccato woodwinds
        dict(name="SCHERZO", root="G", scale="AEOLIAN", bpm=88, division=4, beats_per_bar=3, chord_beats=3,
             period_bars=8, melody_density=0.75, melody_base=67, profile=[0.3, 0.7, 0.88, 0.96], max_leap=4,
             staccato=0.75, trill=False, bass_mode="BEAT", pad_mode="REPEAT", pad_base=59, swell=0.2,
             echo=0.35, perc_mode="OFF",
             orch=[("PIZZ", "FLUTE", "STRINGS"), ("PIZZ", "CLARINET", "STRINGS"), ("BASSOON", "FLUTE", "PIZZ")],
             orch_mode="RANDOM"),
        # the Songs without Words: a singing tune over broken chords, piano alone
        dict(name="LIED", root="E", bpm=66, division=4, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.35, melody_range=14, profile=[0.08, 0.72, 0.9, 0.97], staccato=0.0, trill=False,
             appoggiatura=0.2, bass_mode="ROOT", pad_mode="BROKEN", pad_base=47, swell=0.4, rubato=0.3,
             arrival_bars=2, reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the oratorios (Elijah, St Paul): chorales and choruses, choir and organ
        dict(name="ORATORIO", root="D", scale="AEOLIAN", bpm=72, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_density=0.3, melody_range=12, profile=[0.12, 0.84, 0.95, 0.99], max_leap=4,
             mel_min_ticks=2, staccato=0.0, trill=False, suspension=0.2, bass_mode="WALKING", pad_mode="BLOCK",
             pad_tones=4, pad_base=53, fifth_bias=2.0, swell=0.5, rubato=0.4, arrival_bars=2,
             perc_mode="PERIOD", perc=[5, 0, 0, 0, 0, 0], reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "CHOIR", "STRINGS"),
                   ("TUBA", "CHOIR", "BRASS")], orch_mode="RANDOM"),
    ],
    "FANNY MENDELSSOHN": [
        # the profile: her piano Lieder (Lieder fur das Pianoforte, Das Jahr): a song over wide broken chords
        dict(name="LIED"),
        # the songs for voice and piano (some 250): a simpler, syllabic line, the piano repeating the chord
        dict(name="SONG", bpm=72, division=2, chord_beats=2, melody_density=0.35, melody_range=12,
             profile=[0.1, 0.78, 0.93, 0.98], max_leap=4, mel_min_ticks=1, pad_mode="REPEAT", pad_base=51,
             rubato=0.25, swell=0.4,
             orch=[("PIANO", "VOICE", "PIANO"), ("PIANO", "CLARINET", "PIANO")], orch_mode="RANDOM"),
        # the Easter Sonata and the dramatic piano pieces: minor, fast rising arpeggios, accents
        dict(name="SONATA", root="C", scale="AEOLIAN", bpm=120, division=4, beats_per_bar=4, chord_beats=4,
             period_bars=8, melody_density=0.55, melody_range=19, profile=[0.12, 0.55, 0.8, 0.93], max_leap=6,
             octave=0.15, appoggiatura=0.1, pad_mode="ARPEGGIO", pad_base=43, sforzando=0.1, swell=0.8,
             rubato=0.15, reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the cantatas and the oratorio (Hiob, Oratorium nach Bildern der Bibel): choir and orchestra
        dict(name="CANTATA", root="D", scale="AEOLIAN", bpm=66, division=2, beats_per_bar=4, chord_beats=2,
             melody_density=0.3, melody_range=12, profile=[0.12, 0.8, 0.94, 0.99], max_leap=4, mel_min_ticks=2,
             suspension=0.3, pad_mode="BLOCK", pad_tones=4, pad_base=53, fifth_bias=1.8, rubato=0.3,
             perc_mode="PERIOD", perc=[5, 0, 0, 0, 0, 0], reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("BASSOON", "CHOIR", "STRINGS"),
                   ("CELLO", "OBOE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "CHOPIN": [
        # the profile: the nocturnes (E-flat, Op. 9 no. 2: 12/8, a singing line over wide broken chords)
        dict(name="NOCTURNE"),
        # the mazurkas: 3/4, accent on two or three, Lydian fourths, dotted figures, rubato
        dict(name="MAZURKA", root="A", scale="AEOLIAN", bpm=132, division=2, beats_per_bar=3, chord_beats=3,
             period_bars=8, melody_density=0.5, melody_range=14, profile=[0.15, 0.68, 0.88, 0.96], syncopation=0.2,
             sforzando=0.15, bass_mode="BAR", pad_mode="AFTERBEAT", pad_base=52, color_scale="LYDIAN", color=0.25,
             repeat=0.6, rubato=0.4, swell=0.3, mixture=0.0,
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the polonaises: heroic 3/4, sixteenth-note pickup figures, chords on every beat, accents
        dict(name="POLONAIS", root="G#", scale="IONIAN", bpm=92, division=4, beats_per_bar=3, chord_beats=3,
             period_bars=8, melody_density=0.55, melody_base=63, melody_range=19, profile=[0.2, 0.5, 0.78, 0.92],
             max_leap=6, octave=0.25, sforzando=0.15, syncopation=0.0, bass_mode="BAR", pad_mode="BLOCK",
             pad_tones=4, pad_base=55, spread=True, swell=0.9, rubato=0.15, appoggiatura=0.1,
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the etudes: fast sixteenth-note arpeggios over the whole keyboard, a tune in the top
        dict(name="ETUDE", root="C", scale="AEOLIAN", bpm=144, division=4, beats_per_bar=4, chord_beats=4,
             period_bars=8, melody_density=0.5, melody_base=67, profile=[0.15, 0.55, 0.8, 0.93], max_leap=6,
             octave=0.2, sforzando=0.1, bass_mode="BAR", pad_mode="ARPEGGIO", pad_base=43, swell=0.8, rubato=0.15,
             trill=False,
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
    ],
    "ROBERT SCHUMANN": [
        # the profile: the piano cycles (Carnaval, Kreisleriana, Fantasie): dreamy Eusebius and wild Florestan
        dict(name="FANTASIE"),
        # the song cycles (Dichterliebe): voice over a rippling piano, phrases left open, long postludes
        dict(name="LIED", root="F#", scale="AEOLIAN", bpm=60, division=4, beats_per_bar=2, chord_beats=2,
             period_bars=4, melody_density=0.3, melody_range=12, profile=[0.15, 0.78, 0.92, 0.98], max_leap=4,
             mel_min_ticks=2, swing=0.0, syncopation=0.15, pad_mode="ARPEGGIO", pad_base=50, open=0.35,
             alt_every=0, rubato=0.4, swell=0.4, sforzando=0.0,
             orch=[("PIANO", "VOICE", "PIANO"), ("PIANO", "CELLO", "PIANO")], orch_mode="RANDOM"),
        # the symphonies (Spring, Rhenish): E-flat 3/4 with hemiolas, horns and full strings
        dict(name="SYMPHONY", root="D#", bpm=152, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.5, melody_range=19, profile=[0.12, 0.55, 0.8, 0.93], max_leap=6, swing=0.0,
             syncopation=0.3, hemiola=True, bass_mode="BAR", pad_mode="REPEAT", pad_base=55, alt_every=0,
             sforzando=0.12, swell=0.8, rubato=0.1, perc_mode="PERIOD", perc=[7, 0, 0, 0, 0, 0], reverb="HALL",
             orch=[("CONTRABASS", "HORN", "STRINGS"), ("CELLO", "VIOLIN", "STRINGS"),
                   ("TUBA", "BRASS", "STRINGS")], orch_mode="RANDOM"),
    ],
    "CLARA SCHUMANN": [
        # the profile: the Romances (piano; Op. 22 with violin): G minor, a song over flowing arpeggios
        dict(name="ROMANCE"),
        # the Piano Concerto in A minor: piano solo with the orchestra, bold, timpani
        dict(name="CONCERTO", root="A", bpm=112, division=4, beats_per_bar=4, chord_beats=4, period_bars=8,
             melody_density=0.55, melody_base=64, melody_range=19, profile=[0.12, 0.55, 0.8, 0.93], octave=0.25,
             sforzando=0.1, bass_mode="BAR", pad_mode="REPEAT", pad_base=55, swell=0.8, rubato=0.15,
             perc_mode="PERIOD", perc=[6, 0, 0, 0, 0, 0], reverb="HALL",
             orch=[("CONTRABASS", "PIANO", "STRINGS"), ("CELLO", "PIANO", "STRINGS"), ("PIANO", "PIANO", "PIANO"),
                   ("CONTRABASS", "HORN", "STRINGS")], orch_mode="RANDOM"),
        # the Lieder (Liebst du um Schonheit, Er ist gekommen): voice and piano, in major, simpler
        dict(name="LIED", root="A#", scale="IONIAN", bpm=72, division=2, chord_beats=2, melody_density=0.35,
             melody_base=65, melody_range=12, profile=[0.1, 0.78, 0.93, 0.98], max_leap=4, mel_min_ticks=1,
             pad_mode="REPEAT", pad_base=51, rubato=0.3, swell=0.4, octave=0.0,
             orch=[("PIANO", "VOICE", "PIANO")], orch_mode="FIXED"),
    ],
    "LISZT": [
        # the profile: the Liebestraume and Consolations (notturnos): a tune in the tenor/alto over arpeggios
        dict(name="NOCTURNE"),
        # the Hungarian Rhapsodies: a slow, ornamented lassan in the gypsy minor, then the fast friska
        dict(name="RHAPSODY", root="C#", scale="HARM_MINOR", bpm=60, division=4, beats_per_bar=2, chord_beats=2,
             period_bars=4, melody_density=0.45, melody_range=17, profile=[0.12, 0.62, 0.85, 0.95], turn=0.35,
             appoggiatura=0.3, color=0.5, bass_mode="BAR", pad_mode="AFTERBEAT", pad_base=50, sforzando=0.15,
             rubato=0.5, swell=0.6, alt_every=1, **{"alt.bpm": 2.2, "alt.density": 1.3, "alt.pad_mode": "OOMPAH"},
             orch=[("PIANO", "PIANO", "PIANO"), ("CONTRABASS", "VIOLIN", "PIANO")], orch_mode="RANDOM"),
        # the virtuoso etudes (La campanella, Transcendental): G-sharp minor 6/8, bell-like leaps high up
        dict(name="ETUDE", root="G#", scale="HARM_MINOR", bpm=96, division=3, beats_per_bar=2, chord_beats=2,
             period_bars=8, melody_density=0.7, melody_base=72, melody_range=19, profile=[0.25, 0.45, 0.7, 0.85],
             max_leap=7, octave=0.4, staccato=0.6, turn=0.0, color=0.0, whole_tone=0.0, bass_mode="BAR",
             pad_mode="REPEAT", pad_base=52, rubato=0.15, swell=0.6, sforzando=0.08,
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the symphonic poems (Les preludes, Tasso): one theme transformed, brass and full orchestra, whole-tone
        dict(name="TONEPOEM", root="C", bpm=84, division=2, beats_per_bar=4, chord_beats=4, period_bars=8,
             melody_density=0.4, motif=0.85, motif_use=0.9, profile=[0.08, 0.6, 0.82, 0.93], turn=0.05,
             color_scale="WHOLE_TONE", color=0.1, whole_tone=0.0, bass_mode="ROOT", pad_mode="TREMOLO",
             pad_base=52, swell=0.9, rubato=0.3, sforzando=0.08, perc_mode="PERIOD", perc=[8, 0, 0, 5, 0, 0],
             reverb="HALL",
             orch=[("TUBA", "BRASS", "STRINGS"), ("CONTRABASS", "HORN", "STRINGS"), ("CELLO", "VIOLIN", "STRINGS"),
                   ("CONTRABASS", "TRUMPET", "STRINGS")], orch_mode="RANDOM"),
    ],
}
