"""Hides or shows the player's own body while Play-In-Editor runs, so a screenshot can look at something else.

    ghost.py on | off
"""
import sys

import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
hide = (sys.argv[1] if len(sys.argv) > 1 else "on") == "on"
pawn.mesh.set_visibility(not hide, True)
print("player body", "hidden" if hide else "shown")
