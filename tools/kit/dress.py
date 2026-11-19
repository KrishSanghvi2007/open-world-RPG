"""Dresses a zone: puts the kit's art on top of the grey-box layout. Called by the level builder
(tools/greybox/build_matriarch_greybox.py) at the end of each level, so a rebuilt level is always dressed.

The rule that keeps this safe: **gameplay is never moved.** The grey-box shapes stay where they are and stay solid;
this pass only hides how they look and stands kit pieces over them. Things the player uses (Lampstands, nodes,
stations, notes, caches, doors) keep their place and are given a "skin". Solid furniture goes only where the routes
do not (tools/dev/check_route.py lists the routes), and a scripted playthrough is re-run after each zone.

Everything here is in metres, in the level's own coordinates, like the builder.
To build a level without its dressing (to compare, or to debug the layout), pass "grey" to the builder.
"""

import math
import os
import random
import sys

import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
for folder in ("stylekit", "kit"):
    path = os.path.join(ROOT, "tools", folder)
    if path not in sys.path:
        sys.path.insert(0, path)
import stylekit                     # noqa: E402
import unreal_kit                   # noqa: E402
import shoulder                     # noqa: E402

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary


# Small or flat things whose shadows nobody would miss. Every shadow is drawn again for each band of the sun's shadow.
NO_SHADOW = ("SM_Jars", "SM_Basket", "SM_PotPlant", "SM_Paving_Patch", "SM_Paving_Ring", "SM_PathWorn_A", "SM_PathWorn_B", "SM_CanvasEdge",
             "SM_Tuft_A", "SM_Tuft_B", "SM_RampSkin", "SM_Rocks_A", "SM_MossPatch", "SM_GrassTuft", "SM_GrassTuft_Tall", "SM_Fern", "SM_Fern_B",
             "SM_Boulder_XS", "SM_EraserCrumbs", "SM_Smudge", "SM_Node_Ochre", "SM_Cache")


def v(p):
    return unreal.Vector(p[0] * 100.0, p[1] * 100.0, p[2] * 100.0)


class Dresser:
    def __init__(self, zone, seed, shadow, area_of=None):
        self.zone, self.rng, self.shadow, self.count = zone, random.Random(seed), shadow, 0
        self.area_of = area_of or (lambda pos: "All")     # which part of the zone a point belongs to, for merging
        self.loose = {}                                    # (area, casts a shadow) -> pieces to be merged into one mesh
        self.level = {}
        for actor in actors.get_all_level_actors():
            self.level.setdefault(actor.get_actor_label(), actor)

    # ---- the grey box -------------------------------------------------------------------------------------------

    def hide(self, *prefixes):
        """Stops grey-box shapes being seen. They stay solid."""
        n = 0
        for label, actor in self.level.items():
            if label.startswith(prefixes) and isinstance(actor, unreal.StaticMeshActor):
                actor.static_mesh_component.set_visibility(False)
                actor.static_mesh_component.set_cast_shadow(False)
                n += 1
        return n

    def unsolid(self, *labels):
        """For the few grey-box shapes that the art replaces as a solid too (a house you can now walk into)."""
        for label in labels:
            actor = self.level.get(label)
            if actor:
                actor.static_mesh_component.set_collision_profile_name("NoCollision")

    def repaint(self, prefix, hex_colour, surface="Ground"):
        for label, actor in self.level.items():
            if label.startswith(prefix) and isinstance(actor, unreal.StaticMeshActor):
                actor.static_mesh_component.set_material(0, stylekit.glaze(hex_colour, surface, self.shadow))

    def hide_markers(self):
        """Dev labels and placeholder markers have done their job. Hidden, not deleted: un-hide to debug."""
        for actor in actors.get_all_level_actors():
            folder = str(actor.get_folder_path())
            if folder.startswith("Markers"):
                actor.set_actor_hidden_in_game(True)
                for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
                    comp.set_visibility(False)

    # ---- kit pieces ---------------------------------------------------------------------------------------------

    def put(self, piece, pos, yaw=0.0, scale=(1.0, 1.0, 1.0), pitch=0.0, roll=0.0, kit=None, palette=None, folder="Dressing", shadow=True, merge=True):
        mesh = eal.load_asset("/Game/Kit/%s/%s" % (kit or self.zone, piece))
        if mesh is None:
            unreal.log_error("DRESS missing piece %s" % piece)
            return None
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, v(pos), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        self.count += 1
        actor.set_actor_label("Dress_%s_%03d" % (piece.replace("SM_", ""), self.count))
        actor.set_folder_path(folder)
        actor.tags = [unreal.Name("Dressing")]
        if piece not in unreal_kit.SOLID_BOX + unreal_kit.SOLID_EXACT + unreal_kit.WALK_IN:
            comp.set_collision_profile_name("NoCollision")
        if not shadow or piece in NO_SHADOW:
            comp.set_cast_shadow(False)
        if palette:
            names = [str(slot.get_editor_property("material_slot_name")) for slot in mesh.get_editor_property("static_materials")]
            for index, name in enumerate(names):
                comp.set_material(index, unreal_kit.paint(kit or self.zone, name, palette))
        solid = piece in unreal_kit.SOLID_BOX + unreal_kit.SOLID_EXACT
        if merge and not solid:
            self.loose.setdefault((self.area_of(pos), bool(shadow and piece not in NO_SHADOW)), []).append(actor)
        return actor

    def finish(self, level_name):
        """Welds each area's scenery into one mesh (two: one that casts shadows, one that does not).
        A village of several hundred separate pieces is several thousand things for the Mac to draw each frame, and it
        was the difference between 53 frames a second and 41. Welded, it is a few dozen. Solid props stay separate, so
        they keep their own collision; so does anything placed with merge=False."""
        sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        welded = 0
        for (area, casts), group in sorted(self.loose.items()):
            if len(group) < 2:
                continue
            name = "%s_%s_%s" % (level_name.replace("L_Matriarch_", ""), area, "Shadow" if casts else "Flat")
            path = "/Game/Maps/Matriarch/Dressed/" + name          # the engine saves it as SM_<name>
            if eal.does_asset_exist("/Game/Maps/Matriarch/Dressed/SM_" + name):
                eal.delete_asset("/Game/Maps/Matriarch/Dressed/SM_" + name)
            options = unreal.MergeStaticMeshActorsOptions()
            options.base_package_name = path
            options.destroy_source_actors = True
            options.new_actor_label = "Dressed_%s_%s" % (area, "Shadow" if casts else "Flat")
            options.spawn_merged_actor = True
            settings = unreal.MeshMergingSettings()
            settings.merge_materials = False              # keep each paint as it is; pieces sharing a paint are drawn together
            settings.generate_light_map_uv = False
            settings.bake_vertex_data_to_mesh = False
            options.mesh_merging_settings = settings
            merged = sm.merge_static_mesh_actors(group, options)
            if merged is None:
                unreal.log_error("DRESS could not weld %s" % name)
                continue
            comp = merged.static_mesh_component
            comp.set_collision_profile_name("NoCollision")
            comp.set_cast_shadow(casts)
            merged.set_folder_path("Dressing/Welded")
            merged.tags = [unreal.Name("Dressing")]
            eal.save_loaded_asset(comp.static_mesh)       # or the level would point at a mesh that was never written
            unreal.log("DRESS welded %d pieces into %s" % (len(group), comp.static_mesh.get_path_name()))
            welded += 1
        self.loose = {}
        return welded

    def run(self, pieces, start, end, z, depth=1.0, height=1.0, unit=4.0, folder="Dressing/Walls"):
        """Lays pieces end to end from start to end (x, y), stretched a little so they fit exactly.
        Each piece's face looks to the left of the direction of travel."""
        length = math.hypot(end[0] - start[0], end[1] - start[1])
        n = max(1, int(round(length / unit)))
        seg = length / n
        heading = math.degrees(math.atan2(end[1] - start[1], end[0] - start[0]))
        ux, uy = (end[0] - start[0]) / length, (end[1] - start[1]) / length
        for i in range(n):
            centre = (start[0] + ux * seg * (i + 0.5), start[1] + uy * seg * (i + 0.5), z)
            self.put(pieces[(i + self.rng.randrange(len(pieces))) % len(pieces)], centre, yaw=heading, scale=(seg / unit, depth, height), folder=folder)

    def scatter(self, pieces, points, z, size=(0.8, 1.3), folder="Dressing/Small", kit=None):
        for (x, y) in points:
            s = self.rng.uniform(*size)
            self.put(self.rng.choice(pieces), (x, y, z), yaw=self.rng.uniform(0, 360), scale=(s, s, s), folder=folder, kit=kit, shadow=False)

    def path(self, pieces, points, z, folder="Dressing/Ground"):
        """Lays worn ground along a line of points: where people walk, shown."""
        for (x0, y0), (x1, y1) in zip(points, points[1:]):
            length = math.hypot(x1 - x0, y1 - y0)
            n = max(1, int(round(length / 4.6)))
            heading = math.degrees(math.atan2(y1 - y0, x1 - x0))
            for i in range(n):
                t = (i + 0.5) / n
                self.put(self.rng.choice(pieces), (x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z), yaw=heading + self.rng.uniform(-6, 6),
                         scale=(length / n / 4.6 * 1.05, self.rng.uniform(0.85, 1.15), 1.0), folder=folder, shadow=False)

    def along(self, start, end, count, jitter=0.4):
        """Points strewn along a line: the foot of a wall, the edge of a path."""
        out = []
        for _ in range(count):
            t = self.rng.random()
            out.append((start[0] + (end[0] - start[0]) * t + self.rng.uniform(-jitter, jitter), start[1] + (end[1] - start[1]) * t + self.rng.uniform(-jitter, jitter)))
        return out

    # ---- things the player uses ---------------------------------------------------------------------------------

    def skin(self, kind, piece, drop, yaw=0.0, data_id=None, kit=None):
        """Gives every gameplay actor of a kind its finished look. drop: how far below the actor the ground is (m)."""
        mesh = eal.load_asset("/Game/Kit/%s/%s" % (kit or self.zone, piece))
        n = 0
        for actor in actors.get_all_level_actors():
            tags = [str(t) for t in actor.tags]
            if len(tags) < 3 or tags[0] != "Marker" or tags[1] != kind or (data_id and tags[2] != data_id):
                continue
            try:
                actor.set_editor_property("skin_offset", unreal.Vector(0.0, 0.0, -drop * 100.0))
                actor.set_editor_property("skin_yaw", yaw)
                actor.set_editor_property("skin_mesh", mesh)
                n += 1
            except Exception:
                pass                      # a placeholder marker, not a working actor: it has no skin to set
        return n

    def ambience(self, kind, pos, extent, count, colour, size=1.0, speed=1.0, wind=None):
        if not hasattr(unreal, "UnderpaintingAmbience"):
            return None
        actor = actors.spawn_actor_from_class(unreal.UnderpaintingAmbience, v(pos), unreal.Rotator(0, 0, 0))
        actor.set_editor_property("kind", getattr(unreal.AmbienceKind, kind))
        actor.set_editor_property("count", count)
        actor.set_editor_property("extent", unreal.Vector(extent[0] * 100.0, extent[1] * 100.0, extent[2] * 100.0))
        actor.set_editor_property("colour", stylekit.lin(colour))
        actor.set_editor_property("size", size)
        actor.set_editor_property("speed", speed)
        if wind:
            actor.set_editor_property("wind", unreal.Vector(*wind))
        self.count += 1
        actor.set_actor_label("Ambience_%s_%03d" % (kind.title(), self.count))
        actor.set_folder_path("Dressing/Ambience")
        return actor


def turned(pos, yaw, local):
    """A point given in a piece's own frame (x along it, y out of its front, z up), in the level."""
    a = math.radians(yaw)
    return (pos[0] + local[0] * math.cos(a) - local[1] * math.sin(a), pos[1] + local[0] * math.sin(a) + local[1] * math.cos(a), pos[2] + local[2])


# ======================================================================================================================
# Varnish
# ======================================================================================================================

FACE_LAMP, FACE_SPINE, FACE_NORTH, FACE_SOUTH = 180.0, 0.0, -90.0, 90.0      # which way a piece's front looks: west (-Y), east, +X, -X

# (grey-box house number, x, y, base, piece, facing). Positions are the builder's; they are not changed here.
VARNISH_HOUSES = [
    (1, -20, 37, 8, "SM_House_V1", FACE_LAMP), (2, 0, 37, 8, "SM_House_V3", FACE_LAMP), (3, 20, 37, 8, "SM_House_V4", FACE_LAMP),
    (4, -8, 13, 4, "SM_House_Grinding", FACE_LAMP), (5, 6, 13, 4, "SM_House_Lampworks", FACE_LAMP), (6, 20, 13, 4, "SM_House_V2", FACE_LAMP),
    (7, -28, -4, 4, "SM_House_V5", FACE_NORTH), (8, 28, -5, 4, "SM_House_V1", FACE_SOUTH),
    (9, -24, -31, 0, "SM_House_BlackDoor", FACE_SPINE), (10, -10, -31, 0, "SM_House_V2", FACE_SPINE),
    (11, 4, -31, 0, "SM_House_Cistern", FACE_SPINE), (12, 27, -31, 0, "SM_House_FairGrind", FACE_SPINE),
]
# Houses with a doorway: where the door is along the front, and half the house's depth (tools/kit/blender_varnish.py).
DOORWAYS = {"SM_House_Lampworks": (-1.5, 2.8, "SM_Door_Teal"), "SM_House_Grinding": (1.5, 2.8, "SM_Door_Wood"), "SM_House_Cistern": (0.0, 2.7, "SM_Door_Wood")}
# Where each house's chimney is, in its own frame, for smoke.
CHIMNEYS = {1: (1.55, -0.9, 6.5), 2: (1.6, -0.9, 6.6), 4: (-1.55, -0.9, 6.5), 5: (1.55, -0.9, 6.5), 10: (-1.4, -0.9, 6.0)}
HEM_AFTER = None       # filled in below: the Hem's houses after the Turning, down to bare canvas


def dress_village(shifted):
    def area_of(pos):
        if abs(pos[0]) > 35.5 or pos[1] > 43.5 or pos[1] < -39.5:
            return "Wild"
        return "Top" if pos[2] >= 7.5 else ("Square" if pos[2] >= 3.5 else "Hem")

    d = Dresser("Varnish", 4101, "#4A3558", area_of)
    d.hide("House_", "Parapet_", "Ramp_", "Shoulder", "LowerGate_Post", "Furrow_Closed")
    d.hide_markers()
    d.repaint("Terrace_Sunward", "#E6C090")
    d.repaint("Terrace_LampstandSquare", "#E8C598")
    d.repaint("Terrace_Hem", "#D8CBB0" if shifted else "#DDBE96", "Plaster" if shifted else "Ground")

    # ---- her shoulder, the far colossi, the Lamp -----------------------------------------------------------------
    d.put("SM_Terrain_Varnish", (0, 0, 0), kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)     # two kilometres of ground need cast no shadow
    plain = -420.0                                   # the land far below her, out to the horizon; the others stand on it
    d.put("SM_Vista_Plain", (0, 0, plain), scale=(14500, 14500, 1), kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)
    d.put("SM_Vista_Sleeper", (-2600, -6800, plain), yaw=-12, kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)
    d.put("SM_Vista_Kneeler", (3400, -7600, plain), yaw=14, kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)
    if shifted:      # the Wader has come closer, and moved
        d.put("SM_Vista_Wader", (900, -4300, plain), yaw=6, kit="Vista", folder="Dressing/Vista", shadow=False, merge=False,
              palette=dict(unreal_kit.PALETTES["Vista"], SilhouetteNear=True))
    else:
        d.put("SM_Vista_Wader", (-300, -9200, plain), yaw=0, scale=(0.8, 1, 0.8), kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)
    lamp_up = math.radians(7.0 if shifted else 9.0)
    d.put("SM_Vista_Lamp", (0, -11000 * math.cos(lamp_up), 11000 * math.sin(lamp_up)), scale=(520, 1, 520), kit="Vista", folder="Dressing/Vista", shadow=False, merge=False)

    if shifted:
        # The Furrow has shut: two of her vertebrae have risen through the gap in the square's north parapet.
        d.put("SM_SpineStep", (36.0, 2.2, 9.2), yaw=20, scale=(1.0, 0.9, 1.3), kit="Scumblewood", folder="Dressing/Vista")
        d.put("SM_SpineStep", (36.4, 6.2, 8.1), yaw=200, scale=(0.9, 0.85, 1.2), kit="Scumblewood", folder="Dressing/Vista")
        d.put("SM_Boulder_L", (34.4, 8.6, 4.0), yaw=70, kit="Scumblewood", folder="Dressing/Vista", palette=dict(unreal_kit.PALETTES["Scumblewood"], Stone=("#CFC3A8", "Stone")))
        # And the Stair of Spines has come up to the Hem: vertebrae stepping away down the slope beyond the lower gate.
        for i, (x, y, z) in enumerate(((0.5, -44.5, 0.1), (2.5, -51.5, -1.0), (6.0, -59.0, -2.6), (11.0, -67.0, -4.9), (17.5, -75.0, -7.8), (25.0, -83.5, -11.2))):
            d.put("SM_SpineStep", (x, y, z), yaw=i * 47.0, scale=(1.0 + 0.06 * i, 1.0 + 0.06 * i, 1.0), kit="Scumblewood", folder="Dressing/Vista")

    # What grows and lies on her, beyond the parapets: nobody can go there, so it is there only to be looked at.
    def on_her(x, y):
        return (x, y, shoulder.shoulder_height(x, y) - 0.08)

    wild = random.Random(977)
    for i in range(46):
        a, r = wild.uniform(0, math.tau), wild.uniform(48, 150)
        x, y = math.cos(a) * r * 1.1, math.sin(a) * r + 2.0
        if abs(x) < 39 and -43 < y < 47:
            continue
        if x > 35 and abs(y - 4) < 14:
            continue                                  # keep the Furrow's mouth clear
        pick = wild.random()
        if pick < 0.30:
            s1 = wild.uniform(0.8, 1.5)
            d.put(wild.choice(("SM_Tree_A", "SM_Tree_B")), on_her(x, y), yaw=wild.uniform(0, 360), scale=(s1, s1, s1), folder="Dressing/Vista/Wild")
        elif pick < 0.62:
            s1 = wild.uniform(1.2, 3.2)
            d.put(wild.choice(("SM_Rocks_A", "SM_Rocks_B")), on_her(x, y), yaw=wild.uniform(0, 360), scale=(s1, s1, s1 * 0.8), folder="Dressing/Vista/Wild", shadow=False)
        else:
            for k in range(5):                        # a patch of dry grass
                gx, gy = x + wild.uniform(-2.5, 2.5), y + wild.uniform(-2.5, 2.5)
                s1 = wild.uniform(1.6, 2.8)
                d.put(wild.choice(("SM_Tuft_A", "SM_Tuft_B")), on_her(gx, gy), yaw=wild.uniform(0, 360), scale=(s1, s1, s1), folder="Dressing/Vista/Wild", shadow=False)
    # The Furrow leaving the north gate: a worn way down the groove, and the first trees of the Scumblewood at its turn.
    for i in range(9):
        along = 3.0 + i * 6.5
        y = 4.0 + 9.0 * math.sin(along * 0.018) * shoulder._smooth(along / 60.0)
        px = shoulder.FOOT_X + along
        d.put("SM_PathWorn_A", (px, y, shoulder.shoulder_height(px, y) + 0.03), yaw=wild.uniform(-8, 8), pitch=-12.0, scale=(1.4, 1.2, 1), folder="Dressing/Vista/Wild", shadow=False)
    if not shifted:
        for (along, side, s1) in ((34, -1, 0.9), (44, 1, 1.0), (58, -1, 1.1), (66, 1, 0.85), (82, -1, 1.2), (96, 1, 1.0), (112, -1, 1.25)):
            px = shoulder.FOOT_X + along
            y = 4.0 + 9.0 * math.sin(along * 0.018) * shoulder._smooth(along / 60.0) + side * (9.0 + along * 0.05)
            d.put(wild.choice(("SM_ForestTree_A", "SM_ForestTree_C")), on_her(px, y), yaw=wild.uniform(0, 360), scale=(s1, s1, s1), kit="Scumblewood", folder="Dressing/Vista/Wild")

    # ---- houses ---------------------------------------------------------------------------------------------------
    for index, x, y, base, piece, facing in VARNISH_HOUSES:
        faded = shifted and base == 0
        yaw = facing + d.rng.uniform(-2.5, 2.5)
        d.put(piece, (x, y, base), yaw=yaw, folder="Dressing/Houses", palette=HEM_AFTER if faded else None)
        if piece in DOORWAYS:                      # shut until its inside is furnished (step 4 of the finishing pass)
            door_x, half_depth, leaf = DOORWAYS[piece]
            d.put(leaf, turned((x, y, base), yaw, (door_x - 0.5, half_depth - 0.17, 0.5)), yaw=yaw, folder="Dressing/Houses", palette=HEM_AFTER if faded else None)
        front = turned((x, y, base), yaw, (0, 3.6, 0))
        d.put("SM_Paving_Patch", (front[0], front[1], base), yaw=d.rng.uniform(0, 360), scale=(0.75, 0.75, 1), folder="Dressing/Ground", shadow=False)
        side = d.rng.choice((-1, 1))
        pot = turned((x, y, base), yaw, (side * 2.4, 3.15, 0))
        d.put("SM_PotPlant", pot, yaw=d.rng.uniform(0, 360), folder="Dressing/Small")
        if index in CHIMNEYS and not faded:
            d.ambience("SMOKE", turned((x, y, base), yaw, CHIMNEYS[index]), (1, 1, 7), 8, "#CDBDAE", size=1.5, speed=1.0, wind=(6, 22, 0))

    # ---- walls that hold the terraces, parapets, ramps -------------------------------------------------------------
    walls = ["SM_RetainingWall_A", "SM_RetainingWall_B"]
    for (x0, x1) in ((34, -20), (-24, -34)):          # the square's back wall, under the Sunward Terraces (gap: the ramp)
        d.run(walls, (x0, 17.9), (x1, 17.9), 4.0, depth=0.5, height=0.955)
    for (x0, x1) in ((34, 24), (20, -34)):            # the Hem's back wall, under the square
        d.run(walls, (x0, -10.1), (x1, -10.1), 0.0, depth=0.5, height=0.955)
    parapets = ["SM_Parapet_A", "SM_Parapet_B"]
    for (a, b, z) in (((-34, 42.5), (34, 42.5), 8), ((34, -38.5), (-34, -38.5), 0),
                      ((-34.5, 18), (-34.5, 42), 8), ((34.5, 42), (34.5, 18), 8), ((-34.5, -10), (-34.5, 18), 4),
                      ((-34.5, -38), (-34.5, -10), 0), ((34.5, -10), (34.5, -38), 0),
                      ((34.5, 0), (34.5, -10), 4), ((34.5, 18), (34.5, 8), 4)):
        d.run(parapets, a, b, z, height=1.15, folder="Dressing/Parapets")
    for (start, end) in (((-22, 6, 4), (-22, 18, 8)), ((22, -22, 0), (22, -10, 4))):
        run_m, rise = end[1] - start[1], end[2] - start[2]
        mid = ((start[0] + end[0]) / 2.0, (start[1] + end[1]) / 2.0, (start[2] + end[2]) / 2.0 + 0.02)
        d.put("SM_RampSkin", mid, yaw=90.0, pitch=math.degrees(math.atan2(rise, run_m)), folder="Dressing/Ramps")
        for side in (-1, 1):                           # the stone that holds it up, one each side
            d.put("SM_RampSide", (start[0] + side * 1.82, start[1], start[2]), yaw=90.0, scale=(1, side, 1), folder="Dressing/Ramps")

    # ---- the ground: paving, worn places, what grows at the foot of walls --------------------------------------------
    d.put("SM_Paving_Ring", (0, 0, 4.0), folder="Dressing/Ground", shadow=False)
    d.put("SM_Paving_Patch", (-8, -5.6, 4.0), yaw=40, scale=(1.2, 0.9, 1), folder="Dressing/Ground", shadow=False)
    d.put("SM_Paving_Patch", (9, -5.6, 4.0), yaw=200, scale=(1.1, 0.8, 1), folder="Dressing/Ground", shadow=False)
    d.put("SM_Paving_Patch", (31.5, 4, 4.0), yaw=90, scale=(1.0, 1.3, 1), folder="Dressing/Ground", shadow=False)
    worn = ["SM_PathWorn_A", "SM_PathWorn_B"]
    for points, z in (([(-2, 2.5), (-20.5, 5.2)], 4), ([(2.5, 3.3), (31, 4)], 4), ([(1.5, -2), (21, -8.8)], 4), ([(-1.5, -1.8), (-7.4, -3.4)], 4),
                      ([(2, -1.8), (8.4, -3.4)], 4), ([(-21, 19.6), (-12, 24), (10, 24.2), (26, 24.6)], 8), ([(-8, 26), (0, 31), (16, 33.4)], 8),
                      ([(22, -23.2), (12, -27.5), (1, -33)], 0), ([(20.5, -22.6), (-1, -15.2), (-16, -21), (-23.5, -27)], 0)):
        d.path(worn, points, z)
    tufts = ["SM_Tuft_A", "SM_Tuft_B"]
    d.scatter(tufts, d.along((-33, 17.2), (33, 17.2), 26), 4.0)
    d.scatter(tufts, d.along((-33, -10.8), (33, -10.8), 26), 0.0)
    d.scatter(tufts, d.along((-33, 41.6), (33, 41.6), 20), 8.0)
    d.scatter(tufts, d.along((-33, -37.6), (33, -37.6), 22), 0.0)
    for (x, z0, z1) in ((-33.6, 0, 0), (33.6, 0, 0)):
        d.scatter(tufts, d.along((x, -37), (x, -11), 10), 0.0)
        d.scatter(tufts, d.along((x, 19), (x, 41), 8), 8.0)
    d.scatter(tufts, d.along((-33.6, -9), (-33.6, 17), 9), 4.0)
    for (x, y, z, piece, s) in ((-32.6, 16.4, 4, "SM_Rocks_B", 1.0), (32.4, 40.6, 8, "SM_Rocks_A", 1.2), (-32.5, -36.6, 0, "SM_Rocks_B", 1.1),
                                (31.8, -12.0, 0, "SM_Rocks_A", 1.0), (14.5, 19.6, 8, "SM_Rocks_A", 0.8), (-32.8, 20.2, 8, "SM_Rocks_A", 1.0)):
        d.put(piece, (x, y, z), yaw=d.rng.uniform(0, 360), scale=(s, s, s), folder="Dressing/Small")
    if shifted:
        # The Hem's paint has thinned: bare canvas showing through in patches, with the drawing under it.
        for (x, y) in ((-17, -22), (-5, -19), (9, -24), (18, -17), (-26, -18), (0, -27), (13, -33), (-12, -35)):
            d.put("SM_CanvasEdge", (x, y, 0.0), yaw=d.rng.uniform(0, 360), scale=(1.6, 1.6, 1), kit="Scumblewood", folder="Dressing/Ground", shadow=False)
    else:
        for (x, y) in ((-17, -22), (-13, -20)):        # "the Hem: paint starting to flake"
            d.put("SM_CanvasEdge", (x, y, 0.0), yaw=d.rng.uniform(0, 360), kit="Scumblewood", folder="Dressing/Ground", shadow=False)

    # ---- the square: work, trade, somewhere to sit ---------------------------------------------------------------------
    props = [
        ("SM_Stall_Madder", 13.0, 8.4, 4, 180), ("SM_Crates", 10.4, 8.9, 4, 30), ("SM_Barrel", 15.7, 9.1, 4, 0),
        ("SM_Stall_Ochre", -15.0, 8.6, 4, 172), ("SM_Sacks", -12.4, 9.3, 4, 80), ("SM_Basket", -17.2, 8.9, 4, 0),
        ("SM_FlaxPress", 16.6, 3.9, 4, 150), ("SM_Bench", -3.4, -3.4, 4, 35), ("SM_Jars", -10.4, -5.6, 4, 10),
        ("SM_Barrel", -5.4, -7.9, 4, 0), ("SM_Crates", 11.6, -6.6, 4, 110), ("SM_Handcart", 26.8, 12.2, 4, -70),
        ("SM_Trough", -27.2, 1.4, 4, 90), ("SM_Tree_A", -31.0, 14.6, 4, 40), ("SM_Tree_B", 31.2, 15.4, 4, 200),
        ("SM_Bench", 24.2, -7.6, 4, 95), ("SM_Sacks", 24.6, 9.6, 4, 200),
        # the Sunward Terraces: where the ochre is lifted and dried
        ("SM_DryingRack", -15.0, 20.6, 8, 8), ("SM_DryingRack", 1.2, 20.4, 8, -6), ("SM_DryingRack", 17.2, 20.7, 8, 4),
        ("SM_Basket", -12.6, 21.2, 8, 0), ("SM_Basket", 3.6, 21.0, 8, 0), ("SM_Tree_A", -31.4, 39.8, 8, 120), ("SM_Tree_B", 10.2, 39.2, 8, 300),
        ("SM_Fence", -10.0, 33.6, 8, 2), ("SM_Fence", 10.0, 33.8, 8, -3), ("SM_Crates", 24.6, 35.2, 8, 20),
        # the Hem: washing, water, the things of people who are getting by
        ("SM_DyeLine", -17.0, -30.4, 0, 0), ("SM_DyeLine", -3.0, -30.8, 0, 4), ("SM_Cistern", -6.0, -11.9, 0, 180),
        ("SM_Trough", -8.6, -12.2, 0, 2), ("SM_Fence", 12.0, -33.6, 0, 0), ("SM_Fence", 16.2, -33.5, 0, 3), ("SM_Tree_B", 15.6, -35.4, 0, 60),
        ("SM_Tree_A", -31.6, -35.2, 0, 10), ("SM_Barrel", 7.9, -28.4, 0, 0), ("SM_Sacks", 23.2, -28.6, 0, 150), ("SM_Bench", -20.4, -27.6, 0, 180),
        ("SM_Handcart", 28.6, -14.4, 0, 190), ("SM_Woodpile", 9.5, -11.25, 0, 180), ("SM_Woodpile", -20.0, -11.25, 0, 180),
        ("SM_Woodpile", -3.0, 16.75, 4, 180), ("SM_Woodpile", 27.5, 16.75, 4, 180), ("SM_Crates", -27.4, -12.0, 0, 15), ("SM_Jars", 6.0, -27.4, 0, 60),
        ("SM_Basket", -14.4, -27.6, 0, 0), ("SM_Barrel", 30.6, -27.4, 0, 0), ("SM_Jars", -1.2, 9.4, 4, 100), ("SM_Sacks", 30.8, 21.0, 8, 30),
    ]
    for piece, x, y, z, yaw in props:
        d.put(piece, (x, y, z), yaw=yaw, folder="Dressing/Props")
    d.put("SM_Ladder", (2.6, 33.9, 8.0), yaw=0, roll=-10, folder="Dressing/Props")

    # ---- what the player uses ---------------------------------------------------------------------------------------------
    d.skin("Lampstand", "SM_Lampstand", 1.5)
    d.skin("Node", "SM_Node_Ochre", 0.4)
    d.skin("Station", "SM_GrindingSlab", 0.45, data_id="GrindingSlab")
    d.skin("Station", "SM_ArmatureBench", 0.45, data_id="ArmatureBench")
    d.skin("Cache", "SM_Cache", 0.3)
    d.skin("Lore", "SM_LoreNote", 1.2, yaw=-90.0)
    d.skin("Exit", "SM_ExitArch", 2.0)
    d.skin("Note", "SM_Jars", 0.5, data_id="FlaxPress")

    # ---- life ---------------------------------------------------------------------------------------------------------------
    d.ambience("MOTES", (0, 4, 6.5), (32, 26, 3.5), 28, "#D9A441", size=1.0)
    d.ambience("MOTES", (0, -24, 2.5), (32, 13, 2.5), 12, "#8F86A6" if shifted else "#D9A441", size=0.9)
    d.ambience("BIRDS", (-10, -170, 30), (120, 70, 14), 7, "#3A2C46", size=1.3)
    welded = d.finish("L_Matriarch_Village_B" if shifted else "L_Matriarch_Village_A")
    unreal.log("DRESS village: %d pieces placed, welded into %d meshes" % (d.count, welded))
    return d.count


HEM_AFTER = dict(unreal_kit.PALETTES["Varnish"], Plaster=("#D8CBB0", "Plaster"), Roof=("#6C5A7C", "Roof"), Glow=None)

DRESSERS = {"L_Matriarch_Village_A": lambda: dress_village(False), "L_Matriarch_Village_B": lambda: dress_village(True)}


def dress_level(level_name):
    if level_name not in DRESSERS:
        return 0
    count = DRESSERS[level_name]()
    unreal.log("DRESS %s: %d pieces" % (level_name, count))
    return count
