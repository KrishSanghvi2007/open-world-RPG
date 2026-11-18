"""Prints every enemy in the running game: where it is, whether it is hidden, its health and state."""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
print("player", pawn.get_actor_location())
for enemy in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UnderpaintingEnemy):
    print(enemy.get_name(), enemy.get_actor_location(), "hidden", enemy.get_editor_property("hidden"), "state", enemy.get_state(), "controller", enemy.get_controller())
