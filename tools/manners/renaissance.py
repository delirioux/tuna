# SPDX-License-Identifier: GPL-3.0-only
# Manners of the medieval and Renaissance composers (tools/tuna_manners.py): the first is the profile in
# tuna_style.c as it is.

MANNERS = {
    "HILDEGARD": [
        # the antiphons and responsories: one soaring melismatic line over a drone, unmetered
        dict(name="ANTIPHON"),
        # the sequences and hymns (O ignis spiritus, Ave generosa): syllabic, narrower, paired versicles sung to
        # the same music, a choir of nuns in unison with a second line in parallel fourths (organum)
        dict(name="HYMN", bpm=72, melody_density=0.8, melody_range=13, profile=[0.12, 0.78, 0.92, 0.98],
             max_leap=3, repeat=0.7, period_bars=2, motif=0.6, motif_use=0.7, var_alter=0.2, rubato=0.1,
             pad_mode="ORGANUM", pad_interval=5, reverb="CATHEDRAL",
             orch=[("CELLO", "CHOIR", "VOICE"), ("DRONE", "VOICE", "VOICE"), ("CELLO", "VOICE", "FLUTE")],
             orch_mode="RANDOM"),
    ],
    "TALLIS": [
        # the Latin motets and Lamentations: Dorian polyphony, suspensions, Amen endings
        dict(name="MOTET"),
        # the English anthems (If ye love me, O Lord give thy Holy Spirit): F major, homophonic, syllabic, a
        # chord on every beat, short phrases
        dict(name="ANTHEM", root="F", scale="IONIAN", bpm=66, chord_beats=1, period_bars=2, melody_density=0.35,
             melody_range=11, profile=[0.15, 0.82, 0.95, 0.99], max_leap=3, mel_min_ticks=2, suspension=0.2,
             arrival_bars=1, pad_mode="BLOCK", pad_tones=4, pad_base=53, motif=0.3, motif_use=0.4, rubato=0.35,
             reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("BASSOON", "VOICE", "PIPEORGAN"),
                   ("CONTRABASS", "CHOIR", "STRINGS")], orch_mode="RANDOM"),
        # the psalm tunes for Parker's Psalter, the eighth of them Tallis's Canon: G major, two-bar phrases,
        # the tune again in the tenor an octave below, one bar later
        dict(name="CANON", scale="IONIAN", bpm=76, chord_beats=2, period_bars=2, melody_density=0.45,
             melody_range=10, profile=[0.06, 0.86, 0.96, 0.99], max_leap=3, mel_min_ticks=2, suspension=0.1,
             arrival_bars=1, pad_mode="CANON", canon_ticks=8, canon_steps=-7, repeat=0.5, motif=0.6,
             motif_use=0.7, plagal=False, rubato=0.25,
             orch=[("PIPEORGAN", "CHOIR", "VOICE"), ("BASSOON", "VOICE", "VOICE"),
                   ("PIPEORGAN", "VOICE", "PIPEORGAN")], orch_mode="RANDOM"),
    ],
    "PALESTRINA": [
        # the masses (Missa Papae Marcelli) and motets: smooth imitative counterpoint, prepared suspensions
        dict(name="MASS"),
        # the hymns for the whole year: plainchant verses and polyphonic verses in turn (alternatim), the
        # polyphony often in triple time
        dict(name="HYMN", beats_per_bar=3, chord_beats=3, bpm=72, period_bars=4, alt_every=1,
             **{"alt.mel_mode": "CHANT", "alt.bass_mode": "DRONE", "alt.harmony": "DRONE", "alt.density": 1.3},
             reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "VOICE"), ("BASSOON", "VOICE", "VOICE")], orch_mode="RANDOM"),
        # the madrigals (Vestiva i colli): secular, livelier and more syllabic, voices with viols and lute
        dict(name="MADRIGAL", bpm=80, division=4, melody_density=0.55, melody_range=14,
             profile=[0.1, 0.74, 0.9, 0.97], suspension=0.3, syncopation=0.1, motif_use=0.5, var_alter=0.35,
             rubato=0.15, reverb="ROOM",
             orch=[("CELLO", "VOICE", "VOICE"), ("OUD", "VOICE", "OUD"), ("CELLO", "VIOLIN", "OUD")],
             orch_mode="RANDOM"),
    ],
    "ALLEGRI": [
        # one work and one idiom: the Miserere
        dict(name="MISERERE"),
    ],
    "MONTEVERDI": [
        # the operas and dramatic works (Orfeo, Combattimento): a voice over continuo, concitato ritornelli
        dict(name="OPERA"),
        # the madrigal books (Cruda Amarilli, Si ch'io vorrei morire): five voices, harsh prepared dissonance,
        # chromatic leanings, imitative
        dict(name="MADRIGAL", bpm=66, division=2, chord_beats=2, melody_density=0.45,
             profile=[0.1, 0.75, 0.9, 0.97], suspension=0.5, appoggiatura=0.4, chromatic=True, pad_mode="COUNTER",
             pad_base=50, alt_every=0, rubato=0.25, reverb="ROOM",
             orch=[("CELLO", "VOICE", "VOICE"), ("CELLO", "VOICE", "CHOIR"), ("BASSOON", "VOICE", "VOICE")],
             orch_mode="RANDOM"),
        # the Vespers of 1610 (Deus in adjutorium, Dixit Dominus): D major, a held chord under cornett
        # fanfares, falsobordone recitation, sackbuts and organ in St Mark's
        dict(name="VESPERS", scale="IONIAN", bpm=84, division=4, chord_beats=4, melody_density=0.6,
             melody_range=17, profile=[0.25, 0.6, 0.82, 0.94], appoggiatura=0.1, bass_mode="PEDAL",
             pad_mode="BLOCK", pad_tones=4, swell=0.5, rubato=0.15, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "TRUMPET", "BRASS"), ("CONTRABASS", "CHOIR", "PIPEORGAN"),
                   ("BASSOON", "TRUMPET", "STRINGS")], orch_mode="CONTRAST"),
        # the laments over a ground (Lamento della ninfa): the descending tetrachord A G F E, a plaint above it
        dict(name="LAMENTO", root="A", bpm=56, division=2, chord_beats=2, period_bars=4, harmony="CYCLE",
             cycle=["i", "bVII", "bVI", "V"], bass_mode="LINE", bass_notes=[0, -2, -4, -5],
             melody_density=0.35, appoggiatura=0.4, suspension=0.5, chromatic=True, direction=-0.3,
             alt_every=0, rubato=0.3, reverb="HALL",
             orch=[("CELLO", "VOICE", "OUD"), ("OUD", "VOICE", "HARPSICHORD"), ("CELLO", "VOICE", "PIPEORGAN")],
             orch_mode="RANDOM"),
    ],
    "SWEELINCK": [
        # the fantasias and echo fantasias: running figuration over modal counterpoint, echoes
        dict(name="FANTASIA"),
        # the toccatas: free, virtuoso runs over long held chords and an organ pedal
        dict(name="TOCCATA", bpm=84, chord_beats=4, melody_density=0.9, melody_range=22,
             profile=[0.02, 0.8, 0.92, 0.98], motif=0.3, motif_use=0.5, bass_mode="PEDAL", pad_mode="BLOCK",
             echo=0, rubato=0.2, swell=0.4,
             orch=[("PIPEORGAN", "PIPEORGAN", "PIPEORGAN"), ("HARPSICHORD", "HARPSICHORD", "HARPSICHORD")],
             orch_mode="RANDOM"),
        # the song variations (Mein junges Leben hat ein End, Unter der Linden grune): the tune's harmony
        # again and again in D Dorian, the figuration growing
        dict(name="VARIATIO", root="D", scale="DORIAN", melody_density=0.5, repeat=0.85, motif=0.6,
             motif_use=0.85, echo=0, evolve_periods=2, evolve_depth=0.8,
             orch=[("HARPSICHORD", "HARPSICHORD", "HARPSICHORD"), ("PIPEORGAN", "PIPEORGAN", "PIPEORGAN")],
             orch_mode="CYCLE"),
        # the Genevan psalms for voices (Pseaumes de David): choral polyphony, slower, suspensions
        dict(name="PSALM", bpm=64, division=2, melody_density=0.4, melody_range=13,
             profile=[0.05, 0.82, 0.94, 0.99], suspension=0.45, motif_beats=4, motif_use=0.6, echo=0,
             swell=0.3, rubato=0.15, reverb="CATHEDRAL",
             orch=[("PIPEORGAN", "CHOIR", "VOICE"), ("BASSOON", "VOICE", "VOICE")], orch_mode="RANDOM"),
    ],
}
