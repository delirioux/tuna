# SPDX-License-Identifier: GPL-3.0-only
# Manners of the Romantic composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

MANNERS = {
    "VERDI": [
        # the profile: the opera house's oom-pah-pah under a big singable tune, the banda
        dict(name="ARIA"),
        # the great choruses (Va, pensiero; the Anvil): a broad unison tune for the chorus over held strings,
        # 4/4, slow, rising to a swell; F-sharp major
        dict(name="CHORUS", root="F#", bpm=76, beats_per_bar=4, chord_beats=2, period_bars=4, melody_density=0.35,
             melody_range=12, profile=[0.1, 0.8, 0.93, 0.98], max_leap=4, mel_min_ticks=2, bass_mode="ROOT",
             pad_mode="BLOCK", pad_tones=4, swell=0.8, rubato=0.25, motif_beats=4, perc=[4, 0, 0, 3, 0, 0],
             reverb="HALL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("TUBA", "CHOIR", "BRASS"), ("BASSOON", "CHOIR", "CLARINET")],
             orch_mode="RANDOM"),
        # the Requiem's Dies irae: G minor, rushing tremolo strings, the bass drum's blows off the beat, the chorus
        # declaiming on repeated notes, falling
        dict(name="REQUIEM", root="G", scale="AEOLIAN", bpm=120, beats_per_bar=4, division=4, chord_beats=4,
             period_bars=4, melody_density=0.45, melody_range=12, profile=[0.25, 0.75, 0.9, 0.97], max_leap=4,
             direction=-0.3, bass_mode="BAR", pad_mode="TREMOLO", pad_tones=3, sforzando=0.3, swell=0.9,
             rubato=0.05, motif_beats=4, perc=[8, 0, 10, 6, 0, 0], reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "STRINGS"), ("TUBA", "BRASS", "STRINGS"),
                   ("CONTRABASS", "TRUMPET", "STRINGS")], orch_mode="RANDOM"),
    ],
    "WAGNER": [
        # the profile: Tristan's endless melody, the half-diminished chord, swells that never land
        dict(name="DRAMA"),
        # the festive marches (Meistersinger, Tannhauser's entry): C major, diatonic, brass over a marching bass,
        # cadences that do land
        dict(name="MARCH", root="C", scale="IONIAN", bpm=96, chord_beats=2, melody_density=0.45, melody_range=15,
             max_leap=5, appoggiatura=0.15, suspension=0.1, chromatic=0, arrival_bars=1, bass_mode="MARCH",
             pad_mode="BLOCK", pad_tones=4, fifth_bias=1.5, open=0.1, modulate=0.1, swell=0.6, rubato=0.1,
             perc=[7, 3, 4, 6, 2, 0], reverb="HALL",
             w_major={"I": 10, "IV": 7, "V": 8, "V7": 8, "ii": 4, "ii7": 4, "vi": 4, "iii": 2, "V7/V": 5, "bVI": 1},
             orch=[("TUBA", "TRUMPET", "BRASS"), ("CONTRABASS", "HORN", "STRINGS"), ("TUBA", "BRASS", "STRINGS")],
             orch_mode="RANDOM"),
        # the Lohengrin prelude: A major, high divided violins held wide and soft, a slow stepping line, no drums
        dict(name="PRELUDE", root="A", scale="IONIAN", bpm=50, chord_beats=4, melody_base=72, melody_density=0.25,
             melody_range=12, profile=[0.1, 0.85, 0.95, 0.99], max_leap=3, mel_min_ticks=4, appoggiatura=0.2,
             chromatic=0, bass_mode="ROOT", pad_mode="BLOCK", pad_base=64, pad_tones=4, spread=1, open=0.3,
             modulate=0.1, swell=1, perc_mode="OFF", perc=[3, 0, 0, 2, 0, 0], reverb="CATHEDRAL",
             w_major={"I": 10, "vi": 6, "IV": 7, "ii": 4, "V": 6, "V7": 4, "iii": 4, "bVI": 2, "V7/V": 2},
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "FLUTE", "STRINGS"),
                   ("CONTRABASS", "OBOE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "GOUNOD": [
        # the profile: the Ave Maria, a long plain melody over Bach's broken-chord figure
        dict(name="PRAYER"),
        # Faust's waltz and Juliet's "Je veux vivre": F major, 3/4, quick, oom-pah-pah, a leaping coloratura tune
        dict(name="WALTZ", root="F", bpm=160, division=2, beats_per_bar=3, chord_beats=3, period_bars=8, lilt=0.4,
             melody_density=0.5, profile=[0.1, 0.55, 0.8, 0.93], max_leap=5, mel_min_ticks=1, plagal=0,
             bass_mode="BAR", pad_mode="OOMPAH", pad_base=55, motif_beats=3, swell=0.4, rubato=0.1,
             perc_mode="PERIOD", perc=[2, 0, 0, 2, 4, 0], reverb="HALL",
             orch=[("CONTRABASS", "VOICE", "PIZZ"), ("PIZZ", "FLUTE", "STRINGS"), ("CELLO", "VIOLIN", "HORN")],
             orch_mode="RANDOM"),
        # the Funeral March of a Marionette: D minor, 6/8, staccato bassoons and oboe over the afterbeat, the trio
        # in major
        dict(name="MARCH", root="D", scale="AEOLIAN", bpm=66, division=3, beats_per_bar=2, chord_beats=2,
             period_bars=8, melody_density=0.55, melody_base=62, melody_range=12, profile=[0.15, 0.7, 0.88, 0.96],
             max_leap=4, mel_min_ticks=1, staccato=0.6, plagal=0, bass_mode="BAR", pad_mode="AFTERBEAT",
             mixture=0.3, swell=0.2, rubato=0.1, motif=0.6, motif_beats=2, motif_use=0.7,
             perc_mode="PERIOD", perc=[3, 0, 2, 0, 0, 0], reverb="ROOM",
             orch=[("BASSOON", "BASSOON", "OBOE"), ("BASSOON", "OBOE", "CLARINET"), ("PIZZ", "CLARINET", "BASSOON")],
             orch_mode="RANDOM"),
    ],
    "FRANCK": [
        # the profile: the Violin Sonata (A major, 9/8, chromatic, the canon of the finale)
        dict(name="SONATA"),
        # the organ chorales (Trois Chorals, Prelude Choral et Fugue): E major, 4/4, a chord on every beat,
        # chromatic sliding inner voices, keys melting, the full organ in the cathedral
        dict(name="CHORALE", root="E", bpm=56, division=2, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.3, melody_range=12, profile=[0.03, 0.85, 0.95, 0.99], max_leap=3, mel_min_ticks=2,
             suspension=0.3, arrival_bars=1, bass_mode="ROOT", pad_mode="BLOCK", pad_base=52, pad_tones=4,
             modulate=0.4, open=0, rubato=0.4, motif_beats=4, alt_every=0, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("PIPEORGAN", "REEDORGAN", "PIPEORGAN"),
                   ("CONTRABASS", "PIPEORGAN", "PIPEORGAN")], orch_mode="RANDOM"),
        # Panis angelicus: a plain sung line over broken chords, the cello answering it in canon
        dict(name="MOTET", bpm=60, division=2, beats_per_bar=4, chord_beats=4, period_bars=8, melody_density=0.3,
             melody_range=12, profile=[0.1, 0.8, 0.93, 0.98], max_leap=4, mel_min_ticks=2, chromatic=0,
             sideslip=0, modulate=0.15, open=0.15, swell=0.6, motif_beats=4, canon_ticks=8, canon_steps=-7,
             pad_base=48, reverb="CATHEDRAL",
             orch=[("CONTRABASS", "VOICE", "CELLO"), ("PIPEORGAN", "VOICE", "PIANO"),
                   ("PIPEORGAN", "CHOIR", "PIPEORGAN")], orch_mode="RANDOM"),
    ],
    "SMETANA": [
        # the profile: Vltava, the river's flowing figure and the broad folk tune
        dict(name="TONEPOEM"),
        # the Bartered Bride's polkas: F major, 2/4, bass and afterbeat, short bouncing tune, village band
        dict(name="POLKA", root="F", scale="IONIAN", bpm=120, division=4, beats_per_bar=2, chord_beats=2,
             melody_density=0.6, profile=[0.15, 0.55, 0.8, 0.93], max_leap=5, syncopation=0.1, staccato=0.4,
             bass_mode="BAR", pad_mode="AFTERBEAT", motif_beats=2, swell=0.4, rubato=0,
             perc=[3, 0, 3, 3, 4, 2], reverb="ROOM",
             orch=[("TUBA", "CLARINET", "ACCORDION"), ("PIZZ", "VIOLIN", "STRINGS"), ("TUBA", "TRUMPET", "HORN")],
             orch_mode="RANDOM"),
        # the furiant (Bartered Bride, Czech Dances): D major, fast 3/4 with hemiolas, accents off the beat
        dict(name="FURIANT", root="D", scale="IONIAN", bpm=176, division=2, beats_per_bar=3, chord_beats=3,
             hemiola=1, melody_density=0.6, profile=[0.12, 0.6, 0.82, 0.94], max_leap=5, sforzando=0.3,
             bass_mode="BAR", pad_mode="BLOCK", motif_beats=3, swell=0.6, rubato=0.05,
             perc=[5, 0, 3, 4, 0, 3], reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("TUBA", "CLARINET", "HORN"), ("CONTRABASS", "FLUTE", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "BRUCKNER": [
        # the profile: the symphonies, tremolo from nothing, brass chorales, pauses, pedal points
        dict(name="SYMPHONY"),
        # the motets (Locus iste, Os justi): C major, a cappella choir in plain chords, pauses between phrases
        dict(name="MOTET", root="C", scale="IONIAN", bpm=60, division=2, beats_per_bar=4, chord_beats=2,
             period_bars=4, melody_base=64, melody_density=0.35, melody_range=12, profile=[0.03, 0.85, 0.95, 0.99],
             max_leap=4, suspension=0.2, octave=0, bass_mode="ROOT", pad_mode="BLOCK", pad_tones=4,
             pad_base=52, modulate=0.15, gp=0, swell=0.6, rubato=0.3, alt_every=0, perc_mode="OFF",
             reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("BASSOON", "CHOIR", "REEDORGAN"), ("CELLO", "CHOIR", "STRINGS")],
             orch_mode="RANDOM"),
        # the scherzi: D minor, fast 3/4 in one, driving repeated strings, octave leaps, brass blows; a slower trio
        dict(name="SCHERZO", bpm=168, division=2, beats_per_bar=3, chord_beats=3, melody_density=0.55,
             profile=[0.12, 0.45, 0.7, 0.85], max_leap=7, octave=0.3, staccato=0.4, sforzando=0.2,
             bass_mode="BAR", pad_mode="REPEAT", motif_beats=3, swell=0.8, gp=0.15, rubato=0, **{"alt.bpm": 0.8},
             orch=[("TUBA", "BRASS", "STRINGS"), ("CONTRABASS", "HORN", "STRINGS"), ("CONTRABASS", "TRUMPET", "STRINGS")],
             orch_mode="CONTRAST"),
    ],
    "JOHANN STRAUSS": [
        # the profile: the Viennese waltz
        dict(name="WALTZ"),
        # the quick polkas (Tritsch-Tratsch, Unter Donner und Blitz, the Pizzicato Polka): 2/4, bass and afterbeat,
        # staccato, thunder from the bass drum and cymbals
        dict(name="POLKA", bpm=152, division=2, beats_per_bar=2, chord_beats=2, lilt=0, melody_density=0.65,
             profile=[0.15, 0.5, 0.75, 0.9], staccato=0.4, sforzando=0.1, bass_mode="BAR", pad_mode="AFTERBEAT",
             motif_beats=2, perc=[3, 4, 6, 6, 3, 0], reverb="HALL",
             orch=[("PIZZ", "VIOLIN", "HORN"), ("TUBA", "FLUTE", "BRASS"), ("CONTRABASS", "CLARINET", "PIZZ"),
                   ("PIZZ", "PIZZ", "PIZZ")], orch_mode="RANDOM"),
        # the marches (Egyptian, Persian, the family's Radetzky): 4/4, root and fifth, snare drum, trumpet tune
        dict(name="MARCH", bpm=112, division=2, beats_per_bar=4, chord_beats=2, lilt=0, melody_density=0.5,
             syncopation=0.05, bass_mode="MARCH", pad_mode="OOMPAH", motif_beats=4, swell=0.5,
             perc=[3, 8, 5, 5, 0, 0], reverb="HALL",
             orch=[("TUBA", "TRUMPET", "BRASS"), ("TUBA", "FLUTE", "BRASS"), ("CONTRABASS", "VIOLIN", "HORN")],
             orch_mode="RANDOM"),
    ],
    "BRAHMS": [
        # the profile: the chamber music (Clarinet and Horn Trios, the intermezzi): 3/4, thirds and sixths,
        # hemiolas
        dict(name="CHAMBER"),
        # the Lieder (Wiegenlied): E-flat, a gentle sung tune over the piano's afterbeat
        dict(name="LIED", root="D#", bpm=92, melody_density=0.35, melody_range=12, profile=[0.15, 0.7, 0.88, 0.96],
             max_leap=5, syncopation=0, suspension=0.1, hemiola=0, bass_mode="BAR", pad_mode="AFTERBEAT",
             swell=0.2, rubato=0.2, perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "VOICE", "PIANO"), ("CELLO", "VOICE", "PIANO"), ("PIANO", "CLARINET", "PIANO")],
             orch_mode="RANDOM"),
        # the Hungarian Dances: G minor (harmonic), 2/4, the gypsy band's slow-fast alternation, wild rubato,
        # accents
        dict(name="CSARDAS", root="G", scale="HARM_MINOR", bpm=116, division=4, beats_per_bar=2, chord_beats=2,
             melody_density=0.65, syncopation=0.25, staccato=0.25, appoggiatura=0.2, chromatic=1, turn=0.2,
             sforzando=0.2, hemiola=0, plagal=0, bass_mode="BAR", pad_mode="AFTERBEAT", motif_beats=2,
             rubato=0.6, **{"alt.bpm": 1.4}, alt_every=2, perc_mode="PERIOD", perc=[3, 0, 2, 4, 0, 2],
             reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO"), ("PIZZ", "VIOLIN", "STRINGS"), ("CELLO", "VIOLIN", "PIZZ"),
                   ("CONTRABASS", "DULCIMER", "PIZZ")], orch_mode="RANDOM"),
    ],
    "SAINT-SAENS": [
        # the profile: the concertos (G minor Piano Concerto): sixteenth arpeggios, trills, brilliance
        dict(name="CONCERTO"),
        # Danse macabre: G minor, a macabre waltz, xylophone bones and the solo violin over pizzicato
        dict(name="WALTZ", bpm=150, division=2, beats_per_bar=3, chord_beats=3, melody_density=0.55,
             staccato=0.5, chromatic=1, trill=0, bass_mode="BAR", pad_mode="OOMPAH", pad_base=55, motif_beats=3,
             perc=[3, 0, 0, 3, 0, 0], reverb="HALL",
             orch=[("PIZZ", "XYLOPHONE", "PIZZ"), ("CONTRABASS", "VIOLIN", "PIZZ"), ("PIZZ", "FLUTE", "STRINGS")],
             orch_mode="RANDOM"),
        # the Swan and the Romances: G major, a slow singing cello over rippling piano
        dict(name="ROMANCE", root="G", scale="IONIAN", bpm=56, melody_base=55, melody_range=15,
             melody_density=0.2, mel_min_ticks=4, trill=0, repeat=0.3, swell=0.6, rubato=0.4, perc_mode="OFF",
             reverb="HALL",
             orch=[("CELLO", "CELLO", "PIANO"), ("PIANO", "CELLO", "PIANO"), ("CONTRABASS", "CELLO", "PIANO")],
             orch_mode="RANDOM"),
        # the Organ Symphony's maestoso: C major, the full organ's chords, a broad tune, timpani
        dict(name="SYMPHONY", root="C", scale="IONIAN", bpm=84, division=2, chord_beats=2, melody_density=0.35,
             melody_range=15, profile=[0.08, 0.7, 0.9, 0.97], max_leap=4, trill=0, arrival_bars=2,
             bass_mode="ROOT", pad_mode="BLOCK", pad_base=48, pad_tones=4, spread=1, swell=1, rubato=0.1,
             perc=[8, 0, 0, 6, 0, 0], reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "BRASS", "PIPEORGAN"), ("PIPEORGAN", "STRINGS", "PIPEORGAN"),
                   ("TUBA", "TRUMPET", "PIPEORGAN")], orch_mode="RANDOM"),
    ],
    "BIZET": [
        # the profile: the Habanera
        dict(name="HABANERA"),
        # the Toreadors (Carmen's march and couplets): A major, 2/4, bass and afterbeat, brass and cymbals
        dict(name="MARCH", root="A", scale="IONIAN", bpm=112, melody_density=0.55, profile=[0.15, 0.55, 0.8, 0.93],
             max_leap=5, direction=0, mel_add=[], mixture=0.1, bass_mode="BAR", pad_mode="AFTERBEAT",
             sforzando=0.1, swell=0.6, rubato=0.05, perc_mode="PERIOD", perc=[3, 4, 6, 6, 4, 2], reverb="HALL",
             orch=[("TUBA", "TRUMPET", "BRASS"), ("CONTRABASS", "VIOLIN", "HORN"), ("TUBA", "FLUTE", "BRASS"),
                   ("CONTRABASS", "VOICE", "PIZZ")], orch_mode="RANDOM"),
        # L'Arlesienne's farandole: D minor turning major, fast running eighths over the tambourin's drone
        dict(name="FARANDOL", bpm=152, division=2, melody_density=0.75, profile=[0.08, 0.75, 0.9, 0.97],
             staccato=0.3, direction=0, mel_add=[], mixture=0.3, bass_mode="PEDAL", pad_mode="BLOCK",
             swell=0.7, rubato=0, perc_mode="PERIOD", perc=[0, 0, 5, 0, 0, 9], reverb="HALL",
             orch=[("CONTRABASS", "FLUTE", "STRINGS"), ("BASSOON", "OBOE", "STRINGS"), ("TUBA", "BRASS", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "MUSSORGSKY": [
        # the profile: the Promenade of the Pictures
        dict(name="PROMENAD"),
        # Night on Bald Mountain: D minor, 4/4, whirling tremolo strings, brass shouts, the dawn bell between
        dict(name="TONEPOEM", root="D", scale="AEOLIAN", bpm=132, division=4, beats_per_bar=4, chord_beats=4,
             meter=[0], meter_bars=0, melody_mask=0, melody_density=0.6, profile=[0.1, 0.6, 0.85, 0.95],
             max_leap=5, chromatic=1, sforzando=0.3, bass_mode="BAR", pad_mode="TREMOLO", motif_beats=4,
             swell=0.8, perc_mode="PERIOD", perc=[8, 0, 6, 8, 0, 0], reverb="HALL",
             orch=[("TUBA", "BRASS", "STRINGS"), ("CONTRABASS", "TRUMPET", "STRINGS"), ("CONTRABASS", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
        # the choruses of Boris Godunov (the Coronation): C major, slow 4/4, the chorus's folk hymn in plain
        # chords, the bells struck low
        dict(name="CHORUS", root="C", bpm=72, division=2, beats_per_bar=4, chord_beats=2, meter=[0], meter_bars=0,
             melody_density=0.35, profile=[0.2, 0.8, 0.93, 0.98], max_leap=3, mel_min_ticks=2, bass_mode="TOLL",
             pad_mode="BLOCK", pad_tones=4, motif_beats=4, swell=0.7, alt_every=0, perc_mode="PERIOD",
             perc=[5, 0, 0, 6, 0, 0], reverb="CATHEDRAL",
             orch=[("TUBA", "CHOIR", "BRASS"), ("CONTRABASS", "CHOIR", "STRINGS"), ("CONTRABASS", "TUBULAR", "BRASS")],
             orch_mode="RANDOM"),
    ],
    "TCHAIKOVSKY": [
        # the profile: the symphonies' sequences and falling lines
        dict(name="SYMPHONY"),
        # the ballet waltzes (Flowers, Sleeping Beauty, Swan Lake): D major, 3/4, horn tune over oom-pah-pah
        dict(name="WALTZ", root="D", scale="IONIAN", bpm=168, division=2, beats_per_bar=3, chord_beats=3,
             period_bars=8, lilt=0.3, melody_density=0.45, profile=[0.08, 0.6, 0.85, 0.95], direction=0,
             bass_mode="BAR", pad_mode="OOMPAH", pad_base=55, motif_beats=3, swell=0.7, rubato=0.15,
             perc=[4, 0, 0, 4, 3, 0], reverb="HALL",
             orch=[("CONTRABASS", "HORN", "STRINGS"), ("PIZZ", "FLUTE", "STRINGS"), ("CONTRABASS", "CLARINET", "STRINGS"),
                   ("PIZZ", "VIOLIN", "HORN")], orch_mode="RANDOM"),
        # the Nutcracker's march (and the character dances): G major, 4/4, staccato, root and fifth, triangle
        dict(name="MARCH", root="G", scale="IONIAN", bpm=132, chord_beats=2, melody_density=0.55,
             profile=[0.15, 0.55, 0.8, 0.93], appoggiatura=0.05, suspension=0, chromatic=0, staccato=0.5,
             direction=0, bass_mode="MARCH", pad_mode="AFTERBEAT", swell=0.3, rubato=0, alt_every=0,
             perc=[3, 3, 0, 4, 4, 0], reverb="HALL",
             orch=[("PIZZ", "TRUMPET", "STRINGS"), ("PIZZ", "CLARINET", "BRASS"), ("CONTRABASS", "FLUTE", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "DVORAK": [
        # the profile: the New World's Largo, a pentatonic tune for the cor anglais over a pedal
        dict(name="SYMPHONY"),
        # the Slavonic Dances' furiant: C major, presto 3/4, hemiolas, accents, triangle and tambourine
        dict(name="FURIANT", root="C", bpm=176, division=2, beats_per_bar=3, chord_beats=3, hemiola=1,
             melody_mask=0, melody_density=0.6, profile=[0.12, 0.6, 0.82, 0.94], max_leap=5, syncopation=0.2,
             sforzando=0.25, bass_mode="BAR", pad_mode="REPEAT", motif_beats=3, swell=0.6, rubato=0.1,
             perc=[5, 0, 3, 5, 6, 6], reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("TUBA", "FLUTE", "STRINGS"), ("CONTRABASS", "OBOE", "HORN")],
             orch_mode="RANDOM"),
        # the dumka (the Dumky Trio): an E minor lament over broken piano chords, suddenly a fast dance
        dict(name="DUMKA", root="E", scale="AEOLIAN", bpm=56, division=4, beats_per_bar=2, chord_beats=2,
             period_bars=4, melody_mask=0, melody_density=0.35, appoggiatura=0.25, suspension=0.2, mixture=0.4,
             bass_mode="ROOT", pad_mode="BROKEN", motif_beats=2, rubato=0.4, perc_mode="OFF",
             **{"alt.bpm": 2.2, "alt.density": 1.4, "alt.bass_mode": "BAR", "alt.pad_mode": "AFTERBEAT"},
             alt_every=2, reverb="ROOM",
             orch=[("PIANO", "CELLO", "PIANO"), ("PIANO", "VIOLIN", "PIANO"), ("CELLO", "VIOLIN", "PIANO")],
             orch_mode="RANDOM"),
    ],
    "GRIEG": [
        # the profile: Morning (Peer Gynt), the pastoral pentatonic tune
        dict(name="PASTORAL"),
        # the hallings (Lyric Pieces, Norwegian Dances, Slatter): D, 2/4, a Hardanger fiddle tune over its drone,
        # stamping accents
        dict(name="HALLING", root="D", bpm=112, division=4, beats_per_bar=2, chord_beats=2, melody_mask=0,
             melody_density=0.7, profile=[0.12, 0.65, 0.85, 0.95], max_leap=5, staccato=0.2, syncopation=0.2,
             sforzando=0.15, turn=0.2, pad_mode="AFTERBEAT", motif_beats=2, swell=0.3, rubato=0.05,
             perc_mode="PERIOD", perc=[0, 0, 3, 0, 0, 0], reverb="ROOM",
             orch=[("DRONE", "VIOLIN", "PIZZ"), ("PIANO", "PIANO", "PIANO"), ("CELLO", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
        # In the Hall of the Mountain King and the March of the Trolls: B minor, staccato steps low in the
        # bassoons, one harmony again and again, growing
        dict(name="MARCH", root="B", scale="HARM_MINOR", bpm=92, division=2, beats_per_bar=4, chord_beats=4,
             period_bars=4, melody_mask=0, melody_base=50, melody_density=0.6, profile=[0.15, 0.8, 0.92, 0.98],
             max_leap=4, staccato=0.7, color=0, mixture=0, bass_mode="MARCH", pad_mode="AFTERBEAT", repeat=0.8,
             motif=0.9, motif_use=0.95, evolve_periods=2, evolve_depth=0.8, swell=0.9, rubato=0,
             perc_mode="PERIOD", perc=[6, 0, 4, 6, 0, 0], reverb="HALL",
             orch=[("BASSOON", "BASSOON", "PIZZ"), ("CONTRABASS", "BASSOON", "PIZZ"), ("CONTRABASS", "VIOLIN", "STRINGS"),
                   ("TUBA", "BRASS", "STRINGS")], orch_mode="CYCLE"),
    ],
    "RIMSKY-KORSAKOV": [
        # the profile: Scheherazade's arabesques
        dict(name="SUITE"),
        # the Flight of the Bumblebee: A minor, a perpetual chromatic run in sixteenths over staccato chords
        dict(name="SCHERZO", root="A", bpm=152, division=4, beats_per_bar=2, chord_beats=2, melody_base=69,
             melody_range=15, melody_density=1.0, mel_min_ticks=1, profile=[0.02, 0.95, 0.99, 1.0], max_leap=2,
             appoggiatura=0, turn=0, color=0, bass_mode="ROOT", pad_mode="AFTERBEAT", staccato=0.1, motif=0.7,
             swell=0.3, rubato=0, perc_mode="OFF", reverb="ROOM",
             orch=[("PIZZ", "VIOLIN", "STRINGS"), ("PIZZ", "FLUTE", "PIZZ"), ("CELLO", "CLARINET", "PIZZ")],
             orch_mode="RANDOM"),
        # Capriccio espagnol's alborada: A major, quick 2/4, driving repeated chords, a Phrygian tinge, the
        # percussion
        dict(name="ALBORADA", root="A", scale="IONIAN", bpm=132, division=4, beats_per_bar=2, chord_beats=2,
             period_bars=8, melody_density=0.65, profile=[0.1, 0.6, 0.85, 0.95], chromatic=0, color_scale="PHRYG_DOM",
             color=0.2, bass_mode="BAR", pad_mode="REPEAT", sforzando=0.15, swell=0.6, rubato=0.05,
             perc=[5, 0, 3, 6, 4, 8], reverb="HALL",
             orch=[("PIZZ", "VIOLIN", "STRINGS"), ("TUBA", "TRUMPET", "BRASS"), ("PIZZ", "CLARINET", "GUITAR"),
                   ("HARP", "FLUTE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "MASSENET": [
        # the profile: Thais's Meditation (an entr'acte), the violin over harp arpeggios
        dict(name="ENTRACTE"),
        # the Elegie: E minor, a falling sung or cello lament over the piano's chords
        dict(name="ELEGIE", root="E", scale="AEOLIAN", bpm=56, division=2, chord_beats=2, melody_base=60,
             melody_density=0.35, direction=-0.4, appoggiatura=0.3, mixture=0.2, rubato=0.4, bass_mode="ROOT",
             pad_mode="BLOCK", perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "CELLO", "PIANO"), ("PIANO", "VOICE", "PIANO"), ("CONTRABASS", "CELLO", "STRINGS")],
             orch_mode="RANDOM"),
        # Le Cid's ballet (Aragonaise, Castillane): F major, fast 3/4, staccato, a Spanish tinge, tambourine
        dict(name="BALLET", root="F", bpm=176, division=2, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.6, profile=[0.1, 0.55, 0.8, 0.93], max_leap=5, mel_min_ticks=1, staccato=0.3,
             appoggiatura=0.15, turn=0.2, sforzando=0.2, color_scale="PHRYG_DOM", color=0.15, bass_mode="BAR",
             pad_mode="OOMPAH", pad_base=55, motif_beats=3, rubato=0.05, perc_mode="PERIOD", perc=[3, 0, 0, 4, 4, 8],
             reverb="HALL",
             orch=[("PIZZ", "OBOE", "PIZZ"), ("CONTRABASS", "FLUTE", "STRINGS"), ("TUBA", "TRUMPET", "BRASS"),
                   ("GUITAR", "VIOLIN", "GUITAR")], orch_mode="RANDOM"),
    ],
    "BRUCH": [
        # the profile: the G minor Violin Concerto's rhapsodic singing violin
        dict(name="CONCERTO"),
        # Kol Nidrei (Adagio on Hebrew melodies): D minor, a very slow chant-like cello, ornamented, turning to
        # major
        dict(name="ADAGIO", root="D", bpm=50, chord_beats=4, melody_base=50, melody_range=17, melody_density=0.3,
             mel_min_ticks=2, profile=[0.08, 0.7, 0.88, 0.96], appoggiatura=0.3, turn=0.3, mixture=0.4, swell=0.7,
             rubato=0.5, octave=0, bass_mode="ROOT", pad_mode="BLOCK", reverb="HALL",
             orch=[("CONTRABASS", "CELLO", "STRINGS"), ("HARP", "CELLO", "STRINGS"), ("PIPEORGAN", "CELLO", "PIPEORGAN")],
             orch_mode="RANDOM"),
        # the Scottish Fantasy: E-flat major, a pentatonic folk tune for the violin over a bagpipe drone and harp
        dict(name="FANTASIA", root="D#", scale="IONIAN", bpm=108, division=2, chord_beats=2, melody_mask=661,
             melody_density=0.5, profile=[0.1, 0.65, 0.85, 0.95], max_leap=5, syncopation=0.2, turn=0.25,
             mixture=0.1, rubato=0.2, bass_mode="PEDAL", pad_mode="BLOCK", perc_mode="PERIOD", perc=[4, 0, 0, 2, 0, 0],
             reverb="HALL",
             orch=[("HARP", "VIOLIN", "STRINGS"), ("CONTRABASS", "VIOLIN", "HORN"), ("DRONE", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
    ],
}
