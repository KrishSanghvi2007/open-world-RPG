"""The Varnish kit: houses, walls, ground and props for the village (docs/ART_BIBLE.md sections 3.1 and 4.1).

Run inside Blender (through the Blender MCP), in an empty file:
    exec(open("<repo>/tools/kit/kitlib.py").read()); exec(open("<repo>/tools/kit/blender_varnish.py").read())

Writes art_source/kit/Varnish/*.fbx and kit.json. tools/kit/unreal_kit.py brings them into Unreal.
"""

ZONE = "Varnish"
FOOT = 0.5          # height of the stone footing every house stands on


def house(name, seed, length=6.2, depth=5.4, wall=3.0, door=-1.3, windows=(1.2,), enterable=False, door_slot="Teal",
          shed=0, chimney=1, sign=False, back_windows=(), side_window=False):
    """A Varnish house. Front faces -Y. door and windows are positions along the front, in metres from its middle.
    enterable houses are hollow, with a real doorway (the door itself is a separate piece) and real window holes."""
    b = Builder(name, seed)
    r = b.rng
    lean = (r.uniform(-0.07, 0.07), r.uniform(-0.05, 0.05))
    hx, hy = length / 2.0, depth / 2.0
    b.block((0, 0, FOOT / 2.0), (length + 0.4, depth + 0.4, FOOT), "Stone", taper=0.04, wobble=0.045)                 # footing
    top = FOOT + wall
    if enterable:
        t = 0.34
        front = [(hx + door, 1.15, 0.0, 2.1)] + [(hx + w, 0.85, 1.05, 2.0) for w in windows]
        b.wall((-hx, -hy + t / 2), (hx, -hy + t / 2), wall, t, "Plaster", base=FOOT, openings=front)
        b.wall((hx, hy - t / 2), (-hx, hy - t / 2), wall, t, "Plaster", base=FOOT, openings=[(hx - w, 0.8, 1.15, 1.95) for w in back_windows])
        side = [(depth / 2.0, 0.8, 1.15, 1.95)] if side_window else []
        b.wall((hx - t / 2, -hy), (hx - t / 2, hy), wall, t, "Plaster", base=FOOT, openings=side)
        b.wall((-hx + t / 2, hy), (-hx + t / 2, -hy), wall, t, "Plaster", base=FOOT)
        b.block((0, 0, FOOT + 0.03), (length - 0.5, depth - 0.5, 0.08), "Wood", wobble=0.01)                           # plank floor
        for x in (-hx * 0.5, 0.0, hx * 0.5):                                                                       # tie beams overhead
            b.block((x, 0, top - 0.1), (0.2, depth - 0.2, 0.2), "Wood", wobble=0.015)
        for w in list(windows):                                                                                    # glowing panes, seen from both sides
            b.block((w, -hy + t / 2, FOOT + 1.52), (0.8, 0.04, 0.9), "Glow", wobble=0.0)
        for w in back_windows:
            b.block((w, hy - t / 2, FOOT + 1.55), (0.75, 0.04, 0.78), "Glow", wobble=0.0)
        if side_window:
            b.block((hx - t / 2, 0, FOOT + 1.55), (0.04, 0.75, 0.78), "Glow", wobble=0.0)
    else:
        b.block((0, 0, FOOT + wall / 2.0), (length, depth, wall), "Plaster", taper=0.04, wobble=0.03, lean=lean)
        front = -hy
        b.block((door, front - 0.05, FOOT + 1.05), (1.25, 0.18, 2.2), "Wood", wobble=0.02)                            # door frame
        b.block((door, front - 0.10, FOOT + 1.0), (0.95, 0.12, 1.95), "Dark", wobble=0.01)
        b.block((door - 0.02, front - 0.14, FOOT + 0.98), (0.86, 0.08, 1.86), door_slot, rot=(0, 0, r.uniform(-0.03, 0.06)), wobble=0.015)
        for w in windows:
            wz = FOOT + wall * 0.58 + r.uniform(-0.08, 0.08)
            ww, wh = r.uniform(0.62, 0.86), r.uniform(0.7, 0.95)                                                 # no two windows the same
            b.block((w, front - 0.05, wz), (ww + 0.22, 0.16, wh + 0.22), "Wood", wobble=0.015)
            b.block((w, front - 0.10, wz), (ww, 0.10, wh), "Glow", wobble=0.0)
            b.block((w, front - 0.13, wz), (0.07, 0.08, wh), "Wood", wobble=0.0)
            b.block((w, front - 0.16, wz - wh / 2 - 0.14), (ww + 0.36, 0.26, 0.1), "Wood", wobble=0.02)
            b.block((w + ww / 2 + 0.3, front - 0.12, wz), (0.4, 0.07, wh + 0.15), "Wood", rot=(0, 0, r.uniform(-0.5, -0.15)), wobble=0.015)
    b.block((door, -hy - 0.3, FOOT - 0.12), (1.5, 0.6, 0.24), "Stone", wobble=0.03)                                    # doorstep
    b.block((door, -hy - 0.75, FOOT - 0.36), (1.3, 0.5, 0.2), "Stone", wobble=0.03, rot=(0, 0, r.uniform(-0.06, 0.06)))

    # Timber: corner posts, a head beam, and a diagonal brace on one side only.
    for x in (-hx + 0.12, hx - 0.12):
        b.block((x, -hy - 0.03, FOOT + wall / 2.0), (0.26, 0.22, wall), "Wood", wobble=0.02, lean=(lean[0], 0))
    b.block((lean[0] * 0.5, -hy - 0.04, top - 0.14), (length + 0.1, 0.22, 0.28), "Wood", wobble=0.02)
    b.block((hx - 0.9, -hy - 0.04, FOOT + wall * 0.55), (0.16, 0.16, wall * 1.05), "Wood", rot=(0, r.uniform(0.45, 0.6), 0), wobble=0.015)

    # Roof: gables, sagging ridge, one patch of re-set tiles (the visible mend), a slipped tile.
    roof_h = r.uniform(2.2, 2.6)
    for x in (-hx + 0.1, hx - 0.1):
        b.gable((x + lean[0], lean[1], top - 0.02), depth * 0.95, roof_h - 0.3, 0.22, "Plaster")
    b.prism_roof((lean[0], lean[1], top - 0.12), length + 1.1, depth + 1.3, roof_h, 0.22, "Roof", sag=r.uniform(0.12, 0.22))
    b.block((lean[0], lean[1], top + roof_h - 0.27), (length + 1.2, 0.26, 0.2), "Wood", wobble=0.03)                   # ridge beam
    patch_x = r.uniform(-hx * 0.5, hx * 0.5)
    slope = math.atan2(roof_h, (depth + 1.3) / 2.0)
    b.block((patch_x, -depth * 0.26, top + roof_h * 0.5 - 0.02), (1.1, 1.0, 0.06), "Wood", rot=(slope, 0, 0), wobble=0.02)      # the mend: bare boards
    b.block((patch_x + 1.6, -depth * 0.44, top + roof_h * 0.2), (0.4, 0.5, 0.07), "Roof", rot=(slope + 0.25, 0, 0.3), wobble=0.01)  # a slipped tile

    for i in range(chimney):
        cx = (-1 if door > 0 else 1) * hx * 0.5
        b.block((cx, 0.9, top + roof_h * 0.72), (0.7, 0.7, 2.1), "Stone", taper=0.12, wobble=0.03, lean=(0.06, -0.03))
        b.block((cx + 0.06, 0.87, top + roof_h * 0.72 + 1.12), (0.9, 0.9, 0.18), "Stone", wobble=0.03)
    if shed:                                                                                                       # a lean-to store on one end
        sx = shed * (hx + 0.95)
        b.block((sx, 0.4, FOOT / 2), (1.9, 3.2, FOOT), "Stone", wobble=0.04)
        b.block((sx, 0.4, FOOT + 0.95), (1.7, 3.0, 1.9), "Wood", taper=0.03, wobble=0.03)
        b.lean_to_roof((sx + shed * 0.05, 0.4, FOOT + 2.45), 3.5, 2.3, 0.75, 0.14, "Roof", yaw=-shed * math.pi / 2)
        b.block((sx, -1.15, FOOT + 0.8), (0.8, 0.08, 1.5), "Dark", wobble=0.01)
    if sign:                                                                                                       # a bracket and a hanging board
        b.block((door + 1.0, -hy - 0.45, FOOT + 2.55), (0.1, 0.9, 0.1), "Wood", wobble=0.01)
        b.block((door + 1.0, -hy - 0.75, FOOT + 2.2), (0.06, 0.5, 0.5), "Wood", rot=(0, 0.1, 0), wobble=0.01)
    return b.build()


def door_leaf(name, slot="Teal", seed=5):
    """A plank door on its own, hinged at its left edge (the origin), so it can stand open."""
    b = Builder(name, seed)
    for i in range(4):
        b.block((0.13 + i * 0.25, 0, 1.0 + b.rng.uniform(-0.02, 0.02)), (0.24, 0.07, 2.0), slot, wobble=0.012)
    for z in (0.35, 1.65):
        b.block((0.5, -0.05, z), (0.95, 0.04, 0.12), "Wood", wobble=0.01)
    b.block((0.5, -0.05, 1.0), (1.1, 0.04, 0.1), "Wood", rot=(0, 0.95, 0), wobble=0.01)
    b.block((0.86, -0.07, 1.05), (0.06, 0.08, 0.16), "Metal", wobble=0.0)
    return b.build()


def retaining_wall(name, seed, length=4.0, height=4.2):
    """A battered stone wall holding a terrace, with a course of capstones. Faces -Y; the terrace is behind it (+Y)."""
    b = Builder(name, seed)
    r = b.rng
    courses = 5
    for c in range(courses):
        z0 = height * c / courses
        h = height / courses
        x = -length / 2.0
        while x < length / 2.0 - 0.05:
            w = min(r.uniform(0.9, 1.7), length / 2.0 - x)
            batter = 0.34 * (1.0 - (c + 0.5) / courses)
            b.block((x + w / 2.0, -batter / 2.0 - 0.1, z0 + h / 2.0), (w - 0.03, 0.5 + batter, h - 0.03), "Stone", wobble=0.04, taper=0.03)
            x += w
    for i in range(3):
        b.block((-length / 2.0 + (i + 0.5) * length / 3.0, -0.12, height + 0.07), (length / 3.0 - 0.04, 0.7, 0.16), "Stone", wobble=0.035,
                rot=(r.uniform(-0.02, 0.02), 0, r.uniform(-0.03, 0.03)))
    b.blob((r.uniform(-1.2, 1.2), -0.42, height * r.uniform(0.3, 0.7)), 0.28, "Moss", squash=(1.3, 0.35, 0.8), detail=1)        # moss in a joint
    return b.build()


def parapet(name, seed, length=4.0):
    """A low wall along a terrace edge: knee-high stone with uneven capstones and one gap-toothed place."""
    b = Builder(name, seed)
    r = b.rng
    x = -length / 2.0
    while x < length / 2.0 - 0.05:
        w = min(r.uniform(0.7, 1.3), length / 2.0 - x)
        h = r.uniform(0.85, 1.05)
        b.block((x + w / 2.0, 0, h / 2.0), (w - 0.03, 0.55, h), "Stone", wobble=0.04, taper=0.05)
        if r.random() > 0.2:
            b.block((x + w / 2.0, 0, h + 0.07), (w + 0.06, 0.7, 0.14), "Stone", wobble=0.03, rot=(0, r.uniform(-0.03, 0.03), 0))
        x += w
    return b.build()


def paving(name, seed, radius=3.2, ring=False):
    """Flat flagstones set in the earth, none of them square. ring leaves the middle open (round the Lampstand)."""
    b = Builder(name, seed)
    r = b.rng
    count = 22 if ring else 14
    for i in range(count):
        a = r.uniform(0, math.tau)
        d = r.uniform(1.3, radius) if ring else radius * math.sqrt(r.random())
        w, l = r.uniform(0.6, 1.2), r.uniform(0.6, 1.3)
        b.block((math.cos(a) * d, math.sin(a) * d, 0.02), (w, l, 0.06), "Stone", rot=(0, 0, r.uniform(0, 3.1)), wobble=0.012, taper=0.06)
    return b.build()


def lampstand(name="SM_Lampstand", seed=7):
    """The village Lampstand (the look test's, finished): stone plinth, crooked post, caged flame, offerings of oil."""
    b = Builder(name, seed)
    b.cylinder((0, 0, 0.0), 0.95, 0.28, "Stone", segments=8, top_radius=0.88, wobble=0.03)
    b.cylinder((0, 0, 0.28), 0.66, 0.26, "Stone", segments=8, top_radius=0.58, wobble=0.03)
    b.cylinder((0, 0, 0.54), 0.20, 2.55, "Wood", segments=6, top_radius=0.13, wobble=0.012, lean=(0.10, -0.05))
    for z in (0.75, 1.9, 2.85):
        t = (z - 0.54) / 2.55
        b.cylinder((0.10 * t, -0.05 * t, z), 0.215 - 0.07 * t, 0.07, "Metal", segments=6, wobble=0.0)
    top = Vector((0.10, -0.05, 3.09))
    b.block((top.x + 0.42, top.y, top.z - 0.06), (1.0, 0.11, 0.11), "Wood", rot=(0, -0.12, 0), wobble=0.01)
    b.block((top.x + 0.30, top.y, top.z - 0.38), (0.72, 0.08, 0.08), "Wood", rot=(0, 0.75, 0), wobble=0.01)
    hang = Vector((top.x + 0.82, top.y, top.z - 0.02))
    b.block((hang.x, hang.y, hang.z - 0.17), (0.03, 0.03, 0.30), "Metal", wobble=0.0)
    cage_top = hang.z - 0.32
    b.cylinder((hang.x, hang.y, cage_top), 0.34, 0.16, "Metal", segments=6, top_radius=0.05, wobble=0.01)
    b.cylinder((hang.x, hang.y, cage_top - 0.46), 0.17, 0.44, "Glow", segments=6, top_radius=0.21, wobble=0.0)
    for i in range(6):
        a = i / 6.0 * math.tau
        b.block((hang.x + math.cos(a) * 0.23, hang.y + math.sin(a) * 0.23, cage_top - 0.24), (0.035, 0.035, 0.50), "Metal", rot=(0, 0, a), wobble=0.0)
    b.cylinder((hang.x, hang.y, cage_top - 0.54), 0.26, 0.07, "Metal", segments=6, wobble=0.0)
    b.cylinder((0.62, -0.42, 0.28), 0.13, 0.26, "PigmentOchre", segments=7, top_radius=0.09, wobble=0.01)
    b.cylinder((0.36, -0.66, 0.28), 0.10, 0.20, "Stone", segments=7, top_radius=0.07, wobble=0.01)
    return b.build()


def grinding_slab(name="SM_GrindingSlab", seed=19):
    b = Builder(name, seed)
    for x in (-0.75, 0.75):
        b.block((x, 0, 0.36), (0.42, 0.85, 0.72), "Stone", taper=0.1, wobble=0.04)
    b.block((0, 0, 0.80), (2.3, 1.15, 0.18), "Stone", wobble=0.035, rot=(0.0, 0.015, 0.02))
    b.block((0.05, 0.02, 0.895), (1.2, 0.7, 0.02), "PigmentOchre", wobble=0.03, rot=(0, 0, 0.2))
    b.cylinder((0.1, 0.05, 0.90), 0.13, 0.20, "Stone", segments=7, top_radius=0.08, wobble=0.01)
    for (x, y, pigment) in ((-0.82, 0.28, "PigmentOchre"), (-0.72, -0.24, "PigmentGreen"), (0.85, 0.22, "PigmentRed")):
        b.cylinder((x, y, 0.89), 0.12, 0.10, "Wood", segments=8, top_radius=0.19, wobble=0.008)
        b.cylinder((x, y, 0.975), 0.165, 0.025, pigment, segments=8, wobble=0.004)
    b.cylinder((1.75, -0.2, 0.0), 0.24, 0.46, "Wood", segments=7, top_radius=0.27, wobble=0.015)
    b.cylinder((-1.6, 0.35, 0.0), 0.2, 0.42, "PigmentRed", segments=8, top_radius=0.12, wobble=0.012)
    b.cylinder((-1.6, 0.35, 0.42), 0.09, 0.14, "PigmentRed", segments=8, top_radius=0.11, wobble=0.008)
    return b.build()


def armature_bench(name="SM_ArmatureBench", seed=23):
    """Where gear is made: a heavy trestle bench with a vice, a half-made frame of rib and hide, tools on pegs."""
    b = Builder(name, seed)
    for x in (-0.95, 0.95):
        for y in (-0.38, 0.38):
            b.block((x, y, 0.42), (0.14, 0.14, 0.84), "Wood", wobble=0.015, lean=(0.03 if x < 0 else -0.03, 0))
    b.block((0, 0, 0.88), (2.4, 1.0, 0.12), "Wood", wobble=0.025, rot=(0, 0.01, 0))
    b.block((0, 0, 0.3), (2.0, 0.12, 0.1), "Wood", wobble=0.015)
    b.block((0.0, 0.52, 1.5), (2.2, 0.08, 1.15), "Wood", wobble=0.02, lean=(0, 0.06))                                # tool board
    for i, x in enumerate((-0.8, -0.45, -0.1, 0.3, 0.7)):
        b.block((x, 0.46, 1.55 + (i % 2) * 0.12), (0.06, 0.05, 0.42 + (i % 3) * 0.1), "Metal" if i % 2 else "Wood", rot=(0, b.rng.uniform(-0.2, 0.2), 0), wobble=0.006)
    b.block((-0.95, -0.3, 1.02), (0.3, 0.26, 0.2), "Metal", wobble=0.01)                                              # vice
    for i in range(4):                                                                                             # a frame of ribs, half covered
        b.block((0.25 + i * 0.22, -0.05, 1.12), (0.05, 0.7, 0.05), "Bone", rot=(0, 0, 0.1 * i - 0.15), wobble=0.01)
    b.block((0.45, -0.05, 1.17), (0.5, 0.6, 0.03), "ClothOchre", rot=(0, 0.05, 0.1), wobble=0.02)
    b.cylinder((1.55, -0.1, 0.0), 0.22, 0.5, "Wood", segments=7, top_radius=0.25, wobble=0.015)                      # stool
    return b.build()


def flax_press(name="SM_FlaxPress", seed=27):
    """The village's oil: a screw press on a stone bed, with a jug under the spout."""
    b = Builder(name, seed)
    b.block((0, 0, 0.25), (1.5, 1.2, 0.5), "Stone", taper=0.06, wobble=0.04)
    for x in (-0.6, 0.6):
        b.block((x, 0, 1.25), (0.2, 0.26, 1.6), "Wood", wobble=0.02, lean=(0.02 * (1 if x < 0 else -1), 0))
    b.block((0, 0, 2.05), (1.6, 0.3, 0.24), "Wood", wobble=0.02)
    b.cylinder((0, 0, 0.95), 0.1, 1.1, "Wood", segments=6, wobble=0.008)                                             # the screw
    b.block((0, 0, 1.75), (1.5, 0.1, 0.1), "Wood", rot=(0, 0, 0.5), wobble=0.01)                                       # turning bar
    b.cylinder((0, 0, 0.5), 0.42, 0.42, "Wood", segments=10, wobble=0.012)                                           # the tub
    b.block((0, 0, 0.95), (0.7, 0.7, 0.1), "Wood", wobble=0.01)
    b.block((0, -0.62, 0.52), (0.14, 0.4, 0.06), "Wood", rot=(0.25, 0, 0), wobble=0.006)                               # spout
    b.cylinder((0, -0.95, 0.0), 0.17, 0.34, "PigmentOchre", segments=8, top_radius=0.1, wobble=0.01)                 # jug of oil
    b.cylinder((0.5, -0.9, 0.0), 0.14, 0.28, "Stone", segments=8, top_radius=0.09, wobble=0.01)
    b.blob((-0.95, -0.5, 0.25), 0.34, "ClothOchre", squash=(1.0, 0.8, 0.8), detail=1, wobble=0.15)                   # a sack of flax
    return b.build()


def stall(name, seed, cloth="Cloth"):
    """A market stall: four crooked poles, an awning that sags, a board with things on it."""
    b = Builder(name, seed)
    r = b.rng
    for x in (-1.1, 1.1):
        for y in (-0.7, 0.7):
            b.cylinder((x, y, 0), 0.06, 2.2 if y > 0 else 1.85, "Wood", segments=5, top_radius=0.045, wobble=0.008, lean=(r.uniform(-0.06, 0.06), r.uniform(-0.05, 0.05)))
    b.lean_to_roof((0, 0, 2.2), 2.7, 1.9, 0.36, 0.04, cloth)
    b.block((0, -1.0, 1.72), (2.7, 0.03, 0.26), cloth, wobble=0.02, rot=(0.1, 0, 0))                                   # valance
    b.block((0, 0, 0.85), (2.3, 1.1, 0.08), "Wood", wobble=0.02, rot=(0, 0.02, 0))
    for x in (-0.95, 0.95):
        b.block((x, 0, 0.42), (0.12, 0.9, 0.84), "Wood", wobble=0.015)
    for i in range(5):
        x = -0.9 + i * 0.45
        slot = ("PigmentOchre", "PigmentRed", "PigmentGreen", "Stone", "ClothOchre")[(i + seed) % 5]
        b.cylinder((x, r.uniform(-0.25, 0.25), 0.89), r.uniform(0.09, 0.14), r.uniform(0.14, 0.3), slot, segments=7, top_radius=r.uniform(0.06, 0.1), wobble=0.008)
    return b.build()


def drying_rack(name="SM_DryingRack", seed=41):
    """Flakes of hide ochre laid out to dry on slatted trays."""
    b = Builder(name, seed)
    for x in (-0.8, 0.8):
        b.block((x, 0, 0.75), (0.08, 0.08, 1.5), "Wood", wobble=0.012, lean=(0, 0.25))
        b.block((x, 0.45, 0.6), (0.07, 0.07, 1.25), "Wood", wobble=0.012, lean=(0, -0.3))
    for i, z in enumerate((0.45, 0.85, 1.25)):
        b.block((0, 0.1 + i * 0.06, z), (1.8, 0.6, 0.04), "Wood", rot=(0.18, 0, 0), wobble=0.012)
        for j in range(5):
            b.block((-0.7 + j * 0.35, 0.1 + i * 0.06, z + 0.04), (0.24, 0.3, 0.03), "PigmentOchre", rot=(0.18, 0, b.rng.uniform(-0.5, 0.5)), wobble=0.012)
    return b.build()


def dye_line(name="SM_DyeLine", seed=43):
    """Cloth hung to dry between two poles: madder, ochre, teal. The line sags."""
    b = Builder(name, seed)
    r = b.rng
    for x in (-2.0, 2.0):
        b.cylinder((x, 0, 0), 0.07, 2.4, "Wood", segments=5, top_radius=0.05, wobble=0.008, lean=(-0.1 if x < 0 else 0.1, 0.03))
    for i in range(8):
        t = (i + 0.5) / 8.0
        x = -1.9 + 3.8 * t
        sag = 0.35 * (1 - (2 * t - 1) ** 2)
        b.block((x, 0, 2.3 - sag), (0.5, 0.02, 0.02), "Wood", rot=(0, (t - 0.5) * 0.35, 0), wobble=0.0)
        if i in (1, 2, 4, 5, 6):
            h = r.uniform(0.7, 1.2)
            b.block((x, r.uniform(-0.02, 0.02), 2.28 - sag - h / 2), (0.44, 0.03, h), ("Cloth", "ClothOchre", "ClothTeal")[i % 3], wobble=0.02, rot=(r.uniform(-0.06, 0.06), 0, 0))
    return b.build()


def small(name, seed, kind):
    """Crates, barrels, sacks, jars, baskets, benches: the small things that say someone works here."""
    b = Builder(name, seed)
    r = b.rng
    if kind == "crates":
        b.block((0, 0, 0.3), (0.7, 0.6, 0.6), "Wood", wobble=0.02, rot=(0, 0, 0.1))
        b.block((0.66, 0.1, 0.24), (0.55, 0.5, 0.48), "Wood", wobble=0.02, rot=(0, 0, -0.2))
        b.block((0.2, 0.05, 0.82), (0.5, 0.5, 0.44), "Wood", wobble=0.02, rot=(0, 0, 0.4))
    elif kind == "barrel":
        b.cylinder((0, 0, 0), 0.3, 0.42, "Wood", segments=9, top_radius=0.36, wobble=0.01)
        b.cylinder((0, 0, 0.42), 0.36, 0.42, "Wood", segments=9, top_radius=0.3, wobble=0.01)
        for z in (0.12, 0.72):
            b.cylinder((0, 0, z), 0.345, 0.05, "Metal", segments=9, wobble=0.0)
    elif kind == "sacks":
        b.blob((0, 0, 0.26), 0.36, "ClothOchre", squash=(1.2, 0.85, 0.75), detail=1, wobble=0.14)
        b.blob((0.5, 0.2, 0.22), 0.3, "ClothOchre", squash=(1.0, 1.1, 0.75), detail=1, wobble=0.14)
        b.blob((0.2, 0.1, 0.62), 0.28, "Canvas", squash=(1.1, 0.9, 0.7), detail=1, wobble=0.14)
    elif kind == "jars":
        for i, (x, y) in enumerate(((0, 0), (0.42, 0.1), (0.2, -0.36), (-0.3, 0.3), (0.65, -0.3))):
            h = r.uniform(0.3, 0.55)
            slot = ("PigmentRed", "PigmentOchre", "Stone", "PigmentGreen", "Teal")[i]
            b.cylinder((x, y, 0), r.uniform(0.11, 0.17), h, slot, segments=8, top_radius=r.uniform(0.07, 0.11), wobble=0.01)
            b.cylinder((x, y, h), 0.1, 0.05, "Wood", segments=8, wobble=0.004)
    elif kind == "basket":
        b.cylinder((0, 0, 0), 0.26, 0.34, "ClothOchre", segments=9, top_radius=0.36, wobble=0.012)
        b.blob((0, 0, 0.36), 0.28, "PigmentOchre", squash=(1.0, 1.0, 0.35), detail=1, wobble=0.2)
    elif kind == "bench":
        b.block((0, 0, 0.45), (1.7, 0.36, 0.08), "Wood", wobble=0.02, rot=(0, 0.015, 0))
        for x in (-0.65, 0.65):
            b.block((x, 0, 0.22), (0.1, 0.32, 0.44), "Wood", wobble=0.015, lean=(0.04 if x < 0 else -0.04, 0))
    elif kind == "trough":
        b.block((0, 0, 0.3), (1.9, 0.75, 0.6), "Stone", taper=-0.06, wobble=0.035)
        b.block((0, 0, 0.56), (1.6, 0.5, 0.06), "Water", wobble=0.0)
    elif kind == "ladder":
        for x in (-0.22, 0.22):
            b.block((x, 0, 1.5), (0.06, 0.06, 3.0), "Wood", wobble=0.01)
        for i in range(8):
            b.block((0, 0, 0.3 + i * 0.36), (0.5, 0.045, 0.045), "Wood", rot=(0, r.uniform(-0.06, 0.06), 0), wobble=0.006)
    elif kind == "handcart":
        b.block((0, 0, 0.62), (1.5, 0.95, 0.1), "Wood", wobble=0.02, rot=(0, -0.12, 0))
        for y in (-0.5, 0.5):
            b.block((0, y, 0.82), (1.5, 0.06, 0.34), "Wood", wobble=0.015, rot=(0, -0.12, 0))
            b.cylinder((-0.2, y * 1.12, 0.4), 0.4, 0.07, "Wood", segments=10, wobble=0.008, rot=(math.pi / 2, 0, 0))
        for y in (-0.42, 0.42):
            b.block((1.25, y, 0.5), (1.3, 0.06, 0.06), "Wood", rot=(0, 0.28, 0), wobble=0.008)
        b.blob((-0.1, 0, 0.92), 0.3, "ClothOchre", squash=(1.3, 1.0, 0.7), detail=1, wobble=0.15)
    elif kind == "fence":
        for x in (-1.0, 0.0, 1.0):
            b.cylinder((x, 0, 0), 0.05, r.uniform(1.0, 1.2), "Wood", segments=5, top_radius=0.035, wobble=0.006, lean=(r.uniform(-0.05, 0.05), r.uniform(-0.04, 0.04)))
        for i in range(5):
            b.block((0, (i % 2 - 0.5) * 0.05, 0.2 + i * 0.19), (2.1, 0.035, 0.07), "Wood", rot=(0, r.uniform(-0.03, 0.03), 0), wobble=0.012)
    elif kind == "pot_plant":
        b.cylinder((0, 0, 0), 0.16, 0.3, "Roof", segments=8, top_radius=0.22, wobble=0.01)
        b.blob((0, 0, 0.5), 0.3, "Foliage", squash=(1.0, 1.0, 0.9), detail=1, wobble=0.2)
        b.blob((0.12, 0.05, 0.68), 0.16, "FoliageLight", detail=1, wobble=0.2)
    return b.build()


def tree(name, seed, height=4.6, spread=1.0):
    """A village tree: a leaning trunk, two boughs, and a canopy of rough clumps like dabs of a loaded brush."""
    b = Builder(name, seed)
    r = b.rng
    lx, ly = r.uniform(-0.25, 0.25), r.uniform(-0.15, 0.15)
    half = height * 0.33
    b.cylinder((0, 0, 0), 0.34, half, "Bark", segments=7, top_radius=0.24, wobble=0.03, lean=(lx, ly))
    b.cylinder((lx, ly, half), 0.24, half, "Bark", segments=7, top_radius=0.15, wobble=0.03, lean=(-lx * 0.6, ly + 0.1))
    b.block((lx + 0.45, ly + 0.2, half * 1.7), (1.3, 0.14, 0.14), "Bark", rot=(0, -0.6, 0.4), wobble=0.02)
    b.block((lx - 0.35, ly, half * 1.8), (1.1, 0.12, 0.12), "Bark", rot=(0, 0.7, -0.3), wobble=0.02)
    crown = Vector((lx * 0.4, ly + 0.1, height * 0.82))
    for i in range(7):
        a = i / 7.0 * math.tau + r.uniform(-0.3, 0.3)
        d = (0.0 if i == 0 else r.uniform(0.7, 1.25)) * spread
        rad = (1.5 if i == 0 else r.uniform(0.75, 1.15)) * spread
        b.blob(crown + Vector((math.cos(a) * d, math.sin(a) * d, r.uniform(-0.5, 0.45))), rad, "Foliage" if i % 3 else "FoliageLight",
               squash=(1.0, 1.0, 0.72), detail=2, wobble=0.16)
    return b.build()


def tuft(name, seed, slot="Foliage"):
    """A tuft of rough grass: a few ragged blades, like flicks of a small brush."""
    b = Builder(name, seed)
    r = b.rng
    for i in range(7):
        a = r.uniform(0, math.tau)
        d = r.uniform(0.0, 0.22)
        h = r.uniform(0.25, 0.55)
        b.block((math.cos(a) * d, math.sin(a) * d, h / 2), (0.07, 0.015, h), slot if i % 3 else "FoliageLight", rot=(r.uniform(-0.35, 0.35), r.uniform(-0.35, 0.35), a),
                taper=0.85, wobble=0.004)
    return b.build()


def ramp_skin(name="SM_RampSkin", seed=53, length=12.65, width=4.0):
    """The face of a cart ramp between terraces: packed earth, kerb stones, and timbers set across it for grip.
    Lies flat along X with its top at z = 0; the level tilts it to the slope."""
    b = Builder(name, seed)
    r = b.rng
    b.block((0, 0, -0.12), (length, width, 0.3), "Ground", wobble=0.03)
    for side in (-1, 1):
        x = -length / 2.0
        while x < length / 2.0 - 0.05:
            w = min(r.uniform(0.8, 1.5), length / 2.0 - x)
            b.block((x + w / 2.0, side * (width / 2.0 - 0.18), 0.05), (w - 0.03, 0.4, r.uniform(0.26, 0.4)), "Stone", wobble=0.035, taper=0.06)
            x += w
    for i in range(9):
        b.block((-length / 2.0 + 0.9 + i * 1.36, r.uniform(-0.05, 0.05), 0.03), (0.16, width - 0.9, 0.09), "Wood", rot=(0, 0, r.uniform(-0.04, 0.04)), wobble=0.012)
    for y in (-0.75, 0.75):                                                                                        # cart ruts
        b.block((0, y, 0.035), (length - 0.6, 0.22, 0.02), "Dark", wobble=0.02)
    return b.build()


def yard_wall(name, seed, length=4.0, door=False, window=False):
    """A freestanding plaster wall on a stone footing: yards, and rooms inside houses."""
    b = Builder(name, seed)
    b.block((0, 0, 0.25), (length + 0.1, 0.5, 0.5), "Stone", wobble=0.035, taper=0.04)
    holes = [(length / 2.0, 1.15, 0.0, 2.1)] if door else ([(length / 2.0, 0.85, 1.05, 2.0)] if window else [])
    b.wall((-length / 2.0, 0), (length / 2.0, 0), 2.6, 0.34, "Plaster", base=0.5, openings=holes)
    b.block((0, 0, 3.14), (length + 0.12, 0.46, 0.1), "Roof", wobble=0.02, rot=(0, b.rng.uniform(-0.01, 0.01), 0))            # a tile coping
    return b.build()


def rocks(name, seed, size=1.0, moss=False):
    b = Builder(name, seed)
    r = b.rng
    b.blob((0, 0, size * 0.32), size * 0.62, "Stone", squash=(1.2, 0.95, 0.7), detail=2, wobble=0.16)
    b.blob((size * 0.6, size * 0.2, size * 0.18), size * 0.34, "Stone", squash=(1.1, 1.0, 0.7), detail=1, wobble=0.18)
    b.blob((-size * 0.5, -size * 0.3, size * 0.14), size * 0.26, "Stone", squash=(1.0, 1.2, 0.7), detail=1, wobble=0.18)
    if moss:
        b.blob((0.02, 0, size * 0.66), size * 0.42, "Moss", squash=(1.25, 1.0, 0.3), detail=1, wobble=0.12)
    return b.build()


def cistern(name="SM_Cistern", seed=51):
    """The Hem's water: a banded stone tank on a plinth with a standpipe and a tap. Lacuna's charge."""
    b = Builder(name, seed)
    b.block((0, 0, 0.2), (2.4, 2.0, 0.4), "Stone", taper=0.04, wobble=0.04)
    b.cylinder((0, 0, 0.4), 0.9, 1.5, "Stone", segments=10, top_radius=0.84, wobble=0.025)
    for z in (0.7, 1.6):
        b.cylinder((0, 0, z), 0.93, 0.07, "Metal", segments=10, wobble=0.0)
    b.cylinder((0, 0, 1.9), 0.95, 0.1, "Wood", segments=10, wobble=0.012)
    b.cylinder((0, -0.92, 0.75), 0.05, 0.4, "PigmentOchre", segments=6, wobble=0.0, rot=(math.pi / 2, 0, 0))             # brass pipe
    b.block((0, -1.3, 0.9), (0.1, 0.1, 0.22), "PigmentOchre", wobble=0.004)
    b.block((0, -1.3, 1.03), (0.3, 0.05, 0.05), "PigmentOchre", wobble=0.004)
    b.cylinder((0, -1.35, 0.4), 0.2, 0.3, "Wood", segments=8, top_radius=0.26, wobble=0.012)                           # a pail under it
    return b.build()


def gameplay_skins():
    """Final shapes for things the game places itself (nodes, caches, notes, the way out). The game swaps them in by name."""
    out = []
    b = Builder("SM_Node_Ochre", 61)                                    # hide ochre: flakes of sun-baked skin lifting from the ground
    for i in range(9):
        a = b.rng.uniform(0, math.tau)
        d = b.rng.uniform(0.05, 0.6)
        b.block((math.cos(a) * d, math.sin(a) * d, 0.08 + b.rng.uniform(0, 0.12)), (b.rng.uniform(0.25, 0.5), b.rng.uniform(0.2, 0.4), 0.05), "PigmentOchre",
                rot=(b.rng.uniform(-0.5, 0.5), b.rng.uniform(-0.5, 0.5), a), wobble=0.02)
    b.blob((0, 0, 0.0), 0.7, "Ground", squash=(1.0, 1.0, 0.14), detail=1, wobble=0.1)
    out.append(b.build())

    b = Builder("SM_Cache", 63)                                         # a bundle someone left: oilcloth, cord, a stone on top
    b.blob((0, 0, 0.22), 0.42, "ClothTeal", squash=(1.2, 0.9, 0.6), detail=1, wobble=0.12)
    b.block((0, 0, 0.24), (1.0, 0.05, 0.05), "Wood", rot=(0, 0, 0.5), wobble=0.01)
    b.block((0, 0, 0.24), (0.8, 0.05, 0.05), "Wood", rot=(0, 0, -0.9), wobble=0.01)
    b.blob((0.05, 0.02, 0.5), 0.14, "Stone", squash=(1.2, 1.0, 0.6), detail=1, wobble=0.15)
    out.append(b.build())

    b = Builder("SM_LoreNote", 65)                                      # something to read: paper pinned to a leaning stake
    b.cylinder((0, 0, 0), 0.045, 1.25, "Wood", segments=5, top_radius=0.035, wobble=0.006, lean=(0.08, 0.02))
    b.block((0.08, -0.04, 1.05), (0.34, 0.02, 0.44), "Paper", rot=(0.08, 0, 0.06), wobble=0.008)
    b.block((0.08, -0.055, 1.24), (0.04, 0.02, 0.04), "Metal", wobble=0.0)
    out.append(b.build())

    b = Builder("SM_ExitArch", 67)                                      # a way out of the village: two posts and a lintel hung with cloth
    for x in (-1.5, 1.5):
        b.cylinder((x, 0, 0), 0.16, 3.3, "Wood", segments=6, top_radius=0.12, wobble=0.012, lean=(0.06 if x < 0 else -0.04, 0.03))
        b.block((x, 0, 0.2), (0.6, 0.6, 0.4), "Stone", wobble=0.03, taper=0.1)
    b.block((0, 0.02, 3.3), (3.6, 0.2, 0.22), "Wood", rot=(0, 0.02, 0), wobble=0.02)
    for i, x in enumerate((-0.9, -0.3, 0.4, 1.0)):
        b.block((x, 0.02, 3.3 - 0.11 - (0.25 + (i % 2) * 0.12)), (0.36, 0.02, 0.5 + (i % 2) * 0.24), ("Cloth", "ClothOchre", "ClothTeal", "Cloth")[i], wobble=0.015)
    out.append(b.build())
    return out


pieces = [
    # Houses nobody goes into: five that differ, and two with a story.
    house("SM_House_V1", 3, door=-1.3, windows=(1.3,), shed=1),
    house("SM_House_V2", 8, length=5.6, depth=5.0, wall=2.8, door=1.1, windows=(-1.2,), door_slot="Wood"),
    house("SM_House_V3", 12, length=6.4, depth=5.4, wall=3.2, door=0.0, windows=(-2.0, 2.0), chimney=1, door_slot="Wood"),
    house("SM_House_V4", 17, length=5.8, depth=5.6, wall=2.9, door=-1.6, windows=(0.2, 1.7), shed=-1),
    house("SM_House_V5", 21, length=6.0, depth=5.0, wall=3.1, door=1.5, windows=(-0.6,), chimney=0, door_slot="Roof"),
    house("SM_House_BlackDoor", 29, door=-1.2, windows=(1.4,), door_slot="Black", chimney=1),          # Ansel's: set, and gone black
    house("SM_House_FairGrind", 33, length=6.0, depth=5.6, door=1.4, windows=(-1.3,), door_slot="Wood"),
    # Houses with a way in.
    house("SM_House_Lampworks", 37, length=6.2, depth=5.6, door=-1.5, windows=(1.3,), enterable=True, sign=True, back_windows=(1.4,), side_window=True),
    house("SM_House_Grinding", 39, length=6.2, depth=5.6, door=1.5, windows=(-1.3,), enterable=True, sign=True, back_windows=(-1.2,)),
    house("SM_House_Cistern", 45, length=6.0, depth=5.4, wall=2.8, door=0.0, windows=(-2.0,), enterable=True, chimney=0),
    door_leaf("SM_Door_Teal", "Teal", 5), door_leaf("SM_Door_Wood", "Wood", 6),
    retaining_wall("SM_RetainingWall_A", 71), retaining_wall("SM_RetainingWall_B", 73),
    parapet("SM_Parapet_A", 75), parapet("SM_Parapet_B", 77),
    paving("SM_Paving_Patch", 79), paving("SM_Paving_Ring", 81, radius=3.4, ring=True),
    lampstand(), grinding_slab(), armature_bench(), flax_press(), cistern(),
    stall("SM_Stall_Madder", 83, "Cloth"), stall("SM_Stall_Ochre", 85, "ClothOchre"),
    drying_rack(), dye_line(),
    small("SM_Crates", 87, "crates"), small("SM_Barrel", 88, "barrel"), small("SM_Sacks", 89, "sacks"), small("SM_Jars", 90, "jars"),
    small("SM_Basket", 91, "basket"), small("SM_Bench", 92, "bench"), small("SM_Trough", 93, "trough"), small("SM_Ladder", 94, "ladder"),
    small("SM_Handcart", 95, "handcart"), small("SM_Fence", 96, "fence"), small("SM_PotPlant", 97, "pot_plant"),
    tree("SM_Tree_A", 31), tree("SM_Tree_B", 35, height=3.8, spread=0.8),
    tuft("SM_Tuft_A", 101), tuft("SM_Tuft_B", 103, "FoliageLight"),
    ramp_skin(), yard_wall("SM_Wall_4m", 105), yard_wall("SM_Wall_2m", 106, length=2.0), yard_wall("SM_Wall_Door", 107, door=True),
    yard_wall("SM_Wall_Window", 108, window=True), rocks("SM_Rocks_A", 109, 0.8), rocks("SM_Rocks_B", 110, 1.3, moss=True),
] + gameplay_skins()
KIT = kit_finish(ZONE, pieces)
