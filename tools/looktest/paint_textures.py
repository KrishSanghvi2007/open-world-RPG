"""Paints the brush-stroke textures used by the painterly materials and the painted skies.

Run with any Python that has numpy and Pillow:
    python tools/looktest/paint_textures.py <output folder>

Brushwork lives in textures, not in a screen filter (docs/GDD.md section 9). Each surface gets one tileable
"stroke mask": the material turns it into colour, so one mask serves every palette and style.
    R = tone of the stroke (0 dark .. 1 light). The material maps it onto three values: dark, middle, light.
    G = which strokes take the accent colour (0 none .. 1 full).
    B = bristle streaks inside each stroke, for a little breakup.
Skies are painted directly in colour as 2:1 panoramas.
"""

import math
import os
import sys

import numpy as np
from PIL import Image

SIZE = 1024


def tile_noise(rng, size, octaves=3, base=2):
    """Smooth noise that tiles: a sum of sine waves with whole-number frequencies."""
    y, x = np.mgrid[0:size, 0:size] / float(size) * 2.0 * math.pi
    out = np.zeros((size, size))
    amp = 1.0
    for o in range(octaves):
        for _ in range(4):
            fx, fy = rng.integers(-base * (o + 1), base * (o + 1) + 1, 2)
            if fx == 0 and fy == 0:
                fx = 1
            out += amp * np.sin(fx * x + fy * y + rng.uniform(0, 2 * math.pi))
        amp *= 0.5
    out -= out.min()
    return out / out.max()


def stamp(canvas, cx, cy, angle, length, width, colour, rng, opacity=1.0, wrap_y=True, bristle_gap=0.35):
    """One brush stroke: a tapered bar with bristle streaks, blended into canvas (H, W, C), wrapping at the edges."""
    h, w, _ = canvas.shape
    half = int(math.ceil((length + width) / 2.0)) + 2
    ys = np.arange(int(cy) - half, int(cy) + half + 1)
    xs = np.arange(int(cx) - half, int(cx) + half + 1)
    gx, gy = np.meshgrid(xs - cx, ys - cy)
    ca, sa = math.cos(angle), math.sin(angle)
    u = gx * ca + gy * sa          # along the stroke
    v = -gx * sa + gy * ca         # across the stroke

    # The stroke bows slightly and its width tapers toward the lifted end.
    v = v + (u / max(length, 1.0)) ** 2 * width * rng.uniform(-0.6, 0.6)
    t = np.clip(u / length + 0.5, 0.0, 1.0)
    local_width = width * (1.0 - 0.45 * t ** 2)
    across = np.abs(v) / (local_width / 2.0)

    bristles = rng.uniform(1.0 - bristle_gap, 1.0, 24)
    bristles = np.convolve(np.pad(bristles, 2, mode="edge"), np.ones(3) / 3.0, mode="same")[2:-2]
    bristle_index = np.clip(((v / width) + 0.5) * 23.0, 0, 23).astype(int)
    streak = bristles[bristle_index]

    edge = np.clip((1.0 - across) * 4.0, 0.0, 1.0)                 # crisp sides
    ends = np.clip((0.5 - np.abs(u / length)) * length / (width * 0.35), 0.0, 1.0)
    dry = np.where(t > 0.6, 1.0 - (t - 0.6) / 0.4 * (1.0 - streak) * 1.8, 1.0)   # paint runs out at the end
    alpha = np.clip(edge * ends * np.clip(dry, 0.0, 1.0), 0.0, 1.0) * opacity

    yi = ys % h if wrap_y else np.clip(ys, 0, h - 1)
    xi = xs % w
    if not wrap_y:
        alpha = alpha * ((ys >= 0) & (ys < h))[:, None]
    region = canvas[np.ix_(yi, xi)]
    paint = np.empty_like(region)
    paint[..., :] = colour
    if canvas.shape[2] == 3 and len(colour) == 3 and colour[2] < 0:   # mask mode: B carries the bristle streaks
        paint[..., 2] = streak
    canvas[np.ix_(yi, xi)] = region + (paint - region) * alpha[..., None]


def stroke_mask(seed, count, length, width, tones, tone_weights, angle=0.0, spread=0.6, accent_share=0.15,
                ground=0.5, bristle_gap=0.35, rows=None):
    """Builds one tileable stroke mask. tones are the R values strokes may take."""
    rng = np.random.default_rng(seed)
    canvas = np.zeros((SIZE, SIZE, 3))
    canvas[..., 0] = ground
    canvas[..., 2] = 0.8
    value_field = tile_noise(rng, SIZE, 3, 2)
    flow_field = tile_noise(rng, SIZE, 2, 1)
    accent_field = tile_noise(rng, SIZE, 2, 2)
    tones = np.array(tones)
    cumulative = np.cumsum(np.array(tone_weights, dtype=float) / sum(tone_weights))

    for i in range(count):
        cx, cy = rng.uniform(0, SIZE, 2)
        if rows:                                   # snap strokes into rows (roof tiles, planks)
            cy = (round(cy / rows) + rng.uniform(-0.12, 0.12)) * rows
        local = value_field[int(cy) % SIZE, int(cx) % SIZE]
        pick = np.clip(local * 0.7 + rng.uniform(0, 0.3), 0, 0.999)
        tone = tones[np.searchsorted(cumulative, pick)] + rng.uniform(-0.04, 0.04)
        accent = 1.0 if (accent_field[int(cy) % SIZE, int(cx) % SIZE] * 0.6 + rng.uniform(0, 0.4)) > 1.0 - accent_share else 0.0
        a = angle + (flow_field[int(cy) % SIZE, int(cx) % SIZE] - 0.5) * 2.0 * spread + rng.normal(0, 0.08)
        stamp(canvas, cx, cy, a, length * rng.uniform(0.6, 1.3), width * rng.uniform(0.7, 1.25),
              (float(np.clip(tone, 0, 1)), accent, -1.0), rng, opacity=rng.uniform(0.85, 1.0), bristle_gap=bristle_gap)
    return (np.clip(canvas, 0, 1) * 255).astype(np.uint8)


def hex_rgb(h):
    h = h.lstrip("#")
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)])


def painted_sky(seed, stops, sun_colour, cloud_light, cloud_dark, sun_azimuth=0.25, sun_height=0.07,
                strokes=5200, cloudiness=0.45, stroke_len=120, stroke_width=26):
    """A dusk sky panorama. stops: (height 0..1 above the horizon, colour) pairs. Left-right wraps."""
    rng = np.random.default_rng(seed)
    w, h = 2048, 1024
    horizon = h // 2

    def gradient(elevation):
        e = np.clip(elevation, 0, 1)
        colour = np.zeros(3)
        for (h0, c0), (h1, c1) in zip(stops, stops[1:]):
            if h0 <= e <= h1:
                f = (e - h0) / max(h1 - h0, 1e-6)
                f = f * f * (3 - 2 * f)
                colour = hex_rgb(c0) * (1 - f) + hex_rgb(c1) * f
        return colour

    canvas = np.zeros((h, w, 3))
    for y in range(h):
        canvas[y, :, :] = gradient((horizon - y) / float(horizon)) if y < horizon else hex_rgb(stops[0][1]) * 0.55

    cloud_field = np.zeros((h, w))
    yy, xx = np.mgrid[0:h, 0:w]
    for _ in range(10):
        fx, fy = rng.integers(1, 6), rng.uniform(4, 16)
        cloud_field += np.sin(fx * xx / w * 2 * math.pi + rng.uniform(0, 6.28)) * np.sin(fy * yy / h * math.pi + rng.uniform(0, 6.28))
    cloud_field = (cloud_field - cloud_field.min()) / (cloud_field.max() - cloud_field.min())

    sun_x, sun_y = sun_azimuth * w, horizon - sun_height * horizon
    sun = hex_rgb(sun_colour)
    order = rng.uniform(0, 1, strokes)
    for i in range(strokes):
        cx = rng.uniform(0, w)
        cy = rng.uniform(0, horizon + 30)
        elevation = (horizon - cy) / float(horizon)
        colour = gradient(elevation) + rng.normal(0, 0.018, 3)
        dx = min(abs(cx - sun_x), w - abs(cx - sun_x))
        glow = math.exp(-((dx / (w * 0.16)) ** 2 + ((cy - sun_y) / (h * 0.16)) ** 2))
        colour = colour + (sun - colour) * glow * 0.75
        length, width, angle = stroke_len, stroke_width, rng.normal(0, 0.10)
        if cloud_field[int(cy) % h, int(cx) % w] > 1.0 - cloudiness and 0.04 < elevation < 0.75 and order[i] > 0.35:
            lit = np.clip(glow * 1.6 + 0.25 + (0.2 if rng.uniform() > 0.5 else 0.0), 0, 1)
            colour = hex_rgb(cloud_dark) * (1 - lit) + hex_rgb(cloud_light) * lit + rng.normal(0, 0.02, 3)
            length, width = stroke_len * 1.5, stroke_width * 0.8
        stamp(canvas, cx, cy, angle, length * rng.uniform(0.6, 1.4), width * rng.uniform(0.6, 1.3),
              tuple(np.clip(colour, 0, 1)), rng, opacity=rng.uniform(0.75, 1.0), wrap_y=False, bristle_gap=0.25)

    # The dying sun: a soft disc, low in the west.
    disc = np.exp(-(((np.minimum(np.abs(xx - sun_x), w - np.abs(xx - sun_x))) ** 2 + (yy - sun_y) ** 2) / (2 * 26.0 ** 2)))
    canvas += (np.clip(sun * 1.25, 0, 1) - canvas) * np.clip(disc * 1.4, 0, 1)[..., None]
    return (np.clip(canvas, 0, 1) * 255).astype(np.uint8)


def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)

    masks = {
        # Plaster: broad, loose strokes, mostly light with warm middles.
        "T_Stroke_Plaster": dict(seed=11, count=2600, length=120, width=44, tones=(0.12, 0.5, 0.88),
                                 tone_weights=(1, 4, 5), angle=0.5, spread=1.3, accent_share=0.22, ground=0.6),
        # Roof: rows of short downward dabs, like tiles laid with the flat of the brush.
        "T_Stroke_Roof": dict(seed=23, count=3400, length=70, width=40, tones=(0.1, 0.5, 0.9),
                              tone_weights=(3, 5, 2), angle=math.pi / 2, spread=0.15, accent_share=0.3, ground=0.2, rows=64),
        # Wood: long thin dragged strokes along the grain.
        "T_Stroke_Wood": dict(seed=37, count=3600, length=260, width=16, tones=(0.08, 0.45, 0.85),
                              tone_weights=(4, 5, 2), angle=0.0, spread=0.12, accent_share=0.15, ground=0.3, bristle_gap=0.6),
        # Stone: short fat dabs with dark gaps left between them.
        "T_Stroke_Stone": dict(seed=41, count=2300, length=62, width=50, tones=(0.1, 0.5, 0.9),
                               tone_weights=(2, 5, 3), angle=0.0, spread=3.1, accent_share=0.2, ground=0.08),
        # Ground: wide sweeping strokes for packed earth and worn paving.
        "T_Stroke_Ground": dict(seed=53, count=2200, length=190, width=38, tones=(0.15, 0.5, 0.85),
                                tone_weights=(3, 5, 3), angle=0.2, spread=0.8, accent_share=0.25, ground=0.4),
        # Foliage and cloth: quick small comma strokes.
        "T_Stroke_Dab": dict(seed=67, count=5200, length=46, width=20, tones=(0.1, 0.5, 0.92),
                             tone_weights=(4, 4, 3), angle=1.0, spread=2.2, accent_share=0.3, ground=0.2),
    }
    for name, spec in masks.items():
        Image.fromarray(stroke_mask(**spec), "RGB").save(os.path.join(out_dir, name + ".png"))
        print("painted", name)

    skies = {
        # A: gouache. Bold flat bands, strong rose and teal.
        "T_Sky_Gouache": dict(seed=101, stops=[(0.0, "#F2B45A"), (0.10, "#E98A5B"), (0.26, "#C4574B"), (0.50, "#6E4A6E"), (1.0, "#2E5E6E")],
                              sun_colour="#FFE9A8", cloud_light="#F6C38A", cloud_dark="#5B3F63",
                              strokes=3000, stroke_len=210, stroke_width=46, cloudiness=0.40),
        # B: glaze. Luminous gold low down, thin warm layers, soft small strokes.
        "T_Sky_Glaze": dict(seed=202, stops=[(0.0, "#F7D08A"), (0.14, "#F0A868"), (0.34, "#D98A78"), (0.62, "#8C7396"), (1.0, "#4A5E7A")],
                            sun_colour="#FFF3C8", cloud_light="#FBDCA6", cloud_dark="#8A6A86",
                            strokes=9000, stroke_len=90, stroke_width=18, cloudiness=0.5),
        # C: ink and wash. Pale paper sky, few muted strokes, colour held back for the accents.
        "T_Sky_Wash": dict(seed=303, stops=[(0.0, "#EBCB9A"), (0.16, "#DDB69A"), (0.40, "#B9A6A8"), (0.70, "#8E9AA6"), (1.0, "#6E8591")],
                           sun_colour="#FFF1D6", cloud_light="#F0DDC0", cloud_dark="#7E7C8C",
                           strokes=2600, stroke_len=260, stroke_width=34, cloudiness=0.3),
    }
    for name, spec in skies.items():
        Image.fromarray(painted_sky(**spec), "RGB").save(os.path.join(out_dir, name + ".png"))
        print("painted", name)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "art_source/looktest/textures")
