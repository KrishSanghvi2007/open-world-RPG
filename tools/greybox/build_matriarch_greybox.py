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
import sys
import os

import unreal

MAP_DIR = "/Game/Maps/Matriarch"
MAT_DIR = "/Game/Greybox/Materials"

CUBE = "/Engine/BasicShapes/Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder"
SPHERE = "/Engine/BasicShapes/Sphere"

import importlib.util

_spec = importlib.util.spec_from_file_location("stylekit", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "stylekit", "stylekit.py"))
stylekit = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(stylekit)

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


# Which kind of painted surface a piece of geometry is, from the start of its name (see stylekit.SURFACES).
SURFACE_BY_NAME = (
    (("Floor", "Terrace", "Pad", "Path", "Shoulder", "Bedrock", "Entry", "Gallery_PitFloor", "RibStair", "Gallery_FarLedge", "Heart_Floor", "Ramp", "LessonReturn"), "Ground"),
    (("Wall", "Vertebra", "Gorge", "Parapet", "Pillar", "Rib_", "Heart_Wall", "NapeOverlook", "FalseFront"), "Stone"),
    (("Trunk",), "Wood"),
    (("House",), "Plaster"),
)


def surface_for(name):
    if name.endswith("_Roof"):
        return "Roof"
    for prefixes, surface in SURFACE_BY_NAME:
        if name.startswith(prefixes):
            return surface
    return "Plaster"


def material(hex_colour, unlit=False, surface="Plaster"):
    """The style-kit material for a colour: painted Glaze in lit zones, paper Sketch in sketch zones."""
    if unlit:
        return stylekit.sketch(hex_colour, surface)
    return stylekit.glaze(hex_colour, surface, Zone.shadow_tint)


def flat_material(hex_colour, unlit=True):
    """A plain flat colour, for markers that must stay readable in any light."""
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
    shadow_tint = "#4A3558"      # the zone's shadow colour (GDD palette table)
    look = "VillageDusk"         # a preset in stylekit.LOOKS
    flat = False                 # True while placing markers


def v(p):
    return unreal.Vector(p[0] * 100.0, p[1] * 100.0, p[2] * 100.0)


def _mesh_actor(name, mesh_path, pos, scale, colour, folder, rot=None, collide=True, tags=()):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, v(pos), rot or unreal.Rotator(0, 0, 0))
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(mesh_path))
    comp.set_material(0, flat_material(colour) if Zone.flat else material(colour, Zone.unlit, surface_for(name)))
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
    "Lore": (CUBE, (0.05, 0.5, 0.65), "#F2E3C0"),
}

PIGMENT_COLOURS = {
    "Ochre": "#D9A441", "TerreVerte": "#6B8F71", "Sanguine": "#9E2B25",
    "BoneBlack": "#2A2826", "Ceruse": "#EDE8DC", "SlumberBlue": "#2F4BA8",
}


# Markers that have a working C++ actor are placed as that actor; the rest stay labelled placeholder shapes.
STATIONS = {"GrindingSlab": "GRINDING_SLAB", "ArmatureBench": "ARMATURE_BENCH"}
LIT_FROM_THE_START = {"SunwardClearing", "RibStairHeart", "DevBlotter", "DevMadderback", "DevPentiment", "DevThePalimpsest"}      # wild pilot flames; the others must be relit
PICKUPS = {"FlaxPress": ("LinseedOil", 2, "the flax press")}
ENEMY_CLASSES = {"Blotter": ("BlotterEnemy", 1.3), "Madderback": ("MadderbackEnemy", 0.9),
                 "Pentiment": ("PentimentEnemy", 1.3), "ThePalimpsest": ("OldPalimpsest", 2.4)}


def _spawn_working_actor(kind, data_id, text, ground, yaw):
    """Returns the real actor for this marker, or None if the game has no class for it yet."""
    def spawn(cls_name, height):
        cls = getattr(unreal, cls_name, None)
        if cls is None:
            return None
        return actors.spawn_actor_from_class(cls, v((ground[0], ground[1], ground[2] + height)), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))

    actor = None
    if kind == "Node":
        actor = spawn("UnderpaintingHarvestNode", 0.4)
        if actor:
            actor.set_editor_property("pigment_id", unreal.Name(data_id))
    elif kind == "Lampstand":
        actor = spawn("UnderpaintingLampstand", 1.5)
        if actor:
            actor.set_editor_property("lampstand_id", unreal.Name(data_id))
            actor.set_editor_property("starts_lit", data_id in LIT_FROM_THE_START)
    elif kind == "NPC":
        actor = spawn("UnderpaintingNPC", 0.9)
        if actor:
            actor.set_editor_property("npc_id", unreal.Name(data_id))
            actor.set_editor_property("display_name", text)
    elif kind == "Station" and data_id in STATIONS:
        actor = spawn("UnderpaintingCraftingStation", 0.45)
        if actor:
            actor.set_editor_property("station", getattr(unreal.CraftingStation, STATIONS[data_id]))
    elif kind == "Note" and data_id in PICKUPS:
        actor = spawn("UnderpaintingPickup", 0.5)
        if actor:
            item, count, source = PICKUPS[data_id]
            actor.set_editor_property("item_id", unreal.Name(item))
            actor.set_editor_property("count", count)
            actor.set_editor_property("source_name", source)
    elif kind == "Enemy" and data_id in ENEMY_CLASSES:
        cls_name, height = ENEMY_CLASSES[data_id]
        actor = spawn(cls_name, height)
    elif kind == "Exit":
        actor = spawn("UnderpaintingZoneExit", 2.0)
        if actor:
            actor.set_editor_property("target_zone", unreal.Name(data_id))
    elif kind == "Cache":
        actor = spawn("UnderpaintingCache", 0.3)
    elif kind == "Lore":
        actor = spawn("UnderpaintingLoreNote", 1.2)
        if actor:
            actor.set_editor_property("lore_id", unreal.Name(data_id))
    return actor


def marker(kind, data_id, text, ground, colour=None, yaw=0.0, labelled=True):
    """A labelled gameplay object standing on the ground point given (metres): the working actor where the
    game has one, otherwise a placeholder shape. Either way it carries the tags Marker, its kind and its ID."""
    mesh, size, default_colour = MARKER_STYLES[kind]
    index = _counts.get((kind, data_id), 0) + 1
    _counts[(kind, data_id)] = index
    name = "%s_%s_%02d" % (kind, data_id, index)

    actor = _spawn_working_actor(kind, data_id, text, ground, yaw)
    if actor:
        actor.set_actor_label(name)
        actor.set_folder_path("Gameplay/" + kind)
        actor.tags = [unreal.Name(t) for t in ("Marker", kind, data_id)]
    else:
        centre = (ground[0], ground[1], ground[2] + size[2] / 2.0)
        Zone.flat = True                          # markers stay readable in any light
        actor = _mesh_actor(name, mesh, centre, size, colour or default_colour,
                            "Markers/" + kind, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw),
                            collide=False, tags=("Marker", kind, data_id))
        Zone.flat = False
    if kind != "Lore" and labelled:      # things to find, and things that may not be there, do not announce themselves
        label(text, (ground[0], ground[1], ground[2] + size[2] + 0.5), "Markers/" + kind + "/Labels")
    return actor


LORE = {row["Name"]: row for row in json.load(open(os.path.join(unreal.Paths.project_dir(), "data", "lore.json")))}


def lore(lore_id, ground, yaw=0.0):
    """Something to read (a row of data/lore.json)."""
    return marker("Lore", lore_id, LORE[lore_id]["Title"], ground, yaw=yaw)


def cache(cache_id, text, ground, contents, labelled=True):
    """A one-time store. contents: (pigment or item ID, count) pairs."""
    actor = marker("Cache", cache_id, text, ground, labelled=labelled)
    if hasattr(unreal, "UnderpaintingCache") and isinstance(actor, unreal.UnderpaintingCache):
        entries = []
        for item_id, count in contents:
            entry = unreal.IdCount()
            entry.set_editor_property("id", unreal.Name(item_id))
            entry.set_editor_property("count", count)
            entries.append(entry)
        actor.set_editor_property("contents", entries)
    return actor


def place(place_id, name, centre, half=(5.0, 5.0, 3.0)):
    """A named place: walking in discovers it and counts as reaching it for quests. Invisible."""
    if not hasattr(unreal, "UnderpaintingPlace"):
        return None
    actor = actors.spawn_actor_from_class(unreal.UnderpaintingPlace, v((centre[0], centre[1], centre[2] + half[2])), unreal.Rotator(0, 0, 0))
    actor.set_editor_property("place_id", unreal.Name(place_id))
    actor.set_editor_property("display_name", name)
    actor.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(half[0] * 100.0, half[1] * 100.0, half[2] * 100.0))
    actor.set_actor_label("Place_" + place_id)
    actor.set_folder_path("Gameplay/Places")
    actor.tags = [unreal.Name(t) for t in ("Marker", "Place", place_id)]
    _counts[("Place", place_id)] = _counts.get(("Place", place_id), 0) + 1
    return actor


def inspectable(inspect_id, title, text, ground):
    """Something to look at closely. Counts as the Inspect event for quests."""
    actor = actors.spawn_actor_from_class(unreal.UnderpaintingInspectable, v((ground[0], ground[1], ground[2] + 0.13)), unreal.Rotator(0, 0, 0))
    actor.set_editor_property("inspect_id", unreal.Name(inspect_id))
    actor.set_editor_property("title", title)
    actor.set_editor_property("text", text)
    index = _counts.get(("Inspect", inspect_id), 0) + 1
    _counts[("Inspect", inspect_id)] = index
    actor.set_actor_label("Inspect_%s_%02d" % (inspect_id, index))
    actor.set_folder_path("Gameplay/Inspect")
    actor.tags = [unreal.Name(t) for t in ("Marker", "Inspect", inspect_id)]
    return actor


def choice_point(quest_id, display_name, centre, yaw=0.0, size=(0.2, 2.2, 2.6)):
    """Where a quest is ended (its Choices in data/quests.json are offered here)."""
    actor = actors.spawn_actor_from_class(unreal.UnderpaintingChoicePoint, v(centre), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    actor.set_editor_property("quest_id", unreal.Name(quest_id))
    actor.set_editor_property("display_name", display_name)
    actor.set_actor_scale3d(unreal.Vector(*size))
    actor.set_actor_label("Choice_" + quest_id)
    actor.set_folder_path("Gameplay/Choices")
    actor.tags = [unreal.Name(t) for t in ("Marker", "Choice", quest_id)]
    _counts[("Choice", quest_id)] = 1
    return actor


def needs_flag(actor, flag):
    """The actor does not exist in the world until this flag is set."""
    actor.set_editor_property("requires_flag", unreal.Name(flag))
    return actor


def sketch_platform(name, x0, x1, y0, y1, top, thickness=0.4, tags=()):
    """A sketched platform: solid only while a Flare lights it, or once fixed with a Fixative Flask."""
    if not hasattr(unreal, "UnderpaintingSketchPlatform"):
        return slab(name, x0, x1, y0, y1, top, "#2F4BA8", "Geometry/SketchPlatforms", bottom=top - thickness, tags=("SketchPlatform",) + tuple(tags))
    centre = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, top - thickness / 2.0)
    actor = actors.spawn_actor_from_class(unreal.UnderpaintingSketchPlatform, v(centre), unreal.Rotator(0, 0, 0))
    actor.set_actor_scale3d(unreal.Vector(abs(x1 - x0), abs(y1 - y0), thickness))
    actor.set_actor_label(name)
    actor.set_folder_path("Gameplay/SketchPlatforms")
    actor.tags = [unreal.Name(t) for t in ("SketchPlatform",) + tuple(tags)]
    return actor


def passable_back(name, centre, size, front_yaw, tags=()):
    """A front with nothing behind it. Solid from its painted side (front_yaw points out of that side);
    from behind, where the charcoal cross-hatch mark is, you can walk through."""
    if not hasattr(unreal, "UnderpaintingPassableBack"):
        return box(name, centre, size, "#3A3A3A", "Geometry/PassableBacks", yaw=front_yaw, collide=False, tags=("PassableBack",) + tuple(tags))
    actor = actors.spawn_actor_from_class(unreal.UnderpaintingPassableBack, v(centre), unreal.Rotator(roll=0.0, pitch=0.0, yaw=front_yaw))
    actor.set_actor_scale3d(unreal.Vector(*size))
    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    comp.set_material(0, material("#3A3A3A", Zone.unlit, "Stone"))
    actor.set_actor_label(name)
    actor.set_folder_path("Gameplay/PassableBacks")
    actor.tags = [unreal.Name(t) for t in ("PassableBack",) + tuple(tags)]
    # The mark that makes it fair: a pale cross-hatched patch on the back, always the same.
    back = math.radians(front_yaw + 180.0)
    mark = (centre[0] + math.cos(back) * (size[0] / 2.0 + 0.03), centre[1] + math.sin(back) * (size[0] / 2.0 + 0.03), centre[2])
    box(name + "_Mark", mark, (0.04, min(size[1], 1.6), min(size[2], 1.6)), "#D8CBB0", "Geometry/PassableBacks", yaw=front_yaw, collide=False)
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

def sun_and_sky(*_args, **_kwargs):
    """Sky, sun, tinted sky light, haze and post-process for the zone being built, from the style kit."""
    stylekit.apply_look(Zone.look)


def fill_light(*_args, **_kwargs):
    stylekit.apply_look(Zone.look)


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
    expected_lore = sorted(name for name, entry in LORE.items() if entry["Zone"] == zone_id)
    placed_lore = sorted(data_id for (kind, data_id) in _counts if kind == "Lore")
    if expected_lore != placed_lore:
        problems.append("%s: lore placed %s, data says %s" % (zone_id, placed_lore, expected_lore))
    for (kind, data_id) in _counts:
        if kind == "Enemy" and data_id not in row["Enemies"] + row["EnemiesAfterShift"]:
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

def build_village(shifted=False):
    """Varnish. shifted=True builds the "B" level: after the Turning her shoulder has dropped, the Hem is in
    shadow and half-painted, the Furrow is shut and the Stair of Spines arrives at a new lower gate."""
    begin_level("L_Matriarch_Village_B" if shifted else "L_Matriarch_Village_A")
    Zone.unlit, Zone.label_yaw, Zone.shadow_tint, Zone.look = False, -90.0, "#4A3558", "VillageAfter" if shifted else "VillageDusk"
    OCHRE, ROSE, CREAM, TEAL, VIOLET = "#D9A441", "#C4574B", "#F2E3C0", "#2E5E6E", "#4A3558"
    CANVAS = "#D8CBB0"

    sun_and_sky(sun_colour="#FFD9A0", sky_tint=TEAL, sun_lux=7.0, sky_intensity=1.2)

    # Her shoulder: a wide base so nobody falls out of the world.
    slab("Shoulder", -60, 60, -70, 60, -0.01, VIOLET, bottom=-3.0)

    # Terraces. Top (east, y 18..42, height 8), middle (y -10..18, height 4), the Hem (y -38..-10, height 0).
    slab("Terrace_Sunward", -34, 34, 18, 42, 8.0, CREAM, bottom=-1.0)
    slab("Terrace_LampstandSquare", -34, 34, -10, 18, 4.0, CREAM, bottom=-1.0)
    slab("Terrace_Hem", -34, 34, -38, -10, 0.0, CANVAS if shifted else CREAM, bottom=-1.0)

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
    if shifted:
        # The Furrow has closed: the gap is a wall of risen bone now.
        slab("Furrow_Closed", 34, 37, 0, 8, 9.0, VIOLET, "Geometry/Parapets", bottom=4.0)

    def house(index, x, y, base):
        # After the Turning the Hem is in shadow: its houses are down to bare canvas, the roofs dulled.
        faded = shifted and base == 0
        box("House_%02d" % index, (x, y, base + 2.5), (6, 6, 5), CANVAS if faded else CREAM, "Geometry/Houses")
        box("House_%02d_Roof" % index, (x, y, base + 5.4), (7, 7, 0.8), VIOLET if faded else ROSE, "Geometry/Houses")

    houses = [(-20, 37, 8), (0, 37, 8), (20, 37, 8),                       # Sunward Terraces
              (-8, 13, 4), (6, 13, 4), (20, 13, 4), (-28, -4, 4), (28, -5, 4),  # Lampstand square
              (-24, -31, 0), (-10, -31, 0), (4, -31, 0), (27, -31, 0)]          # the Hem
    for i, (x, y, base) in enumerate(houses, 1):
        house(i, x, y, base)

    # Lampstand square
    marker("Lampstand", "Varnish", "Lampstand", (0, 0, 4))
    marker("NPC", "HesperVane", "Hesper Vane", (3.5, 2.5, 4))        # beside the Lampstand, out of its reach
    marker("NPC", "BartramSedge", "Bartram Sedge", (-8, -7.8, 4))      # behind his slab, clear of it
    marker("Station", "GrindingSlab", "Grinding Slab", (-8, -5, 4))
    marker("Station", "ArmatureBench", "Armature Bench", (9, -5, 4))
    marker("Note", "FlaxPress", "Flax press", (15, 3, 4))
    if not shifted:
        marker("Exit", "Scumblewood", "To the Scumblewood (the Furrow)", (33, 4, 4), yaw=90.0)

    # Sunward Terraces: the four Ochre nodes
    for x in (-27, -10, 10, 27):
        node("Ochre", (x, 24, 8))

    # The Hem
    if shifted:
        marker("Exit", "Scumblewood", "The lower gate: the Stair of Spines", (0, -37, 0))
        for side in (-1, 1):
            box("LowerGate_Post_%s" % ("L" if side < 0 else "R"), (side * 2.6, -37.5, 2.5), (0.8, 0.8, 5.0), VIOLET, "Geometry/Parapets")
        # Corin, if he was finished in Bone Black, stands guard here and the Blotters stay away.
        guard = marker("NPC", "CorinHalfSeen", "Corin, at the gate", (6, -34, 0), labelled=False)
        if guard:
            guard.set_editor_property("requires_flag", unreal.Name("Corin_Gatekeeper"))
        for p in ((-17, -22, 0), (13, -19, 0)):
            blotter = enemy("Blotter", p)
            if isinstance(blotter, unreal.UnderpaintingEnemy):
                blotter.set_editor_property("removed_by_flag", unreal.Name("Village_HemGuarded"))
    else:
        marker("Note", "LowerGate", "Lower gate (appears after the shift)", (0, -37, 0))
        marker("Note", "HemFlaking", "The Hem: paint starting to flake", (-17, -22, 0))

    # A Fair Grind: the first wall of the Hem, by the ramp. It takes the colour Bartram is given, for good.
    choice_point("SQ_AFairGrind", "the first wall of the Hem", (27, -27.85, 1.5), yaw=90.0, size=(0.1, 2.6, 2.8))

    # Things to read, and the named places.
    lore("Odile_01", (3.0, -36.5, 0), yaw=90.0)
    lore("Ledger_Hesper", (1.6, 3.4, 4), yaw=200.0)
    lore("Ledger_Bartram", (-6.4, -6.2, 4), yaw=120.0)
    place("LampstandSquare", "The Lampstand Square", (0, 4, 4), half=(14, 12, 3))
    place("SunwardTerraces", "The Sunward Terraces", (0, 30, 8), half=(30, 11, 3))
    place("TheHem", "The Hem", (0, -24, 0), half=(30, 12, 3))

    # The far ends of the Hem and of the top terrace are dead ends: each has something small for whoever walks there.
    cache("HemCorner", "Cache", (-30, -14, 0), [("ShedHide", 2)])
    cache("TerraceCorner", "Cache", (31, 39, 8), [("LinseedOil", 2)])

    # No combat in the village before the shift (GDD). Combat can be tried in the rooms under Maps/Dev.

    player_start((-4, 4, 4), yaw=0.0)
    return end_level("VarnishVillage")


# --------------------------------------------------------------------------------------
# Zone 2: the Scumblewood. The Furrow, a gorge winding down between two vertebrae.
# --------------------------------------------------------------------------------------

def build_scumblewood(shifted=False):
    """The Scumblewood. shifted=True builds the "B" level: her spine has arched, the top of the Furrow is shut
    with the First Well inside it, and five vertebrae have risen into the Stair of Spines."""
    begin_level("L_Matriarch_Scumblewood_B" if shifted else "L_Matriarch_Scumblewood_A")
    Zone.unlit, Zone.label_yaw, Zone.shadow_tint, Zone.look = False, 180.0, "#2E5E6E", "ForestAfter" if shifted else "ForestHalfLight"
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
    STAIR_TOP = (174.0, 50.0, 14.0)      # the fifth vertebra, where the lower gate to Varnish stands
    CAMP = (223.0, 50.0, 2.0)            # the third vertebra: Ilka's camp after the First Well was sealed
    if shifted:
        # The Entry and the First Well are gone. What stood there has moved onto the Stair.
        at["Entry"], at["FirstWell"] = STAIR_TOP, CAMP
        stops = stops[2:]

    def off(stop, dx=0.0, dy=0.0):
        x, y, z = at[stop]
        if shifted and stop == "FirstWell":
            dx, dy = dx * 0.6, dy * 0.6      # a vertebra is smaller than the old clearing
        return (x + dx, y + dy, z)

    slab("Bedrock", -30, 310, -60, 80, -22.0, CHARCOAL, bottom=-24.0)

    WALL_H = 14.0
    for name, x, y, z, half in stops:
        slab("Pad_" + name, x - half, x + half, y - half, y + half, z, GREEN)
        # Sunlit (west, -Y) wall is painted; the sun-away (east, +Y) wall is bare canvas.
        slab("Vertebra_W_" + name, x - half - 2, x + half + 2, y - half - 6, y - half, z + WALL_H, UMBER,
             "Geometry/Vertebrae", bottom=z - 6)
        if shifted and name == "RibGate":
            continue                          # this wall is where the Stair of Spines rose
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
    if shifted:
        trunks = trunks[2:]
    for i, p in enumerate(trunks, 1):
        cylinder("Trunk_%02d" % i, p, 0.6, 9.0, UMBER, "Geometry/Trunks")

    # One-sided rule: passable backs (GDD section 6). There are 6. The main route needs the trick exactly once,
    # beside the Sunward Clearing Lampstand: going down, you meet the bare back of a painted cliff and walk
    # through it. Coming back up you face its painted front, which is solid, so a ramp climbs over it and you
    # drop down on the village side.
    heading = math.atan2(at["Descent"][1] - at["SunwardClearing"][1], at["Descent"][0] - at["SunwardClearing"][0])
    ux, uy = math.cos(heading), math.sin(heading)
    lx, ly, lz = at["SunwardClearing"][0] + ux * 19.0, at["SunwardClearing"][1] + uy * 19.0, at["SunwardClearing"][2] - 0.4
    passable_back("PassableBack_01_Lesson", (lx, ly, lz + 1.75), (1.0, 12.0, 3.5), math.degrees(heading), tags=("MainRoute",))
    marker("Note", "PassableBackLesson", "Bare canvas, cross-hatched: walk through", (lx - ux * 2.5, ly - uy * 2.5, lz + 0.3))
    side = (-uy * 4.0, ux * 4.0)                                        # the ramp hugs one side of the gorge
    ramp("LessonReturn_Ramp", (lx + ux * 10.0 + side[0], ly + uy * 10.0 + side[1], lz - 0.9),
         (lx + ux * 0.5 + side[0], ly + uy * 0.5 + side[1], lz + 3.5), 3.0, UMBER)

    # The other five hide something: a painted rock or trunk that is only a front, entered from its sun-away side.
    hidden = [
        (off("FirstWell", 4, 6), "Cache01", [("TerreVerte", 2)]),
        (off("Bend", -4, -5), "Cache02", [("BoneBlack", 2), ("RibSplinter", 1)]),
        (off("Descent", 2, 5), "Cache03", [("ShedHide", 2)]),
        (off("HollowVertebra", 8, 9), "Cache04", [("Sanguine", 2), ("RibSplinter", 2)]),
        (off("Approach", -3, 5), "Cache05", [("LampOil", 1), ("ShedHide", 1)]),
    ]
    margin_anchor = hidden[2][0]
    if shifted:
        hidden = hidden[1:]                   # the first cache is sealed in the Furrow with the well
    for i, (p, cache_id, contents) in enumerate(hidden, 2):
        passable_back("PassableBack_%02d" % i, (p[0], p[1] - 2.0, p[2] + 2.5), (0.6, 4.4, 5.0), -90.0, tags=("Optional",))   # front faces the sun (-Y)
        for j, x_offset in enumerate((-2.2, 2.2)):
            box("FalseFront_%02d_Side%d" % (i, j), (p[0] + x_offset, p[1], p[2] + 2.5), (0.5, 4.0, 5.0), UMBER, "Geometry/PassableBacks")
        box("FalseFront_%02d_Top" % i, (p[0], p[1], p[2] + 5.2), (4.9, 4.4, 0.5), UMBER, "Geometry/PassableBacks")
        cache(cache_id, "Cache", p, contents)
    lore("Margin_05", (margin_anchor[0] + 1.2, margin_anchor[1] + 0.5, margin_anchor[2]), yaw=-90.0)

    # The Nape Overlook: visible from the route, not reachable before the shift.
    slab("NapeOverlook", 140, 160, 44, 58, 14.0, CANVAS, bottom=-22.0)
    if not shifted:
        marker("Note", "NapeOverlook", "Nape Overlook (reached only after the shift)", (150, 46, 14))

    if shifted:
        BONE = "#E6E1D6"
        # The Furrow is shut at the Bend. The First Well is inside the rock.
        box("FurrowSeal", (70, 20, 1.0), (6.0, 30.0, 22.0), UMBER, "Geometry/Shift")
        marker("Note", "FirstWellPentimento", "A faint outline on the rock: the First Well was here", (74.5, 20, -6))

        # The Stair of Spines: five risen vertebrae from the Rib Gate up to the lower gate of Varnish.
        slab("Stair_Foot", 259, 285, 26, 31, -16.0, GREEN, "Geometry/Shift", bottom=-22.0)
        ramp("Stair_Ramp_0", (272, 18, -16), (272, 31, -10), 10.0, BONE, "Geometry/Shift")
        slab("Vertebra_1", 264, 281, 31, 57, -10.0, BONE, "Geometry/Shift", bottom=-22.0)
        # Each ramp runs exactly from the edge of one vertebra to the edge of the next, so there is no lip to trip on.
        rises = [(264.0, 251.0, -10.0, -4.0, 241.0), (241.0, 228.0, -4.0, 2.0, 218.0), (218.0, 205.0, 2.0, 8.0, 195.0),
                 (195.0, 182.0, 8.0, 14.0, 165.0)]
        for i, (x0, x1, z0, z1, far_edge) in enumerate(rises, 2):
            ramp("Stair_Ramp_%d" % (i - 1), (x0, 50, z0), (x1, 50, z1), 14.0, BONE, "Geometry/Shift")
            slab("Vertebra_%d" % i, far_edge, x1, 43, 57, z1, BONE, "Geometry/Shift", bottom=-22.0)
        # Her flanks: nobody falls off the Stair.
        slab("Stair_Flank_S", 160, 264, 42, 43, 17.0, UMBER, "Geometry/Shift", bottom=-22.0)
        slab("Stair_Flank_N", 160, 282, 57, 58, 17.0, CANVAS, "Geometry/Shift", bottom=-22.0)
        slab("Stair_Flank_E", 281, 282, 26, 58, 17.0, CANVAS, "Geometry/Shift", bottom=-22.0)
        slab("Stair_Flank_W", 263, 264, 26, 42, 17.0, UMBER, "Geometry/Shift", bottom=-22.0)
        place("StairOfSpines", "The Stair of Spines", (272, 40, -10), half=(8, 9, 4))

        # The Nape Overlook, reachable at last: a short bridge from the top vertebra.
        slab("Nape_Bridge", 160, 165, 46, 54, 14.0, BONE, "Geometry/Shift", bottom=13.0)
        for name, x0, x1, y0, y1 in (("W", 139, 140, 43, 59), ("S", 139, 160, 43, 44), ("N", 139, 160, 58, 59)):
            slab("Nape_Parapet_" + name, x0, x1, y0, y1, 15.1, UMBER, "Geometry/Shift", bottom=14.0)
        place("NapeOverlook", "The Nape Overlook", (150, 51, 14), half=(9, 6, 4))
        cache("NapeCache", "Cache", (146, 55, 14), [("SlumberBlue", 2), ("ShedHide", 3)])
        marker("Note", "NextColossus", "On the horizon, something vast is wading", (141.5, 51, 14))
        # The next colossus: a shape on the horizon and nothing more (Act 2 is not built).
        for name, pos, size in (("Body", (-1500, 200, 160), (260, 520, 240)), ("Neck", (-1500, -120, 330), (90, 90, 260)),
                                ("Head", (-1500, -200, 480), (150, 220, 90)), ("Leg_A", (-1500, 60, 0), (70, 70, 260)),
                                ("Leg_B", (-1500, 360, 0), (70, 70, 260))):
            box("Wader_" + name, pos, size, "#2E5E6E", "Geometry/Horizon", collide=False)

    # Named places
    if shifted:
        marker("Exit", "VarnishVillage", "The lower gate: up to Varnish", (STAIR_TOP[0], 56.0, STAIR_TOP[2]))
    else:
        marker("Exit", "VarnishVillage", "To Varnish", off("Entry", -8, 0))
        marker("Note", "FirstWell", "The First Well", off("FirstWell", 0, -6))
    marker("Lampstand", "SunwardClearing", "Lampstand", off("SunwardClearing", 11, -3))
    corin = marker("NPC", "CorinHalfSeen", "Corin Half-Seen", off("SunwardClearing", -2, 4), labelled=not shifted)
    if shifted:
        # Finished, he has somewhere to be: the foot of the Stair (Terre Verte) or the village gate (Bone Black).
        if corin:
            corin.set_editor_property("hidden_by_flag", unreal.Name("Corin_Moved"))
        warden = marker("NPC", "CorinHalfSeen", "Corin, warden of the wood", (280, 20, -16), labelled=False)
        if warden:
            warden.set_editor_property("requires_flag", unreal.Name("Corin_Warden"))
    marker("Note", "BroodNest", "Hollow Vertebra: Madderback brood nest", off("HollowVertebra", 0, 10))
    marker("Lampstand", "RibGate", "Lampstand", off("RibGate", -6, -6))
    marker("NPC", "IlkaThrum", "Ilka Thrum", off("FirstWell", -3, -4))
    marker("Exit", "GrisailleVault", "The Rib Gate: to the Grisaille Vault", off("RibGate", 10, 0))

    # Things to read, and the named places.
    # In plain sight at the far corner of two clearings: the materials gear is made of. Nobody should have to find
    # a hidden wall to be able to craft at all, and walking to the end of a clearing should be worth it.
    cache("BendCorner", "Cache", off("Bend", 7, -7), [("RibSplinter", 2)])
    cache("DescentCorner", "Cache", off("Descent", -7, 7), [("ShedHide", 1), ("RibSplinter", 1)])

    lore("Odile_02", off("FirstWell", 2, -7), yaw=90.0)
    lore("Ledger_Ilka", off("FirstWell", -5, -5), yaw=60.0)
    lore("Odile_03", off("SunwardClearing", -8, 2), yaw=0.0)
    lore("Odile_04", off("RibGate", 3, -7), yaw=90.0)
    if not shifted:
        place("FirstWell", "The First Well", at["FirstWell"], half=(9, 9, 4))
    place("SunwardClearing", "The Sunward Clearing", at["SunwardClearing"], half=(14, 14, 4))
    place("CorinsBack", "", off("SunwardClearing", -2, 6.5), half=(1.5, 1.2, 2))
    place("BroodNest", "The Hollow Vertebra", at["HollowVertebra"], half=(13, 13, 4))
    place("RibGate", "The Rib Gate", at["RibGate"], half=(10, 10, 4))

    # The Tick-Warden's Ledger: three blood wells with Ilka's chalk beside them, and the nest to decide about.
    wells = [
        (off("HollowVertebra", -10, 8.5), "Bitten this season: 14. Flow: free and bright. I. T."),
        (off("HollowVertebra", 10, 3.5), "Bitten this season: 9. Flow: free. The scab at the rim is last year's, where they stopped coming. I. T."),
        (off("Approach", 6, 4.5), "Bitten this season: 0. Scabbed shut since spring. Note the moss has gone grey round it. I. T."),
    ]
    for position, chalk in wells:
        inspectable("BloodWell", "a blood well, and the chalk beside it", chalk, position)
    choice_point("SQ_TickWardensLedger", "the brood nest in the Hollow Vertebra", off("HollowVertebra", 0, 12.4) [:2] + (at["HollowVertebra"][2] + 1.3,), yaw=90.0)

    # The Unseen Side: Corin's bare back, and the cache only an unfinished Corin can point to.
    choice_point("SQ_TheUnseenSide", "Corin's unpainted back", off("SunwardClearing", -2, 5.0)[:2] + (at["SunwardClearing"][2] + 1.0,), yaw=90.0, size=(0.1, 0.7, 1.6))
    needs_flag(cache("CorinsCache", "Cache", off("SunwardClearing", -12, 12), [("SlumberBlue", 2)], labelled=False), "Scumblewood_PentimentoCacheRevealed")

    # Enemies: 6 Madderbacks and 6 Blotters. Burning the nest halves the Madderbacks.
    for i, p in enumerate((off("Bend", 3, -3), off("HollowVertebra", -5, -3), off("HollowVertebra", 4, 4),
                           off("HollowVertebra", 6, -6), off("Approach", -2, -3), off("Approach", 4, 2))):
        tick = enemy("Madderback", p)
        if i % 2 == 1 and isinstance(tick, unreal.UnderpaintingEnemy):
            tick.set_editor_property("removed_by_flag", unreal.Name("Scumblewood_MadderbackHalved"))
    # After the Turning the enemies have moved, and two quest endings change their numbers.
    # Before the Turning the first one stands alone in the road, in plain sight from the door: the first fight is one
    # enemy, seen coming. The second waits at the First Well, too far off to join in.
    first_two = (off("Bend", -3, 3), off("SunwardClearing", 6, -6)) if shifted else ((22.0, 0.0, -1.8), off("FirstWell", 9, -3))
    for i, p in enumerate(first_two + (off("Descent", -3, -2), off("Descent", 4, 3), off("Approach", 6, -5), off("RibGate", -8, 2))):
        blotter = enemy("Blotter", p)
        if shifted and i >= 4 and isinstance(blotter, unreal.UnderpaintingEnemy):
            blotter.set_editor_property("removed_by_flag", unreal.Name("Scumblewood_BlotterMinus2"))
    if shifted:
        for p in (off("Descent", 0, -5), off("Approach", -5, 4)):
            extra = enemy("Madderback", p)
            if isinstance(extra, unreal.UnderpaintingEnemy):
                extra.set_editor_property("requires_flag", unreal.Name("Scumblewood_MadderbackPlus2"))

    # Harvest nodes: 5 Terre Verte (sunlit spots), 3 Sanguine, 3 Bone Black
    for p in (off("FirstWell", -6, -7), off("Bend", -5, -6), off("SunwardClearing", -10, -11),
              off("SunwardClearing", 4, -12), off("Descent", -5, -6)):
        node("TerreVerte", p)
    for p in (off("HollowVertebra", -10, 10), off("HollowVertebra", 10, 5), off("Approach", 6, 6)):
        node("Sanguine", p)
    for p in (off("Bend", 6, 6), off("Descent", 6, 6), off("RibGate", 6, 8)):
        node("BoneBlack", p)

    player_start((STAIR_TOP[0], STAIR_TOP[1], STAIR_TOP[2]) if shifted else off("Entry", -3, 0), yaw=0.0)
    return end_level("Scumblewood")


# --------------------------------------------------------------------------------------
# Zone 3: the Grisaille Vault. Rib Stair, Cartoon Gallery, the Heart. Low gravity, unlit paper.
# --------------------------------------------------------------------------------------

def build_vault(shifted=False):
    """The Grisaille Vault. shifted=True builds the "B" level: the Heart is warm, a first amber glaze lies over
    the grey, the drawings nearest the Heart have become solid, and nothing hostile is left."""
    begin_level("L_Matriarch_Vault_B" if shifted else "L_Matriarch_Vault_A")
    Zone.unlit, Zone.label_yaw, Zone.shadow_tint, Zone.look = True, 180.0, "#4B4A48", "VaultWarm" if shifted else "VaultSketch"
    PAPER, GREY, DARK, GRAPHITE, BLUE, HEART = "#E6E1D6", "#8C8A85", "#4B4A48", "#2E2E30", "#2F4BA8", "#E8923A"
    if shifted:
        PAPER, GREY = "#EDE0C4", "#A89880"      # the amber glaze
    FLOOR = -10.0   # main floor height
    PIT = -18.0     # floor of the gallery pit

    fill_light(PAPER, lux=2.0)

    # Rib Stair: the entry, descending under rib arches.
    slab("Entry_Landing", -10, 2, -6, 6, 0.0, PAPER)
    ramp("RibStair", (2, 0, 0), (30, 0, FLOOR), 8.0, PAPER)      # edge of the landing to edge of the foot: no lip
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
        if shifted and i >= 5:
            # Nearest the Heart, the sketch has taken: these two are simply there now.
            slab("TakenPlatform_%02d" % i, x - 2, x + 2, y - 2, y + 2, FLOOR + dz, HEART, "Geometry/Shift", bottom=FLOOR + dz - 0.4)
            continue
        sketch_platform("SketchPlatform_%02d" % i, x - 2, x + 2, y - 2, y + 2, FLOOR + dz)
    sketch_platform("SketchPlatform_Side", 79, 83, -12, -8, FLOOR, tags=("Optional",))
    # A way back up for anyone who falls: there is no fall damage here, so a fall costs time, not health.
    # It lands on the open part of the Rib Stair's foot (y -12..12), not behind the foot's side wall.
    ramp("Gallery_ReturnRamp", (68, -10.5, PIT), (46, -10.5, FLOOR), 3.0, GREY)

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
    cylinder("Heart_Glow", (153, 0, FLOOR), 3.2 if shifted else 2.0, 7.0 if shifted else 5.0, HEART, "Geometry/Heart", collide=False, tags=("Heart",))

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

    # Things to read. The Hand's marginalia are invisible until a Flare is lit near them.
    lore("Margin_01", (12, 3.6, -3.6), yaw=-90.0)
    lore("Margin_02", (24, -3.6, -7.6), yaw=90.0)
    lore("Odile_05", (43, 9, FLOOR), yaw=180.0)
    lore("Margin_03", (45.0, -3, FLOOR), yaw=180.0)
    lore("Margin_04", (108, 9, FLOOR), yaw=0.0)
    lore("Odile_06", (119, -10, FLOOR), yaw=90.0)
    lore("Margin_06", (121, 3, FLOOR), yaw=0.0)
    lore("Margin_07", (150, 6, FLOOR), yaw=180.0)
    lore("Margin_08", (150, -6, FLOOR), yaw=180.0)
    # What Old Palimpsest was curled around is only there once it is gone (docs/STORY.md, Act 1).
    needs_flag(lore("Odile_07", (150.5, 2.0, FLOOR), yaw=180.0), "Killed.ThePalimpsest")
    lantern = actors.spawn_actor_from_class(unreal.UnderpaintingPickup, v((150.2, 0.6, FLOOR + 0.25)), unreal.Rotator(0, 0, 0))
    lantern.set_editor_property("item_id", unreal.Name("OdilesLantern"))
    lantern.set_editor_property("count", 1)
    lantern.set_editor_property("source_name", "the hollow")
    lantern.set_actor_scale3d(unreal.Vector(0.3, 0.3, 0.5))
    lantern.set_actor_label("Pickup_OdilesLantern")
    lantern.set_folder_path("Gameplay/Heart")
    lantern.tags = [unreal.Name(t) for t in ("Marker", "Pickup", "OdilesLantern")]
    needs_flag(lantern, "Killed.ThePalimpsest")

    # The Heart wall: two studies of a figure with a lantern and a knife, and a blank where a third was.
    for i, y in enumerate((-4.4, -1.6)):
        box("Study_%d" % (i + 1), (157.6, y, FLOOR + 1.4), (0.06, 0.9, 2.4), GRAPHITE, "Geometry/Heart", collide=False)
    choice_point("MQ_ColdUnderTheRibs", "the blank on the Heart wall", (157.5, 1.4, FLOOR + 1.4), yaw=0.0, size=(0.1, 1.2, 2.5))
    slab("Hollow", 149.2, 152.2, -0.6, 2.4, FLOOR + 0.02, GREY, "Geometry/Heart", bottom=FLOOR - 0.02)
    place("RibStair", "The Rib Stair", (16, 0, -5), half=(15, 5, 6))
    place("CartoonGallery", "The Cartoon Gallery", (40, 0, FLOOR), half=(5, 11, 3))
    place("GalleryFarLedge", "", (112, 0, FLOOR), half=(5, 11, 3))
    place("TheHeart", "The Heart", (140, 0, FLOOR), half=(14, 14, 4))

    # Enemies: 6 Pentiments, 4 Blotters and the boss. After the Turning nothing comes back here.
    if not shifted:
        for p in ((36, -6, FLOOR), (40, 7, FLOOR), (61, 3, FLOOR + 0.5), (81, 3, FLOOR - 0.5),
                  (88, 10, PIT), (110, 6, FLOOR)):
            enemy("Pentiment", p)
        for p in ((56, 8, PIT), (74, -6, PIT), (109, -5, FLOOR), (118, 4, FLOOR)):
            enemy("Blotter", p)
        enemy("ThePalimpsest", (142, 0, FLOOR))

    # The far corner of the pit is a long way from the ramp back up: it pays for the walk.
    cache("PitCorner", "Cache", (103, 13, PIT), [("RibSplinter", 2), ("GraphiteShard", 1)])

    # Harvest nodes: 3 Bone Black, 3 Slumber Blue (one-time)
    for p in ((43, -10, FLOOR), (96, -10, PIT), (120, 10, FLOOR)):
        node("BoneBlack", p)
    for p in ((60, 13, PIT), (81, -10, FLOOR), (128, -4, FLOOR)):
        node("SlumberBlue", p)

    player_start((-6, 0, 0), yaw=0.0)
    return end_level("GrisailleVault")


def build_all():
    problems = []
    wanted = [a for a in sys.argv[1:] if a in ("A", "B")] or ["B", "A"]      # "A" last, so the village is left open
    with unreal.ScopedSlowTask(3 * len(wanted), "Building the Sleeping Matriarch grey-box") as task:
        task.make_dialog(True)
        for version in wanted:
            for build in (build_scumblewood, build_vault, build_village):
                task.enter_progress_frame(1)
                problems += build(shifted=(version == "B"))
    eal.save_directory("/Game/Greybox")
    eal.save_directory("/Game/StyleKit")
    unreal.log("GREYBOX DONE. Problems: %s" % (problems or "none"))
    return problems


if __name__ == "__main__":
    print(build_all() or "no problems")
