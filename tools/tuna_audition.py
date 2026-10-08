#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Measure TUNA's instrument families against each other: which sound alike.

    python3 tools/tuna_audition.py OUTDIR      (after build/host/tuna_audition OUTDIR)

Reads OUTDIR/dry/NN_NAME.wav (a held C4 for 1.2 s and its release, then a staccato C5) and describes each sound
by its envelope (attack, decay, sustain, release), its spectrum (the first 12 harmonics, brightness, noise and
inharmonic energy) early and late in the note. The descriptions are standardised and compared; the report lists
each family's nearest neighbour and every pair closer than a threshold (sound-alikes)."""
import pathlib
import sys
import wave

import numpy as np

F0 = 261.6256


def load(path):
    with wave.open(str(path)) as w:
        sr = w.getframerate()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64) / 32768.0
    return x, sr


def env_db(x, sr, hop=0.01):
    n = int(sr * hop)
    m = len(x) // n
    r = np.sqrt(np.mean(x[: m * n].reshape(m, n) ** 2, axis=1) + 1e-12)
    return 20 * np.log10(r + 1e-9), hop


def spectrum_features(seg, sr, f0):
    w = np.hanning(len(seg))
    s = np.abs(np.fft.rfft(seg * w)) + 1e-12
    fr = np.fft.rfftfreq(len(seg), 1 / sr)
    tot = np.sum(s ** 2)
    harm = []
    hmask = np.zeros_like(s, dtype=bool)
    for k in range(1, 13):
        b = (fr > k * f0 * 0.97) & (fr < k * f0 * 1.03)
        hmask |= b
        harm.append(np.sum(s[b] ** 2))
    harm = np.array(harm)
    hdb = 10 * np.log10(harm / (harm.max() + 1e-18) + 1e-9)
    hdb = np.maximum(hdb, -60)
    cent = np.sum(fr * s ** 2) / tot / f0
    noise = 1 - np.sum(s[hmask] ** 2) / tot
    low = np.sum(s[fr < f0 * 0.9] ** 2) / tot           # below the fundamental: sub-octaves, thumps
    return hdb, np.log2(cent + 1e-6), noise, low


def describe(path):
    x, sr = load(path)
    e, hop = env_db(x, sr)
    on = e[: int(1.2 / hop)]
    peak = on.max()
    if peak < -70:
        return None
    t90 = np.argmax(on >= peak - 1) * hop
    at = lambda t: e[min(int(t / hop), len(e) - 1)] - peak
    after = e[int(1.2 / hop): int(2.0 / hop)]
    rel = (np.argmax(after < peak - 30) * hop) if np.any(after < peak - 30) else 0.8
    st = e[int(2.0 / hop): int(3.0 / hop)]
    st_peak = st.max()
    st_dec = (np.argmax(st[np.argmax(st):] < st_peak - 30) * hop) if np.any(st < st_peak - 30) else 1.0
    h1, c1, n1, l1 = spectrum_features(x[int(0.10 * sr): int(0.10 * sr) + 8192], sr, F0)
    h2, c2, n2, l2 = spectrum_features(x[int(0.85 * sr): int(0.85 * sr) + 8192], sr, F0)
    hs, cs, ns, ls = spectrum_features(x[int(2.0 * sr): int(2.0 * sr) + 4096], sr, 2 * F0)
    f = {
        "attack": np.log10(t90 + 0.005), "at0.5": at(0.5), "at1.1": at(1.1), "release": rel,
        "stacc_decay": st_dec, "bright": c1, "bright_late": c2 - c1, "noise": n1, "noise_late": n2,
        "sub": l1, "stacc_bright": cs,
    }
    for k in range(12):
        f[f"h{k + 1}"] = h1[k] / 10
        f[f"h{k + 1}_late"] = (h2[k] - h1[k]) / 20
    return f


WEIGHTS = {"attack": 2.0, "at0.5": 1.5, "at1.1": 1.5, "release": 1.0, "stacc_decay": 1.0, "bright": 2.0,
           "bright_late": 1.0, "noise": 1.5, "noise_late": 0.5, "sub": 0.5, "stacc_bright": 1.0}


def cpu(out):
    """instructions of Felucca's mix per sample of each family's phrase (one callgrind dump each, in the order
    of families.txt): the cost of the sound itself, a held note, a run and a triad"""
    fams = [l[3:l.index(" engine")].strip() for l in (out / "families.txt").read_text().splitlines()]
    dumps = sorted(out.glob("callgrind.out.*.*"), key=lambda p: int(p.name.rsplit(".", 1)[1]))
    frames = 44100 * 6.5
    rows = []
    for name, p in zip(fams, dumps):
        tot = 0
        for line in p.read_text().splitlines():
            if line.startswith("summary:") or line.startswith("totals:"):
                tot = int(line.split()[1])
        rows.append((tot / frames, name))
    ref = dict((n, v) for v, n in rows).get("PIANO", 1)
    print("instructions per sample of the mix (relative to PIANO):")
    for v, n in sorted(rows, reverse=True):
        print(f"  {n:<10} {v:8.0f}  {v / ref:5.2f}x")


def main():
    if "--cpu" in sys.argv:
        cpu(pathlib.Path(sys.argv[1]))
        return
    out = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    names, feats = [], []
    for p in sorted((out / "dry").glob("*.wav")):
        d = describe(p)
        if d is None:
            print(f"{p.stem}: silent")
            continue
        names.append(p.stem[3:])
        feats.append(d)
    keys = list(feats[0].keys())
    X = np.array([[f[k] for k in keys] for f in feats])
    sd = X.std(axis=0)
    sd[sd < 1e-9] = 1
    Z = (X - X.mean(axis=0)) / sd
    w = np.array([WEIGHTS.get(k, 0.4) for k in keys])
    Z *= np.sqrt(w)
    D = np.sqrt(((Z[:, None, :] - Z[None, :, :]) ** 2).sum(-1))
    np.fill_diagonal(D, np.inf)
    med = np.median(D[np.isfinite(D)])
    print(f"{len(names)} families; distances relative to the median pair ({med:.2f})")
    order = np.argsort(D.min(axis=1))
    print("\nnearest neighbour, closest first:")
    for i in order:
        j = int(np.argmin(D[i]))
        print(f"  {names[i]:<11} ~ {names[j]:<11} {D[i, j] / med:5.2f}")
    thr = float(sys.argv[2]) if len(sys.argv) > 2 else 0.35
    pairs = [(D[i, j] / med, names[i], names[j]) for i in range(len(names)) for j in range(i + 1, len(names))
             if D[i, j] / med < thr]
    print(f"\nsound-alikes (below {thr} of the median): {len(pairs)}")
    for d, a, b in sorted(pairs):
        print(f"  {a:<11} {b:<11} {d:5.2f}")


if __name__ == "__main__":
    main()
