"""Makes the sounds no free pack had: sword-less swings, the Flare, chimes, the heartbeat, the Turning's rumble
and the air of each zone. They are ours, made from noise and sine waves by this script.

    python tools/audio/make_sounds.py [output folder]        (needs numpy; default art_source/audio)

Every loop is built from a spectrum with whole-number cycles, so it joins itself without a click.
"""

import os
import sys
import wave

import numpy as np

RATE = 44100


def save(folder, name, samples, peak=0.9):
    samples = np.asarray(samples, dtype=np.float64)
    top = np.max(np.abs(samples)) or 1.0
    data = (samples / top * peak * 32767.0).astype(np.int16)
    with wave.open(os.path.join(folder, name + ".wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(data.tobytes())


def noise(rng, seconds, shape):
    """Noise whose spectrum is shape(frequency in Hz) -> gain. Periodic, so it loops."""
    n = int(seconds * RATE)
    freqs = np.fft.rfftfreq(n, 1.0 / RATE)
    spectrum = shape(freqs) * np.exp(1j * rng.uniform(0, 2 * np.pi, len(freqs)))
    spectrum[0] = 0
    return np.fft.irfft(spectrum, n)


def band(centre, width):
    return lambda f: np.exp(-0.5 * ((np.log(np.maximum(f, 1.0)) - np.log(centre)) / width) ** 2)


def times(seconds):
    return np.arange(int(seconds * RATE)) / RATE


def swell(t, attack, release, total):
    """Rises over attack, falls over release, ending at total."""
    return np.clip(t / attack, 0, 1) * np.clip((total - t) / release, 0, 1)


def whoosh(rng, seconds, low, high, tone=0.0):
    """Air moved fast: a band of noise that sweeps up and back, loudest in the middle."""
    t = times(seconds)
    sweep = np.sin(np.pi * t / seconds) ** 1.5
    a = noise(rng, seconds, band(low, 0.45))
    b = noise(rng, seconds, band(high, 0.4))
    out = (a * (1 - sweep) + b * sweep * 1.4) * sweep ** 1.2
    if tone:
        out += tone * np.sin(2 * np.pi * (low * 1.5 + (high - low) * 0.5 * sweep) * t) * sweep ** 2 * 0.25
    return out


def chime(notes, seconds, decay):
    t = times(seconds)
    out = np.zeros_like(t)
    for start, freq, gain in notes:
        local = np.clip(t - start, 0, None)
        ring = np.exp(-local * decay) * (t >= start)
        out += gain * ring * (np.sin(2 * np.pi * freq * local) + 0.35 * np.sin(2 * np.pi * freq * 2.76 * local) * np.exp(-local * decay * 2))
    return out * np.clip(t / 0.004, 0, 1)


def air(rng, seconds, layers, gusts):
    """A looping bed: noise layers (centre Hz, width, gain), swelling in slow whole-number cycles."""
    t = times(seconds)
    out = np.zeros_like(t)
    for index, (centre, width, gain) in enumerate(layers):
        cycles = gusts[index % len(gusts)]
        swell_curve = 0.6 + 0.4 * np.sin(2 * np.pi * cycles * t / seconds + rng.uniform(0, 6.28)) * np.sin(2 * np.pi * (cycles + 2) * t / seconds + rng.uniform(0, 6.28))
        out += gain * noise(rng, seconds, band(centre, width)) / 0.05 * swell_curve
    return out


def main(folder):
    os.makedirs(folder, exist_ok=True)
    rng = np.random.default_rng(1201)

    for i in range(3):
        save(folder, "whoosh_light_%d" % (i + 1), whoosh(rng, 0.20 + 0.02 * i, 900, 3200 + 300 * i), 0.7)
    for i in range(2):
        save(folder, "whoosh_heavy_%d" % (i + 1), whoosh(rng, 0.36 + 0.04 * i, 380, 1900 + 200 * i), 0.85)
    for i in range(2):
        save(folder, "whoosh_stroke_%d" % (i + 1), whoosh(rng, 0.5 + 0.05 * i, 500, 2600, tone=1.0), 0.85)
    save(folder, "whoosh_dodge", whoosh(rng, 0.28, 260, 1200), 0.6)

    # The Flare: a strike, a catch, and a second of burning.
    t = times(1.1)
    flare = noise(rng, 1.1, band(2600, 0.7)) * np.exp(-t * 26) * 3.0 + noise(rng, 1.1, band(700, 0.6)) * swell(t, 0.08, 0.9, 1.1) \
        + 0.12 * np.sin(2 * np.pi * (180 + 240 * np.clip(t / 0.25, 0, 1)) * t) * swell(t, 0.05, 0.8, 1.1)
    save(folder, "flare_ignite", flare, 0.8)

    save(folder, "chime_rest", chime([(0.0, 392.0, 1.0), (0.22, 523.25, 0.8), (0.5, 659.25, 0.6)], 2.2, 2.2), 0.6)
    save(folder, "chime_attune", chime([(0.0, 440.0, 0.8), (0.12, 554.37, 0.8), (0.24, 659.25, 0.9), (0.4, 880.0, 0.7)], 2.0, 2.6), 0.6)
    save(folder, "chime_quest_new", chime([(0.0, 587.33, 0.8), (0.16, 783.99, 0.7)], 1.3, 3.2), 0.5)
    save(folder, "chime_quest_done", chime([(0.0, 523.25, 0.8), (0.14, 659.25, 0.8), (0.28, 783.99, 0.9), (0.5, 1046.5, 0.6)], 2.4, 2.0), 0.6)
    save(folder, "chime_contrast", chime([(0.0, 1318.5, 1.0), (0.03, 1975.5, 0.5)], 0.6, 7.0), 0.5)
    save(folder, "chime_perfect", chime([(0.0, 1760.0, 1.0), (0.05, 2637.0, 0.6)], 0.5, 9.0), 0.45)
    save(folder, "chime_glim", chime([(0.0, 987.77, 0.8)], 0.5, 8.0), 0.35)

    # Her heart: two soft thumps.
    t = times(1.0)
    thump = lambda at: np.sin(2 * np.pi * (58 - 18 * np.clip((t - at) / 0.2, 0, 1)) * np.clip(t - at, 0, None)) * np.exp(-np.clip(t - at, 0, None) * 16) * (t >= at)
    save(folder, "heartbeat", thump(0.0) + 0.75 * thump(0.28), 0.9)

    # The Turning: nine seconds of something enormous moving.
    t = times(9.0)
    rumble = noise(rng, 9.0, band(48, 0.5)) * 4.0 + noise(rng, 9.0, band(140, 0.6)) + 0.5 * noise(rng, 9.0, band(900, 0.8)) * np.clip(1 - t / 3.0, 0, 1)
    save(folder, "rumble_turning", rumble * swell(t, 1.2, 4.5, 9.0) * (0.75 + 0.25 * np.sin(2 * np.pi * 0.9 * t)), 0.95)

    # The death of the player and of an enemy: a low exhale.
    t = times(1.4)
    save(folder, "exhale_low", noise(rng, 1.4, band(220, 0.7)) * swell(t, 0.06, 1.3, 1.4) + 0.3 * np.sin(2 * np.pi * (120 - 50 * t / 1.4) * t) * np.exp(-t * 3), 0.8)

    # The air of each zone, 24 seconds, looping.
    save(folder, "amb_village", air(rng, 24.0, [(260, 0.75, 1.0), (900, 0.6, 0.35), (3200, 0.5, 0.08)], [2, 3, 5]), 0.5)
    save(folder, "amb_forest", air(rng, 24.0, [(320, 0.7, 0.8), (1600, 0.55, 0.5), (5200, 0.4, 0.22)], [3, 5, 7]), 0.55)
    t = times(24.0)
    vault = air(rng, 24.0, [(110, 0.6, 1.0), (520, 0.5, 0.2)], [1, 2]) + 0.06 * np.sin(2 * np.pi * 55 * t) * (0.6 + 0.4 * np.sin(2 * np.pi * 3 * t / 24.0))
    save(folder, "amb_vault", vault, 0.5)
    print("made", len([f for f in os.listdir(folder) if f.endswith(".wav")]), "sounds in", folder)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "art_source/audio")
