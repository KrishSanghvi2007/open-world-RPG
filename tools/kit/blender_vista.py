"""What lies round Varnish: her shoulder as ground, the other colossi on the horizon, and the Lamp
(docs/ART_BIBLE.md section 10).

Run inside Blender, in an empty file:
    exec(open("<repo>/tools/kit/kitlib.py").read()); exec(open("<repo>/tools/kit/blender_vista.py").read())

The village's terraces are a grey-box slab each, and stay so (they are what is walked on). This makes the body they
are cut into: it comes up to just outside the parapets and falls away in a long curve on every side. Skin, not rock.
Nothing here can be reached, so nothing here has collision.
"""

ZONE = "Vista"

exec(open(os.path.join(REPO, "tools", "kit", "shoulder.py")).read())      # FOOT_X, FOOT_Y0, FOOT_Y1, shoulder_height


def _lines(lo, hi, far, count, keep):
    """Grid lines: close together near the village, wide apart far away, with the lines in keep exactly on them."""
    out = set(keep)
    for i in range(count + 1):
        u = -1.0 + 2.0 * i / count
        mid, half = (lo + hi) / 2.0, (hi - lo) / 2.0
        out.add(round(mid + (half + 20.0) * u + (far - half - 20.0) * (u ** 3), 2))
    out = sorted(out)
    merged = [out[0]]
    for value in out[1:]:
        if value - merged[-1] < 0.25 and value not in keep:
            continue
        merged.append(value)
    return merged


def terrain(name="SM_Terrain_Varnish", reach=1100.0):
    xs = _lines(-FOOT_X, FOOT_X, reach, 76, [-FOOT_X - 0.6, -FOOT_X, FOOT_X, FOOT_X + 0.6])
    ys = _lines(FOOT_Y0, FOOT_Y1, reach, 76, [FOOT_Y0 - 0.6, FOOT_Y0, FOOT_Y1, FOOT_Y1 + 0.6, -1.0, 1.5, 4.0, 6.5, 9.0])
    bm = bmesh.new()
    # The export mirrors Y, so the ground is built mirrored here and arrives in Unreal the right way round.
    grid = [[bm.verts.new((x, -y, shoulder_height(x, y))) for x in xs] for y in ys]
    for j in range(len(ys) - 1):
        for i in range(len(xs) - 1):
            inside = abs(xs[i]) < FOOT_X and abs(xs[i + 1]) <= FOOT_X and FOOT_Y0 <= ys[j] and ys[j + 1] <= FOOT_Y1
            if inside and abs(xs[i]) < FOOT_X - 3 and FOOT_Y0 + 3 < ys[j] < FOOT_Y1 - 3:
                continue                                    # nothing is ever seen under the middle of the terraces
            bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mat("Hide"))
    for poly in mesh.polygons:
        poly.use_smooth = True                              # skin: no facets
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    if obj.data.polygons[0].normal.z < 0:
        obj.data.flip_normals()
    return obj


def silhouette(name, profile, width, height, slot="Silhouette"):
    """A colossus on the horizon: a flat shape cut along its back-line, the way a distant hill is painted.
    profile is (across, up) pairs from 0 to 1. Stands in the XZ plane, facing -Y, its foot at z = 0."""
    bm = bmesh.new()
    top = [bm.verts.new(((u - 0.5) * width, 0.0, v * height)) for (u, v) in profile]
    low = [bm.verts.new(((u - 0.5) * width, 0.0, -0.15 * height)) for (u, v) in profile]
    back_top = [bm.verts.new(v.co) for v in top]
    back_low = [bm.verts.new(v.co) for v in low]
    for i in range(len(profile) - 1):
        bm.faces.new((low[i], low[i + 1], top[i + 1], top[i]))
        bm.faces.new((back_top[i], back_top[i + 1], back_low[i + 1], back_low[i]))      # seen from either side
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mat(slot))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def far_plain(name="SM_Vista_Plain"):
    """The land far below, out to the horizon: a flat disc for the other colossi to stand on. One unit across; scaled in the level."""
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, segments=48, radius=1.0)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mat("Plain"))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    if obj.data.polygons[0].normal.z < 0:
        obj.data.flip_normals()
    return obj


def lamp_disc(name="SM_Vista_Lamp", radius=1.0):
    bm = bmesh.new()
    for turn in (math.pi / 2.0, -math.pi / 2.0):                                             # seen from either side
        bmesh.ops.create_circle(bm, cap_ends=True, segments=40, radius=radius, matrix=Matrix.Rotation(turn, 4, "X"))
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mat("Lamp"))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


PREVIEW.update({"Hide": (0.48, 0.30, 0.14), "Silhouette": (0.6, 0.45, 0.5), "Lamp": (1.0, 0.9, 0.7)})

# Each back-line is different, so each reads as someone (art bible, section 10).
SLEEPER = [(0.0, 0.0), (0.06, 0.10), (0.14, 0.30), (0.22, 0.52), (0.30, 0.64), (0.40, 0.58), (0.50, 0.50), (0.60, 0.55), (0.68, 0.70), (0.74, 0.86),
           (0.79, 1.0), (0.83, 0.94), (0.87, 0.66), (0.92, 0.34), (0.96, 0.14), (1.0, 0.0)]                      # lying on her side: hip, waist, shoulder
KNEELER = [(0.0, 0.0), (0.10, 0.08), (0.20, 0.20), (0.30, 0.42), (0.38, 0.70), (0.44, 0.92), (0.50, 1.0), (0.55, 0.96), (0.60, 0.74), (0.64, 0.80),
           (0.69, 0.72), (0.74, 0.48), (0.82, 0.30), (0.90, 0.16), (1.0, 0.0)]                                    # folded forward, head on knees
STANDER = [(0.0, 0.0), (0.18, 0.04), (0.30, 0.10), (0.36, 0.34), (0.40, 0.66), (0.43, 0.84), (0.46, 0.90), (0.48, 1.0), (0.52, 0.99), (0.55, 0.88),
           (0.60, 0.82), (0.66, 0.60), (0.70, 0.30), (0.78, 0.12), (0.90, 0.05), (1.0, 0.0)]                      # upright: the Wader, who walks
for _old in [o for o in bpy.data.objects if o.name.startswith(("SM_Terrain", "SM_Vista"))]:
    bpy.data.objects.remove(_old, do_unlink=True)
pieces = [
    terrain(),
    silhouette("SM_Vista_Sleeper", SLEEPER, 5200.0, 620.0),
    silhouette("SM_Vista_Kneeler", KNEELER, 2600.0, 880.0),
    silhouette("SM_Vista_Wader", STANDER, 1500.0, 1350.0),
    lamp_disc(),
    far_plain(),
]
KIT = kit_finish(ZONE, pieces, per_row=1, gap=0.0)
for obj in pieces:
    obj.location = (0, 0, 0)
print("terrain triangles", KIT[0]["triangles"], "size", KIT[0]["size_m"])
