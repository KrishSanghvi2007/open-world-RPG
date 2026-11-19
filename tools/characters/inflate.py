"""Step 2b of /make-character: give a flat drawing thickness without any online service.

    python tools/characters/inflate.py <id> [--grid 512]

Some drawings come back from the image-to-3D service as a flat card (line art with little shading does that).
This is the way round it, and it needs nothing but this Mac. The painted front view's outline is blown up like a
balloon: thin in the pipes and fingers, thick in the middle of a coat, the same on the back as on the front.
The plan file can say which parts should be rounder ("depth": {"head": 1.6}).

Reads   art_source/characters/<id>/front.png, front.json, the plan, and the cast file's part boxes
Writes  art_source/characters/<id>/raw/inflate.obj   (x across, y depth, z up, about 1 unit tall; front faces -y)

Needs numpy, scipy and Pillow.
"""

import json
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def main(args):
    char_id = args[0]
    grid = int(args[args.index("--grid") + 1]) if "--grid" in args else 512
    drawings = args[args.index("--drawings") + 1] if "--drawings" in args else os.path.join(ROOT, "..", "exquisite-corpse")
    folder = os.path.join(ROOT, "art_source", "characters", char_id)
    plan = json.load(open(os.path.join(ROOT, "tools", "characters", "plans", char_id + ".json")))
    front = json.load(open(os.path.join(folder, "front.json")))
    cast = {c["id"]: c for c in json.load(open(os.path.join(drawings, "tools", "film", "cast.json")))}[char_id]

    alpha = np.array(Image.open(os.path.join(folder, "front.png")).split()[3].resize((grid, grid), Image.LANCZOS)) > 127
    alpha = ndimage.binary_opening(alpha, iterations=1) | alpha        # keep hairlines; drop nothing
    inside = ndimage.binary_fill_holes(alpha) & alpha
    distance = ndimage.distance_transform_edt(inside)

    # How round each part is. The boxes are in sheet fractions; front.json says where the square crop sits on the sheet.
    depth = np.full((grid, grid), float(plan.get("depth", {}).get("default", 1.0)), dtype=np.float32)
    sheet_w, sheet_h = front["sheet_px"]
    cx0, cy0, cx1, cy1 = front["crop_px"]
    side = front["square_px"]
    off_x, off_y = (side - (cx1 - cx0)) / 2.0, (side - (cy1 - cy0)) / 2.0

    def to_grid(fx, fy):
        return ((fx * sheet_w - cx0 + off_x) / side * grid, (fy * sheet_h - cy0 + off_y) / side * grid)

    for part in cast["parts"]:
        if part["name"] in plan.get("depth", {}):
            (gx0, gy0), (gx1, gy1) = to_grid(part["bbox"][0], part["bbox"][1]), to_grid(part["bbox"][2], part["bbox"][3])
            depth[max(0, int(gy0)):int(gy1) + 1, max(0, int(gx0)):int(gx1) + 1] = plan["depth"][part["name"]]
    depth = ndimage.gaussian_filter(depth, grid / 90.0)             # no steps where two parts meet

    # Height of the balloon over each cell: fat tubes stay round, wide areas do not become spheres.
    height = 2.4 * np.power(distance, 0.62) * depth
    height[~inside] = 0.0
    height = ndimage.gaussian_filter(height, 2.2)          # take the ridge off the middle: rounder, less like a folded card
    height[~inside] = 0.0

    # Corners of the cells are the vertices. A corner on the outline has height 0 and is shared by front and back.
    pad_inside = np.pad(inside, 1)
    pad_height = np.pad(height, 1)
    corner_inside = pad_inside[:-1, :-1] & pad_inside[:-1, 1:] & pad_inside[1:, :-1] & pad_inside[1:, 1:]     # all four cells in
    corner_any = pad_inside[:-1, :-1] | pad_inside[:-1, 1:] | pad_inside[1:, :-1] | pad_inside[1:, 1:]
    corner_height = (pad_height[:-1, :-1] + pad_height[:-1, 1:] + pad_height[1:, :-1] + pad_height[1:, 1:]) / 4.0
    corner_height[~corner_inside] = 0.0
    corner_height = ndimage.gaussian_filter(corner_height, 0.8) * corner_inside

    front_index = np.full(corner_any.shape, -1, dtype=np.int64)
    back_index = np.full(corner_any.shape, -1, dtype=np.int64)
    vertices = []
    for cy, cx in zip(*np.nonzero(corner_any)):
        x, z, h = cx / grid - 0.5, 0.5 - cy / grid, corner_height[cy, cx] / grid
        front_index[cy, cx] = len(vertices)
        vertices.append((x, -h, z))
        if corner_inside[cy, cx] and h > 0:
            back_index[cy, cx] = len(vertices)
            vertices.append((x, h, z))
        else:
            back_index[cy, cx] = front_index[cy, cx]

    faces = []
    for y, x in zip(*np.nonzero(inside)):
        a, b, c, d = (y, x), (y, x + 1), (y + 1, x + 1), (y + 1, x)
        faces.append((front_index[a], front_index[d], front_index[c], front_index[b]))        # front: faces -y
        back = (back_index[a], back_index[b], back_index[c], back_index[d])
        if len(set(back)) >= 3 and back != (front_index[a], front_index[b], front_index[c], front_index[d]):
            faces.append(back)

    os.makedirs(os.path.join(folder, "raw"), exist_ok=True)
    out = os.path.join(folder, "raw", "inflate.obj")
    with open(out, "w") as handle:
        handle.write("# inflated from front.png by tools/characters/inflate.py\n")
        for v in vertices:
            handle.write("v %.5f %.5f %.5f\n" % v)
        for f in faces:
            ids = [int(i) + 1 for i in dict.fromkeys(f)]
            if len(ids) >= 3:
                handle.write("f " + " ".join(str(i) for i in ids) + "\n")
    print("wrote", out, len(vertices), "vertices", len(faces), "faces; thickest point %.3f of the height" % (2 * corner_height.max() / grid))


if __name__ == "__main__":
    main(sys.argv[1:])
