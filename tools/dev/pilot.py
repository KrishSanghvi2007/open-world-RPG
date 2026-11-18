"""Drives the game's autopilot (Source/Underpainting/Game/UnderpaintingAutopilot.h) while Play-In-Editor runs.

    pilot.py run "<script>"            start a script (steps separated by semicolons; see the header for the verbs)
    pilot.py status                    where it has got to, times noted, anything that went wrong
    pilot.py record <name> <seconds> [fps]     save frames of the game view to Saved/FightClips/<name>/
    pilot.py clip <name> [seconds]     record the standard scripted fight against whatever is in the room
"""

import sys

import unreal

# The same fight every time, so clips can be compared: walk up, lock on, and press these at these moments.
CLIP_FIGHT = ("follow on 1.7; wait 0.3; lock; wait 1.2; light; wait 0.9; light; wait 0.9; light; wait 1.2; dodge; wait 0.9; heavy; "
              "wait 1.7; light; wait 0.8; light; wait 1.0; dodge; wait 0.9; light; wait 0.9; light; wait 0.8; heavy; wait 1.6; "
              "light; wait 0.8; light; wait 0.8; light; wait 1.2; dodge; wait 0.8; light; wait 0.8; light; wait 1.0; follow off")


def pawn():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError("Play-In-Editor is not running.")
    return unreal.GameplayStatics.get_player_pawn(world, 0)


def main(args):
    command = args[0] if args else "status"
    player = pawn()
    if command == "run":
        player.dev_autopilot(args[1])
    elif command == "record":
        player.dev_record(args[1], float(args[2]), float(args[3]) if len(args) > 3 else 15.0)
    elif command == "clip":
        player.dev_record(args[1], float(args[2]) if len(args) > 2 else 20.0, 15.0)
        player.dev_autopilot(CLIP_FIGHT)
    print(player.dev_autopilot_status())


if __name__ == "__main__":
    main(sys.argv[1:])
