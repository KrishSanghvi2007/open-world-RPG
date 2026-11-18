"""Builds the grey-box levels for the Sleeping Matriarch (the "A" versions, before the shift).

Run inside the Unreal editor (Python). It is safe to run again: each level is cleared and rebuilt,
so the layout lives in this file and can be reviewed and changed like any other code.

Levels made (in /Game/Maps/Matriarch):
    L_Matriarch_Village_A       Varnish, the painted village (three terraces)
    L_Matriarch_Scumblewood_A   the half-painted forest (the Furrow gorge)
    L_Matriarch_Vault_A         the Grisaille Vault (Rib Stair, Cartoon Gallery, the Heart)

Conventions:
    - All positions in this file are in METRES. +X is the direction of travel, +Z is up.
      The sun stands in the west, which is -Y, so sunlit faces look toward -Y.
    - Every gameplay placeholder is a "marker": a small coloured shape with a text label, tagged
      "Marker", its kind ("Enemy", "Node", "Lampstand", "NPC", "Station", "Exit", "Cache", "Note") and its data ID
      (a row name from data/*.json where one exists). Spawners will later replace markers by reading those tags.
    - Node and Lampstand counts are checked against data/regions.json at the end of each zone.
"""

import json
import math
import os

import unreal

MAP_DIR = "/Game/Maps/Matriarch"
MAT_DIR = "/Game/Greybox/Materials"

CUBE = "/Engine/BasicShapes/Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder"
SPHERE = "/Engine/BasicShapes/Sphere"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

REGIONS = {
    row["Name"]: row
    for row in json.load(open(os.path.join(unreal.Paths.project_dir(), "data", "regions.json")))
}

_counts = {}


# --------------------------------------------------------------------------------------
# Materials: one lit and one unlit parent with a "Colour" parameter, and an instance per colour
# --------------------------------------------------------------------------------------

def _hex_to_linear(hex_colour):
    """#RRGGBB as it looks on screen (sRGB) to the linear colour Unreal materials and lights expect."""
    h = hex_colour.lstrip("#")
    r, g, b = (int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4))
    return unreal.LinearColor(pow(r, 2.2), pow(g, 2.2), pow(b, 2.2), 1.0)


def _parent_material(name, unlit):
    path = "%s/%s" % (MAT_DIR, name)
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    mat = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    colour = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    colour.set_editor_property("parameter_name", "Colour")
    colour.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    if unlit:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 250)
        rough.set_editor_property("r", 0.9)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_asset(path)
    return mat


_material_cache = {}


def material(hex_colour, unlit=False):
    key = (hex_colour.upper(), unlit)
    if key in _material_cache:
        return _material_cache[key]
    name = "MI_GB_%s%s" % (hex_colour.lstrip("#").upper(), "_Unlit" if unlit else "")
    path = "%s/%s" % (MAT_DIR, name)
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        parent = _parent_material("M_GreyboxUnlit" if unlit else "M_Greybox", unlit)
        mi = asset_tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Colour", _hex_to_linear(hex_colour))
        eal.save_asset(path)
    _material_cache[key] = mi
    return mi


# --------------------------------------------------------------------------------------
# Building blocks
# --------------------------------------------------------------------------------------

class Zone:
    """Settings shared by everything placed in the level being built."""
    unlit = False
    label_yaw = 180.0


def v(p):
    return unreal.Vector(p[0] * 100.0, p[1] * 100.0, p[2] * 100.0)


def _mesh_actor(name, mesh_path, pos, scale, colour, folder, rot=None, collide=True, tags=()):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, v(pos), rot or unreal.Rotator(0, 0, 0))
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(mesh_path))
    comp.set_material(0, material(colour, Zone.unlit))
    if not collide:
        comp.set_collision_profile_name("NoCollision")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    if tags:
        actor.tags = [unreal.Name(t) for t in tags]
    return actor


def box(name, centre, size, colour, folder="Geometry", yaw=0.0, pitch=0.0, collide=True, tags=()):
    """A block. centre and size in metres."""
    return _mesh_actor(name, CUBE, centre, size, colour, folder,
                       unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw), collide, tags)


def slab(name, x0, x1, y0, y1, top, colour, folder="Geometry", bottom=None, tags=()):
    """A block given by its edges and the height of its walkable top."""
    bottom = top - 1.0 if bottom is None else bottom
    return box(name, ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (top + bottom) / 2.0),
               (abs(x1 - x0), abs(y1 - y0), top - bottom), colour, folder, tags=tags)


def ramp(name, start, end, width, colour, folder="Geometry", thickness=1.0):
    """A walkable slope whose top surface runs from start to end (metres)."""
    dx, dy, dz = end[0] - start[0], end[1] - start[1], end[2] - start[2]
    run = math.hypot(dx, dy)
    length = math.sqrt(run * run + dz * dz)
    pitch = math.degrees(math.atan2(dz, run))
    yaw = math.degrees(math.atan2(dy, dx))
    drop = (thickness / 2.0) / math.cos(math.radians(pitch))
    centre = ((start[0] + end[0]) / 2.0, (start[1] + end[1]) / 2.0, (start[2] + end[2]) / 2.0 - drop)
    return box(name, centre, (length, width, thickness), colour, folder, yaw=yaw, pitch=pitch)


def cylinder(name, base, radius, height, colour, folder="Geometry", collide=True, tags=()):
    centre = (base[0], base[1], base[2] + height / 2.0)
    return _mesh_actor(name, CYLINDER, centre, (radius * 2.0, radius * 2.0, height), colour, folder,
                       collide=collide, tags=tags)


_label_material = None


def label_material():
    """Text that is drawn on its front only. The engine's text materials are two-sided, so from behind a label
    reads mirrored; with this, labels are placed back to back and each side reads the right way round."""
    global _label_material
    if _label_material is None:
        path = MAT_DIR + "/MI_GreyboxLabel"
        if eal.does_asset_exist(path):
            _label_material = eal.load_asset(path)
        else:
            mi = asset_tools.create_asset("MI_GreyboxLabel", MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            mel.set_material_instance_parent(mi, unreal.load_asset("/Engine/EngineMaterials/UnlitText"))
            overrides = mi.get_editor_property("base_property_overrides")
            overrides.set_editor_property("override_two_sided", True)
            overrides.set_editor_property("two_sided", False)
            mi.set_editor_property("base_property_overrides", overrides)
            mel.update_material_instance(mi)
            eal.save_asset(path)
            _label_material = mi
    return _label_material


def label(text, pos, folder, size=60.0, colour="#2E2E30"):
    h = colour.lstrip("#")
    for side, yaw in (("Front", Zone.label_yaw), ("Back", Zone.label_yaw + 180.0)):
        actor = actors.spawn_actor_from_class(unreal.TextRenderActor, v(pos), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        comp = actor.text_render
        comp.set_text_material(label_material())
        comp.set_text(text)
        comp.set_world_size(size)
        comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        comp.set_text_render_color(unreal.Color(r=int(h[0:2], 16), g=int(h[2:4], 16), b=int(h[4:6], 16), a=255))
        actor.set_actor_label("Label_%s_%s" % (text.replace(" ", "_")[:40], side))
        actor.set_folder_path(folder)


MARKER_STYLES = {
    # kind: (mesh, (x, y, z) size in metres, default colour)
    "Enemy": (CYLINDER, (0.8, 0.8, 1.8), "#9E2B25"),
    "NPC": (CYLINDER, (0.6, 0.6, 1.8), "#2E5E6E"),
    "Node": (SPHERE, (0.8, 0.8, 0.8), "#D9A441"),
    "Lampstand": (CYLINDER, (0.4, 0.4, 3.0), "#E8923A"),
    "Station": (CUBE, (1.6, 1.0, 1.0), "#6B4F3A"),
    "Exit": (CUBE, (4.0, 0.3, 4.0), "#2F4BA8"),
    "Cache": (CUBE, (0.8, 0.8, 0.6), "#C4574B"),
    "Note": (SPHERE, (0.4, 0.4, 0.4), "#4A3558"),
}

PIGMENT_COLOURS = {
    "Ochre": "#D9A441", "TerreVerte": "#6B8F71", "Sanguine": "#9E2B25",
    "BoneBlack": "#2A2826", "Ceruse": "#EDE8DC", "SlumberBlue": "#2F4BA8",
}


def marker(kind, data_id, text, ground, colour=None, yaw=0.0):
    """A labelled placeholder standing on the ground point given (metres)."""
    mesh, size, default_colour = MARKER_STYLES[kind]
    centre = (ground[0], ground[1], ground[2] + size[2] / 2.0)
    index = _counts.get((kind, data_id), 0) + 1
    _counts[(kind, data_id)] = index
    was_unlit, Zone.unlit = Zone.unlit, True  # markers stay readable in any light
    actor = _mesh_actor("%s_%s_%02d" % (kind, data_id, index), mesh, centre, size, colour or default_colour,
                        "Markers/" + kind, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw),
                        collide=False, tags=("Marker", kind, data_id))
    Zone.unlit = was_unlit
    label(text, (ground[0], ground[1], ground[2] + size[2] + 0.5), "Markers/" + kind + "/Labels")
    return actor


def node(pigment, ground):
    return marker("Node", pigment, pigment + " node", ground, PIGMENT_COLOURS[pigment])


def enemy(enemy_id, ground):
    return marker("Enemy", enemy_id, enemy_id, ground)


def player_start(ground, yaw=0.0):
    actor = actors.spawn_actor_from_class(unreal.PlayerStart, v((ground[0], ground[1], ground[2] + 1.0)),
                                          unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    actor.set_folder_path("Gameplay")
    return actor


# --------------------------------------------------------------------------------------
# Lighting (GDD section 9: one shadow-casting sun and one tinted sky light per zone)
# --------------------------------------------------------------------------------------

def sun_and_sky(sun_colour, sky_tint, sun_lux=6.0, sky_intensity=1.0, pitch=-25.0):
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, v((0, 0, 30)),
                                        unreal.Rotator(roll=0.0, pitch=pitch, yaw=90.0))  # shining from the west (-Y)
    sun.set_actor_label("Sun")
    sun.set_folder_path("Lighting")
    comp = sun.light_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_intensity(sun_lux)
    comp.set_light_color(_hex_to_linear(sun_colour))

    atmosphere = actors.spawn_actor_from_class(unreal.SkyAtmosphere, v((0, 0, 0)), unreal.Rotator(0, 0, 0))
    atmosphere.set_folder_path("Lighting")

    sky = actors.spawn_actor_from_class(unreal.SkyLight, v((0, 0, 40)), unreal.Rotator(0, 0, 0))
    sky.set_actor_label("SkyLight_ZoneTint")
    sky.set_folder_path("Lighting")
    sky_comp = sky.light_component
    sky_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_comp.set_editor_property("real_time_capture", True)
    sky_comp.set_intensity(sky_intensity)
    sky_comp.set_light_color(_hex_to_linear(sky_tint))
    sky_comp.set_editor_property("cast_shadows", False)


def fill_light(colour, lux=2.0):
    """Shadowless fill so characters are visible in the unlit sketch zone."""
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, v((0, 0, 30)),
                                          unreal.Rotator(roll=0.0, pitch=-60.0, yaw=20.0))
    light.set_actor_label("Fill_NoShadow")
    light.set_folder_path("Lighting")
    comp = light.light_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_intensity(lux)
    comp.set_light_color(_hex_to_linear(colour))
    comp.set_cast_shadows(False)


# --------------------------------------------------------------------------------------
# Level handling
# --------------------------------------------------------------------------------------

def begin_level(name):
    path = "%s/%s" % (MAP_DIR, name)
    if eal.does_asset_exist(path):
        levels.load_level(path)
        for actor in actors.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings) and actor.get_class().get_name() != "Brush":
                actors.destroy_actor(actor)
    else:
        levels.new_level(path)
    _counts.clear()
    return path


def end_level(zone_id):
    """Checks marker counts against data/regions.json, then saves."""
    row = REGIONS[zone_id]
    problems = []
    for pigment, expected in zip(row["Pigments"], row["HarvestNodeCounts"]):
        placed = _counts.get(("Node", pigment), 0)
        if placed != expected:
            problems.append("%s: %d %s nodes placed, data says %d" % (zone_id, placed, pigment, expected))
    lampstands = sum(n for (kind, _), n in _counts.items() if kind == "Lampstand")
    if lampstands != row["Lampstands"]:
        problems.append("%s: %d Lampstands placed, data says %d" % (zone_id, lampstands, row["Lampstands"]))
    for (kind, data_id) in _counts:
        if kind == "Enemy" and data_id not in row["Enemies"]:
            problems.append("%s: enemy %s is not listed for this zone" % (zone_id, data_id))

    world_settings = editor.get_editor_world().get_world_settings()
    if row["GravityScale"] != 1.0:
        world_settings.set_editor_property("global_gravity_set", True)
        world_settings.set_editor_property("global_gravity_z", -980.0 * row["GravityScale"])

    levels.save_current_level()
    summary = {"%s:%s" % key: n for key, n in sorted(_counts.items())}
    unreal.log("GREYBOX %s markers: %s" % (zone_id, json.dumps(summary)))
    for problem in problems:
        unreal.log_error("GREYBOX " + problem)
    return problems


# --------------------------------------------------------------------------------------
# Zone 1: Varnish, the painted village. Three terraces stepping down toward the sun.
# --------------------------------------------------------------------------------------

def build_village():
    begin_level("L_Matriarch_Village_A")
    Zone.unlit, Zone.label_yaw = False, -90.0
    OCHRE, ROSE, CREAM, TEAL, VIOLET = "#D9A441", "#C4574B", "#F2E3C0", "#2E5E6E", "#4A3558"

    sun_and_sky(sun_colour="#FFD9A0", sky_tint=TEAL, sun_lux=7.0, sky_intensity=1.2)

    # Her shoulder: a wide base so nobody falls out of the world.
    slab("Shoulder", -60, 60, -70, 60, -0.01, VIOLET, bottom=-3.0)

    # Terraces. Top (east, y 18..42, height 8), middle (y -10..18, height 4), the Hem (y -38..-10, height 0).
    slab("Terrace_Sunward", -34, 34, 18, 42, 8.0, CREAM, bottom=-1.0)
    slab("Terrace_LampstandSquare", -34, 34, -10, 18, 4.0, CREAM, bottom=-1.0)
    slab("Terrace_Hem", -34, 34, -38, -10, 0.0, CREAM, bottom=-1.0)

    ramp("Ramp_Square_to_Sunward", (-22, 6, 4), (-22, 18, 8), 4.0, OCHRE)
    ramp("Ramp_Hem_to_Square", (22, -22, 0), (22, -10, 4), 4.0, OCHRE)

    # Parapets on the outer edges.
    for name, x0, x1, y0, y1, base in (
        ("Parapet_East", -34, 34, 42, 43, 8.0), ("Parapet_West", -34, 34, -39, -38, 0.0),
        ("Parapet_South_Top", -35, -34, 18, 42, 8.0), ("Parapet_North_Top", 34, 35, 18, 42, 8.0),
        ("Parapet_South_Mid", -35, -34, -10, 18, 4.0),
        ("Parapet_South_Hem", -35, -34, -38, -10, 0.0), ("Parapet_North_Hem", 34, 35, -38, -10, 0.0),
    ):
        slab(name, x0, x1, y0, y1, base + 1.2, TEAL, "Geometry/Parapets", bottom=base)
    # The north edge of the square is open in the middle: the way down to the Furrow.
    slab("Parapet_North_Mid_a", 34, 35, -10, 0, 5.2, TEAL, "Geometry/Parapets", bottom=4.0)
    slab("Parapet_North_Mid_b", 34, 35, 8, 18, 5.2, TEAL, "Geometry/Parapets", bottom=4.0)

    def house(index, x, y, base):
        box("House_%02d" % index, (x, y, base + 2.5), (6, 6, 5), CREAM, "Geometry/Houses")
        box("House_%02d_Roof" % index, (x, y, base + 5.4), (7, 7, 0.8), ROSE, "Geometry/Houses")

    houses = [(-20, 37, 8), (0, 37, 8), (20, 37, 8),                       # Sunward Terraces
              (-8, 13, 4), (6, 13, 4), (20, 13, 4), (-28, -4, 4), (28, -5, 4),  # Lampstand square
              (-24, -31, 0), (-10, -31, 0), (4, -31, 0), (27, -31, 0)]          # the Hem
    for i, (x, y, base) in enumerate(houses, 1):
        house(i, x, y, base)

    # Lampstand square
    marker("Lampstand", "Varnish", "Lampstand", (0, 0, 4))
    marker("NPC", "HesperVane", "Hesper Vane", (3, 2, 4))
    marker("NPC", "BartramSedge", "Bartram Sedge", (-10, -5, 4))
    marker("Station", "GrindingSlab", "Grinding Slab", (-8, -5, 4))
    marker("Station", "ArmatureBench", "Armature Bench", (9, -5, 4))
    marker("Note", "FlaxPress", "Flax press", (15, 3, 4))
    marker("Exit", "Scumblewood", "To the Scumblewood (the Furrow)", (33, 4, 4), yaw=90.0)

    # Sunward Terraces: the four Ochre nodes
    for x in (-27, -10, 10, 27):
        node("Ochre", (x, 24, 8))

    # The Hem
    marker("Note", "LowerGate", "Lower gate (appears after the shift)", (0, -37, 0))
    marker("Note", "HemFlaking", "The Hem: paint starting to flake", (-17, -22, 0))

    player_start((-4, 4, 4), yaw=0.0)
    return end_level("VarnishVillage")


# --------------------------------------------------------------------------------------
# Zone 2: the Scumblewood. The Furrow, a gorge winding down between two vertebrae.
# --------------------------------------------------------------------------------------

def build_scumblewood():
    begin_level("L_Matriarch_Scumblewood_A")
    Zone.unlit, Zone.label_yaw = False, 180.0
    GREEN, UMBER, CANVAS, CHARCOAL, SANGUINE = "#6B8F71", "#6B4F3A", "#D8CBB0", "#3A3A3A", "#9E2B25"

    sun_and_sky(sun_colour="#FFC98A", sky_tint="#4A3558", sun_lux=4.0, sky_intensity=0.8, pitch=-32.0)

    # Stops along the route: (name, x, y, z, half-size of its flat pad)
    stops = [
        ("Entry", 0, 0, 0, 9),
        ("FirstWell", 40, 0, -3, 10),
        ("Bend", 80, 20, -6, 9),
        ("SunwardClearing", 120, 20, -8, 15),
        ("Descent", 160, 0, -11, 9),
        ("HollowVertebra", 200, 0, -13, 14),
        ("Approach", 240, 15, -15, 9),
        ("RibGate", 272, 15, -16, 11),
    ]
    at = {name: (x, y, z) for name, x, y, z, _ in stops}

    def off(stop, dx=0.0, dy=0.0):
        x, y, z = at[stop]
        return (x + dx, y + dy, z)

    slab("Bedrock", -30, 310, -60, 80, -22.0, CHARCOAL, bottom=-24.0)

    WALL_H = 14.0
    for name, x, y, z, half in stops:
        slab("Pad_" + name, x - half, x + half, y - half, y + half, z, GREEN)
        # Sunlit (west, -Y) wall is painted; the sun-away (east, +Y) wall is bare canvas.
        slab("Vertebra_W_" + name, x - half - 2, x + half + 2, y - half - 6, y - half, z + WALL_H, UMBER,
             "Geometry/Vertebrae", bottom=z - 6)
        slab("Vertebra_E_" + name, x - half - 2, x + half + 2, y + half, y + half + 6, z + WALL_H, CANVAS,
             "Geometry/Vertebrae", bottom=z - 6)

    for (name_a, xa, ya, za, half_a), (name_b, xb, yb, zb, half_b) in zip(stops, stops[1:]):
        heading = math.atan2(yb - ya, xb - xa)
        ux, uy = math.cos(heading), math.sin(heading)
        # Trim each end so the slope meets the edge of the pads rather than their centres.
        start = (xa + ux * (half_a - 1), ya + uy * (half_a - 1), za)
        end = (xb - ux * (half_b - 1), yb - uy * (half_b - 1), zb)
        ramp("Path_%s_to_%s" % (name_a, name_b), start, end, 12.0, GREEN)
        mid = ((start[0] + end[0]) / 2.0, (start[1] + end[1]) / 2.0, (za + zb) / 2.0)
        length = math.hypot(end[0] - start[0], end[1] - start[1]) + 6.0
        yaw = math.degrees(heading)
        for side, colour in ((-1, UMBER), (1, CANVAS)):
            nx, ny = -uy * side * 9.0, ux * side * 9.0
            box("Gorge_%s_%s_%s" % ("W" if side * ux < 0 or (ux == 0 and side < 0) else "E", name_a, name_b),
                (mid[0] + nx, mid[1] + ny, mid[2] + WALL_H / 2.0 - 3.0), (length, 6.0, WALL_H + 6.0),
                colour, "Geometry/Gorge", yaw=yaw)

    # Tree trunks for cover and sightline breaks.
    trunks = [off("FirstWell", -5, 5), off("FirstWell", 6, -6), off("Bend", 4, 5), off("SunwardClearing", -9, -8),
              off("SunwardClearing", 8, 9), off("SunwardClearing", -4, 10), off("HollowVertebra", -9, 8),
              off("HollowVertebra", 9, -9), off("Approach", 3, -5), off("RibGate", -6, 6)]
    for i, p in enumerate(trunks, 1):
        cylinder("Trunk_%02d" % i, p, 0.6, 9.0, UMBER, "Geometry/Trunks")

    # One-sided rule: passable backs. Each is a front with no collision, carrying the charcoal cross-hatch mark.
    # 1 of 6 is the lesson on the main route, beside the Sunward Clearing Lampstand; the other 5 hide caches.
    lesson = off("SunwardClearing", 15.5, -7)
    box("PassableBack_01_Lesson", (lesson[0], lesson[1], lesson[2] + 4.0), (1.0, 12.0, 8.0), CHARCOAL,
        "Geometry/PassableBacks", collide=False, tags=("PassableBack", "MainRoute"))
    marker("Note", "PassableBackLesson", "Passable back (lesson): walk through", (lesson[0] - 2, lesson[1], lesson[2]))
    hidden = [off("FirstWell", 4, 6), off("Bend", -4, -5), off("Descent", 2, 5),
              off("HollowVertebra", 8, 9), off("Approach", -3, 5)]
    for i, p in enumerate(hidden, 2):
        box("PassableBack_%02d" % i, (p[0], p[1], p[2] + 3.0), (4.0, 4.0, 6.0), CHARCOAL,
            "Geometry/PassableBacks", collide=False, tags=("PassableBack", "Optional"))
        marker("Cache", "Cache%02d" % (i - 1), "Cache (inside a passable back)", p)

    # The Nape Overlook: visible from the route, not reachable before the shift.
    slab("NapeOverlook", 140, 160, 44, 58, 14.0, CANVAS, bottom=-22.0)
    marker("Note", "NapeOverlook", "Nape Overlook (reached only after the shift)", (150, 46, 14))

    # Named places
    marker("Exit", "VarnishVillage", "To Varnish", off("Entry", -8, 0))
    marker("Note", "FirstWell", "The First Well", off("FirstWell", 0, -6))
    marker("Lampstand", "SunwardClearing", "Lampstand", off("SunwardClearing", 11, -3))
    marker("NPC", "CorinHalfSeen", "Corin Half-Seen", off("SunwardClearing", -2, 4))
    marker("Note", "BroodNest", "Hollow Vertebra: Madderback brood nest", off("HollowVertebra", 0, 10))
    marker("Lampstand", "RibGate", "Lampstand", off("RibGate", -6, -6))
    marker("NPC", "IlkaThrum", "Ilka Thrum", off("RibGate", -3, 5))
    marker("Exit", "GrisailleVault", "The Rib Gate: to the Grisaille Vault", off("RibGate", 10, 0))

    # Enemies: 6 Madderbacks and 6 Blotters
    for p in (off("Bend", 3, -3), off("HollowVertebra", -5, -3), off("HollowVertebra", 4, 4),
              off("HollowVertebra", 6, -6), off("Approach", -2, -3), off("Approach", 4, 2)):
        enemy("Madderback", p)
    for p in (off("FirstWell", 6, 2), off("FirstWell", 9, -3), off("Descent", -3, -2), off("Descent", 4, 3),
              off("Approach", 6, -5), off("RibGate", -8, 2)):
        enemy("Blotter", p)

    # Harvest nodes: 5 Terre Verte (sunlit spots), 3 Sanguine, 3 Bone Black
    for p in (off("FirstWell", -6, -7), off("Bend", -5, -6), off("SunwardClearing", -10, -11),
              off("SunwardClearing", 4, -12), off("Descent", -5, -6)):
        node("TerreVerte", p)
    for p in (off("HollowVertebra", -10, 10), off("HollowVertebra", 10, 5), off("Approach", 6, 6)):
        node("Sanguine", p)
    for p in (off("Bend", 6, 6), off("Descent", 6, 6), off("RibGate", 6, 8)):
        node("BoneBlack", p)

    player_start(off("Entry", -3, 0), yaw=0.0)
    return end_level("Scumblewood")


# --------------------------------------------------------------------------------------
# Zone 3: the Grisaille Vault. Rib Stair, Cartoon Gallery, the Heart. Low gravity, unlit paper.
# --------------------------------------------------------------------------------------

def build_vault():
    begin_level("L_Matriarch_Vault_A")
    Zone.unlit, Zone.label_yaw = True, 180.0
    PAPER, GREY, DARK, GRAPHITE, BLUE, HEART = "#E6E1D6", "#8C8A85", "#4B4A48", "#2E2E30", "#2F4BA8", "#E8923A"
    FLOOR = -10.0   # main floor height
    PIT = -18.0     # floor of the gallery pit

    fill_light(PAPER, lux=2.0)

    # Rib Stair: the entry, descending under rib arches.
    slab("Entry_Landing", -10, 2, -6, 6, 0.0, PAPER)
    ramp("RibStair", (1, 0, 0), (31, 0, FLOOR), 8.0, PAPER)
    for i, x in enumerate(range(4, 30, 6), 1):
        z = -(x - 1) / 3.0
        for side in (-1, 1):
            box("Rib_%02d_%s" % (i, "L" if side < 0 else "R"), (x, side * 5.0, z + 4.0), (1.0, 1.0, 10.0),
                GRAPHITE, "Geometry/Ribs")
        box("Rib_%02d_Arch" % i, (x, 0, z + 9.5), (1.0, 11.0, 1.0), GRAPHITE, "Geometry/Ribs")
    slab("RibStair_Foot", 30, 46, -12, 12, FLOOR, PAPER, bottom=PIT - 1.0)

    # Cartoon Gallery: sketched platforms over a pit. Gaps are 6 m, which needs this zone's 0.6x gravity.
    slab("Gallery_PitFloor", 46, 106, -16, 16, PIT, GREY)
    platforms = [(51, 0, 0.0), (61, 3, 0.5), (71, -2, 0.0), (81, 3, -0.5), (91, -1, 0.5), (101, 0, 0.0)]
    for i, (x, y, dz) in enumerate(platforms, 1):
        slab("SketchPlatform_%02d" % i, x - 2, x + 2, y - 2, y + 2, FLOOR + dz, BLUE,
             "Geometry/SketchPlatforms", bottom=FLOOR + dz - 0.4, tags=("SketchPlatform",))
    slab("SketchPlatform_Side", 79, 83, -12, -8, FLOOR, BLUE, "Geometry/SketchPlatforms",
         bottom=FLOOR - 0.4, tags=("SketchPlatform", "Optional"))
    # A way back up for anyone who falls: there is no fall damage here, so a fall costs time, not health.
    ramp("Gallery_ReturnRamp", (68, -14, PIT), (46, -14, FLOOR), 3.0, GREY)
    slab("RibStair_Foot_Ledge", 44, 46, -16, -12, FLOOR, PAPER, bottom=PIT - 1.0)

    # Far side, then the Heart.
    slab("Gallery_FarLedge", 106, 122, -12, 12, FLOOR, PAPER, bottom=PIT - 1.0)
    cylinder("Heart_Floor", (140, 0, FLOOR - 1.0), 18.0, 1.0, PAPER, "Geometry/Heart")
    for i in range(16):
        angle = math.radians(i * 22.5)
        if abs(math.degrees(angle) - 180.0) < 20.0:
            continue  # the doorway from the far ledge
        x, y = 140 + math.cos(angle) * 18.5, math.sin(angle) * 18.5
        box("Heart_Wall_%02d" % i, (x, y, FLOOR + 5.0), (1.5, 7.6, 12.0), DARK, "Geometry/Heart",
            yaw=math.degrees(angle))
    cylinder("Heart_Glow", (153, 0, FLOOR), 2.0, 5.0, HEART, "Geometry/Heart", collide=False, tags=("Heart",))

    # Enclosing walls
    slab("Wall_South", 30, 124, -17, -16, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=PIT - 1.0)
    slab("Wall_North", 30, 124, 16, 17, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=PIT - 1.0)
    slab("Wall_Foot_S", 30, 46, -16, -12, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=FLOOR)
    slab("Wall_Foot_N", 30, 46, 12, 16, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=PIT - 1.0)
    slab("Wall_Far_S", 106, 122, -16, -12, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=PIT - 1.0)
    slab("Wall_Far_N", 106, 122, 12, 16, FLOOR + 14.0, DARK, "Geometry/Walls", bottom=PIT - 1.0)

    # Named places
    marker("Exit", "Scumblewood", "To the Rib Gate", (-9, 0, 0))
    marker("Note", "RibStair", "The Rib Stair", (33, 0, FLOOR))
    marker("Note", "CartoonGallery", "Cartoon Gallery: platforms are solid only while lit", (44, 6, FLOOR))
    marker("Lampstand", "RibStairHeart", "Lampstand", (116, -8, FLOOR))
    marker("Note", "TheHeart", "The Heart: boss arena and the Heart choice", (126, 0, FLOOR))

    # Enemies: 6 Pentiments, 4 Blotters and the boss
    for p in ((36, -6, FLOOR), (40, 7, FLOOR), (61, 3, FLOOR + 0.5), (81, 3, FLOOR - 0.5),
              (88, 10, PIT), (110, 6, FLOOR)):
        enemy("Pentiment", p)
    for p in ((56, 8, PIT), (74, -6, PIT), (109, -5, FLOOR), (118, 4, FLOOR)):
        enemy("Blotter", p)
    enemy("ThePalimpsest", (142, 0, FLOOR))

    # Harvest nodes: 3 Bone Black, 3 Slumber Blue (one-time)
    for p in ((43, -10, FLOOR), (96, -10, PIT), (120, 10, FLOOR)):
        node("BoneBlack", p)
    for p in ((60, 13, PIT), (81, -10, FLOOR), (128, -4, FLOOR)):
        node("SlumberBlue", p)

    player_start((-6, 0, 0), yaw=0.0)
    return end_level("GrisailleVault")


def build_all():
    problems = []
    with unreal.ScopedSlowTask(3, "Building the Sleeping Matriarch grey-box") as task:
        task.make_dialog(True)
        for build in (build_scumblewood, build_vault, build_village):  # village last, so it is left open
            task.enter_progress_frame(1)
            problems += build()
    eal.save_directory("/Game/Greybox")
    unreal.log("GREYBOX DONE. Problems: %s" % (problems or "none"))
    return problems


if __name__ == "__main__":
    print(build_all() or "no problems")
