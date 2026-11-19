"""Lays a zone's whole kit out in rows under that zone's light, to look at. Run inside the editor:

    run_script("kit/showroom.py", ["Varnish"])

Makes /Game/Maps/Dev/L_Kit_<Zone>. Not part of the game; rebuild it whenever the kit changes.
"""

import json
import os
import sys

import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
sys.path.insert(0, os.path.join(ROOT, "tools", "stylekit"))
import stylekit                     # noqa: E402

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eal = unreal.EditorAssetLibrary

LOOK = {"Varnish": "VillageDusk", "Scumblewood": "ForestHalfLight", "Vault": "VaultSketch"}
GROUND = {"Varnish": ("#E3C39A", "Ground", "#4A3558"), "Scumblewood": ("#7E8A66", "Ground", "#2E5E6E"), "Vault": ("#E6E1D6", "Ground", "#4B4A48")}


def main(args):
    zone = args[0]
    path = "/Game/Maps/Dev/L_Kit_%s" % zone
    if eal.does_asset_exist(path):
        levels.load_level(path)
        for actor in actors.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings) and actor.get_class().get_name() != "Brush":
                actors.destroy_actor(actor)
    else:
        levels.new_level(path)
    stylekit.apply_look(LOOK[zone])

    listing = json.load(open(os.path.join(ROOT, "art_source", "kit", zone, "kit.json")))
    listing.sort(key=lambda p: -max(p["size_m"][0], p["size_m"][1]))
    x, y, row_depth, placed = 0.0, 0.0, 0.0, []
    for piece in listing:
        w, d = piece["size_m"][0] + 2.0, piece["size_m"][1] + 2.0
        if x + w > 70.0:
            x, y, row_depth = 0.0, y - row_depth - 1.0, 0.0
        mesh = eal.load_asset("/Game/Kit/%s/%s" % (zone, piece["name"]))
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector((x + w / 2.0) * 100.0, (y - d / 2.0) * 100.0, 0.0), unreal.Rotator(0, 0, 0))
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.set_actor_label(piece["name"])
        placed.append((piece["name"], round(x + w / 2.0, 1), round(y - d / 2.0, 1)))
        x, row_depth = x + w, max(row_depth, d)

    colour, surface, tint = GROUND[zone]
    ground = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(3500.0, (y - row_depth) * 50.0, -50.0), unreal.Rotator(0, 0, 0))
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.static_mesh_component.set_material(0, stylekit.sketch(colour, surface) if zone == "Vault" else stylekit.glaze(colour, surface, tint))
    ground.set_actor_scale3d(unreal.Vector(110.0, abs(y - row_depth) + 40.0, 1.0))
    ground.set_actor_label("Ground")
    levels.editor_set_game_view(True)
    levels.save_current_level()
    print("SHOWROOM", zone, json.dumps(placed))


if __name__ == "__main__":
    main(sys.argv[1:])
