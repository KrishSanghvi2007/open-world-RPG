"""Builds the look-test props in Blender and exports them as FBX.

Run inside Blender (Scripting tab, or through the Blender MCP):
    exec(open("<repo>/tools/looktest/blender_build_props.py").read())
Set REPO below if the repository is somewhere else.

Nothing here is a neat box: walls lean, ridges sag and edges wander, so the silhouettes look drawn by hand.
Material slot names (Plaster, Roof, Wood, Stone, Metal, Glow, Dark, PigmentOchre, PigmentGreen, PigmentRed)
are what the Unreal look-test script uses to assign painterly materials.
"""

import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

REPO = os.path.expanduser("~/Documents/GitHub/open-world-RPG")
OUT = os.path.join(REPO, "art_source", "looktest", "props")
os.makedirs(OUT, exist_ok=True)

PREVIEW = {
    "Plaster": (0.90, 0.80, 0.56), "Roof": (0.55, 0.12, 0.09), "Wood": (0.15, 0.09, 0.05),
    "Stone": (0.42, 0.36, 0.30), "Metal": (0.03, 0.04, 0.05), "Glow": (1.0, 0.45, 0.10),
    "Dark": (0.04, 0.03, 0.05), "PigmentOchre": (0.70, 0.36, 0.05), "PigmentGreen": (0.14, 0.27, 0.16),
    "PigmentRed": (0.34, 0.03, 0.02), "Foliage": (0.14, 0.27, 0.16), "Erasure": (1.0, 1.0, 1.0),
}


def mat(name):
    m = bpy.data.materials.get(name)
    if m is None:
        m = bpy.data.materials.new(name)
        m.diffuse_color = (*PREVIEW[name], 1.0)
        m.use_nodes = True
        bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Base Color"].default_value = (*PREVIEW[name], 1.0)
    return m


class Builder:
    """Collects pieces into one mesh, each with its material."""

    def __init__(self, name, seed):
        self.name, self.rng = name, random.Random(seed)
        self.bm = bmesh.new()
        self.slots = []

    def slot(self, material):
        if material not in self.slots:
            self.slots.append(material)
        return self.slots.index(material)

    def _finish(self, verts, material, wobble):
        index = self.slot(material)
        faces = {f for v in verts for f in v.link_faces}
        for f in faces:
            f.material_index = index
        for v in verts:
            v.co += Vector((self.rng.uniform(-wobble, wobble), self.rng.uniform(-wobble, wobble), self.rng.uniform(-wobble, wobble)))

    def block(self, centre, size, material, rot=(0, 0, 0), taper=0.0, wobble=0.02, lean=(0, 0)):
        """A box. taper narrows the top (0.1 = 10%); lean shifts the top sideways in metres."""
        result = bmesh.ops.create_cube(self.bm, size=1.0)
        verts = result["verts"]
        for v in verts:
            top = v.co.z > 0
            v.co.x *= size[0] * ((1.0 - taper) if top else 1.0)
            v.co.y *= size[1] * ((1.0 - taper) if top else 1.0)
            v.co.z *= size[2]
            if top:
                v.co.x += lean[0]
                v.co.y += lean[1]
        m = Matrix.Translation(Vector(centre)) @ (Matrix.Rotation(rot[2], 4, "Z") @ Matrix.Rotation(rot[1], 4, "Y") @ Matrix.Rotation(rot[0], 4, "X"))
        bmesh.ops.transform(self.bm, matrix=m, verts=verts)
        self._finish(verts, material, wobble)

    def cylinder(self, base, radius, height, material, segments=8, top_radius=None, wobble=0.015, lean=(0, 0)):
        top_radius = radius if top_radius is None else top_radius
        result = bmesh.ops.create_cone(self.bm, cap_ends=True, segments=segments, radius1=radius, radius2=top_radius, depth=height)
        verts = result["verts"]
        for v in verts:
            if v.co.z > 0:
                v.co.x += lean[0]
                v.co.y += lean[1]
        bmesh.ops.translate(self.bm, verts=verts, vec=Vector((base[0], base[1], base[2] + height / 2.0)))
        self._finish(verts, material, wobble)

    def prism_roof(self, centre, length, width, height, thickness, material, sag=0.12, overhang_wobble=0.06):
        """A gabled roof along X: two thick slabs meeting at a ridge that sags in the middle."""
        hx, hy = length / 2.0, width / 2.0
        for side in (-1, 1):
            verts = []
            for x in (-hx, -hx / 3.0, hx / 3.0, hx):
                dip = sag * (1.0 - (x / hx) ** 2)
                eave_dip = dip * 0.4 + self.rng.uniform(-overhang_wobble, overhang_wobble)
                for (y, z) in ((0.0, height - dip), (side * hy, -eave_dip)):
                    verts.append(self.bm.verts.new((centre[0] + x, centre[1] + y, centre[2] + z)))
                    verts.append(self.bm.verts.new((centre[0] + x, centre[1] + y, centre[2] + z - thickness)))
            # verts per station: ridge top, ridge bottom, eave top, eave bottom
            for i in range(3):
                a, b = i * 4, (i + 1) * 4
                quads = [(a, b, b + 2, a + 2), (a + 1, a + 3, b + 3, b + 1), (a + 2, b + 2, b + 3, a + 3)]
                for q in quads:
                    try:
                        self.bm.faces.new([verts[j] for j in q])
                    except ValueError:
                        pass
            for end in (0, 12):
                try:
                    self.bm.faces.new([verts[end], verts[end + 2], verts[end + 3], verts[end + 1]])
                except ValueError:
                    pass
            self._finish(verts, material, 0.0)

    def gable(self, centre, width, height, depth, material):
        """A triangular wall end under the roof, facing along X."""
        hy = width / 2.0
        pts = [(-hy, 0.0), (hy, 0.0), (0.0, height)]
        verts = []
        for x in (-depth / 2.0, depth / 2.0):
            for (y, z) in pts:
                verts.append(self.bm.verts.new((centre[0] + x, centre[1] + y, centre[2] + z)))
        self.bm.faces.new(verts[0:3])
        self.bm.faces.new(verts[3:6][::-1])
        for i in range(3):
            j = (i + 1) % 3
            self.bm.faces.new([verts[i], verts[i + 3], verts[j + 3], verts[j]])
        self._finish(verts, material, 0.01)

    def build(self):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for material in self.slots:
            mesh.materials.append(mat(material))
        soft = {i for i, m in enumerate(self.slots) if m == "Foliage"}       # foliage is rounded; everything else is faceted
        for poly in mesh.polygons:
            poly.use_smooth = poly.material_index in soft
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def build_house(name, seed, length=7.0, depth=5.5, wall=3.3, door_side=-1):
    """A Varnish house. Front faces -Y (toward the sun). Origin at ground centre."""
    b = Builder(name, seed)
    r = b.rng
    lean = (r.uniform(-0.08, 0.08), r.uniform(-0.06, 0.06))
    b.block((0, 0, 0.3), (length + 0.35, depth + 0.35, 0.6), "Stone", taper=0.03, wobble=0.04)                  # stone footing
    b.block((0, 0, 0.6 + wall / 2.0), (length, depth, wall), "Plaster", taper=0.05, wobble=0.03, lean=lean)        # walls
    roof_z = 0.6 + wall
    for x in (-length / 2.0 + 0.1, length / 2.0 - 0.1):
        b.gable((x + lean[0], lean[1], roof_z - 0.02), depth * 0.95, 2.1, 0.22, "Plaster")
    b.prism_roof((lean[0], lean[1], roof_z - 0.12), length + 1.1, depth + 1.3, 2.45, 0.22, "Roof", sag=0.16)
    b.block((lean[0], lean[1], roof_z + 2.30), (length + 1.2, 0.26, 0.2), "Wood", wobble=0.03)                    # ridge beam

    front = -depth / 2.0
    # Timber frame on the front: corner posts, a sill beam and a head beam.
    for x in (-length / 2.0 + 0.12, length / 2.0 - 0.12):
        b.block((x, front - 0.03, 0.6 + wall / 2.0), (0.26, 0.22, wall), "Wood", wobble=0.02, lean=(lean[0], 0))
    b.block((lean[0] * 0.5, front - 0.04, roof_z - 0.14), (length + 0.1, 0.22, 0.28), "Wood", wobble=0.02)
    b.block((0, front - 0.04, 0.72), (length + 0.05, 0.2, 0.2), "Wood", wobble=0.02)

    # Door: frame, planks and a dark gap above the threshold.
    dx = door_side * length * 0.22
    b.block((dx, front - 0.05, 0.6 + 1.05), (1.25, 0.18, 2.2), "Wood", wobble=0.02)
    b.block((dx, front - 0.10, 0.6 + 1.0), (0.95, 0.12, 1.95), "Dark", wobble=0.01)
    b.block((dx - 0.02, front - 0.14, 0.6 + 0.98), (0.86, 0.08, 1.86), "Wood", rot=(0, 0, 0.05), wobble=0.015)
    b.block((dx, front - 0.25, 0.5), (1.5, 0.5, 0.22), "Stone", wobble=0.03)                                      # doorstep

    # Two windows lit from inside, with a sill and one shutter each.
    for wx in (-door_side * length * 0.12, -door_side * length * 0.36):
        wz = 0.6 + wall * 0.60
        b.block((wx, front - 0.05, wz), (0.95, 0.16, 1.05), "Wood", wobble=0.015)
        b.block((wx, front - 0.10, wz), (0.72, 0.10, 0.82), "Glow", wobble=0.0)
        b.block((wx, front - 0.13, wz), (0.07, 0.08, 0.84), "Wood", wobble=0.0)
        b.block((wx, front - 0.13, wz), (0.74, 0.08, 0.07), "Wood", wobble=0.0)
        b.block((wx, front - 0.16, wz - 0.58), (1.1, 0.26, 0.1), "Wood", wobble=0.02)
        b.block((wx + 0.72, front - 0.12, wz), (0.42, 0.07, 1.0), "Wood", rot=(0, 0, r.uniform(-0.5, -0.2)), wobble=0.015)

    # Chimney, leaning a little.
    cx = -door_side * length * 0.28
    b.block((cx, 0.9, roof_z + 1.9), (0.7, 0.7, 2.2), "Stone", taper=0.12, wobble=0.03, lean=(0.06, -0.03))
    b.block((cx + 0.06, 0.87, roof_z + 3.05), (0.9, 0.9, 0.18), "Stone", wobble=0.03)
    return b.build()


def build_lampstand(name="SM_Lampstand", seed=7):
    """The village Lampstand: a stone plinth, a crooked post, and a caged flame under a small roof."""
    b = Builder(name, seed)
    b.cylinder((0, 0, 0.0), 0.95, 0.28, "Stone", segments=8, top_radius=0.88, wobble=0.03)
    b.cylinder((0, 0, 0.28), 0.66, 0.26, "Stone", segments=8, top_radius=0.58, wobble=0.03)
    b.cylinder((0, 0, 0.54), 0.20, 2.55, "Wood", segments=6, top_radius=0.13, wobble=0.012, lean=(0.10, -0.05))
    for z in (0.75, 1.9, 2.85):                                                 # iron bands
        t = (z - 0.54) / 2.55
        b.cylinder((0.10 * t, -0.05 * t, z), 0.215 - 0.07 * t, 0.07, "Metal", segments=6, wobble=0.0)
    top = Vector((0.10, -0.05, 3.09))
    # Bracket arm reaching toward the square, with a brace.
    b.block((top.x + 0.42, top.y, top.z - 0.06), (1.0, 0.11, 0.11), "Wood", rot=(0, -0.12, 0), wobble=0.01)
    b.block((top.x + 0.30, top.y, top.z - 0.38), (0.72, 0.08, 0.08), "Wood", rot=(0, 0.75, 0), wobble=0.01)
    hang = Vector((top.x + 0.82, top.y, top.z - 0.02))
    b.block((hang.x, hang.y, hang.z - 0.17), (0.03, 0.03, 0.30), "Metal", wobble=0.0)                         # chain
    cage_top = hang.z - 0.32
    b.cylinder((hang.x, hang.y, cage_top), 0.34, 0.16, "Metal", segments=6, top_radius=0.05, wobble=0.01)       # cap
    b.cylinder((hang.x, hang.y, cage_top - 0.46), 0.17, 0.44, "Glow", segments=6, top_radius=0.21, wobble=0.0)  # flame glass
    for i in range(6):                                                                                         # cage bars
        a = i / 6.0 * math.tau
        b.block((hang.x + math.cos(a) * 0.23, hang.y + math.sin(a) * 0.23, cage_top - 0.24), (0.035, 0.035, 0.50), "Metal",
                rot=(0, 0, a), wobble=0.0)
    b.cylinder((hang.x, hang.y, cage_top - 0.54), 0.26, 0.07, "Metal", segments=6, wobble=0.0)                  # base ring
    # Offerings of oil at the foot: two small jars.
    b.cylinder((0.62, -0.42, 0.28), 0.13, 0.26, "PigmentOchre", segments=7, top_radius=0.09, wobble=0.01)
    b.cylinder((0.36, -0.66, 0.28), 0.10, 0.20, "Stone", segments=7, top_radius=0.07, wobble=0.01)
    return b.build()


def build_grinding_slab(name="SM_GrindingSlab", seed=19):
    """Bartram's Grinding Slab: a worn stone table with a muller and bowls of ground pigment."""
    b = Builder(name, seed)
    for x in (-0.75, 0.75):
        b.block((x, 0, 0.36), (0.42, 0.85, 0.72), "Stone", taper=0.1, wobble=0.04)
    b.block((0, 0, 0.80), (2.3, 1.15, 0.18), "Stone", wobble=0.035, rot=(0.0, 0.015, 0.02))
    b.block((0.05, 0.02, 0.895), (1.2, 0.7, 0.02), "PigmentOchre", wobble=0.03, rot=(0, 0, 0.2))             # ground pigment smear
    b.cylinder((0.1, 0.05, 0.90), 0.13, 0.20, "Stone", segments=7, top_radius=0.08, wobble=0.01)                 # muller
    for (x, y, pigment) in ((-0.82, 0.28, "PigmentOchre"), (-0.72, -0.24, "PigmentGreen"), (0.85, 0.22, "PigmentRed")):
        b.cylinder((x, y, 0.89), 0.12, 0.10, "Wood", segments=8, top_radius=0.19, wobble=0.008)                  # bowl
        b.cylinder((x, y, 0.975), 0.165, 0.025, pigment, segments=8, wobble=0.004)                               # pigment
    # A stool and a linseed-oil jug beside it.
    b.cylinder((1.75, -0.2, 0.0), 0.24, 0.46, "Wood", segments=7, top_radius=0.27, wobble=0.015)
    b.cylinder((-1.6, 0.35, 0.0), 0.2, 0.42, "PigmentRed", segments=8, top_radius=0.12, wobble=0.012)
    b.cylinder((-1.6, 0.35, 0.42), 0.09, 0.14, "PigmentRed", segments=8, top_radius=0.11, wobble=0.008)
    return b.build()


def build_tree(name="SM_Tree", seed=31):
    """A small village tree: a leaning trunk and a canopy of rough clumps, like dabs of a loaded brush."""
    b = Builder(name, seed)
    r = b.rng
    b.cylinder((0, 0, 0.0), 0.34, 1.5, "Wood", segments=7, top_radius=0.24, wobble=0.03, lean=(0.18, 0.06))
    b.cylinder((0.18, 0.06, 1.5), 0.24, 1.5, "Wood", segments=7, top_radius=0.15, wobble=0.03, lean=(-0.10, 0.12))
    b.block((0.45, 0.2, 2.5), (1.3, 0.14, 0.14), "Wood", rot=(0, -0.6, 0.4), wobble=0.02)
    b.block((-0.35, 0.0, 2.7), (1.1, 0.12, 0.12), "Wood", rot=(0, 0.7, -0.3), wobble=0.02)
    clumps = [((0.1, 0.2, 3.9), 1.55), ((1.2, 0.5, 3.4), 1.1), ((-1.1, -0.1, 3.5), 1.15), ((0.3, -0.9, 3.6), 1.0),
              ((0.5, 0.9, 4.5), 0.95), ((-0.5, 0.5, 4.6), 0.85), ((0.9, -0.3, 4.3), 0.8)]
    for centre, radius in clumps:
        result = bmesh.ops.create_icosphere(b.bm, subdivisions=2, radius=radius)
        verts = result["verts"]
        for v in verts:
            v.co *= r.uniform(0.82, 1.18)
            v.co.z *= 0.8
        bmesh.ops.translate(b.bm, verts=verts, vec=Vector(centre))
        b._finish(verts, "Foliage", 0.0)
    return b.build()


def build_blotter(name="SM_Blotter", seed=44):
    """Style probe, first enemy: the Blotter. A tall figure of wound rag with no face, bowed over its work, with
    cloth arms long enough to dab the ground. The only pure-white thing on screen. Faces -Y."""
    b = Builder(name, seed)
    r = b.rng
    # The body: rag wound in uneven turns, wider at the hem, each turn a little off the last.
    turns = [(0.00, 0.50, 0.92, 0.80), (0.42, 0.46, 0.78, 0.66), (0.80, 0.44, 0.64, 0.56), (1.16, 0.42, 0.54, 0.46), (1.50, 0.40, 0.48, 0.40)]
    for i, (z, h, wx, wy) in enumerate(turns):
        b.block((r.uniform(-0.04, 0.04), -0.03 * i, z + h / 2.0), (wx, wy, h), "Erasure", rot=(0, 0, 0.35 * (i % 2) - 0.17 + r.uniform(-0.1, 0.1)),
                taper=0.14, wobble=0.025, lean=(r.uniform(-0.04, 0.04), -0.03))
    # Shoulders and a bowed, hooded head with nothing in it.
    b.block((0.0, -0.20, 2.02), (0.74, 0.40, 0.34), "Erasure", rot=(-0.35, 0, 0.05), taper=0.2, wobble=0.02)
    b.cylinder((0.0, -0.36, 2.10), 0.21, 0.42, "Erasure", segments=6, top_radius=0.13, wobble=0.015, lean=(0.02, -0.22))
    b.block((0.0, -0.50, 2.22), (0.24, 0.10, 0.26), "Dark", rot=(-0.5, 0, 0), wobble=0.0)          # the dark inside the hood
    # Arms: three lengths of cloth each, hanging forward to the ground, ending in a wad.
    for side in (-1, 1):
        x = side * 0.40
        b.block((x, -0.34, 1.72), (0.17, 0.20, 0.62), "Erasure", rot=(-0.5, 0, side * 0.10), taper=0.1, wobble=0.015)
        b.block((x * 1.12, -0.62, 1.12), (0.15, 0.17, 0.74), "Erasure", rot=(-0.28, 0, side * 0.06), taper=0.1, wobble=0.015)
        b.block((x * 1.18, -0.78, 0.48), (0.13, 0.15, 0.72), "Erasure", rot=(-0.10, 0, side * 0.04), taper=0.0, wobble=0.015)
        b.block((x * 1.2, -0.84, 0.10), (0.30, 0.34, 0.20), "Erasure", rot=(0, 0, r.uniform(-0.6, 0.6)), wobble=0.03)   # the wad it dabs with
    # Loose ends trailing behind.
    for (x, y, length, yaw) in ((-0.22, 0.42, 1.0, 0.3), (0.15, 0.48, 1.3, -0.2), (0.34, 0.36, 0.8, 0.5)):
        b.block((x, y, length / 2.0 + 0.02), (0.16, 0.035, length), "Erasure", rot=(0.22, 0, yaw), wobble=0.02)
    return b.build()


def build_hesper(name="SM_Hesper", seed=61):
    """Style probe, first person: Hesper Vane, lampwright. Sixty-odd, sturdy, a little stooped from thirty years of
    looking up at a lamp. Long ochre work dress, cream apron, a madder-rose shawl, grey hair pinned up, and the
    lighting pole she is never without, its wick still glowing. Faces -Y."""
    b = Builder(name, seed)
    r = b.rng
    # The dress: three uneven tiers, widest at the hem, leaning forward a touch.
    for i, (z, h, wx, wy) in enumerate(((0.06, 0.56, 0.74, 0.62), (0.60, 0.44, 0.60, 0.50), (1.02, 0.36, 0.50, 0.40))):
        b.block((0.0, -0.02 * i, z + h / 2.0), (wx, wy, h), "PigmentOchre", rot=(0, 0, r.uniform(-0.08, 0.08)), taper=0.16, wobble=0.02, lean=(0.0, -0.025))
    # Boots, just showing.
    for side in (-1, 1):
        b.block((side * 0.15, -0.16, 0.05), (0.15, 0.30, 0.10), "Wood", rot=(0, 0, side * 0.12), wobble=0.01)
    # The apron: a cream panel down the front, and its tie at the back.
    b.block((0.0, -0.30, 0.74), (0.40, 0.035, 0.92), "Plaster", rot=(-0.07, 0, 0.02), taper=-0.12, wobble=0.012)
    b.block((0.0, 0.24, 1.06), (0.26, 0.05, 0.10), "Plaster", rot=(0, 0, 0.2), wobble=0.01)
    # Shoulders rounded forward under the shawl, which hangs longer on one side (she pins it badly).
    b.block((0.0, -0.05, 1.44), (0.62, 0.40, 0.24), "Roof", rot=(-0.22, 0, 0.03), taper=0.25, wobble=0.02)
    b.block((-0.24, -0.22, 1.18), (0.20, 0.06, 0.52), "Roof", rot=(-0.12, 0, 0.10), taper=0.2, wobble=0.015)
    b.block((0.22, -0.22, 1.28), (0.18, 0.06, 0.32), "Roof", rot=(-0.12, 0, -0.12), taper=0.2, wobble=0.015)
    b.block((0.0, 0.18, 1.30), (0.50, 0.05, 0.42), "Roof", rot=(0.10, 0, 0), taper=0.3, wobble=0.015)
    # Neck and head, tipped up toward the lamp out of habit; grey hair in a bun, a loose strand.
    b.cylinder((0.0, -0.10, 1.54), 0.07, 0.10, "Plaster", segments=6, wobble=0.005)
    b.block((0.0, -0.14, 1.74), (0.21, 0.23, 0.26), "Plaster", rot=(0.16, 0, 0.04), taper=0.12, wobble=0.012)
    b.block((0.0, -0.08, 1.86), (0.24, 0.26, 0.10), "Stone", rot=(0.16, 0, 0.04), taper=0.2, wobble=0.012)
    b.block((0.0, 0.04, 1.76), (0.23, 0.10, 0.22), "Stone", rot=(0.16, 0, 0.0), wobble=0.012)
    b.cylinder((0.0, 0.10, 1.84), 0.085, 0.11, "Stone", segments=6, top_radius=0.06, wobble=0.008, lean=(0.0, 0.03))
    b.block((0.115, -0.20, 1.70), (0.02, 0.03, 0.16), "Stone", rot=(0.1, 0, -0.1), wobble=0.004)
    # Her left arm hangs with an oil can; the sleeve is rolled.
    b.block((-0.36, -0.06, 1.22), (0.13, 0.15, 0.50), "PigmentOchre", rot=(0.08, 0, -0.10), taper=0.1, wobble=0.012)
    b.block((-0.40, -0.10, 0.90), (0.10, 0.11, 0.26), "Plaster", rot=(0.05, 0, -0.06), wobble=0.01)
    b.cylinder((-0.41, -0.14, 0.62), 0.07, 0.16, "Metal", segments=6, top_radius=0.05, wobble=0.006)
    b.block((-0.41, -0.24, 0.74), (0.02, 0.16, 0.02), "Metal", rot=(0.5, 0, 0), wobble=0.002)
    # Her right arm is raised, holding the lighting pole upright beside her.
    b.block((0.36, -0.12, 1.34), (0.13, 0.15, 0.34), "PigmentOchre", rot=(-0.9, 0, 0.12), taper=0.1, wobble=0.012)
    b.block((0.40, -0.32, 1.44), (0.10, 0.11, 0.24), "Plaster", rot=(-0.3, 0, 0.05), wobble=0.01)
    b.cylinder((0.42, -0.38, 0.02), 0.022, 2.55, "Wood", segments=5, top_radius=0.018, wobble=0.004, lean=(0.03, 0.02))
    b.block((0.46, -0.37, 2.58), (0.10, 0.02, 0.02), "Metal", wobble=0.002)                 # the hook for lifting lamp glasses
    b.cylinder((0.45, -0.36, 2.58), 0.03, 0.09, "Metal", segments=5, wobble=0.003)          # the wick holder
    b.block((0.45, -0.36, 2.71), (0.05, 0.05, 0.10), "Glow", rot=(0, 0, 0.6), taper=0.6, wobble=0.004)   # the taper, lit
    return b.build()


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT, obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="-Y", axis_up="Z", mesh_smooth_type="OFF", use_mesh_modifiers=True,
                             bake_space_transform=True, object_types={"MESH"})
    return path


def main():
    for name in ("SM_House_A", "SM_House_B", "SM_Lampstand", "SM_GrindingSlab", "SM_Tree", "SM_Blotter", "SM_Hesper"):
        old = bpy.data.objects.get(name)
        if old:
            bpy.data.objects.remove(old, do_unlink=True)
    built = [
        build_house("SM_House_A", seed=3, length=7.0, depth=5.5, wall=3.3, door_side=-1),
        build_house("SM_House_B", seed=8, length=5.6, depth=5.0, wall=3.0, door_side=1),
        build_lampstand(),
        build_grinding_slab(),
        build_tree(),
        build_blotter(),
        build_hesper(),
    ]
    for i, obj in enumerate(built):
        path = export(obj)
        obj.location.x = (i - 1.5) * 9.0      # lay them out side by side for viewing
        print("exported", path, "tris", sum(len(p.vertices) - 2 for p in obj.data.polygons), "slots", [m.name for m in obj.data.materials])


main()
