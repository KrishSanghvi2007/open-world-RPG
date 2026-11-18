"""Step 1 of /make-character: the front view of a character, painted in the Glaze look, from the owners' drawing.

    python tools/characters/paint_front.py <id> [--drawings ../exquisite-corpse]

Reads   <drawings>/drawings/redrawn/<id>/redrawn.png      the clean ink redraw (black on white)
        <drawings>/tools/film/cast.json                   where each part of the figure is on the sheet
        tools/characters/plans/<id>.json                  which parts to keep and what colour each is
Writes  art_source/characters/<id>/front.png              the painted figure, transparent outside it
        art_source/characters/<id>/front_white.png        the same on white, for the image-to-3D service
        art_source/characters/<id>/front_ink.png          the ink alone (to draw the lines back onto the model)
        art_source/characters/<id>/source/                the drawings used, copied in (and nothing else)

How: the ink is kept exactly as drawn. Everything the ink encloses is the figure. Each part's box says which colour
that bit of the figure gets. Then light from the upper left, a little brush grain, and a darkening toward the lines,
which is what makes the flat colour read as glazed paint and gives the 3D service some volume to work from.

Needs numpy, scipy and Pillow (free; `pip install numpy scipy pillow`).
"""

import json
import os
import shutil
import sys

import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def rgb(hex_colour):
    h = hex_colour.lstrip("#")
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], dtype=np.float32)


def box_mask(shape, box, pad=0.0):
    h, w = shape
    x0, y0, x1, y1 = box
    mask = np.zeros(shape, dtype=bool)
    mask[max(0, int((y0 - pad) * h)):min(h, int((y1 + pad) * h) + 1), max(0, int((x0 - pad) * w)):min(w, int((x1 + pad) * w) + 1)] = True
    return mask


def ellipse_mask(shape, box):
    h, w = shape
    x0, y0, x1, y1 = box
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy, rx, ry = (x0 + x1) / 2 * w, (y0 + y1) / 2 * h, (x1 - x0) / 2 * w, (y1 - y0) / 2 * h
    return ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1.0


def main(args):
    char_id = args[0]
    drawings = args[args.index("--drawings") + 1] if "--drawings" in args else os.path.join(ROOT, "..", "exquisite-corpse")
    plan = json.load(open(os.path.join(ROOT, "tools", "characters", "plans", char_id + ".json")))
    cast = {c["id"]: c for c in json.load(open(os.path.join(drawings, "tools", "film", "cast.json")))}[char_id]
    parts = {p["name"]: p for p in cast["parts"]}
    redrawn = os.path.join(drawings, "drawings", "redrawn", char_id, "redrawn.png")
    sheet = np.array(Image.open(redrawn).convert("L"))
    shape = sheet.shape
    ink = sheet < 128

    # 1. Only the parts the plan keeps.
    keep = np.zeros(shape, dtype=bool)
    for name in plan["keep"]:
        keep |= box_mask(shape, parts[name]["bbox"], plan.get("pad", 0.004))
    for box in plan.get("erase", []):
        keep &= ~box_mask(shape, box)
    ink &= keep

    # 2. Thin strokes that are not the figure (web strands, a thread) go, where the plan asks: what survives being
    #    worn away by `open` pixels is kept, with whatever touches it within `regrow` pixels (the hairs on a leg).
    if plan.get("open", 0) > 0:
        solid = ndimage.binary_opening(ink, structure=np.ones((3, 3)), iterations=plan["open"])
        grown = ndimage.binary_dilation(solid, iterations=plan.get("regrow", 0), mask=ink) if plan.get("regrow", 0) else solid
        protect = np.zeros(shape, dtype=bool)
        for name in plan.get("open_protect", []):
            protect |= box_mask(shape, parts[name]["bbox"])
        ink = (ink & protect) | grown

    # 3. The figure is the ink and everything it encloses. Small gaps in the line are closed first.
    closed = ndimage.binary_closing(ink, structure=np.ones((3, 3)), iterations=plan.get("close", 4))
    figure = ndimage.binary_fill_holes(closed | ink)
    for box in plan.get("fill_boxes", []):          # a part whose outline is open (a tank the pipes run into)
        figure |= box_mask(shape, box)
    for box in plan.get("hole_boxes", []):          # a gap the lines happen to enclose (the space between two legs)
        figure &= ~box_mask(shape, box) | ink
    # Loose specks (drips, splash marks) that are not part of anything go.
    labels, count = ndimage.label(figure)
    sizes = ndimage.sum(figure, labels, range(1, count + 1))
    small = np.isin(labels, [i + 1 for i, size in enumerate(sizes) if size < plan.get("min_piece", 900)])
    figure &= ~small
    ink &= figure

    # 4. Colour by part, then by the plan's extra regions (hair, a face, a button).
    colour = np.zeros(shape + (3,), dtype=np.float32)
    colour[:] = rgb(plan.get("default", "#F2E3C0"))
    for name in plan["keep"]:
        if name in plan["colours"]:
            colour[box_mask(shape, parts[name]["bbox"], plan.get("pad", 0.004)) & figure] = rgb(plan["colours"][name])
    for region in plan.get("regions", []):
        mask = ellipse_mask(shape, region["box"]) if region.get("ellipse") else box_mask(shape, region["box"])
        if "minus_ellipse" in region:
            mask &= ~ellipse_mask(shape, region["minus_ellipse"])
        colour[mask & figure] = rgb(region["colour"])

    # 5. The Glaze look: warm light from the upper left, shade pooling toward the lines, a little brush grain.
    h, w = shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    ys, xs = np.nonzero(figure)
    y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
    light = 1.0 - 0.30 * (0.55 * (xx - x0) / max(1, x1 - x0) + 0.45 * (yy - y0) / max(1, y1 - y0))
    depth = ndimage.distance_transform_edt(figure & ~ink)
    round_off = 0.72 + 0.28 * np.clip(depth / plan.get("round", 26.0), 0.0, 1.0) ** 0.6          # darker at edges and lines
    rng = np.random.default_rng(7)
    grain = ndimage.gaussian_filter(rng.standard_normal(shape).astype(np.float32), (2.0, 9.0))  # long soft strokes
    grain = 1.0 + 0.06 * grain / max(1e-6, float(np.abs(grain).max()))
    shaded = colour * (light * round_off * grain)[..., None]
    warm = np.array([1.03, 1.0, 0.94], dtype=np.float32)                                         # the Lamp is low and amber
    shaded = np.clip(shaded * warm, 0, 255)

    # 6. Ink on top, exactly as drawn. Black parts (a spider's legs) are simply ink.
    ink_soft = np.array(Image.fromarray((ink * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.6)), dtype=np.float32) / 255.0
    ink_colour = rgb(plan.get("ink", "#1B1512"))
    out = shaded * (1.0 - ink_soft[..., None]) + ink_colour * ink_soft[..., None]
    alpha = np.array(Image.fromarray((figure * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8)), dtype=np.float32)

    # Crop to the figure with a margin, square it (the 3D service wants the subject centred).
    margin = int(0.06 * max(y1 - y0, x1 - x0))
    cy0, cy1, cx0, cx1 = max(0, y0 - margin), min(h, y1 + margin), max(0, x0 - margin), min(w, x1 + margin)
    rgba = np.dstack([out, alpha]).astype(np.uint8)[cy0:cy1, cx0:cx1]
    ink_rgba = np.dstack([np.zeros(shape + (3,), dtype=np.float32) + ink_colour, ink_soft * 255]).astype(np.uint8)[cy0:cy1, cx0:cx1]
    side = max(rgba.shape[0], rgba.shape[1])

    def square(image_array):
        canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
        tile = Image.fromarray(image_array)
        canvas.paste(tile, ((side - tile.width) // 2, (side - tile.height) // 2))
        return canvas.resize((1024, 1024), Image.LANCZOS)

    out_dir = os.path.join(ROOT, "art_source", "characters", char_id)
    os.makedirs(os.path.join(out_dir, "source"), exist_ok=True)
    front = square(rgba)
    front.save(os.path.join(out_dir, "front.png"))
    white = Image.new("RGB", front.size, "white")
    white.paste(front, mask=front.split()[3])
    white.save(os.path.join(out_dir, "front_white.png"))
    square(ink_rgba).save(os.path.join(out_dir, "front_ink.png"))

    # Where the crop sits on the sheet, so the part boxes can be carried onto the model later.
    json.dump({"id": char_id, "sheet_px": [w, h], "crop_px": [int(cx0), int(cy0), int(cx1), int(cy1)], "square_px": int(side),
               "figure_px": [int(x0), int(y0), int(x1), int(y1)]}, open(os.path.join(out_dir, "front.json"), "w"), indent=2)

    # The drawings used, and only those.
    shutil.copy(redrawn, os.path.join(out_dir, "source", "redrawn.png"))
    original = os.path.join(drawings, "drawings", "originals", char_id + ".jpg")
    if os.path.exists(original):
        small = Image.open(original).convert("RGB")
        small.thumbnail((1400, 1400))
        small.save(os.path.join(out_dir, "source", "original.jpg"), quality=88)
    painted = os.path.join(drawings, "drawings", "painted", char_id, plan.get("painted", "painted.jpg"))
    if os.path.exists(painted):
        small = Image.open(painted).convert("RGB")
        small.thumbnail((1400, 1400))
        small.save(os.path.join(out_dir, "source", "painted.jpg"), quality=88)
    print("wrote", out_dir, "figure covers %d x %d px of the sheet" % (x1 - x0, y1 - y0))


if __name__ == "__main__":
    main(sys.argv[1:])
