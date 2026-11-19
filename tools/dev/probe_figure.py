"""Prints where a Folded character's bones are in the running game (to line things up with them).

    probe_figure.py <Tag>        e.g. Lacuna, Hatcher
"""
import sys

import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
tag = sys.argv[1] if len(sys.argv) > 1 else "Lacuna"
for actor in unreal.GameplayStatics.get_all_actors_with_tag(world, unreal.Name(tag)):
    print(actor.get_name(), "at", actor.get_actor_location(), "yaw", round(actor.get_actor_rotation().yaw))
    for component in actor.get_components_by_class(unreal.SkeletalMeshComponent):
        print("  mesh", component.get_name(), "at", component.get_world_location(), "rot", component.get_world_rotation())
        for bone in ("face", "head", "handL", "abdomen"):
            if component.get_bone_index(unreal.Name(bone)) >= 0:
                print("   ", bone, component.get_socket_location(unreal.Name(bone)))
    for other in unreal.GameplayStatics.get_all_actors_with_tag(world, unreal.Name("Choice")):
        print("  choice", other.get_actor_label() if hasattr(other, "get_actor_label") else other.get_name(), other.get_actor_location())
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        print("  static", component.get_name(), "at", component.get_world_location(), "scale", component.get_world_scale(), "visible", component.is_visible(),
              "mesh", component.static_mesh.get_name() if component.static_mesh else None, "parent", component.get_attach_parent().get_name() if component.get_attach_parent() else None)
