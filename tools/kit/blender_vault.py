"""The Grisaille Vault kit: a ribcage, drawn (docs/ART_BIBLE.md sections 3.3 and 4.3).

Run inside Blender, in an empty file:
    exec(open("<repo>/tools/kit/kitlib.py").read()); exec(open("<repo>/tools/kit/blender_vault.py").read())

The one place in Act 1 that is regular: this is anatomy, so ribs repeat and pieces are symmetrical. Wobble is small.
"Line" is graphite, "BlueLine" is the under-drawing's blue, "Paper" is the ground everything is drawn on.
Lines are thin solid geometry, so they need no see-through material.
"""

ZONE = "Vault"


def rib_arch(name, seed, span=10.0, rise=8.0, radius=0.42):
    """A pair of ribs meeting overhead, with a confident outline drawn beside each and hatching on the underside.
    You walk through it along Y."""
    b = Builder(name, seed)
    for side in (-1, 1):
        pts, inner = [], []
        for i in range(11):
            t = i / 10.0
            x = side * (span / 2.0) * math.cos(t * math.pi / 2.0) ** 0.8
            z = rise * math.sin(t * math.pi / 2.0)
            pts.append((x, 0.0, z))
            inner.append((x * 0.9 - side * 0.12, -radius * 1.02, z * 0.93 - 0.1))
        b.tube(pts, radius, "Bone", end_radius=radius * 0.62, segments=8, wobble=0.008)
        b.tube([(x * 1.035 + side * 0.1, -radius * 0.9, z * 1.03 + 0.1) for (x, _, z) in pts], 0.035, "Line", segments=4, wobble=0.0)     # the outline
        for i in range(2, 10):                                                                                   # hatching under the curve
            (x, y, z) = inner[i]
            b.block((x, y, z), (0.5, 0.02, 0.03), "Line", rot=(0, side * 0.9, 0), wobble=0.0)
        b.cylinder((side * span / 2.0, 0, 0), radius * 1.5, 0.3, "Bone", segments=8, top_radius=radius * 1.1, wobble=0.01)
    b.blob((0, 0, rise), radius * 1.1, "Bone", squash=(1.3, 1.0, 0.8), detail=1, wobble=0.05)                         # the keystone: sternum
    return b.build()


def rib_post(name="SM_RibPost", seed=303, height=6.0):
    b = Builder(name, seed)
    b.tube([(0, 0, 0), (0.12, 0, height * 0.5), (0, 0, height)], 0.36, "Bone", end_radius=0.26, segments=8, wobble=0.008)
    b.cylinder((0, 0, 0), 0.55, 0.26, "Bone", segments=8, top_radius=0.4, wobble=0.01)
    b.tube([(0.46, -0.3, 0.1), (0.56, -0.3, height * 0.5), (0.44, -0.3, height)], 0.03, "Line", segments=4, wobble=0.0)
    for i in range(7):
        b.block((-0.3, -0.36, 0.6 + i * 0.75), (0.34, 0.02, 0.028), "Line", rot=(0, 0.8, 0), wobble=0.0)
    return b.build()


def rib_span(name="SM_RibSpan", seed=305, length=8.0):
    """A rib laid level: a beam, a bridge edge, a lintel. Runs along X."""
    b = Builder(name, seed)
    b.tube([(-length / 2, 0, 0), (-length / 6, 0, 0.22), (length / 6, 0, 0.22), (length / 2, 0, 0)], 0.3, "Bone", end_radius=0.3, segments=8, wobble=0.008)
    b.tube([(-length / 2, -0.3, 0.36), (0, -0.3, 0.6), (length / 2, -0.3, 0.36)], 0.028, "Line", segments=4, wobble=0.0)
    return b.build()


def paper_floor(name="SM_PaperFloor", seed=307, size=4.0):
    """A sheet of the floor: paper, a little cockled, with a deckle edge and faint ruled lines."""
    b = Builder(name, seed)
    b.block((0, 0, -0.05), (size, size, 0.1), "Paper", wobble=0.012)
    for i in range(3):
        b.block((0, -size / 2 + (i + 1) * size / 4, 0.004), (size * 0.96, 0.012, 0.004), "BlueLine", wobble=0.0)
    return b.build()


def paper_wall(name="SM_PaperWall", seed=309, width=4.0, height=6.0):
    b = Builder(name, seed)
    b.block((0, 0, height / 2), (width, 0.14, height), "Paper", wobble=0.015, lean=(0, b.rng.uniform(-0.05, 0.05)))
    for i in range(9):                                                                                           # a wash of hatching in one corner
        b.block((-width * 0.3 + i * 0.12, -0.085, height * 0.25 + i * 0.07), (0.9, 0.012, 0.022), "Line", rot=(0, -0.8, 0), wobble=0.0)
    b.block((0, -0.085, 0.06), (width * 0.98, 0.012, 0.03), "Line", wobble=0.0)
    return b.build()


def torn_edge(name="SM_TornEdge", seed=311, length=4.0):
    """Where the paper was torn off: a ragged strip to finish an edge over a drop."""
    b = Builder(name, seed)
    r = b.rng
    x = -length / 2.0
    while x < length / 2.0 - 0.02:
        w = min(r.uniform(0.25, 0.6), length / 2.0 - x)
        d = r.uniform(0.15, 0.6)
        b.block((x + w / 2, -d / 2, -0.03), (w + 0.03, d, 0.06), "Paper", rot=(r.uniform(-0.12, 0.05), 0, r.uniform(-0.15, 0.15)), taper=0.3, wobble=0.008)
        x += w
    return b.build()


def construction_line(name="SM_ConstructionLine", seed=313, length=12.0):
    """A ruled line that overshoots where it was needed, as a draughtsman's does. Along X."""
    b = Builder(name, seed)
    b.block((0, 0, 0), (length, 0.02, 0.02), "Line", wobble=0.0)
    for x in (-length / 2 + 0.6, length / 2 - 0.6):
        b.block((x, 0, 0), (0.02, 0.02, 0.3), "Line", wobble=0.0)
    return b.build()


def vanishing_fan(name="SM_VanishingFan", seed=315, reach=14.0):
    """Lines of perspective thrown from one point. Lies in the XZ plane, fanning toward +X."""
    b = Builder(name, seed)
    for i in range(9):
        a = math.radians(-32 + i * 8)
        b.block((math.cos(a) * reach / 2, 0, math.sin(a) * reach / 2), (reach, 0.018, 0.018), "BlueLine" if i % 2 else "Line", rot=(0, -a, 0), wobble=0.0)
    b.cylinder((0, -0.02, 0), 0.07, 0.04, "Line", segments=8, wobble=0.0, rot=(math.pi / 2, 0, 0))
    return b.build()


def ticks(name="SM_MeasuringTicks", seed=317, length=8.0):
    b = Builder(name, seed)
    b.block((0, 0, 0), (length, 0.016, 0.016), "Line", wobble=0.0)
    n = int(length / 0.5)
    for i in range(n + 1):
        b.block((-length / 2 + i * 0.5, 0, 0.12 if i % 4 == 0 else 0.06), (0.016, 0.016, 0.24 if i % 4 == 0 else 0.12), "Line", wobble=0.0)
    return b.build()


def triple_circle(name="SM_TripleCircle", seed=319, radius=3.0):
    """Three tries at one circle, none of them closed: the Hand finding a curve. Lies flat."""
    b = Builder(name, seed)
    r = b.rng
    for k in range(3):
        rad = radius * (1.0 + (k - 1) * 0.045)
        start = r.uniform(0, 1.0)
        pts = b.arc((r.uniform(-0.08, 0.08), r.uniform(-0.08, 0.08), 0.01 + k * 0.004), rad, start, start + math.tau * r.uniform(0.86, 0.97), 28, plane="XY")
        b.tube(pts, 0.016, "Line" if k != 1 else "BlueLine", segments=4, wobble=0.0)
    return b.build()


def sketch_platform(name="SM_SketchPlatformLines", seed=321, x=4.0, y=4.0, t=0.4):
    """The drawn edges of a platform that is not there yet: blue line, no fill. Unit size 4 x 4; scaled to fit."""
    b = Builder(name, seed)
    for (sx, sy) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        b.block((sx * x / 2, sy * y / 2, -t / 2), (0.035, 0.035, t + 0.3), "BlueLine", wobble=0.0)
    for z in (0.0, -t):
        for sy in (-1, 1):
            b.block((0, sy * y / 2, z), (x + 0.5, 0.035, 0.035), "BlueLine", wobble=0.0)
        for sx in (-1, 1):
            b.block((sx * x / 2, 0, z), (0.035, y + 0.5, 0.035), "BlueLine", wobble=0.0)
    b.block((0, 0, 0.0), (math.hypot(x, y), 0.02, 0.02), "BlueLine", rot=(0, 0, math.atan2(y, x)), wobble=0.0)            # the diagonals that find its middle
    b.block((0, 0, 0.0), (math.hypot(x, y), 0.02, 0.02), "BlueLine", rot=(0, 0, -math.atan2(y, x)), wobble=0.0)
    return b.build()


def margin_note(name="SM_MarginNote", seed=323):
    """The Hand's writing: a slip of paper pinned up, with lines of script and an arrow."""
    b = Builder(name, seed)
    b.block((0, 0, 1.5), (0.9, 0.03, 0.6), "Paper", rot=(0.04, 0, -0.05), wobble=0.006)
    for i in range(4):
        b.block((-0.05 + (i % 2) * 0.04, -0.022, 1.68 - i * 0.11), (0.62 - (i % 3) * 0.1, 0.008, 0.02), "Line", rot=(0, 0, 0), wobble=0.0)
    b.block((0.2, -0.022, 1.2), (0.5, 0.008, 0.018), "BlueLine", rot=(0, 0.7, 0), wobble=0.0)
    b.block((0.38, -0.022, 1.02), (0.14, 0.008, 0.018), "BlueLine", rot=(0, 0.0, 0), wobble=0.0)
    return b.build()


def eraser_crumbs(name="SM_EraserCrumbs", seed=325):
    b = Builder(name, seed)
    r = b.rng
    for i in range(16):
        a, d = r.uniform(0, math.tau), r.uniform(0, 0.7) ** 1.4
        b.block((math.cos(a) * d, math.sin(a) * d, 0.02), (r.uniform(0.04, 0.12), 0.03, 0.03), "Graphite" if i % 4 == 0 else "Paper", rot=(0, 0, r.uniform(0, 3)), wobble=0.006)
    return b.build()


def smudge(name="SM_Smudge", seed=327):
    """A thumb dragged through graphite: lies flat on a floor or stands against a wall."""
    b = Builder(name, seed)
    r = b.rng
    for i in range(6):
        b.block((r.uniform(-0.5, 0.5), r.uniform(-0.2, 0.2), 0.006 + i * 0.001), (r.uniform(0.8, 1.6), r.uniform(0.2, 0.45), 0.006), "Graphite", rot=(0, 0, r.uniform(-0.25, 0.25)), wobble=0.0)
    return b.build()


def heart(name="SM_Heart", seed=329):
    """The only round, warm, soft-edged thing in the Vault. About three metres across, hung from four vessels."""
    b = Builder(name, seed)
    b.blob((0, 0, 0), 1.5, "Heart", squash=(1.0, 0.9, 1.15), detail=3, wobble=0.04)
    b.blob((0.7, 0.1, 0.5), 0.9, "Heart", squash=(1.0, 0.9, 1.0), detail=2, wobble=0.05)
    b.blob((-0.6, -0.1, 0.7), 0.8, "Heart", squash=(1.0, 0.9, 1.0), detail=2, wobble=0.05)
    for (x, y) in ((0.5, 0.2), (-0.5, 0.1), (0.1, -0.4), (-0.1, 0.5)):
        b.tube([(x, y, 1.2), (x * 1.6, y * 1.6, 2.6), (x * 2.6, y * 2.6, 4.6)], 0.26, "Bone", end_radius=0.14, segments=7, wobble=0.01)
    return b.build()


def hollow(name="SM_Hollow", seed=331):
    """A hollow the size of a sleeping person, pressed into the floor of the Heart's chamber."""
    b = Builder(name, seed)
    r = b.rng
    for i in range(14):
        a = i / 14.0 * math.tau
        b.blob((math.cos(a) * 1.25, math.sin(a) * 0.62, 0.05), 0.26, "Paper", squash=(1.2, 1.0, 0.45), detail=1, wobble=0.12)
    b.blob((0, 0, 0.0), 1.0, "Graphite", squash=(1.15, 0.52, 0.05), detail=1, wobble=0.03)
    pts = b.arc((0, 0, 0.09), 1.0, 0.2, math.tau - 0.3, 22, plane="XY")
    b.tube([(x * 1.42, y * 0.8, z) for (x, y, z) in pts], 0.02, "Line", segments=4, wobble=0.0)
    return b.build()


def study_panel(name, seed, blank=False):
    """A figure study on the wall: a tall sheet with a drawn figure, or (the third) nothing but its guidelines."""
    b = Builder(name, seed)
    b.block((0, 0, 1.9), (1.6, 0.05, 3.0), "Paper", rot=(0.02, 0, b.rng.uniform(-0.02, 0.02)), wobble=0.008)
    y = -0.035
    b.block((0, y, 1.9), (0.012, 0.008, 2.7), "BlueLine", wobble=0.0)                                                 # the plumb line
    for z in (3.0, 2.55, 1.9, 1.2, 0.6):                                                                          # the proportions: head, shoulder, hip, knee, foot
        b.block((0, y, z), (0.5, 0.008, 0.012), "BlueLine", wobble=0.0)
    if not blank:
        head = b.arc((0, y, 2.82), 0.2, 0, math.tau, 12, plane="XZ")
        b.tube(head, 0.014, "Line", segments=4, wobble=0.0)
        lean = b.rng.uniform(-0.08, 0.08)
        for pts in ([(0, y, 2.6), (lean, y, 1.9), (lean * 1.5, y, 1.75)],                                           # spine
                    [(-0.34, y, 2.5), (0.34, y, 2.5)], [(-0.24 + lean, y, 1.78), (0.24 + lean, y, 1.78)],              # shoulders, hips
                    [(-0.34, y, 2.5), (-0.44, y, 1.95), (-0.4, y, 1.45)], [(0.34, y, 2.5), (0.5, y, 2.05), (0.42, y, 1.6)],
                    [(-0.2 + lean, y, 1.78), (-0.24, y, 1.15), (-0.2, y, 0.5)], [(0.2 + lean, y, 1.78), (0.3, y, 1.15), (0.24, y, 0.5)]):
            b.tube(pts, 0.016, "Line", segments=4, wobble=0.0)
    return b.build()


def skins():
    out = []
    b = Builder("SM_Node_Blue", 341)                                     # slumber blue: a cold bloom on the paper
    for (x, y, rad) in ((0, 0, 0.45), (0.4, 0.15, 0.3), (-0.3, 0.3, 0.26)):
        b.blob((x, y, rad * 0.3), rad, "PigmentBlue", squash=(1.0, 1.0, 0.5), detail=1, wobble=0.14)
    out.append(b.build())
    b = Builder("SM_VaultLamp", 343)                                     # a Lampstand in the Vault: a lantern on a rib, drawn
    b.tube([(0, 0, 0), (0.1, 0, 1.6), (0, 0, 3.0), (0.5, 0, 3.4), (1.0, 0, 3.2)], 0.17, "Bone", end_radius=0.07, segments=7, wobble=0.006)
    b.cylinder((0, 0, 0), 0.5, 0.22, "Bone", segments=8, top_radius=0.36, wobble=0.01)
    b.block((1.0, 0, 3.0), (0.025, 0.025, 0.4), "Line", wobble=0.0)
    b.cylinder((1.0, 0, 2.74), 0.3, 0.14, "Graphite", segments=6, top_radius=0.05, wobble=0.0)
    b.cylinder((1.0, 0, 2.32), 0.15, 0.42, "Glow", segments=6, top_radius=0.19, wobble=0.0)
    for i in range(6):
        a = i / 6.0 * math.tau
        b.block((1.0 + math.cos(a) * 0.21, math.sin(a) * 0.21, 2.52), (0.025, 0.025, 0.46), "Line", rot=(0, 0, a), wobble=0.0)
    b.cylinder((1.0, 0, 2.26), 0.24, 0.06, "Graphite", segments=6, wobble=0.0)
    out.append(b.build())
    return out


pieces = [
    rib_arch("SM_RibArch", 301), rib_arch("SM_RibArch_Wide", 302, span=16.0, rise=11.0, radius=0.55), rib_post(), rib_span(),
    paper_floor(), paper_wall(), torn_edge(), construction_line(), vanishing_fan(), ticks(), triple_circle(), sketch_platform(),
    margin_note(), eraser_crumbs(), smudge(), heart(), hollow(),
    study_panel("SM_Study_A", 333), study_panel("SM_Study_B", 335), study_panel("SM_Study_Blank", 337, blank=True),
] + skins()
KIT = kit_finish(ZONE, pieces)
