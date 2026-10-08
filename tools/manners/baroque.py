# SPDX-License-Identifier: GPL-3.0-only
# Manners of the Baroque composers (tools/tuna_manners.py): the first is the profile in tuna_style.c as it is.

MANNERS = {
    "BACH": [
        dict(name="FUGUE"),
        # the four-part chorales: homophonic, a chord on every beat, stepping bass, phrases of two bars that halt
        # on a fermata (rubato into the cadence), choir and organ
        dict(name="CHORALE", reverb="CATHEDRAL", bpm=64, division=2, chord_beats=1, period_bars=2, melody_density=0.3,
             melody_range=10, profile=[0.12, 0.86, 0.96, 0.995], max_leap=3, mel_min_ticks=2, suspension=0.2,
             bass_mode="WALKING", pad_mode="BLOCK", pad_tones=4, pad_base=55, fifth_bias=3.0, open=0.25,
             rubato=0.6, motif=0.1, motif_use=0.3, arrival_bars=1,
             orch=[("PIPEORGAN", "CHOIR", "PIPEORGAN"), ("CONTRABASS", "CHOIR", "STRINGS"),
                   ("BASSOON", "OBOE", "PIPEORGAN")], orch_mode="RANDOM"),
        # the suites' gigues: 12/8, leaping lines over a running bass, two voices
        dict(name="GIGUE", bpm=96, division=3, beats_per_bar=4, chord_beats=2, period_bars=4, melody_density=0.8,
             profile=[0.04, 0.55, 0.8, 0.93], max_leap=5, suspension=0.1, bass_mode="WALKING",
             pad_mode="COUNTER", motif=0.5, motif_use=0.9,
             orch=[("CELLO", "VIOLIN", "HARPSICHORD"), ("OUD", "OUD", "OUD"),
                   ("HARPSICHORD", "HARPSICHORD", "HARPSICHORD")], orch_mode="RANDOM"),
    ],
}
