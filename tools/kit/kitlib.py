"""The environment kit's toolbox, for Blender. Every kit script starts by running this file.

    exec(open("<repo>/tools/kit/kitlib.py").read())

It is the look test's way of modelling (tools/looktest/blender_build_props.py) made reusable: pieces are put
together from crooked blocks, tapered posts and sagging roofs, so nothing is square, new or matched
(docs/ART_BIBLE.md, section 1). Pieces carry no textures. Their material SLOT NAMES (Plaster, Roof, Wood, Stone,
Metal, Glow, Dark, Cloth, Foliage, Ground, Canvas, Bone, Paper, and pigment names) are what Unreal paints, with the
Glaze material in each zone's palette (tools/kit/unreal_kit.py).

Units are metres. A piece's origin is on the ground at its centre unless its function says otherwise.
Front faces -Y, as in the look test.
"""

import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

REPO = os.path.expanduser("~/Documents/GitHub/open-world-RPG")
KIT_OUT = os.path.join(REPO, "art_source", "kit")

PREVIEW = {
    "Plaster": (0.90, 0.80, 0.56), "Roof": (0.55, 0.12, 0.09), "Wood": (0.15, 0.09, 0.05), "Stone": (0.33, 0.29, 0.40),
    "Metal": (0.03, 0.04, 0.05), "Glow": (1.0, 0.45, 0.10), "Dark": (0.04, 0.03, 0.05), "Cloth": (0.55, 0.12, 0.09),
    "ClothTeal": (0.03, 0.11, 0.15), "ClothOchre": (0.70, 0.36, 0.05), "Foliage": (0.14, 0.27, 0.16), "FoliageLight": (0.33, 0.44, 0.25),
    "Ground": (0.75, 0.58, 0.42), "Canvas": (0.69, 0.60, 0.44), "Charcoal": (0.04, 0.04, 0.04), "Bone": (0.62, 0.55, 0.39),
    "Paper": (0.79, 0.76, 0.67), "Graphite": (0.03, 0.03, 0.03), "Blue": (0.03, 0.07, 0.38), "Moss": (0.33, 0.44, 0.25),
    "Bark": (0.15, 0.09, 0.05), "Black": (0.01, 0.01, 0.012), "Teal": (0.03, 0.11, 0.15), "Water": (0.10, 0.22, 0.26),
    "PigmentOchre": (0.70, 0.36, 0.05), "PigmentGreen": (0.14, 0.27, 0.16), "PigmentRed": (0.34, 0.03, 0.02),
    "Paving": (0.6, 0.5, 0.45), "Path": (0.85, 0.66, 0.42), "Cliff": (0.15, 0.09, 0.05), "Line": (0.03, 0.03, 0.03), "BlueLine": (0.03, 0.07, 0.38), "Heart": (0.80, 0.29, 0.04), "Scab": (0.10, 0.04, 0.03),
    "PigmentBlack": (0.02, 0.02, 0.02), "PigmentBlue": (0.03, 0.07, 0.38), "Sanguine": (0.34, 0.03, 0.02), "Erasure": (1.0, 1.0, 1.0),
}


def mat(name):
    m = bpy.data.materials.get(name)
    if m is None:
        m = bpy.data.materials.new(name)
        colour = PREVIEW.get(name, (0.5, 0.5, 0.5))
        m.diffuse_color = (*colour, 1.0)
        m.use_nodes = True
        bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Base Color"].default_value = (*colour, 1.0)
        bsdf.inputs["Roughness"].default_value = 1.0
    return m


class Builder:
    """Collects pieces into one mesh, each with its material slot."""

    def __init__(self, name, seed):
        self.name, self.rng = name, random.Random(seed)
        self.bm = bmesh.new()
        self.slots = []
        self.soft = {"Foliage", "FoliageLight", "Moss", "Heart"}        # rounded things; everything else is faceted

    def slot(self, material):
        if material not in self.slots:
            self.slots.append(material)
        return self.slots.index(material)

    def _finish(self, verts, material, wobble):
        index = self.slot(material)
        for f in {f for v in verts for f in v.link_faces}:
            f.material_index = index
        for v in verts:
            v.co += Vector((self.rng.uniform(-wobble, wobble), self.rng.uniform(-wobble, wobble), self.rng.uniform(-wobble, wobble)))

    def block(self, centre, size, material, rot=(0, 0, 0), taper=0.0, wobble=0.02, lean=(0, 0)):
        """A box. taper narrows the top (0.1 = 10%); lean shifts the top sideways in metres."""
        verts = bmesh.ops.create_cube(self.bm, size=1.0)["verts"]
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

    def cylinder(self, base, radius, height, material, segments=8, top_radius=None, wobble=0.015, lean=(0, 0), rot=(0, 0, 0)):
        top_radius = radius if top_radius is None else top_radius
        verts = bmesh.ops.create_cone(self.bm, cap_ends=True, segments=segments, radius1=radius, radius2=top_radius, depth=height)["verts"]
        for v in verts:
            if v.co.z > 0:
                v.co.x += lean[0]
                v.co.y += lean[1]
            v.co.z += height / 2.0
        m = Matrix.Translation(Vector(base)) @ (Matrix.Rotation(rot[2], 4, "Z") @ Matrix.Rotation(rot[1], 4, "Y") @ Matrix.Rotation(rot[0], 4, "X"))
        bmesh.ops.transform(self.bm, matrix=m, verts=verts)
        self._finish(verts, material, wobble)

    def blob(self, centre, radius, material, squash=(1.0, 1.0, 0.8), detail=2, wobble=0.12):
        """A rough lump: a clump of leaves, a rock, a sack. wobble is a share of the radius."""
        verts = bmesh.ops.create_icosphere(self.bm, subdivisions=detail, radius=radius)["verts"]
        for v in verts:
            push = 1.0 + self.rng.uniform(-wobble, wobble)
            v.co = Vector((v.co.x * squash[0] * push, v.co.y * squash[1] * push, v.co.z * squash[2] * push)) + Vector(centre)
        self._finish(verts, material, 0.0)

    def tube(self, points, radius, material, end_radius=None, segments=6, wobble=0.01):
        """A bent limb through the given points: a root, a rib, a rope, a bough. Tapers from radius to end_radius."""
        end_radius = radius if end_radius is None else end_radius
        pts = [Vector(p) for p in points]
        for i in range(len(pts) - 1):
            a, b = pts[i], pts[i + 1]
            along = b - a
            if along.length < 1e-4:
                continue
            r0 = radius + (end_radius - radius) * i / (len(pts) - 1)
            r1 = radius + (end_radius - radius) * (i + 1) / (len(pts) - 1)
            length = along.length * 1.08                         # overlap the next piece a little, so joints do not gape
            verts = bmesh.ops.create_cone(self.bm, cap_ends=True, segments=segments, radius1=r0, radius2=r1, depth=length)["verts"]
            turn = Vector((0, 0, 1)).rotation_difference(along.normalized()).to_matrix().to_4x4()
            bmesh.ops.transform(self.bm, matrix=Matrix.Translation((a + b) / 2.0) @ turn, verts=verts)
            self._finish(verts, material, wobble)

    def arc(self, centre, radius, start, end, steps, plane="XZ"):
        """Points along an arc, for tube(): angles in radians, in the XZ plane (an arch you walk through along Y) or XY."""
        out = []
        for i in range(steps + 1):
            t = start + (end - start) * i / steps
            if plane == "XZ":
                out.append((centre[0] + math.cos(t) * radius, centre[1], centre[2] + math.sin(t) * radius))
            else:
                out.append((centre[0] + math.cos(t) * radius, centre[1] + math.sin(t) * radius, centre[2]))
        return out

    def prism_roof(self, centre, length, width, height, thickness, material, sag=0.12, overhang_wobble=0.06, yaw=0.0):
        """A gabled roof along X: two thick slabs meeting at a ridge that sags in the middle."""
        hx, hy = length / 2.0, width / 2.0
        made = []
        for side in (-1, 1):
            verts = []
            for x in (-hx, -hx / 3.0, hx / 3.0, hx):
                dip = sag * (1.0 - (x / hx) ** 2)
                eave_dip = dip * 0.4 + self.rng.uniform(-overhang_wobble, overhang_wobble)
                for (y, z) in ((0.0, height - dip), (side * hy, -eave_dip)):
                    verts.append(self.bm.verts.new((x, y, z)))
                    verts.append(self.bm.verts.new((x, y, z - thickness)))
            for i in range(3):
                a, b = i * 4, (i + 1) * 4
                for q in ((a, b, b + 2, a + 2), (a + 1, a + 3, b + 3, b + 1), (a + 2, b + 2, b + 3, a + 3)):
                    try:
                        self.bm.faces.new([verts[j] for j in q])
                    except ValueError:
                        pass
            for end in (0, 12):
                try:
                    self.bm.faces.new([verts[end], verts[end + 2], verts[end + 3], verts[end + 1]])
                except ValueError:
                    pass
            made += verts
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(centre)) @ Matrix.Rotation(yaw, 4, "Z"), verts=made)
        self._finish(made, material, 0.0)

    def lean_to_roof(self, centre, length, width, drop, thickness, material, yaw=0.0):
        """A single sloping slab along X, high at +Y and dropping toward -Y: a porch, an awning, a shed."""
        hx, hy = length / 2.0, width / 2.0
        verts = []
        for x in (-hx, hx):
            for (y, z) in ((hy, 0.0), (-hy, -drop + self.rng.uniform(-0.04, 0.04))):
                verts.append(self.bm.verts.new((x, y, z)))
                verts.append(self.bm.verts.new((x, y, z - thickness)))
        for q in ((0, 4, 6, 2), (1, 3, 7, 5), (0, 1, 5, 4), (2, 6, 7, 3), (0, 2, 3, 1), (4, 5, 7, 6)):
            self.bm.faces.new([verts[j] for j in q])
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(centre)) @ Matrix.Rotation(yaw, 4, "Z"), verts=verts)
        self._finish(verts, material, 0.0)

    def gable(self, centre, width, height, depth, material, yaw=0.0):
        """A triangular wall end under the roof, facing along X."""
        hy = width / 2.0
        verts = []
        for x in (-depth / 2.0, depth / 2.0):
            for (y, z) in ((-hy, 0.0), (hy, 0.0), (0.0, height)):
                verts.append(self.bm.verts.new((x, y, z)))
        self.bm.faces.new(verts[0:3])
        self.bm.faces.new(verts[3:6][::-1])
        for i in range(3):
            j = (i + 1) % 3
            self.bm.faces.new([verts[i], verts[i + 3], verts[j + 3], verts[j]])
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(centre)) @ Matrix.Rotation(yaw, 4, "Z"), verts=verts)
        self._finish(verts, material, 0.01)

    def wall(self, start, end, height, thickness, material, base=0.0, openings=(), frame="Wood", wobble=0.02):
        """A wall from start to end (x, y) with holes in it. openings: (distance along the wall to the hole's centre,
        width, sill height, head height). A sill of 0 is a doorway. Each hole gets a timber frame."""
        sx, sy = start
        ex, ey = end
        length = math.hypot(ex - sx, ey - sy)
        yaw = math.atan2(ey - sy, ex - sx)

        def piece(a, b, z0, z1, mat_name, thick=thickness, out=0.0, w=wobble):
            if b - a < 0.02 or z1 - z0 < 0.02:
                return
            mid = (a + b) / 2.0
            cx, cy = sx + math.cos(yaw) * mid - math.sin(yaw) * out, sy + math.sin(yaw) * mid + math.cos(yaw) * out
            self.block((cx, cy, base + (z0 + z1) / 2.0), (b - a, thick, z1 - z0), mat_name, rot=(0, 0, yaw), wobble=w)

        cuts = sorted(openings)
        cursor = 0.0
        for centre, width, sill, head in cuts:
            a, b = centre - width / 2.0, centre + width / 2.0
            piece(cursor, a, 0.0, height, material)                      # solid wall up to the hole
            piece(a, b, 0.0, sill, material)                             # under a window
            piece(a, b, head, height, material)                          # over the hole
            if frame:
                piece(a - 0.09, a + 0.03, sill, head + 0.1, frame, thickness + 0.08, w=0.012)
                piece(b - 0.03, b + 0.09, sill, head + 0.1, frame, thickness + 0.08, w=0.012)
                piece(a - 0.12, b + 0.12, head, head + 0.14, frame, thickness + 0.10, w=0.012)
                if sill > 0.05:
                    piece(a - 0.14, b + 0.14, sill - 0.09, sill, frame, thickness + 0.22, w=0.012)
            cursor = b
        piece(cursor, length, 0.0, height, material)

    def build(self):
        bmesh.ops.remove_doubles(self.bm, verts=self.bm.verts, dist=0.0005)
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for material in self.slots:
            mesh.materials.append(mat(material))
        soft = {i for i, m in enumerate(self.slots) if m in self.soft}
        for poly in mesh.polygons:
            poly.use_smooth = poly.material_index in soft
        old = bpy.data.objects.get(self.name)
        if old:
            bpy.data.objects.remove(old, do_unlink=True)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def kit_export(obj, zone):
    """Writes one piece to art_source/kit/<zone>/<name>.fbx."""
    folder = os.path.join(KIT_OUT, zone)
    os.makedirs(folder, exist_ok=True)
    for other in bpy.data.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(folder, obj.name + ".fbx")
    import contextlib
    import io
    with contextlib.redirect_stdout(io.StringIO()):          # the exporter is chatty; forty pieces of it is too much
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                                 axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", use_mesh_modifiers=True,
                                 bake_space_transform=True, object_types={"MESH"})
    return path


def kit_finish(zone, pieces, per_row=8, gap=9.0):
    """Exports every piece, lays them out in rows to look at, and writes the list Unreal imports from."""
    import json
    listing = []
    for index, obj in enumerate(pieces):
        kit_export(obj, zone)
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        size = [round(d, 2) for d in obj.dimensions]
        listing.append({"name": obj.name, "triangles": tris, "size_m": size, "slots": [m.name for m in obj.data.materials]})
        obj.location = ((index % per_row) * gap, -(index // per_row) * gap * 1.2, 0.0)
    json.dump(listing, open(os.path.join(KIT_OUT, zone, "kit.json"), "w"), indent=1)
    print("KIT", zone, len(pieces), "pieces,", sum(p["triangles"] for p in listing), "triangles in all; biggest",
          max(listing, key=lambda p: p["triangles"])["name"], max(p["triangles"] for p in listing))
    return listing
