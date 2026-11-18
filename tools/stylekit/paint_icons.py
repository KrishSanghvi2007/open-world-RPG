"""Style probe: item icons in the Glaze look, painted by script.

    python tools/stylekit/paint_icons.py <output folder>

Each icon is built the way the game's surfaces are: an underpainting in one flat tone, then short loaded brush
strokes laid along the form (violet in the shadow, the item's own colour in the middle, warm cream where the
Lamp catches it), on bare paper so it sits on the satchel's page. No outline: the edge is where the strokes stop.
Writes one PNG per icon (256 px, transparent paper) and a sheet of all of them on paper.
"""

import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SIZE = 256
PAPER = (233, 223, 200)
VIOLET = (74, 53, 88)
CREAM = (250, 236, 200)


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def shape_mask(draw_fn):
    mask = Image.new("L", (SIZE, SIZE), 0)
    draw_fn(ImageDraw.Draw(mask))
    return mask


def paint(mask, colour, rng, flow, light=(-0.6, -0.8)):
    """Fills a mask with brush strokes. flow(x, y) gives the stroke direction there, in radians."""
    layer = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    inside = np.array(mask) > 127
    ys, xs = np.nonzero(inside)
    if len(xs) == 0:
        return layer
    cx, cy = xs.mean(), ys.mean()
    reach = max(xs.max() - xs.min(), ys.max() - ys.min()) / 2.0 + 1.0

    # Underpainting: the whole shape in its shadow tone, soft-edged.
    under = Image.new("RGBA", (SIZE, SIZE), mix(colour, VIOLET, 0.55) + (255,))
    layer.paste(under, (0, 0), mask.filter(ImageFilter.GaussianBlur(1.2)))

    # Strokes, dark to light, each pass smaller and sparser.
    for tone, count, length, width in ((0.35, 150, 26, 9), (0.0, 170, 22, 7), (-0.5, 70, 16, 5)):
        for _ in range(count):
            i = rng.integers(len(xs))
            x, y = xs[i], ys[i]
            # How much this spot faces the light: -1 away .. 1 toward.
            facing = ((x - cx) * light[0] + (y - cy) * light[1]) / reach
            if tone > 0.2 and facing > 0.1:
                continue          # shadow strokes stay on the shadow side
            if tone < -0.2 and facing < 0.25:
                continue          # lights only where the Lamp reaches
            if tone > 0.2:
                c = mix(colour, VIOLET, 0.35 + 0.3 * rng.random())
            elif tone < -0.2:
                c = mix(colour, CREAM, 0.45 + 0.35 * rng.random())
            else:
                c = mix(colour, CREAM if facing > 0 else VIOLET, 0.18 * abs(facing) + 0.08 * rng.random())
            angle = flow(x, y) + rng.normal(0.0, 0.25)
            dx, dy = math.cos(angle) * length / 2.0, math.sin(angle) * length / 2.0
            stroke = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
            ImageDraw.Draw(stroke).line((x - dx, y - dy, x + dx, y + dy), fill=c + (int(150 + 90 * rng.random()),), width=int(width * rng.uniform(0.7, 1.2)))
            # A stroke may run a little over the edge (that is the look), but not far.
            edge = mask.filter(ImageFilter.MaxFilter(5))
            layer = Image.alpha_composite(layer, Image.composite(stroke, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)), edge))
    return layer


def along(angle):
    return lambda x, y: angle


def around(cx, cy):
    return lambda x, y: math.atan2(y - cy, x - cx) + math.pi / 2.0


def lamp_oil(rng):
    """A squat stoppered flask of amber oil."""
    body = shape_mask(lambda d: d.ellipse((62, 104, 194, 224), fill=255))
    neck = shape_mask(lambda d: d.polygon([(108, 118), (148, 118), (142, 66), (114, 66)], fill=255))
    cork = shape_mask(lambda d: d.rounded_rectangle((106, 40, 150, 70), 6, fill=255))
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    for mask, colour, flow in ((body, (217, 164, 65), around(128, 164)), (neck, (226, 190, 110), along(math.pi / 2)), (cork, (107, 79, 58), along(0.0))):
        icon = Image.alpha_composite(icon, paint(mask, colour, rng, flow))
    return icon


def moss_poultice(rng):
    """A folded pad of green moss tied with a strip of hide."""
    pad = shape_mask(lambda d: d.rounded_rectangle((48, 84, 208, 196), 34, fill=255))
    tie = shape_mask(lambda d: d.polygon([(116, 76), (142, 76), (150, 204), (124, 204)], fill=255))
    knot = shape_mask(lambda d: d.ellipse((112, 118, 156, 160), fill=255))
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    for mask, colour, flow in ((pad, (107, 143, 113), along(0.25)), (tie, (216, 203, 176), along(math.pi / 2)), (knot, (196, 176, 140), around(134, 139))):
        icon = Image.alpha_composite(icon, paint(mask, colour, rng, flow))
    return icon


def sanguine(rng):
    """A measure of Sanguine pigment: a heap on a scrap of paper, a dab drawn out beside it."""
    scrap = shape_mask(lambda d: d.polygon([(40, 150), (200, 132), (222, 204), (58, 222)], fill=255))
    heap = shape_mask(lambda d: d.pieslice((70, 86, 186, 250), 180, 360, fill=255))
    dab = shape_mask(lambda d: d.ellipse((168, 176, 214, 198), fill=255))
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    for mask, colour, flow in ((scrap, (236, 228, 208), along(-0.1)), (heap, (158, 43, 37), around(128, 190)), (dab, (158, 43, 37), along(0.1))):
        icon = Image.alpha_composite(icon, paint(mask, colour, rng, flow))
    return icon


ICONS = (("T_Icon_LampOil", lamp_oil), ("T_Icon_MossPoultice", moss_poultice), ("T_Icon_Sanguine", sanguine))


def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    rng = np.random.default_rng(11)
    sheet = Image.new("RGB", (SIZE * len(ICONS) + 40 * (len(ICONS) + 1), SIZE + 80), PAPER)
    for index, (name, painter) in enumerate(ICONS):
        icon = painter(rng)
        icon.save(os.path.join(out_dir, name + ".png"))
        x = 40 + index * (SIZE + 40)
        # A soft violet shadow under each, as on the satchel's page.
        shadow = Image.new("RGBA", (SIZE, SIZE), VIOLET + (0,))
        shadow.putalpha(icon.getchannel("A").filter(ImageFilter.GaussianBlur(6)).point(lambda a: int(a * 0.28)))
        sheet.paste(shadow, (x + 6, 48), shadow)
        sheet.paste(icon, (x, 40), icon)
    sheet.save(os.path.join(out_dir, "icon_probe_sheet.png"))
    print("wrote", len(ICONS), "icons and a sheet to", out_dir)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "art_source/icons")
