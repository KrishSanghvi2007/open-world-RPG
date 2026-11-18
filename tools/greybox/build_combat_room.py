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

LEVEL = "/Game/Maps/Dev/L_Combat_Greybox"
PLAYER_BP = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"


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


def build_room():
    if gb.eal.does_asset_exist(LEVEL):
        gb.levels.load_level(LEVEL)
        for actor in gb.actors.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings) and actor.get_class().get_name() != "Brush":
                gb.actors.destroy_actor(actor)
    else:
        gb.levels.new_level(LEVEL)
    gb._counts.clear()
    gb.Zone.unlit, gb.Zone.label_yaw, gb.Zone.shadow_tint, gb.Zone.look = False, 180.0, "#4A3558", "VillageDusk"
    CREAM, TEAL, VIOLET, UMBER = "#F2E3C0", "#2E5E6E", "#4A3558", "#6B4F3A"

    gb.sun_and_sky(sun_colour="#FFD9A0", sky_tint=TEAL, sun_lux=6.0, sky_intensity=1.2, pitch=-40.0)
    gb.slab("Floor", -16, 16, -16, 16, 0.0, CREAM)
    for name, x0, x1, y0, y1 in (("Wall_N", 16, 17, -17, 17), ("Wall_S", -17, -16, -17, 17), ("Wall_E", -16, 16, 16, 17), ("Wall_W", -16, 16, -17, -16)):
        gb.slab(name, x0, x1, y0, y1, 4.0, VIOLET, "Geometry/Walls", bottom=-1.0)
    # A few pillars: something to dodge around and to break the lunge.
    for i, (x, y) in enumerate(((4, 6), (4, -6), (-5, 0))):
        gb.box("Pillar_%d" % i, (x, y, 1.75), (1.2, 1.2, 3.5), UMBER, "Geometry/Pillars")

    blotter = gb.actors.spawn_actor_from_class(unreal.BlotterEnemy, gb.v((9, 0, 1.3)), unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
    blotter.set_actor_label("Blotter_01")
    blotter.set_folder_path("Gameplay")
    gb.label("Blotter: dodge its warm-lit attacks, strike in the recovery", (12, 0, 3.2), "Markers/Notes", size=50.0)

    gb.player_start((-9, 0, 0), yaw=0.0)
    gb.levels.save_current_level()
    print("built", LEVEL)


if __name__ == "__main__":
    reparent_player()
    build_room()
