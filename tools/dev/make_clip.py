"""Turns folders of frames from fightclip.py into one video, optionally two clips side by side with labels.

    python tools/dev/make_clip.py <out.mp4> <frames folder> [<second frames folder>] [--labels Before After]

Needs Pillow and imageio-ffmpeg (a free ffmpeg build installed with pip).
"""

import os
import subprocess
import sys

import imageio_ffmpeg
from PIL import Image, ImageDraw


def frames(folder):
    return [os.path.join(folder, f) for f in sorted(os.listdir(folder)) if f.endswith(".png")]


def main(args):
    labels = []
    if "--labels" in args:
        i = args.index("--labels")
        labels, args = args[i + 1:], args[:i]
    out, folders = args[0], args[1:]
    sets = [frames(f) for f in folders]
    count = min(len(s) for s in sets)
    work = out + ".frames"
    os.makedirs(work, exist_ok=True)
    for index in range(count):
        tiles = [Image.open(s[index]).convert("RGB") for s in sets]
        w, h = tiles[0].size
        sheet = Image.new("RGB", (w * len(tiles), h))
        for t, tile in enumerate(tiles):
            sheet.paste(tile.resize((w, h)), (t * w, 0))
            if t < len(labels):
                draw = ImageDraw.Draw(sheet)
                draw.rectangle((t * w + 12, 12, t * w + 150, 44), fill=(20, 12, 28))
                draw.text((t * w + 22, 20), labels[t].upper(), fill=(240, 220, 180))
        sheet.save(os.path.join(work, "f%04d.png" % index))
    subprocess.run([imageio_ffmpeg.get_ffmpeg_exe(), "-y", "-loglevel", "error", "-framerate", "15", "-i", os.path.join(work, "f%04d.png"),
                    "-vf", "pad=ceil(iw/2)*2:ceil(ih/2)*2", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "23", out], check=True)
    for f in os.listdir(work):
        os.remove(os.path.join(work, f))
    os.rmdir(work)
    print("wrote", out, count, "frames")


if __name__ == "__main__":
    main(sys.argv[1:])
