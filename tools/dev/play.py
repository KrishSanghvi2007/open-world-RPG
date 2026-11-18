"""Plays the game from a script while Play-In-Editor is running, by moving the player to things and pressing
the same Interact and menu functions the keys call. Used to check the walking skeleton end to end.

    play.py state                 print what the player would see
    play.py newgame               wipe the save and restart the level
    play.py quest1                play "Dusk on the Shoulder" from start to finish
    play.py goto <Kind> <Id> [n]  stand next to a marker (e.g. goto Enemy Blotter)
    play.py light | heavy | dodge one combat action
    play.py save | load
"""

import sys

import unreal


def game():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError("Play-In-Editor is not running.")
    return world, unreal.GameplayStatics.get_player_pawn(world, 0)


def find(world, kind, data_id, index=1):
    matches = [a for a in unreal.GameplayStatics.get_all_actors_with_tag(world, unreal.Name(data_id))
               if a.actor_has_tag(unreal.Name(kind)) and not isinstance(a, unreal.TextRenderActor)]
    matches.sort(key=lambda a: a.get_name())
    if len(matches) < index:
        raise RuntimeError("No %s %s #%d in this level" % (kind, data_id, index))
    return matches[index - 1]


def goto(world, pawn, kind, data_id, index=1, distance=130.0):
    target = find(world, kind, data_id, index).get_actor_location()
    here = unreal.Vector(target.x - distance, target.y, target.z + 120.0)
    pawn.set_actor_location(here, False, True)
    pawn.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), True)


def say(pawn, what):
    print("%-28s %s" % (what, pawn.describe_state()))


def talk(pawn, reply=1):
    """Talks to whoever is in reach, taking the given reply whenever one is offered."""
    pawn.interact()
    for _ in range(12):
        if "menu=1" not in pawn.describe_state():
            break
        print("    line:", pawn.describe_state().split("row=")[1].split(" ")[0])
        pawn.menu_number(reply)
        pawn.interact()


def quest1(world, pawn):
    say(pawn, "start")
    goto(world, pawn, "NPC", "HesperVane"); talk(pawn); say(pawn, "talked to Hesper")
    for i in (1, 2, 3, 4):
        goto(world, pawn, "Node", "Ochre", i); pawn.interact()
    say(pawn, "harvested 4 nodes")
    goto(world, pawn, "Note", "FlaxPress"); pawn.interact(); say(pawn, "took linseed")
    goto(world, pawn, "Station", "GrindingSlab"); pawn.interact(); pawn.menu_number(1); pawn.menu_close(); say(pawn, "crafted Lamp Oil")
    goto(world, pawn, "Lampstand", "Varnish"); pawn.interact(); say(pawn, "poured the oil")
    pawn.interact(); say(pawn, "rested (palette open)")
    pawn.menu_number(1); pawn.menu_close(); say(pawn, "attuned Warm Ground")
    goto(world, pawn, "NPC", "HesperVane"); talk(pawn); say(pawn, "talked to Hesper again")


def main(args):
    world, pawn = game()
    command = args[0] if args else "state"
    if command == "newgame":
        pawn.start_new_game(); print("new game; level restarting"); return
    if command == "quest1":
        quest1(world, pawn)
    elif command == "goto":
        goto(world, pawn, args[1], args[2], int(args[3]) if len(args) > 3 else 1, 260.0 if args[1] == "Enemy" else 130.0)
    elif command == "strike":
        # Step up to the enemy, face it, and swing: what a player does with the stick and the mouse.
        enemy = find(world, "Enemy", args[1] if len(args) > 1 else "Blotter")
        to_me = pawn.get_actor_location() - enemy.get_actor_location()
        to_me.z = 0.0
        direction = to_me.normal() if to_me.length() > 1.0 else unreal.Vector(-1.0, 0.0, 0.0)
        spot = enemy.get_actor_location() + direction * 140.0
        pawn.set_actor_location(unreal.Vector(spot.x, spot.y, pawn.get_actor_location().z), False, True)
        pawn.set_actor_rotation((direction * -1.0).rotator(), True)
        if len(args) > 2 and args[2] == "heavy":
            pawn.start_heavy_attack()
        else:
            pawn.start_light_attack()
    elif command == "light":
        pawn.start_light_attack()
    elif command == "heavy":
        pawn.start_heavy_attack()
    elif command == "dodge":
        pawn.start_dodge()
    elif command == "save":
        pawn.save_now()
    elif command == "load":
        pawn.load_now()
    elif command == "interact":
        pawn.interact()
    say(pawn, command)


if __name__ == "__main__":
    main(sys.argv[1:])
