"""Builds the first playable: a grey-box room with the player and one Blotter (docs/GDD.md section 11, build step 1).

Run inside the Unreal editor. Safe to run again.
    - Points the player Blueprint at the C++ combat character (the Blueprint keeps only mesh, animation and input assets).
    - Builds /Game/Maps/Dev/L_Combat_Greybox.
"""

import importlib.util
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("matriarch_greybox", os.path.join(HERE, "build_matriarch_greybox.py"))
gb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gb)

PLAYER_BP = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"

# One room per enemy, so each can be learned on its own: (level, enemy ID, what the room teaches, sketch zone?)
ROOMS = [
    ("L_Combat_Greybox", "Blotter", "Blotter: dodge its warm-lit attacks, strike in the recovery", False),
    ("L_Combat_Madderback", "Madderback", "Madderback: stand before a pillar, sidestep the charge, punish while it is stuck", False),
    ("L_Combat_Pentiment", "Pentiment", "Pentiment: flare (F) as it fades and it staggers", True),
    ("L_Combat_Hatcher", "Hatcher", "The Hatcher: light a Flare (F) to see her web. Touch a strand and she comes down", False),
    ("L_Combat_Boss", "ThePalimpsest", "Old Palimpsest: Bone Black while it is white, Terre Verte once it is red", True),
]


def reparent_player():
    blueprint = unreal.load_asset(PLAYER_BP)
    defaults = unreal.get_default_object(blueprint.generated_class())
    if not isinstance(defaults, unreal.UnderpaintingPlayerCharacter):
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, unreal.UnderpaintingPlayerCharacter)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_asset(PLAYER_BP)
        print("player Blueprint now uses UnderpaintingPlayerCharacter")
    else:
        print("player Blueprint already uses UnderpaintingPlayerCharacter")


def build_room(level_name, enemy_id, note, sketch):
    level = "/Game/Maps/Dev/" + level_name
    if gb.eal.does_asset_exist(level):
        gb.levels.load_level(level)
        for actor in gb.actors.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings) and actor.get_class().get_name() != "Brush":
                gb.actors.destroy_actor(actor)
    else:
        gb.levels.new_level(level)
    gb._counts.clear()
    CREAM, VIOLET, UMBER = "#F2E3C0", "#4A3558", "#6B4F3A"
    if sketch:
        gb.Zone.unlit, gb.Zone.label_yaw, gb.Zone.shadow_tint, gb.Zone.look = True, 180.0, "#4B4A48", "VaultSketch"
        floor, wall, pillar = "#E6E1D6", "#4B4A48", "#2E2E30"
    else:
        gb.Zone.unlit, gb.Zone.label_yaw, gb.Zone.shadow_tint, gb.Zone.look = False, 180.0, VIOLET, "VillageDusk"
        floor, wall, pillar = CREAM, VIOLET, UMBER

    size = 22 if enemy_id == "ThePalimpsest" else 16
    gb.sun_and_sky()
    gb.slab("Floor", -size, size, -size, size, 0.0, floor)
    for name, x0, x1, y0, y1 in (("Wall_N", size, size + 1, -size - 1, size + 1), ("Wall_S", -size - 1, -size, -size - 1, size + 1),
                                 ("Wall_E", -size, size, size, size + 1), ("Wall_W", -size, size, -size - 1, -size)):
        gb.slab(name, x0, x1, y0, y1, 4.0, wall, "Geometry/Walls", bottom=-1.0)
    # Pillars: something to dodge around, and for a charge to stick in.
    for i, (x, y) in enumerate(((4, 6), (4, -6), (-5, 0))):
        gb.box("Pillar_%d" % i, (x, y, 1.75), (1.2, 1.2, 3.5), pillar, "Geometry/Pillars")

    gb.enemy(enemy_id, (9, 0, 0))
    gb.label(note, (size - 3, 0, 3.2), "Markers/Notes", size=50.0)
    gb.marker("Lampstand", "Dev" + enemy_id, "Lampstand", (-12, 5, 0))
    gb.player_start((-9, 0, 0), yaw=0.0)
    gb.levels.save_current_level()
    print("built", level)


if __name__ == "__main__":
    reparent_player()
    for room in ROOMS:
        build_room(*room)
    gb.eal.save_directory("/Game/StyleKit")
