"""Puts a "before" picture beside an "after" one, labelled, for the devlogs.

    python tools/kit/compare.py <before.png> <after.png> <out.png> [--stack]

Needs Pillow. --stack puts them one above the other instead of side by side.
"""
import sys

from PIL import Image, ImageDraw, ImageFont


def label(image, text):
    draw = ImageDraw.Draw(image, "RGBA")
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Georgia.ttf", 30)
    except OSError:
        font = ImageFont.load_default()
    width = draw.textlength(text, font=font)
    draw.rectangle((18, 16, 18 + width + 28, 62), fill=(42, 31, 46, 190))
    draw.text((32, 22), text, font=font, fill=(242, 227, 192, 255))
    return image


def main(args):
    stack = "--stack" in args
    before, after, out = [a for a in args if not a.startswith("--")][:3]
    a, b = Image.open(before).convert("RGB"), Image.open(after).convert("RGB")
    width = 1100
    a = a.resize((width, int(a.height * width / a.width)))
    b = b.resize((width, int(b.height * width / b.width)))
    label(a, "before")
    label(b, "after")
    gap = 10
    if stack:
        sheet = Image.new("RGB", (width, a.height + b.height + gap), (42, 31, 46))
        sheet.paste(a, (0, 0))
        sheet.paste(b, (0, a.height + gap))
    else:
        sheet = Image.new("RGB", (width * 2 + gap, max(a.height, b.height)), (42, 31, 46))
        sheet.paste(a, (0, 0))
        sheet.paste(b, (width + gap, 0))
    sheet.save(out)
    print("wrote", out, sheet.size)


if __name__ == "__main__":
    main(sys.argv[1:])
