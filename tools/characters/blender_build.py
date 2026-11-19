"""Step 3 of /make-character, run inside Blender: clean the rough shape, paint it, give it an ink edge, and rig it.

Run from Blender's Python (the Blender MCP does this):

    bpy.ops.wm.read_homefile(use_empty=True)          (first, on its own)
    CHAR_ID = "d28"; exec(open("<repo>/tools/characters/blender_build.py").read())

Reads   art_source/characters/<id>/raw/<shape>.glb or inflate.obj   (the plan's "shape" says which)
        art_source/characters/<id>/front.png, back.png, front.json
        tools/characters/plans/<id>.json, and the cast file's part boxes and pivots
Makes   one mesh, <id>_Body, scaled in metres, with the drawing on its front and back, a black shell round it
        (the ink edge), and an armature <id>_Rig with one bone per drawn part, placed at the pivots in the cast file.

What it does, in order:
 1. Finds which way the rough shape faces by laying its outline over the painted front view (eight ways; best wins).
 2. Turns it to stand (or lie, for a creature drawn from above), scales it, simplifies it.
 3. Maps the painting onto it straight through, front and back: the lines stay exactly where they were drawn.
 4. Puts a bone at each part's pivot, and gives every point of the surface to the nearest part.
 5. Adds the ink edge: the same surface, pushed out a little, turned inside out, black.
Animations are added afterwards by tools/characters/anims/<id>.py, and the export by blender_export.py.
"""

import json
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

REPO = "/Users/user/Documents/GitHub/open-world-RPG"
DRAWINGS = os.path.join(REPO, "..", "exquisite-corpse")
char_id = CHAR_ID            # noqa: F821  (set by the caller)
folder = os.path.join(REPO, "art_source", "characters", char_id)
plan = json.load(open(os.path.join(REPO, "tools", "characters", "plans", char_id + ".json")))
front = json.load(open(os.path.join(folder, "front.json")))
cast = {c["id"]: c for c in json.load(open(os.path.join(DRAWINGS, "tools", "film", "cast.json")))}[char_id]
parts = [p for p in cast["parts"] if p["name"] in plan["keep"]]

# ---------------------------------------------------------------- a clean file
# The caller empties the file first, in its own call (Blender forgets its context if that is done in the same breath):
#     bpy.ops.wm.read_homefile(use_empty=True)
for stale in list(bpy.data.objects):
    bpy.data.objects.remove(stale)

# ---------------------------------------------------------------- 0. the rough shape
shape = plan.get("shape", "hunyuan")
if shape == "inflate":
    verts, faces = [], []
    for line in open(os.path.join(folder, "raw", "inflate.obj")):
        if line.startswith("v "):
            verts.append(tuple(float(n) for n in line.split()[1:4]))
        elif line.startswith("f "):
            faces.append(tuple(int(n) - 1 for n in line.split()[1:]))
    mesh = bpy.data.meshes.new(char_id + "_Body")
    mesh.from_pydata(verts, [], faces)
    body = bpy.data.objects.new(char_id + "_Body", mesh)
    bpy.context.collection.objects.link(body)
else:
    bpy.ops.import_scene.gltf(filepath=os.path.join(folder, "raw", shape + ".glb"))
    body = next(o for o in bpy.data.objects if o.type == "MESH")
    world = body.matrix_world.copy()
    body.parent = None
    body.matrix_world = world
    for other in [o for o in bpy.data.objects if o is not body]:
        bpy.data.objects.remove(other)
    body.name = char_id + "_Body"
    body.data.name = char_id + "_Body"
bpy.context.view_layer.objects.active = body
body.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

bm = bmesh.new()
bm.from_mesh(body.data)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
# Loose crumbs go: anything smaller than 1.5% of the whole.
seen, islands = set(), []
for start in bm.verts:
    if start.index in seen:
        continue
    island, stack = [], [start]
    while stack:
        v = stack.pop()
        if v.index in seen:
            continue
        seen.add(v.index)
        island.append(v)
        stack.extend(e.other_vert(v) for e in v.link_edges)
    islands.append(island)
limit = plan.get("min_island", 0.015) * len(bm.verts)
crumbs = [v for island in islands if len(island) < limit for v in island]
bmesh.ops.delete(bm, geom=crumbs, context="VERTS")
bm.to_mesh(body.data)
bm.free()
print("rough shape:", len(body.data.vertices), "vertices,", len(islands), "pieces, kept", sum(1 for i in islands if len(i) >= limit))

# ---------------------------------------------------------------- 1. which way does it face?
image = bpy.data.images.load(os.path.join(folder, "front.png"))
side_px = image.size[0]
pixels = np.array(image.pixels[:], dtype=np.float32).reshape(side_px, side_px, 4)       # rows from the bottom up
alpha = pixels[:, :, 3] > 0.5
rows, cols = np.nonzero(alpha)
fig_u0, fig_u1, fig_v0, fig_v1 = cols.min() / side_px, (cols.max() + 1) / side_px, rows.min() / side_px, (rows.max() + 1) / side_px

GRID = 128


def occupancy(u, v):
    """A GRID x GRID picture of where points fall; (u, v) are 0..1 with the SAME scale both ways (no stretching)."""
    grid = np.zeros((GRID, GRID), dtype=bool)
    grid[np.clip((v * (GRID - 1)).astype(int), 0, GRID - 1), np.clip((u * (GRID - 1)).astype(int), 0, GRID - 1)] = True
    wide = grid.copy()                                   # thicken by one cell so sparse points join up
    wide[1:, :] |= grid[:-1, :]; wide[:-1, :] |= grid[1:, :]; wide[:, 1:] |= grid[:, :-1]; wide[:, :-1] |= grid[:, 1:]
    return wide


def framed(a, b):
    """Centre a cloud of points on its bounds and scale it by its longer side, so its proportions are kept."""
    size = max(a.max() - a.min(), b.max() - b.min())
    return 0.5 + (a - (a.max() + a.min()) / 2.0) / size, 0.5 + (b - (b.max() + b.min()) / 2.0) / size, size


target_u, target_v, fig_size = framed(cols / side_px, rows / side_px)
target = occupancy(target_u, target_v)
co = np.array([v.co[:] for v in body.data.vertices], dtype=np.float64)
extent = co.max(axis=0) - co.min(axis=0)
normal_axis = int(np.argmin(extent))
a_axis, b_axis = [(1, 2), (2, 0), (0, 1)][normal_axis]           # (a, b, normal) is right-handed
best = None
for swap in (False, True):
    for sign_u in (1, -1):
        for sign_v in (1, -1):
            pu = co[:, b_axis if swap else a_axis] * sign_u
            pv = co[:, a_axis if swap else b_axis] * sign_v
            got_u, got_v, _ = framed(pu, pv)
            got = occupancy(got_u, got_v)
            score = (got & target).sum() / max(1, (got | target).sum())
            if best is None or score > best[0]:
                best = (score, swap, sign_u, sign_v)
score, swap, sign_u, sign_v = best
handed = (-1 if swap else 1) * sign_u * sign_v                  # +1: the drawing is what you see from the +normal side
print("faces: thin axis %s, swap %s, signs %d %d, seen from the %s side" % ("XYZ"[normal_axis], swap, sign_u, sign_v, "+" if handed > 0 else "-"))
print("MATCH %.2f  (the share of the drawing's outline and the shape's outline that agree; 1.00 is identical)" % score)
MATCH = score
if score < plan.get("min_match", 0.6):
    raise RuntimeError("REJECTED: this shape does not match the drawing (%.2f). The service has re-imagined the figure. "
                       "Use the inflate step instead: set \"shape\": \"inflate\" in the plan." % score)

pu = co[:, b_axis if swap else a_axis] * sign_u
pv = co[:, a_axis if swap else b_axis] * sign_v
_, _, mesh_size = framed(pu, pv)
per_unit = mesh_size / fig_size                                                     # model units per painted square
u = (fig_u0 + fig_u1) / 2.0 + (pu - (pu.max() + pu.min()) / 2.0) / per_unit         # 0..1 across the painted square
v = (fig_v0 + fig_v1) / 2.0 + (pv - (pv.max() + pv.min()) / 2.0) / per_unit         # 0..1 up it
w = (co[:, normal_axis] - (co[:, normal_axis].max() + co[:, normal_axis].min()) / 2.0) * handed / per_unit   # depth toward the viewer

# ---------------------------------------------------------------- 2. stand it up (or lay it down), in metres
lying = plan.get("pose", "upright") == "lying"
size_m = plan.get("size_m", 1.8)
scale = size_m / (max(fig_u1 - fig_u0, fig_v1 - fig_v0) if lying else (fig_v1 - fig_v0))


def to_sheet(fx, fy):
    """A point given as a fraction of the drawing sheet (as the cast file gives them) -> (u, v) on the painted square."""
    sheet_w, sheet_h = front["sheet_px"]
    cx0, cy0, cx1, cy1 = front["crop_px"]
    side = front["square_px"]
    return ((fx * sheet_w - cx0 + (side - (cx1 - cx0)) / 2.0) / side, 1.0 - (fy * sheet_h - cy0 + (side - (cy1 - cy0)) / 2.0) / side)


root_part = next((p for p in parts if p["name"] == plan.get("root_part")), parts[0])
root_u, root_v = to_sheet((root_part["bbox"][0] + root_part["bbox"][2]) / 2.0, (root_part["bbox"][1] + root_part["bbox"][3]) / 2.0)


def place(pu_, pv_, pw_):
    """(u, v, depth) on the painted square -> metres in the model. Upright: x right, z up, front toward -y.
    Lying (a creature drawn from above): the drawing lies on the ground, its top toward +y, its face toward +z."""
    if lying:
        flip = -1.0 if plan.get("head_dir", "down") == "up" else 1.0     # head toward -y either way
        return ((pu_ - root_u) * scale * flip, (pv_ - root_v) * scale * flip, pw_ * scale)
    return ((pu_ - root_u) * scale, -pw_ * scale, (pv_ - fig_v0) * scale)


for index, vert in enumerate(body.data.vertices):
    vert.co = place(u[index], v[index], w[index])
if lying:
    lowest = min(vert.co.z for vert in body.data.vertices)
    for vert in body.data.vertices:
        vert.co.z -= lowest
body.data.update()

# Simplify. The drawing is mapped on afterwards, so nothing is lost by doing it now.
target_tris = plan.get("tris", 9000)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.quads_convert_to_tris()
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.object.mode_set(mode="OBJECT")
current = len(body.data.polygons)
if current > target_tris:
    modifier = body.modifiers.new("Simplify", "DECIMATE")
    modifier.ratio = target_tris / current
    bpy.ops.object.modifier_apply(modifier=modifier.name)
for poly in body.data.polygons:
    poly.use_smooth = True
print("simplified:", current, "->", len(body.data.polygons), "triangles; size", [round(d, 2) for d in body.dimensions])


def to_uvw(point):
    """Model metres -> (u, v, depth) on the painted square: the inverse of place()."""
    if lying:
        flip = -1.0 if plan.get("head_dir", "down") == "up" else 1.0
        return (point.x / scale / flip + root_u, point.y / scale / flip + root_v, point.z / scale)
    return (point.x / scale + root_u, point.z / scale + fig_v0, -point.y / scale)


# ---------------------------------------------------------------- 3. the drawing, straight through, front and back
# One picture twice as wide: the front view on the left half, the back view on the right. Outside the figure the
# picture is ink-black, not white, so the very edge of the surface never picks up a pale fringe.
ink_rgb = np.array([0.03, 0.025, 0.02], dtype=np.float32)


def padded(path):
    px = np.array(bpy.data.images.load(path).pixels[:], dtype=np.float32).reshape(side_px, side_px, 4)
    solid = px[:, :, 3:4] > 0.6
    return np.concatenate([np.where(solid, px[:, :, :3], ink_rgb), np.ones((side_px, side_px, 1), dtype=np.float32)], axis=2)


front_px = padded(os.path.join(folder, "front.png"))
back_path = os.path.join(folder, "back.png")
back_px = padded(back_path) if os.path.exists(back_path) else front_px
atlas = bpy.data.images.new(char_id + "_Paint", side_px * 2, side_px, alpha=False)
atlas.pixels = np.concatenate([front_px, back_px], axis=1).ravel()
atlas.filepath_raw = os.path.join(folder, char_id + "_paint.png")
atlas.file_format = "PNG"
atlas.save()

uv_layer = body.data.uv_layers.new(name="UVMap") if not body.data.uv_layers else body.data.uv_layers[0]
toward_viewer = Vector((0, 0, 1)) if lying else Vector((0, -1, 0))
for poly in body.data.polygons:
    is_front = poly.normal.dot(toward_viewer) >= 0.0
    for loop_index in poly.loop_indices:
        pu_, pv_, _ = to_uvw(body.data.vertices[body.data.loops[loop_index].vertex_index].co)
        uv_layer.data[loop_index].uv = ((pu_ if is_front else 1.0 + pu_) * 0.5, pv_)


def flat_material(name, texture=None, colour=(0.02, 0.016, 0.014, 1.0)):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    shader = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    shader.inputs["Roughness"].default_value = 1.0
    if texture:
        node = nodes.new("ShaderNodeTexImage")
        node.image = texture
        links.new(node.outputs["Color"], shader.inputs["Base Color"])
        links.new(node.outputs["Color"], shader.inputs["Emission Color"])
        shader.inputs["Emission Strength"].default_value = 0.6          # ink holds in the dark: they do not go black in shade
    else:
        shader.inputs["Base Color"].default_value = colour
    material.use_backface_culling = True
    return material


body.data.materials.clear()
body.data.materials.append(flat_material("M_%s_Paint" % char_id, atlas))
body.data.materials.append(flat_material("M_%s_Ink" % char_id))

# ---------------------------------------------------------------- 4. bones at the pivots, and who owns each point
def distance_to_segment(px, py, a, b):
    ax, ay, bx, by = a[0], a[1], b[0], b[1]
    length2 = (bx - ax) ** 2 + (by - ay) ** 2
    t = 0.0 if length2 == 0 else max(0.0, min(1.0, ((px - ax) * (bx - ax) + (py - ay) * (by - ay)) / length2))
    return math.hypot(px - (ax + t * (bx - ax)), py - (ay + t * (by - ay)))


# Each part is a line from its pivot to its far end, plus the box the cast file gives it.
segments = {}
for part in parts:
    x0, y0, x1, y1 = part["bbox"]
    pivot = to_sheet(*part["pivot"])
    centre = to_sheet((x0 + x1) / 2.0, (y0 + y1) / 2.0)
    tail = to_sheet(*plan["tails"][part["name"]]) if part["name"] in plan.get("tails", {}) else (pivot[0] + 2.0 * (centre[0] - pivot[0]), pivot[1] + 2.0 * (centre[1] - pivot[1]))
    segments[part["name"]] = [pivot, tail, (to_sheet(x0, y1), to_sheet(x1, y0))]

points_uv = [to_uvw(vert.co)[:2] for vert in body.data.vertices]


def assign():
    owners = []
    for pu_, pv_ in points_uv:
        best_name, best_cost = None, 1e9
        for name, (pivot, tail, ((bx0, by0), (bx1, by1))) in segments.items():
            outside = math.hypot(max(bx0 - pu_, 0.0, pu_ - bx1), max(by0 - pv_, 0.0, pv_ - by1))     # 0 inside the part's box
            cost = outside + plan.get("line_weight", 1.0) * distance_to_segment(pu_, pv_, pivot, tail) + plan.get("bias", {}).get(name, 0.0)
            if cost < best_cost:
                best_name, best_cost = name, cost
        owners.append(best_name)
    return owners


owners = assign()
# Now that each part knows its points, aim its line properly: at the next joint if it has exactly one part hanging
# from it (a thigh ends at the knee), otherwise at its own farthest point (a shin ends at the foot).
for part in parts:
    name = part["name"]
    if name in plan.get("tails", {}):
        continue
    children = [c for c in parts if c.get("parent") == name]
    pivot = segments[name][0]
    (bx0, by0), (bx1, by1) = segments[name][2]
    mine = [points_uv[i] for i, owner in enumerate(owners) if owner == name and bx0 <= points_uv[i][0] <= bx1 and by0 <= points_uv[i][1] <= by1]
    if len(children) == 1 and name not in plan.get("no_chain", []):
        segments[name][1] = to_sheet(*children[0]["pivot"])
    elif len(mine) >= 5:
        segments[name][1] = max(mine, key=lambda p: math.hypot(p[0] - pivot[0], p[1] - pivot[1]))
    if math.hypot(segments[name][1][0] - pivot[0], segments[name][1][1] - pivot[1]) < 0.02:
        segments[name][1] = (pivot[0], pivot[1] + 0.06)
owners = assign()

# Tidy: a point that disagrees with nearly all its neighbours joins them (stray points make spikes when a limb bends).
neighbours = [[] for _ in body.data.vertices]
for edge in body.data.edges:
    a_index, b_index = edge.vertices
    neighbours[a_index].append(b_index)
    neighbours[b_index].append(a_index)
for _ in range(3):
    changed = 0
    for index, near in enumerate(neighbours):
        if not near:
            continue
        votes = {}
        for other in near:
            votes[owners[other]] = votes.get(owners[other], 0) + 1
        top = max(votes, key=votes.get)
        if top != owners[index] and votes.get(owners[index], 0) <= 0.3 * len(near):
            owners[index] = top
            changed += 1
    if not changed:
        break

# Parts the plan calls floating (hands that hang in the air beside the body) are separate pieces of the surface.
# Such a part owns the whole of its piece and nothing else: otherwise the body's edge follows the hand and tears.
floating = plan.get("floating", [])
if floating:
    island_of, island_count = [-1] * len(neighbours), 0
    for start in range(len(neighbours)):
        if island_of[start] >= 0:
            continue
        stack = [start]
        while stack:
            index = stack.pop()
            if island_of[index] >= 0:
                continue
            island_of[index] = island_count
            stack.extend(neighbours[index])
        island_count += 1
    island_size = [0] * island_count
    for island in island_of:
        island_size[island] += 1
    for name in floating:
        share = {}
        for index, owner in enumerate(owners):
            if owner == name:
                share[island_of[index]] = share.get(island_of[index], 0) + 1
        if not share:
            continue
        home = max(share, key=lambda island: share[island] / island_size[island])
        for index in range(len(owners)):
            if island_of[index] == home:
                owners[index] = name
            elif owners[index] == name:                      # a point of the body that the hand's box had claimed
                pu_, pv_ = points_uv[index]
                owners[index] = min((n for n in segments if n not in floating),
                                    key=lambda n: distance_to_segment(pu_, pv_, segments[n][0], segments[n][1]))

# Parts the plan calls ink (a spider's solid black legs) are simply black, whatever the picture says at that spot.
ink_parts = set(plan.get("ink_parts", []))
if ink_parts:
    for poly in body.data.polygons:
        if all(owners[v] in ink_parts for v in poly.vertices):
            poly.material_index = 1

rig_data = bpy.data.armatures.new(char_id + "_Rig")
rig = bpy.data.objects.new(char_id + "_Rig", rig_data)
bpy.context.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
depths = [to_uvw(vert.co)[2] for vert in body.data.vertices]
mid_depth = 0.5 * (max(depths) + min(depths))
root_bone = rig_data.edit_bones.new("root")
root_bone.head = (0, 0, 0)
root_bone.tail = (0, 0.25, 0) if lying else (0, 0, 0.25)
for part in parts:
    pivot, tail, _ = segments[part["name"]]
    bone = rig_data.edit_bones.new(part["name"])
    bone.head = place(pivot[0], pivot[1], mid_depth)
    bone.tail = place(tail[0], tail[1], mid_depth)
    bone.align_roll(toward_viewer)             # local z points at the viewer: turning about it swings the part in the drawing's plane
for part in parts:
    parent = part.get("parent")
    rig_data.edit_bones[part["name"]].parent = rig_data.edit_bones[parent] if parent in rig_data.edit_bones else root_bone
# Extra bones the plan asks for: places to hang things on in the game (a face, a hand-hold). They move nothing.
for name, extra in plan.get("extra_bones", {}).items():
    eu, ev = to_sheet(*extra["at"])
    bone = rig_data.edit_bones.new(name)
    bone.head = place(eu, ev, mid_depth + extra.get("front_m", 0.0) / scale)
    bone.tail = Vector(bone.head) + Vector((0, 0, 0.08))
    bone.parent = rig_data.edit_bones.get(extra.get("parent", "root")) or root_bone
bpy.ops.object.mode_set(mode="OBJECT")
rig["sheet_to_model"] = [root_u, root_v, fig_v0, scale, 1.0 if lying else 0.0, -1.0 if plan.get("head_dir", "down") == "up" else 1.0, mid_depth]

groups = {part["name"]: body.vertex_groups.new(name=part["name"]) for part in parts}
owner_count = {name: 0 for name in groups}
for index, name in enumerate(owners):
    groups[name].add([index], 1.0, "REPLACE")
    owner_count[name] += 1
print("points per part:", owner_count)

body.parent = rig
modifier = body.modifiers.new("Armature", "ARMATURE")
modifier.object = rig

# ---------------------------------------------------------------- 5. the ink edge
shell = body.modifiers.new("InkEdge", "SOLIDIFY")
shell.thickness = plan.get("outline_m", 0.012)
shell.offset = 1.0
shell.use_flip_normals = True
shell.use_rim = False
shell.material_offset = 1
bpy.context.view_layer.objects.active = body
bpy.ops.object.modifier_move_to_index(modifier="InkEdge", index=0)
bpy.ops.object.modifier_apply(modifier="InkEdge")
print("with ink edge:", len(body.data.polygons), "triangles")

# A light and a camera-free look: flat world light so the paint reads as it will in the game.
world = bpy.data.worlds.new("Flat") if not bpy.data.worlds else bpy.data.worlds[0]
bpy.context.scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.93, 0.89, 0.80, 1.0)
world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(folder, char_id + ".blend"))
print("BUILT", char_id)
