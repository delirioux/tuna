# SPDX-License-Identifier: GPL-3.0-only
# Manners of the modern composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

# chord weights reused below (by vocabulary)
_CLASSIC_PLAIN = {"I": 10, "vi": 4, "IV": 6, "ii": 4, "V": 8, "V7": 6, "iii": 2, "V7/V": 2}
_CLASSIC_MINOR = {"i": 10, "iv": 6, "V": 7, "V7": 6, "VI": 5, "VII": 4, "III": 3, "ii°": 2, "bII": 2}

MANNERS = {
    "BARTOK": [
        dict(name="DANCE"),
        # the night music (Out of Doors, the Adagio of the Music for Strings, Percussion and Celesta): slow,
        # hushed held clusters over a pedal, octatonic shadows, and the night's cries (insects, birds) in turn
        dict(name="NIGHT", bpm=52, division=4, beats_per_bar=4, meter=[], chord_beats=4, period_bars=4,
             melody_density=0.25, melody_range=17, profile=[0.1, 0.55, 0.8, 0.92], max_leap=6, mel_min_ticks=2,
             bass_mode="PEDAL", pad_mode="BLOCK", pad_tones=4, pad_base=50, repeat=0.3, motif=0.4, motif_use=0.5,
             color=0.5, open=0.6, swell=0.6, rubato=0.3, sforzando=0, perc=[2, 0, 0, 0, 0, 0],
             alt_every=2, **{"alt.mel_mode": "BIRDSONG"}, reverb="HALL",
             orch=[("CONTRABASS", "CELESTA", "STRINGS"), ("CELLO", "CLARINET", "STRINGS"),
                   ("PIANO", "XYLOPHONE", "PIANO")], orch_mode="CONTRAST"),
        # the Romanian Folk Dances, For Children: a modal fiddle tune in 2/4 over plain triads, oom-pah piano,
        # the augmented second of the Romanian scale now and then
        dict(name="FOLKSONG", bpm=108, beats_per_bar=2, meter=[], chord_beats=2, period_bars=8, scale="DORIAN",
             root="D", vocab="MODAL",
             w_major={"i": 8, "iv": 5, "v": 3, "IV": 5, "bVII": 5, "bIII": 3, "V": 4, "5": 2},
             w_minor={"i": 8, "iv": 5, "v": 3, "IV": 5, "bVII": 5, "bIII": 3, "V": 4, "5": 2},
             melody_density=0.6, profile=[0.15, 0.7, 0.88, 0.96], max_leap=4, turn=0.2, appoggiatura=0.15,
             bass_mode="BEAT", pad_mode="AFTERBEAT", motif=0.6, motif_use=0.7, var_invert=0.1, open=0.2,
             color_scale="HUNG_MINOR", color=0.2, perc_mode="OFF", swell=0.3, reverb="ROOM",
             orch=[("PIANO", "VIOLIN", "PIANO"), ("PIZZ", "VIOLIN", "STRINGS"), ("PIANO", "CLARINET", "PIANO")],
             orch_mode="RANDOM"),
    ],
    "STRAVINSKY": [
        dict(name="BALLET"),
        # the neoclassical concertos (Dumbarton Oaks, Pulcinella, the Symphony in C): diatonic E-flat, a
        # motoric walking bass, two lines in counterpoint, dry accents displaced, chamber winds and strings
        dict(name="CONCERTO", root="D#", scale="IONIAN", bpm=132, beats_per_bar=4, meter=[], meter_bars=0,
             chord_beats=2, period_bars=4, vocab="CLASSIC", w_major=_CLASSIC_PLAIN, w_minor=_CLASSIC_MINOR,
             melody_density=0.6, melody_range=15, profile=[0.15, 0.6, 0.82, 0.93], max_leap=5, staccato=0.5,
             bass_mode="WALKING", pad_mode="COUNTER", poly_semis=0, open=0.2, color=0, sforzando=0.3,
             motif=0.8, motif_use=0.9, perc_mode="OFF", reverb="ROOM",
             orch=[("BASSOON", "OBOE", "HARPSICHORD"), ("CONTRABASS", "FLUTE", "CLARINET"),
                   ("CELLO", "TRUMPET", "VIOLIN"), ("BASSOON", "VIOLIN", "OBOE")], orch_mode="RANDOM"),
        # the Symphony of Psalms: a slow chant of few notes over an ostinato, block chords in E Phrygian with
        # octatonic shadows, choir, winds and two pianos, no violins
        dict(name="PSALM", scale="PHRYGIAN", bpm=60, beats_per_bar=4, meter=[], meter_bars=0, chord_beats=2,
             period_bars=4, melody_density=0.35, profile=[0.3, 0.88, 0.96, 0.99], max_leap=2, mel_min_ticks=2,
             pad_mode="BLOCK", pad_tones=4, poly_semis=0, color_scale="OCT_HW", color=0.4, sforzando=0,
             swell=0.6, rubato=0.2, perc=[3, 0, 0, 0, 0, 0], reverb="CATHEDRAL",
             orch=[("CONTRABASS", "CHOIR", "PIANO"), ("BASSOON", "CHOIR", "BRASS"), ("CELLO", "OBOE", "PIANO")],
             orch_mode="RANDOM"),
    ],
    "PROKOFIEV": [
        dict(name="BALLET"),
        # the Toccata op. 11 and the sonatas' finales: D minor, a motoric drive of repeated notes in sixteenths,
        # hammered chords, wide leaps and wrong-key slips, piano alone
        dict(name="TOCCATA", root="D", bpm=144, division=4, chord_beats=4, period_bars=4, swing=0,
             melody_density=0.75, profile=[0.3, 0.6, 0.8, 0.9], max_leap=7, staccato=0.7, octave=0.2,
             bass_mode="REPEAT", pad_mode="REPEAT", sideslip=0.35, sforzando=0.15, motif=0.8, motif_use=0.9,
             perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the Classical Symphony: D major after Haydn, Alberti strings, the flute leaping over two octaves,
        # sudden slips to a far key and straight back
        dict(name="SYMPHONY", root="D", scale="IONIAN", bpm=138, swing=0, melody_density=0.6, melody_base=64,
             melody_range=24, profile=[0.08, 0.4, 0.65, 0.8], max_leap=7, octave=0.25, staccato=0.4,
             bass_mode="MARCH", pad_mode="ALBERTI", sideslip=0.35, perc_mode="OFF", reverb="HALL",
             orch=[("CELLO", "FLUTE", "STRINGS"), ("BASSOON", "VIOLIN", "STRINGS"),
                   ("CONTRABASS", "OBOE", "VIOLIN")], orch_mode="RANDOM"),
    ],
    "POULENC": [
        dict(name="SONATA"),
        # the Lenten motets, the Litanies, the Salve Regina: grave minor, choir in block chords with his
        # sevenths and ninths, no cheek
        dict(name="MOTET", scale="AEOLIAN", bpm=60, period_bars=4, melody_density=0.35,
             profile=[0.15, 0.8, 0.93, 0.98], max_leap=4, staccato=0, bass_mode="ROOT", pad_mode="BLOCK",
             sideslip=0.1, alt_every=0, rubato=0.25, swell=0.5, arrival_bars=2, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "CHOIR", "STRINGS")],
             orch_mode="RANDOM"),
        # the melodies (Hotel, C, Les chemins de l'amour): a voice over slow broken piano chords, lazy sevenths
        dict(name="MELODIE", bpm=66, period_bars=4, melody_density=0.45, profile=[0.1, 0.6, 0.85, 0.95],
             staccato=0, bass_mode="ROOT", pad_mode="BROKEN", sideslip=0.2, alt_every=0, rubato=0.3,
             swell=0.4, reverb="ROOM",
             orch=[("PIANO", "VOICE", "PIANO")], orch_mode="FIXED"),
    ],
    "GERSHWIN": [
        dict(name="SONG"),
        # the Three Preludes: piano alone, stride (bass on 1 and 3, chords on 2 and 4), sharper syncopation
        dict(name="PRELUDE", bpm=116, swing=0, syncopation=0.6, bass_mode="MARCH", pad_mode="OOMPAH",
             staccato=0.25, perc_mode="OFF", reverb="ROOM",
             orch=[("PIANO", "PIANO", "PIANO")], orch_mode="FIXED"),
        # the Rhapsody in Blue, An American in Paris: free tempo, the clarinet's wide gestures, broken piano
        # chords under it, keys that shift, the orchestra swelling
        dict(name="RHAPSODY", bpm=96, swing=0, rubato=0.45, melody_range=24, max_leap=7,
             profile=[0.1, 0.5, 0.75, 0.9], octave=0.15, bass_mode="ROOT", pad_mode="BROKEN", modulate=0.3,
             swell=0.7, perc_mode="PERIOD", perc=[3, 2, 0, 3, 0, 0], reverb="HALL",
             orch=[("CONTRABASS", "CLARINET", "PIANO"), ("CONTRABASS", "STRINGS", "PIANO"),
                   ("TUBA", "MUTETRUMPET", "PIANO"), ("CONTRABASS", "PIANO", "STRINGS")], orch_mode="RANDOM"),
        # Porgy and Bess (Summertime): A minor, slow and rocking, a voice over strings, minor sixths and
        # flat-nine dominants
        dict(name="ARIA", root="A", scale="AEOLIAN", bpm=66, chord_beats=4, swing=0.5, syncopation=0.3,
             melody_density=0.35, profile=[0.1, 0.6, 0.85, 0.95],
             w_minor={"i6": 10, "V7b9": 7, "iv6": 6, "iv7": 5, "iiø7": 5, "V7sus": 3, "VImaj7": 4, "bII7": 2,
                      "im9": 3},
             bass_mode="ROOT", pad_mode="BLOCK", perc_mode="OFF", rubato=0.25, swell=0.5, arrival_bars=2,
             reverb="HALL",
             orch=[("CONTRABASS", "VOICE", "STRINGS"), ("CELLO", "VOICE", "PIANO")], orch_mode="RANDOM"),
    ],
    "ORFF": [
        dict(name="CANTATA"),
        # the Schulwerk: children's music on C pentatonic over a bordun (tonic and fifth held), a little
        # ostinato on the marimba, recorder and xylophone tunes, call and echo
        dict(name="SCHULWRK", root="C", scale="IONIAN", melody_mask=661, harmony="DRONE", bpm=112, beats_per_bar=2,
             chord_beats=2, period_bars=4, melody_density=0.55, profile=[0.2, 0.7, 0.9, 0.97], max_leap=4,
             bass_mode="PEDAL", pad_mode="FIGURE", pad_figure=[0, 1, 2, 1], perc=[3, 0, 4, 0, 0, 3],
             echo=0.4, evolve_periods=0, swell=0.2, reverb="ROOM",
             orch=[("CELLO", "XYLOPHONE", "MARIMBA"), ("BASSOON", "WHISTLE", "MARIMBA"),
                   ("CELLO", "GLOCKEN", "MARIMBA"), ("CONTRABASS", "FLUTE", "PIZZ")], orch_mode="RANDOM"),
        # the lyrical solos of Carmina Burana (In trutina, Dulcissime): F major, slow, a soprano floating over
        # held strings
        dict(name="ARIA", root="F", scale="IONIAN", harmony="PLANNED", bpm=56, beats_per_bar=4, chord_beats=4,
             period_bars=4, w_major=_CLASSIC_PLAIN, w_minor=_CLASSIC_MINOR, melody_density=0.35,
             melody_range=14, profile=[0.1, 0.75, 0.9, 0.97], bass_mode="ROOT", pad_mode="BLOCK", repeat=0.4,
             motif=0.5, motif_use=0.6, perc_mode="OFF", echo=0, evolve_periods=0, rubato=0.3, swell=0.5,
             arrival_bars=2, reverb="HALL",
             orch=[("CONTRABASS", "VOICE", "STRINGS"), ("CELLO", "FLUTE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "RODRIGO": [
        dict(name="ADAGIO"),
        # the Canarios of the Fantasia para un gentilhombre (after Gaspar Sanz): D major 6/8, quick, strummed
        # guitar chords, a dance tune on guitar, trumpet or flute
        dict(name="CANARIO", root="D", scale="IONIAN", harmony="PLANNED",
             w_major={"I": 10, "IV": 6, "V": 8, "V7": 6, "ii": 3, "vi": 3},
             w_minor=_CLASSIC_MINOR, bpm=126, division=3, beats_per_bar=2, chord_beats=1, period_bars=8,
             melody_density=0.65, profile=[0.15, 0.6, 0.85, 0.95], max_leap=5, appoggiatura=0.1, turn=0.2,
             arrival_bars=1, bass_mode="BAR", pad_mode="REPEAT", strum_ms=10, rubato=0, color=0,
             perc_mode="PERIOD", perc=[0, 0, 0, 0, 0, 4], swell=0.4, reverb="ROOM",
             orch=[("PIZZ", "GUITAR", "STRINGS"), ("CONTRABASS", "TRUMPET", "GUITAR"),
                   ("BASSOON", "FLUTE", "GUITAR")], orch_mode="RANDOM"),
        # the Cuatro madrigales amatorios: a voice in 3/4 over a broken guitar, Andalusian Phrygian colour
        dict(name="MADRIGAL", harmony="PLANNED", w_major=_CLASSIC_PLAIN, w_minor=_CLASSIC_MINOR, bpm=80,
             division=2, beats_per_bar=3, chord_beats=3, period_bars=4, melody_density=0.45,
             profile=[0.1, 0.7, 0.88, 0.96], appoggiatura=0.25, arrival_bars=1, bass_mode="ROOT",
             pad_mode="BROKEN", color=0.4, rubato=0.25, reverb="ROOM",
             orch=[("GUITAR", "VOICE", "GUITAR"), ("CELLO", "VOICE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "COPLAND": [
        dict(name="FANFARE"),
        # Rodeo's Hoe-Down: a fiddle tune in quick 2/4, root and fifth plucked, chords off the beat, woodblock
        # cheek and xylophone
        dict(name="HOEDOWN", root="D", bpm=152, beats_per_bar=2, chord_beats=2, period_bars=8,
             melody_density=0.8, profile=[0.2, 0.75, 0.9, 0.97], max_leap=4, syncopation=0.4, staccato=0.4,
             bass_mode="BEAT", pad_mode="AFTERBEAT", pad_base=55, pad_tones=3, spread=False, arrival_bars=1,
             sforzando=0.1, swell=0.3, perc=[0, 3, 4, 2, 0, 0], reverb="HALL",
             orch=[("PIZZ", "VIOLIN", "STRINGS"), ("TUBA", "XYLOPHONE", "BRASS"), ("CONTRABASS", "VIOLIN", "PIANO"),
                   ("BASSOON", "TRUMPET", "STRINGS")], orch_mode="RANDOM"),
        # Appalachian Spring (the opening, Simple Gifts): A major, slow, a pedal under wide-spaced chords,
        # clarinet and flute alone over the strings, no drums
        dict(name="PASTORAL", root="A", bpm=66, melody_density=0.35, profile=[0.06, 0.6, 0.85, 0.95],
             syncopation=0, bass_mode="PEDAL", pad_mode="BLOCK", perc_mode="OFF", swell=0.4, rubato=0.1,
             reverb="HALL",
             orch=[("CONTRABASS", "CLARINET", "STRINGS"), ("CELLO", "FLUTE", "STRINGS"),
                   ("BASSOON", "OBOE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "KHACHATURIAN": [
        dict(name="DANCE"),
        # the Adagio of Spartacus and Phrygia: slow and lush, a long line leaping wide over romantic harmony,
        # strings swelling, harp
        dict(name="ADAGIO", bpm=56, division=4, beats_per_bar=4, chord_beats=2, period_bars=4,
             melody_density=0.4, melody_base=62, melody_range=22, profile=[0.06, 0.55, 0.8, 0.92], max_leap=7,
             octave=0.15, appoggiatura=0.25, vocab="ROMANTIC",
             w_major={"I": 10, "vi": 6, "IV": 6, "ii7": 5, "V7": 7, "V": 5, "iii": 3, "bVI": 4, "iv": 4,
                      "V7/V": 4, "V7/vi": 3, "iiø7": 3},
             w_minor={"i": 10, "VI": 6, "iv": 6, "iiø7": 5, "V7": 7, "V": 5, "III": 4, "bII": 2, "V7/iv": 3,
                      "vii°7": 3},
             bass_mode="ROOT", pad_mode="BLOCK", repeat=0.3, motif=0.6, motif_use=0.7, sforzando=0, swell=0.9,
             rubato=0.35, perc_mode="OFF", mel_add=[], reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("CELLO", "OBOE", "STRINGS"), ("HARP", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
        # the Masquerade Waltz: A minor, a swirling 3/4, chromatic sighs, oom-pah-pah under the strings
        dict(name="WALTZ", root="A", scale="AEOLIAN", bpm=176, beats_per_bar=3, chord_beats=3, period_bars=8,
             melody_density=0.5, profile=[0.05, 0.6, 0.85, 0.95], max_leap=5, appoggiatura=0.3, chromatic=True,
             bass_mode="BAR", pad_mode="OOMPAH", lilt=0.15, swell=0.6, hemiola=True, perc=[3, 0, 2, 3, 0, 0],
             reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("TUBA", "CLARINET", "STRINGS"),
                   ("CONTRABASS", "STRINGS", "BRASS")], orch_mode="RANDOM"),
    ],
    "KORNGOLD": [
        dict(name="FILM"),
        # the opera arias (Die tote Stadt: Marietta's Lied): slow, a soprano over held strings, appoggiaturas
        # and a free tempo
        dict(name="ARIA", bpm=60, division=2, chord_beats=4, melody_density=0.4, melody_base=62, melody_range=19,
             profile=[0.06, 0.6, 0.82, 0.93], max_leap=6, appoggiatura=0.35, octave=0, open=0, bass_mode="ROOT",
             pad_mode="BLOCK", rubato=0.4, swell=0.8, perc_mode="OFF", reverb="HALL",
             orch=[("CONTRABASS", "VOICE", "STRINGS"), ("CELLO", "VOICE", "STRINGS"), ("HARP", "VIOLIN", "STRINGS")],
             orch_mode="RANDOM"),
        # the Violin Concerto: the solo violin's wide-arched themes over celesta and harp arpeggios
        dict(name="CONCERTO", bpm=80, chord_beats=2, melody_density=0.55, melody_range=26, max_leap=7,
             octave=0.2, appoggiatura=0.3, bass_mode="ROOT", pad_mode="ARPEGGIO", swell=0.6, rubato=0.3,
             perc_mode="OFF", reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "CELESTA"), ("CELLO", "VIOLIN", "STRINGS"), ("HORN", "VIOLIN", "PIANO")],
             orch_mode="RANDOM"),
    ],
    "DURUFLE": [
        dict(name="REQUIEM"),
        # the Sicilienne of the Suite op. 5: a lilting 6/8, a modal line on a flute or oboe stop, soft organ
        dict(name="SICILIEN", bpm=56, division=3, beats_per_bar=2, meter=[], meter_bars=0, chord_beats=1,
             period_bars=4, mel_mode="PHRASE", melody_density=0.55, profile=[0.08, 0.65, 0.88, 0.96],
             max_leap=4, swell=0.4, rubato=0.25, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "FLUTE", "PIPEORGAN"), ("PIPEORGAN", "OBOE", "PIPEORGAN"),
                   ("PIPEORGAN", "CLARINET", "REEDORGAN")], orch_mode="RANDOM"),
        # the Toccata of the Suite op. 5: B minor, a perpetual motion in sixteenths over hammered chords and a
        # pedal theme, full organ
        dict(name="TOCCATA", root="B", scale="AEOLIAN", bpm=132, division=4, beats_per_bar=4, meter=[],
             meter_bars=0, chord_beats=2, period_bars=4, mel_mode="PHRASE", melody_density=0.85,
             profile=[0.05, 0.55, 0.8, 0.92], max_leap=5, bass_mode="BAR", pad_mode="REPEAT", plagal=False,
             sforzando=0.1, swell=0.7, staccato=0.2, rubato=0, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("PIPEORGAN", "TRUMPET", "PIPEORGAN")],
             orch_mode="RANDOM"),
    ],
    "SHOSTAKOVICH": [
        dict(name="WALTZ"),
        # the symphonies (the Fifth, the Leningrad): D minor, a grim march in 4/4, snare and brass, octave
        # leaps and accents, building
        dict(name="SYMPHONY", root="D", bpm=112, beats_per_bar=4, chord_beats=4, period_bars=4,
             melody_density=0.45, profile=[0.1, 0.5, 0.75, 0.9], max_leap=7, octave=0.2, sforzando=0.15,
             staccato=0.2, bass_mode="MARCH", pad_mode="REPEAT", swell=0.8, perc=[5, 7, 4, 3, 0, 0],
             reverb="HALL",
             orch=[("TUBA", "TRUMPET", "BRASS"), ("CONTRABASS", "STRINGS", "BRASS"), ("BASSOON", "HORN", "STRINGS"),
                   ("CONTRABASS", "FLUTE", "STRINGS")], orch_mode="RANDOM"),
        # the string quartets (the Eighth's Largo): C minor, very slow, long notes over a held pedal, strings
        # alone, Phrygian darkness
        dict(name="QUARTET", bpm=50, beats_per_bar=4, chord_beats=4, period_bars=4, melody_density=0.3,
             profile=[0.05, 0.8, 0.93, 0.98], max_leap=4, mel_min_ticks=2, appoggiatura=0.1, sforzando=0,
             bass_mode="PEDAL", pad_mode="BLOCK", perc_mode="OFF", swell=0.5, rubato=0.15, color=0.4,
             reverb="ROOM",
             orch=[("CELLO", "VIOLIN", "STRINGS"), ("CELLO", "CELLO", "VIOLIN")], orch_mode="RANDOM"),
        # the polkas (The Golden Age): F major 2/4, staccato and sarcastic, leaps and chords that slip,
        # xylophone and trumpet over tuba
        dict(name="POLKA", root="F", scale="IONIAN", bpm=132, beats_per_bar=2, chord_beats=2, period_bars=8,
             melody_density=0.65, profile=[0.15, 0.55, 0.75, 0.88], max_leap=7, octave=0.2, staccato=0.6,
             appoggiatura=0.15, bass_mode="BAR", pad_mode="AFTERBEAT", sideslip=0.35, sforzando=0.15,
             perc=[0, 3, 3, 2, 2, 0], reverb="HALL",
             orch=[("TUBA", "XYLOPHONE", "PIZZ"), ("BASSOON", "CLARINET", "PIANO"), ("TUBA", "TRUMPET", "PIZZ")],
             orch_mode="RANDOM"),
    ],
    "MESSIAEN": [
        dict(name="QUATUOR"),
        # the Catalogue d'oiseaux, the Reveil des oiseaux: birdsong throughout, high and quick, over piano
        # chords in his modes
        dict(name="OISEAUX", mel_mode="BIRDSONG", alt_every=0, bpm=100, chord_beats=2, melody_density=0.6,
             mel_min_ticks=0, melody_base=76, perc_mode="OFF", reverb="HALL",
             orch=[("PIANO", "PIANO", "PIANO"), ("PIANO", "XYLOPHONE", "PIANO"), ("PIANO", "FLUTE", "CELESTA")],
             orch_mode="RANDOM"),
        # the organ works (Apparition de l'eglise eternelle, La Nativite): enormous slow chords swelling, a
        # pedal, mode 3 colour, the cathedral
        dict(name="ORGAN", bpm=40, alt_every=0, melody_density=0.3, profile=[0.04, 0.85, 0.95, 0.99], max_leap=3,
             mel_min_ticks=4,
             pad_tones=4,
             bass_mode="PEDAL", swell=1.0, color_scale="MESSIAEN3", color=0.5, perc_mode="OFF",
             reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("PIPEORGAN", "REEDORGAN", "PIPEORGAN")],
             orch_mode="RANDOM"),
    ],
    "BRITTEN": [
        dict(name="INTERLUD"),
        # A Ceremony of Carols: trebles and harp, a quicker carol tune, its canon (This little Babe)
        dict(name="CAROL", bpm=84, division=2, chord_beats=2, melody_base=69, melody_range=12,
             melody_density=0.55, profile=[0.12, 0.7, 0.9, 0.97], max_leap=4, mel_min_ticks=0,
             pad_mode="CANON", canon_ticks=4, canon_steps=0, alt_every=0, perc_mode="OFF", color=0.2,
             arrival_bars=1, reverb="CATHEDRAL",
             orch=[("HARP", "VOICE", "VOICE"), ("HARP", "VOICE", "FLUTE")], orch_mode="RANDOM"),
        # the Serenade for Tenor, Horn and Strings: a tenor or the horn over string chords, the horn's
        # natural (just) intervals
        dict(name="SERENADE", bpm=66, division=2, chord_beats=2, melody_base=60, melody_range=14,
             melody_density=0.4, profile=[0.08, 0.65, 0.85, 0.95], mel_min_ticks=0, pad_base=48,
             alt_every=0, perc_mode="OFF", rubato=0.2, tuning="JUST", reverb="HALL",
             orch=[("CELLO", "VOICE", "STRINGS"), ("CONTRABASS", "HORN", "STRINGS"), ("CELLO", "HORN", "STRINGS")],
             orch_mode="RANDOM"),
    ],
    "BARBER": [
        dict(name="ADAGIO"),
        # the songs (Knoxville: Summer of 1915, Sure on this shining night): D-flat major, a lilting triplet
        # pulse, a voice over broken chords
        dict(name="SONG", root="C#", scale="IONIAN", bpm=66, division=3, beats_per_bar=4, meter=[], meter_bars=0,
             chord_beats=2, period_bars=4, melody_density=0.45, profile=[0.08, 0.6, 0.85, 0.95], max_leap=5,
             suspension=0.2, mel_min_ticks=0, arrival_bars=2, pad_mode="BROKEN", evolve_periods=0, swell=0.5,
             open=0.2, reverb="HALL",
             orch=[("PIANO", "VOICE", "PIANO"), ("CONTRABASS", "VOICE", "STRINGS"), ("CELLO", "VOICE", "PIANO")],
             orch_mode="RANDOM"),
        # the overture The School for Scandal: brisk and witty, jagged leaps, hammered string chords, brass
        dict(name="OVERTURE", bpm=144, beats_per_bar=4, meter=[], meter_bars=0, chord_beats=2, period_bars=4,
             melody_density=0.65, profile=[0.1, 0.45, 0.7, 0.85], max_leap=7, octave=0.15, staccato=0.4,
             suspension=0, mel_min_ticks=0, arrival_bars=1, bass_mode="MARCH", pad_mode="REPEAT",
             sforzando=0.1, swell=0.6, rubato=0, evolve_periods=0, open=0.2, mixture=0.25, perc_mode="PERIOD",
             perc=[5, 4, 3, 3, 2, 0], reverb="HALL",
             orch=[("CONTRABASS", "VIOLIN", "STRINGS"), ("TUBA", "TRUMPET", "BRASS"), ("BASSOON", "FLUTE", "STRINGS"),
                   ("CELLO", "OBOE", "STRINGS")], orch_mode="RANDOM"),
    ],
    "PIAZZOLLA": [
        dict(name="TANGO"),
        # the milongas lentas (Oblivion, Milonga del angel): slow 3+3+2, long lines, held chords, no golpes
        dict(name="MILONGA", bpm=60, melody_density=0.3, mel_min_ticks=2, profile=[0.08, 0.75, 0.9, 0.97],
             max_leap=4, syncopation=0.2, pad_mode="BLOCK", rubato=0.35, swell=0.6, perc_mode="OFF",
             sforzando=0, reverb="ROOM",
             orch=[("CONTRABASS", "BANDONEON", "PIANO"), ("CELLO", "VIOLIN", "STRINGS"),
                   ("CONTRABASS", "SAX", "PIANO")], orch_mode="RANDOM"),
        # Fuga y misterio, the fugatas: two lines in counterpoint over a walking bass, tango accents
        dict(name="FUGUE", bpm=132, meter=[], beats_per_bar=4, chord_beats=2, period_bars=4, harmony="PLANNED",
             w_minor={"i6": 6, "iv7": 5, "iiø7": 5, "V7b9": 8, "VImaj7": 4, "V7sus": 3, "bII7": 3, "vii°7": 3,
                      "imaj7": 3, "im9": 4, "V7/iv": 3, "bVII9": 3},
             w_major={"Imaj7": 8, "vi7": 5, "ii7": 6, "V9": 6, "V7b9": 4, "IVmaj7": 4, "iii7": 3},
             bass_mode="WALKING", pad_mode="COUNTER", motif=0.9, motif_use=0.9, staccato=0.3, syncopation=0.3,
             sforzando=0.1, perc_mode="OFF", reverb="ROOM",
             orch=[("CONTRABASS", "BANDONEON", "VIOLIN"), ("CONTRABASS", "VIOLIN", "BANDONEON"),
                   ("CONTRABASS", "EGUITAR", "PIANO")], orch_mode="RANDOM"),
    ],
}
