"""The Scumblewood kit: her vertebrae, trees, roots, rock, undergrowth, and the small marks people leave
(docs/ART_BIBLE.md sections 3.2 and 4.2).

Run inside Blender, in an empty file:
    exec(open("<repo>/tools/kit/kitlib.py").read()); exec(open("<repo>/tools/kit/blender_scumblewood.py").read())

Nothing here was built. Everything is rounded, leaning and grown. Slot "Cliff" is painted bark-brown on the lit wall
and swapped for bare canvas on the far wall when the level is dressed; "Line" is charcoal drawn on the canvas.
"""

ZONE = "Scumblewood"


def vertebra(name, seed, width=24.0, height=20.0, depth=7.0):
    """One of her vertebrae as a gorge wall: a great rounded body with a ridge along the top and ledges for moss.
    Origin at the middle of its foot; the face you see is -Y. Built to stand over a grey-box wall of the same size."""
    b = Builder(name, seed)
    r = b.rng
    b.blob((0, 0, height * 0.42), 1.0, "Cliff", squash=(width * 0.56, depth * 0.62, height * 0.5), detail=3, wobble=0.07)        # the body
    for i in range(3):                                                                                           # shoulders, so no two read alike
        x = (i - 1) * width * 0.3 + r.uniform(-1.5, 1.5)
        b.blob((x, -depth * 0.2, height * r.uniform(0.25, 0.6)), 1.0, "Cliff", squash=(width * 0.2, depth * 0.55, height * r.uniform(0.2, 0.32)), detail=2, wobble=0.1)
    b.blob((r.uniform(-2, 2), 0, height * 0.92), 1.0, "Cliff", squash=(width * 0.3, depth * 0.4, height * 0.16), detail=2, wobble=0.1)   # the spine's ridge
    for i in range(4):                                                                                           # ledges of moss
        x = r.uniform(-width * 0.4, width * 0.4)
        z = height * r.uniform(0.3, 0.8)
        b.blob((x, -depth * 0.5, z), 1.0, "Moss", squash=(r.uniform(1.5, 3.0), 0.9, 0.28), detail=1, wobble=0.14)
    for i in range(5):                                                                                           # contour lines: hers, drawn round the form
        z = height * (0.2 + i * 0.15)
        half = width * 0.5 * math.sqrt(max(0.05, 1 - ((z - height * 0.42) / (height * 0.5)) ** 2)) * 0.8
        b.tube([(-half, -depth * 0.66, z + r.uniform(-0.2, 0.2)), (-half * 0.3, -depth * 0.74, z + 0.15), (half * 0.35, -depth * 0.74, z - 0.1),
                (half, -depth * 0.66, z + r.uniform(-0.2, 0.2))], 0.045, "Line", segments=4, wobble=0.0)
    return b.build()


def forest_tree(name, seed, height=11.0, girth=0.55, lean=1.2, masses=4):
    """A thick leaning trunk and a canopy of a few overlapping masses of dabbed leaf, ragged at the edge."""
    b = Builder(name, seed)
    r = b.rng
    a = r.uniform(0, math.tau)
    top = Vector((math.cos(a) * lean, math.sin(a) * lean, height * 0.62))
    mid = Vector((math.cos(a) * lean * 0.25 + r.uniform(-0.3, 0.3), math.sin(a) * lean * 0.25, height * 0.3))
    b.tube([(0, 0, -0.3), mid, top], girth, "Bark", end_radius=girth * 0.5, segments=7, wobble=0.03)
    for i in range(4):                                                                                           # the root flare
        ra = a + i * math.tau / 4 + r.uniform(-0.4, 0.4)
        b.tube([(math.cos(ra) * girth * 0.4, math.sin(ra) * girth * 0.4, 0.7), (math.cos(ra) * girth * 1.6, math.sin(ra) * girth * 1.6, 0.18),
                (math.cos(ra) * girth * 3.2, math.sin(ra) * girth * 3.2, -0.15)], girth * 0.42, "Bark", end_radius=girth * 0.12, segments=5, wobble=0.02)
    for i in range(masses):
        ba = a + i * math.tau / masses + r.uniform(-0.5, 0.5)
        reach = r.uniform(1.6, 3.2) if i else 0.0
        at = top + Vector((math.cos(ba) * reach, math.sin(ba) * reach, r.uniform(0.2, 1.8)))
        if i:
            b.tube([top - Vector((0, 0, 0.6)), (top + at) / 2.0 + Vector((0, 0, 0.4)), at], girth * 0.3, "Bark", end_radius=girth * 0.1, segments=5, wobble=0.02)
        rad = r.uniform(2.0, 2.9) if i else 3.2
        b.blob(at + Vector((0, 0, rad * 0.35)), rad, "Foliage" if i % 2 == 0 else "FoliageLight", squash=(1.0, 1.0, 0.62), detail=2, wobble=0.2)
        b.blob(at + Vector((r.uniform(-1, 1), r.uniform(-1, 1), rad * 0.75)), rad * 0.55, "FoliageLight", squash=(1.0, 1.0, 0.6), detail=1, wobble=0.22)
    return b.build()


def sapling(name="SM_Sapling", seed=211):
    b = Builder(name, seed)
    b.tube([(0, 0, 0), (0.12, 0.05, 1.2), (-0.05, 0.1, 2.4)], 0.07, "Bark", end_radius=0.03, segments=5)
    for (x, y, z, rad) in ((0, 0.1, 2.5, 0.6), (0.4, -0.1, 2.0, 0.42), (-0.35, 0.2, 2.15, 0.38)):
        b.blob((x, y, z), rad, "FoliageLight", squash=(1, 1, 0.7), detail=1, wobble=0.22)
    return b.build()


def stump(name="SM_Stump", seed=213):
    b = Builder(name, seed)
    b.cylinder((0, 0, 0), 0.62, 0.7, "Bark", segments=9, top_radius=0.5, wobble=0.04)
    b.cylinder((0, 0, 0.7), 0.47, 0.04, "Canvas", segments=9, wobble=0.01)                                           # the cut face: pale, like paper
    for i in range(3):
        ra = i * 2.1 + 0.4
        b.tube([(math.cos(ra) * 0.4, math.sin(ra) * 0.4, 0.35), (math.cos(ra) * 1.2, math.sin(ra) * 1.2, -0.05)], 0.2, "Bark", end_radius=0.07, segments=5)
    b.blob((0.3, -0.5, 0.2), 0.3, "Moss", squash=(1.2, 0.8, 0.5), detail=1)
    return b.build()


def fallen_trunk(name="SM_FallenTrunk", seed=215):
    b = Builder(name, seed)
    b.tube([(-3.2, 0, 0.42), (-1.0, 0.15, 0.5), (1.4, -0.1, 0.44), (3.4, 0.2, 0.3)], 0.5, "Bark", end_radius=0.3, segments=8, wobble=0.03)
    b.tube([(1.2, 0, 0.6), (1.6, 0.6, 1.3), (1.7, 1.1, 1.8)], 0.14, "Bark", end_radius=0.05, segments=5)
    for x in (-2.2, -0.2, 2.2):
        b.blob((x, 0.05, 0.88), 0.5, "Moss", squash=(1.5, 0.8, 0.25), detail=1, wobble=0.15)
    for x in (-1.2, 0.6):                                                                                        # shelves of fungus
        b.cylinder((x, -0.52, 0.45), 0.24, 0.05, "ClothOchre", segments=7, wobble=0.01)
        b.cylinder((x + 0.1, -0.5, 0.62), 0.17, 0.04, "ClothOchre", segments=7, wobble=0.01)
    return b.build()


def root_arch(name="SM_RootArch", seed=217, span=5.0, rise=3.6):
    """A root thrown across the path: the forest's doorway. You walk through it along Y."""
    b = Builder(name, seed)
    r = b.rng
    pts = b.arc((0, 0, -0.3), span / 2.0, math.pi * 0.98, math.pi * 0.02, 9)
    pts = [(x * 1.0, y + math.sin(i * 0.9) * 0.25, z * (rise / (span / 2.0)) * (0.92 + 0.08 * math.sin(i))) for i, (x, y, z) in enumerate(pts)]
    b.tube(pts, 0.42, "Bark", end_radius=0.26, segments=7, wobble=0.03)
    b.tube([pts[2], (pts[2][0] - 0.8, 0.5, 0.6), (pts[2][0] - 1.3, 0.9, -0.2)], 0.16, "Bark", end_radius=0.06, segments=5)
    b.tube([pts[6], (pts[6][0] + 0.5, -0.6, 1.0), (pts[6][0] + 1.0, -1.0, -0.2)], 0.18, "Bark", end_radius=0.06, segments=5)
    for i in (3, 5):
        b.blob((pts[i][0], pts[i][1], pts[i][2] + 0.3), 0.5, "Moss", squash=(1.4, 0.9, 0.3), detail=1)
    for i in (4, 6):                                                                                             # hanging moss
        b.block((pts[i][0], pts[i][1], pts[i][2] - 0.75), (0.2, 0.04, r.uniform(0.7, 1.2)), "Moss", taper=0.7, wobble=0.02)
    return b.build()


def root_step(name="SM_RootStep", seed=219):
    b = Builder(name, seed)
    b.tube([(-2.2, 0.2, 0.0), (-0.8, -0.1, 0.22), (0.9, 0.12, 0.2), (2.3, -0.2, -0.02)], 0.2, "Bark", end_radius=0.13, segments=6, wobble=0.02)
    b.tube([(-0.6, 0, 0.15), (-0.2, 0.8, 0.05), (0.1, 1.5, -0.1)], 0.1, "Bark", end_radius=0.04, segments=5)
    return b.build()


def root_rail(name="SM_RootRail", seed=221):
    """A root running at hip height beside a drop: the forest's railing."""
    b = Builder(name, seed)
    b.tube([(-2.4, 0, -0.2), (-2.0, 0.05, 0.8), (-0.6, -0.08, 1.05), (0.9, 0.06, 0.92), (2.1, 0, 0.75), (2.5, 0, -0.2)], 0.14, "Bark", end_radius=0.1, segments=6, wobble=0.015)
    b.tube([(0.2, 0, 1.0), (0.3, 0.1, 0.4), (0.25, 0.2, -0.2)], 0.08, "Bark", end_radius=0.05, segments=5)
    return b.build()


def boulder(name, seed, size, moss=False):
    b = Builder(name, seed)
    r = b.rng
    b.blob((0, 0, size * 0.3), size * 0.6, "Stone", squash=(r.uniform(1.0, 1.4), r.uniform(0.8, 1.1), r.uniform(0.6, 0.85)), detail=2, wobble=0.17)
    if size > 0.9:
        b.blob((size * 0.55, size * 0.25, size * 0.15), size * 0.32, "Stone", squash=(1.1, 1.0, 0.7), detail=1, wobble=0.2)
    if moss:
        b.blob((0, 0, size * 0.62), size * 0.42, "Moss", squash=(1.3, 1.0, 0.3), detail=1, wobble=0.12)
    return b.build()


def fern(name="SM_Fern", seed=231):
    """Fronds thrown out from a point, each a few flat blades: flicks of a loaded brush."""
    b = Builder(name, seed)
    r = b.rng
    for i in range(8):
        a = i / 8.0 * math.tau + r.uniform(-0.2, 0.2)
        reach, lift = r.uniform(0.6, 0.95), r.uniform(0.35, 0.6)
        for j in range(3):
            t = (j + 0.5) / 3.0
            b.block((math.cos(a) * reach * t, math.sin(a) * reach * t, lift * math.sin(t * 2.6) + 0.05), (reach / 3.0 * 1.15, 0.2 * (1.1 - t), 0.02),
                    "Foliage" if i % 2 else "FoliageLight", rot=(0, -math.cos(t * 2.6) * 0.8, a), wobble=0.01)
    return b.build()


def tuft(name, seed, slot="Foliage", tall=1.0):
    b = Builder(name, seed)
    r = b.rng
    for i in range(7):
        a = r.uniform(0, math.tau)
        d = r.uniform(0.0, 0.22)
        h = r.uniform(0.25, 0.55) * tall
        b.block((math.cos(a) * d, math.sin(a) * d, h / 2), (0.07, 0.015, h), slot if i % 3 else "FoliageLight", rot=(r.uniform(-0.35, 0.35), r.uniform(-0.35, 0.35), a),
                taper=0.85, wobble=0.004)
    return b.build()


def moss_patch(name="SM_MossPatch", seed=237):
    b = Builder(name, seed)
    for (x, y, rad) in ((0, 0, 0.9), (0.9, 0.3, 0.6), (-0.6, 0.7, 0.5), (0.2, -0.8, 0.55)):
        b.blob((x, y, 0.0), rad, "Moss", squash=(1.0, 1.0, 0.12), detail=1, wobble=0.12)
    return b.build()


def mushroom_shelf(name="SM_MushroomShelf", seed=239):
    """Bracket fungus, to set against a trunk or a wall. Grows out along -Y."""
    b = Builder(name, seed)
    for i, (x, z, rad) in enumerate(((0, 0, 0.32), (0.22, 0.2, 0.24), (-0.2, 0.34, 0.2), (0.05, 0.52, 0.15))):
        b.cylinder((x, -rad * 0.6, z), rad, 0.05, "ClothOchre", segments=7, top_radius=rad * 0.8, wobble=0.012)
    return b.build()


def hanging_moss(name="SM_HangingMoss", seed=241):
    b = Builder(name, seed)
    r = b.rng
    for i in range(6):
        h = r.uniform(0.8, 2.0)
        b.block((-0.6 + i * 0.24, r.uniform(-0.05, 0.05), -h / 2.0), (0.16, 0.03, h), "Moss", taper=0.75, wobble=0.02, rot=(0, 0, r.uniform(-0.3, 0.3)))
    return b.build()


def blood_well(name="SM_BloodWell", seed=243):
    """Ilka's: a dark sanguine pool where the colossus bleeds, ringed with scab, with her tally stone beside it."""
    b = Builder(name, seed)
    r = b.rng
    b.cylinder((0, 0, 0.0), 1.15, 0.06, "Sanguine", segments=12, wobble=0.015)
    for i in range(11):
        a = i / 11.0 * math.tau + r.uniform(-0.1, 0.1)
        b.blob((math.cos(a) * 1.3, math.sin(a) * 1.3, 0.08), r.uniform(0.26, 0.42), "Scab", squash=(1.2, 1.0, 0.5), detail=1, wobble=0.2)
    b.blob((1.9, 0.5, 0.3), 0.42, "Stone", squash=(0.8, 0.6, 1.2), detail=1, wobble=0.12)                           # the tally stone
    for i in range(5):
        b.block((1.72 + i * 0.07, 0.24, 0.5), (0.02, 0.02, 0.2), "Paper", rot=(0.2, 0, 0), wobble=0.0)
    b.cylinder((-1.7, 0.4, 0), 0.14, 0.3, "PigmentRed", segments=7, top_radius=0.09, wobble=0.01)                 # her jar
    return b.build()


def tally_stone(name="SM_TallyStone", seed=245):
    b = Builder(name, seed)
    b.blob((0, 0, 0.55), 0.6, "Stone", squash=(0.9, 0.5, 1.3), detail=2, wobble=0.1)
    for row in range(3):
        for i in range(5):
            b.block((-0.2 + i * 0.09, -0.3, 0.9 - row * 0.28), (0.022, 0.02, 0.18), "Paper", rot=(0.12, 0, 0.5 if i == 4 else 0), wobble=0.0)
    return b.build()


def lean_to(name="SM_LeanTo", seed=247):
    """Corin's: a hide stretched over poles against the wall. Open at the front (-Y)."""
    b = Builder(name, seed)
    for x in (-1.4, 1.4):
        b.tube([(x, -1.0, 0), (x * 0.98, -0.95, 1.5)], 0.06, "Wood", segments=5)
        b.tube([(x, -1.0, 1.5), (x, 1.2, 0.3)], 0.05, "Wood", segments=5)
    b.tube([(-1.5, -0.98, 1.5), (1.5, -0.98, 1.52)], 0.05, "Wood", segments=5)
    b.lean_to_roof((0, 0.1, 1.55), 3.0, 2.4, 1.2, 0.04, "ClothOchre", yaw=math.pi)
    b.blob((0, 0.3, 0.1), 0.9, "Cloth", squash=(1.0, 0.45, 0.1), detail=1, wobble=0.08)                               # bedroll
    b.blob((0.75, 0.3, 0.14), 0.2, "Canvas", squash=(1.0, 1.4, 0.7), detail=1)
    return b.build()


def cook_ring(name="SM_CookRing", seed=249):
    b = Builder(name, seed)
    for i in range(8):
        a = i / 8.0 * math.tau
        b.blob((math.cos(a) * 0.5, math.sin(a) * 0.5, 0.1), 0.17, "Stone", squash=(1.1, 1.0, 0.8), detail=1, wobble=0.2)
    b.cylinder((0, 0, 0.02), 0.4, 0.04, "Charcoal", segments=8, wobble=0.02)
    for a in (0.3, 1.5, 2.8):
        b.block((0, 0, 0.12), (0.7, 0.07, 0.07), "Charcoal", rot=(0, 0.15, a), wobble=0.01)
    b.blob((0, 0, 0.16), 0.12, "Glow", squash=(1.0, 1.0, 0.6), detail=1, wobble=0.2)                                  # embers
    for x in (-0.3, 0.3):
        b.tube([(x, 0.75, 0), (x * 0.3, 0.2, 1.0)], 0.03, "Wood", segments=4)
    b.cylinder((0, 0.2, 0.55), 0.16, 0.2, "Metal", segments=7, top_radius=0.19, wobble=0.006)                         # pot
    return b.build()


def rope_line(name="SM_RopeLine", seed=251):
    """Pegs and a rope where someone has been going down for years."""
    b = Builder(name, seed)
    pegs = [(-3.0, 0, 0.0), (-1.0, 0.1, -0.4), (1.2, -0.1, -0.9), (3.0, 0, -1.3)]
    for (x, y, z) in pegs:
        b.block((x, y, z + 0.35), (0.07, 0.07, 0.75), "Wood", rot=(0.1, 0.08, 0), taper=0.3, wobble=0.008)
    pts = []
    for i in range(len(pegs) - 1):
        (x0, y0, z0), (x1, y1, z1) = pegs[i], pegs[i + 1]
        pts += [(x0, y0, z0 + 0.62), ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2 + 0.44)]
    pts.append((pegs[-1][0], pegs[-1][1], pegs[-1][2] + 0.62))
    b.tube(pts, 0.022, "Canvas", segments=4, wobble=0.0)
    return b.build()


def rib_gate(name="SM_RibGate", seed=253, span=7.0, rise=9.0):
    """Two of her ribs as a pointed arch: the first place bone shows, pale against the dark of the Vault."""
    b = Builder(name, seed)
    for side in (-1, 1):
        pts = []
        for i in range(9):
            t = i / 8.0
            pts.append((side * (span / 2.0) * (1.0 - t ** 2.2) + side * 0.25 * math.sin(t * 3.1), 0.0, rise * math.sin(t * math.pi / 2.0) ** 0.9))
        b.tube(pts, 0.62, "Bone", end_radius=0.3, segments=7, wobble=0.03)
        b.blob((side * span / 2.0, 0, 0.2), 0.9, "Bone", squash=(1.0, 1.0, 0.6), detail=1, wobble=0.12)
        b.blob((side * span / 2.0 * 0.96, -0.5, 1.4), 0.5, "Moss", squash=(1.0, 0.6, 0.5), detail=1)
    b.block((0, 0.5, rise * 0.45), (span * 0.8, 0.2, rise * 0.9), "Dark", taper=0.75, wobble=0.0)                         # the dark behind
    return b.build()


def spine_step(name="SM_SpineStep", seed=255, size=5.0):
    """A vertebra risen as a step of the Stair of Spines (after the Turning): a rounded drum of bone with a flat crown."""
    b = Builder(name, seed)
    b.blob((0, 0, -size * 0.3), 1.0, "Bone", squash=(size * 0.62, size * 0.62, size * 0.5), detail=2, wobble=0.06)
    b.cylinder((0, 0, -0.12), size * 0.5, 0.14, "Bone", segments=10, wobble=0.03)
    for i in range(3):
        a = i * 2.1
        b.blob((math.cos(a) * size * 0.5, math.sin(a) * size * 0.5, -size * 0.2), size * 0.18, "Bone", squash=(1.0, 1.0, 1.6), detail=1, wobble=0.12)
    return b.build()


def canvas_edge(name="SM_CanvasEdge", seed=257):
    """Where the paint stops: a ragged-edged patch of bare canvas to lay on the ground, with a few charcoal strokes."""
    b = Builder(name, seed)
    r = b.rng
    for i in range(9):
        a = r.uniform(0, math.tau)
        d = r.uniform(0, 1.6)
        b.block((math.cos(a) * d, math.sin(a) * d, 0.012 + i * 0.001), (r.uniform(0.8, 1.8), r.uniform(0.5, 1.1), 0.012), "Canvas", rot=(0, 0, r.uniform(0, 3.1)), wobble=0.0)
    for i in range(4):
        b.block((r.uniform(-1.2, 1.2), r.uniform(-1.2, 1.2), 0.03), (r.uniform(0.8, 1.6), 0.03, 0.008), "Line", rot=(0, 0, 0.4 + r.uniform(-0.2, 0.2)), wobble=0.0)
    return b.build()


def reed(name="SM_Reeds", seed=259):
    b = Builder(name, seed)
    r = b.rng
    for i in range(9):
        x, y = r.uniform(-0.4, 0.4), r.uniform(-0.4, 0.4)
        h = r.uniform(0.9, 1.6)
        b.block((x, y, h / 2), (0.03, 0.03, h), "FoliageLight", taper=0.5, wobble=0.004, lean=(r.uniform(-0.15, 0.15), r.uniform(-0.15, 0.15)))
        if i % 3 == 0:
            b.block((x, y, h + 0.02), (0.05, 0.05, 0.18), "Bark", wobble=0.004)
    return b.build()


def skins():
    out = []
    b = Builder("SM_Node_Green", 261)                                    # terre verte: a cushion of the green that grows where her skin is damp
    for (x, y, rad) in ((0, 0, 0.5), (0.45, 0.2, 0.34), (-0.35, 0.3, 0.3), (0.1, -0.45, 0.28)):
        b.blob((x, y, rad * 0.35), rad, "PigmentGreen", squash=(1.0, 1.0, 0.6), detail=1, wobble=0.16)
    out.append(b.build())
    b = Builder("SM_Node_Black", 263)                                    # bone black: a charred knuckle of bone
    b.blob((0, 0, 0.3), 0.5, "PigmentBlack", squash=(1.0, 0.8, 0.9), detail=1, wobble=0.14)
    b.tube([(-0.5, 0, 0.1), (0.1, 0.1, 0.5), (0.7, 0, 0.75)], 0.16, "Bone", end_radius=0.1, segments=6)
    out.append(b.build())
    b = Builder("SM_Node_Red", 265)                                      # sanguine: a scab over a seep
    b.cylinder((0, 0, 0), 0.5, 0.05, "Sanguine", segments=9, wobble=0.015)
    for i in range(6):
        a = i / 6.0 * math.tau
        b.blob((math.cos(a) * 0.55, math.sin(a) * 0.55, 0.06), 0.2, "Scab", squash=(1.2, 1.0, 0.5), detail=1, wobble=0.2)
    out.append(b.build())
    b = Builder("SM_ForestLamp", 267)                                    # a Lampstand in the wood: a lantern hung from a bent root
    b.tube([(0, 0, -0.2), (0.15, 0, 1.6), (0.05, 0, 2.9), (0.6, 0, 3.4), (1.1, 0, 3.2)], 0.2, "Bark", end_radius=0.07, segments=6, wobble=0.015)
    for i in range(3):
        a = i * 2.1
        b.blob((math.cos(a) * 0.6, math.sin(a) * 0.6, 0.12), 0.34, "Stone", squash=(1.1, 1.0, 0.6), detail=1, wobble=0.15)
    b.block((1.1, 0, 3.0), (0.025, 0.025, 0.4), "Metal", wobble=0.0)
    b.cylinder((1.1, 0, 2.74), 0.3, 0.14, "Metal", segments=6, top_radius=0.05, wobble=0.01)
    b.cylinder((1.1, 0, 2.32), 0.15, 0.42, "Glow", segments=6, top_radius=0.19, wobble=0.0)
    for i in range(6):
        a = i / 6.0 * math.tau
        b.block((1.1 + math.cos(a) * 0.21, math.sin(a) * 0.21, 2.52), (0.03, 0.03, 0.46), "Metal", rot=(0, 0, a), wobble=0.0)
    b.cylinder((1.1, 0, 2.26), 0.24, 0.06, "Metal", segments=6, wobble=0.0)
    out.append(b.build())
    return out


pieces = [
    vertebra("SM_Vertebra_S", 201, width=22.0), vertebra("SM_Vertebra_M", 203, width=26.0), vertebra("SM_Vertebra_L", 205, width=34.0),
    forest_tree("SM_ForestTree_A", 207), forest_tree("SM_ForestTree_B", 208, height=13.5, girth=0.7, lean=2.0, masses=5),
    forest_tree("SM_ForestTree_C", 209, height=8.5, girth=0.42, lean=1.6, masses=3),
    sapling(), stump(), fallen_trunk(), root_arch(), root_arch("SM_RootArch_Wide", 218, span=8.0, rise=4.4), root_step(), root_rail(),
    boulder("SM_Boulder_XS", 223, 0.5), boulder("SM_Boulder_S", 225, 0.9), boulder("SM_Boulder_M", 227, 1.6), boulder("SM_Boulder_L", 229, 2.8),
    boulder("SM_Boulder_Moss", 230, 1.4, moss=True),
    fern(), fern("SM_Fern_B", 233), tuft("SM_GrassTuft", 235), tuft("SM_GrassTuft_Tall", 236, "FoliageLight", 1.7), moss_patch(), reed(),
    mushroom_shelf(), hanging_moss(), blood_well(), tally_stone(), lean_to(), cook_ring(), rope_line(), rib_gate(), spine_step(), canvas_edge(),
] + skins()
KIT = kit_finish(ZONE, pieces)
