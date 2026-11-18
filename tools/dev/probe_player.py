"""Prints where the player is and what they are standing on."""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
loc = pawn.get_actor_location()
print("player at %.1f %.1f %.1f m" % (loc.x / 100.0, loc.y / 100.0, loc.z / 100.0))
