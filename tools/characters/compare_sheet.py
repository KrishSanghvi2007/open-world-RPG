"""Step 7 of /make-character: one picture that puts the drawing next to what was made from it.

    python tools/characters/compare_sheet.py <id> <out.png> [in-game shot[:x0,y0,x1,y1] ...]

Left to right, top row: the original drawing, the painted front view, the back view. Middle: the turnaround from Blender.
Bottom: screenshots from the game, each cropped to the fractions given after the colon (0..1 of the picture).
Needs Pillow.
"""

import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PAPER = (245, 240, 228)
INK = (40, 32, 28)


def fit(image, height):
    return image.resize((max(1, round(image.width * height / image.height)), height), Image.LANCZOS)


def on_paper(image):
    if image.mode != "RGBA":
        return image.convert("RGB")
    flat = Image.new("RGB", image.size, PAPER)
    flat.paste(image, mask=image.split()[3])
    return flat


def row(images, labels, height, width):
    tiles = [fit(on_paper(image), height) for image in images]
    total = sum(t.width for t in tiles) + 24 * (len(tiles) - 1)
    if total > width:                                    # shrink the whole row to fit
        height = max(1, int(height * width / total))
        tiles = [fit(on_paper(image), height) for image in images]
        total = sum(t.width for t in tiles) + 24 * (len(tiles) - 1)
    strip = Image.new("RGB", (width, height + 34), PAPER)
    draw = ImageDraw.Draw(strip)
    x = (width - total) // 2
    for tile, label in zip(tiles, labels):
        strip.paste(tile, (x, 0))
        draw.text((x + 4, height + 8), label, fill=INK)
        x += tile.width + 24
    return strip


def main(args):
    char_id, out = args[0], args[1]
    folder = os.path.join(ROOT, "art_source", "characters", char_id)
    width = 2000
    rows = [row([Image.open(os.path.join(folder, "source", "original.jpg")), Image.open(os.path.join(folder, "front.png")), Image.open(os.path.join(folder, "back.png"))],
                ["The drawing (Krish & Hannah Sanghvi)", "Front view, painted in the Glaze look by script", "Back view, made the same way"], 760, width)]
    turnaround = os.path.join(folder, "turnaround.png")
    if os.path.exists(turnaround):
        rows.append(row([Image.open(turnaround)], ["The model, with its ink edge: front, three-quarter, side, back"], 480, width))
    shots, labels = [], []
    for spec in args[2:]:
        path, _, rest = spec.partition(":")
        crop, _, label = rest.partition(":")
        image = Image.open(path).convert("RGB")
        if crop:
            x0, y0, x1, y1 = [float(n) for n in crop.split(",")]
            image = image.crop((int(x0 * image.width), int(y0 * image.height), int(x1 * image.width), int(y1 * image.height)))
        shots.append(image)
        labels.append(label or "In the game")
    if shots:
        rows.append(row(shots, labels, 560, width))
    sheet = Image.new("RGB", (width + 80, sum(r.height for r in rows) + 40 * (len(rows) + 1)), PAPER)
    y = 40
    for strip in rows:
        sheet.paste(strip, (40, y))
        y += strip.height + 40
    sheet.save(out)
    print("wrote", out, sheet.size)


if __name__ == "__main__":
    main(sys.argv[1:])
