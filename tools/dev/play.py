"""Plays the game from a script while Play-In-Editor is running, by moving the player to things and pressing
the same Interact and menu functions the keys call. Used to check the walking skeleton end to end.

    play.py state                 print what the player would see
    play.py newgame               wipe the save and restart the level
    play.py quest1                play "Dusk on the Shoulder" from start to finish
    play.py goto <Kind> <Id> [n]  stand next to a marker (e.g. goto Enemy Blotter)
    play.py light | heavy | dodge one combat action
    play.py save | load
    play.py build <Pigment> [<Pigment>]   attune tier 1 of one or two pigments (debug shortcut)
    play.py stroke | lock | swap | poultice | lampoil   combat depth
    play.py flare | flask         light a Flare; throw a Fixative Flask at the platform in view
    play.py read <LoreId>         walk to a note and read it
    play.py lookaway <Kind> <Id>  stand by something but face away from it
    play.py probe                 count solid sketch platforms and open passable backs
    play.py wall front|behind     stand on one side of the forest's lesson wall
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
    look_at(pawn, target)


def look_at(pawn, target):
    """Turns the player and the camera toward a point, as a player would with the mouse. "Use" follows the view."""
    eye = pawn.get_actor_location() + unreal.Vector(0.0, 0.0, 60.0)
    aim = (target - eye).rotator()
    pawn.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=aim.yaw), True)
    pawn.get_controller().set_control_rotation(unreal.Rotator(roll=0.0, pitch=aim.pitch, yaw=aim.yaw))


def say(pawn, what):
    print("%-28s %s" % (what, pawn.describe_state()))


def talk(pawn, reply=1):
    """Talks to whoever is in reach. reply is the number to press whenever a reply is offered, or a list giving
    the number to press on each line in turn (the last one repeats)."""
    replies = reply if isinstance(reply, (list, tuple)) else [reply]
    pawn.interact()
    for line in range(12):
        if "menu=1 " not in pawn.describe_state():
            break
        print("    line:", pawn.describe_state().split("row=")[1].split(" ")[0])
        pawn.menu_number(replies[min(line, len(replies) - 1)])
        if "menu=1 " not in pawn.describe_state():
            break        # that reply ended the conversation; pressing Use again would start it over
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


def ledger(world, pawn, ending):
    """Plays "The Tick-Warden's Ledger" in the forest up to and including its ending (1 = burn, 2 = dress the wells)."""
    goto(world, pawn, "NPC", "IlkaThrum"); talk(pawn); say(pawn, "met Ilka")
    goto(world, pawn, "NPC", "IlkaThrum"); talk(pawn); say(pawn, "took the quest")
    for i in (1, 2, 3):
        goto(world, pawn, "Inspect", "BloodWell", i); pawn.interact(); pawn.interact()
    say(pawn, "read three wells")
    pawn.set_actor_location(find(world, "Place", "BroodNest").get_actor_location(), False, True)
    say(pawn, "stood in the nest")
    goto(world, pawn, "Choice", "SQ_TickWardensLedger"); say(pawn, "at the nest")
    pawn.interact(); say(pawn, "choice menu open")
    pawn.menu_number(ending); say(pawn, "asked to confirm")
    pawn.menu_number(1); say(pawn, "decided")
    ticks = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MadderbackEnemy)]
    print("Madderbacks in the level right now:", len(ticks))


def main(args):
    world, pawn = game()
    command = args[0] if args else "state"
    if command == "newgame":
        pawn.start_new_game(); print("new game; level restarting"); return
    if command == "quest1":
        quest1(world, pawn)
    elif command == "ledger":
        ledger(world, pawn, int(args[1]) if len(args) > 1 else 1)
    elif command == "talk":
        # talk <NpcId> [reply]: walk up to someone and talk, taking that reply whenever one is offered.
        goto(world, pawn, "NPC", args[1]); talk(pawn, int(args[2]) if len(args) > 2 else 1)
    elif command == "use":
        # use <Kind> <Id> [n]: walk up to a marker and press Use.
        goto(world, pawn, args[1], args[2], int(args[3]) if len(args) > 3 else 1); pawn.interact()
    elif command == "menu":
        pawn.menu_number(int(args[1]))
    elif command == "close":
        pawn.menu_close()
    elif command == "place":
        pawn.set_actor_location(find(world, "Place", args[1]).get_actor_location(), False, True)
    elif command == "hidden":
        # hidden <Kind> <Id>: is this marker out of the world right now?
        index = 1
        while True:
            try:
                actor = find(world, args[1], args[2], index)
            except RuntimeError:
                break
            print(actor.get_actor_label() if hasattr(actor, "get_actor_label") else actor.get_name(), "hidden =", actor.get_editor_property("hidden"), "at", actor.get_actor_location())
            index += 1
    elif command == "fill":
        pawn.dev_fill()
    elif command == "craftall":
        # craftall <StationId>: open a station and make one of everything it lists.
        goto(world, pawn, "Station", args[1]); pawn.interact()
        for number in range(1, 10):
            pawn.menu_number(number)
        pawn.menu_close()
    elif command == "draught":
        print("draught used:", pawn.use_draught())
    elif command == "tincture":
        print("tincture used:", pawn.use_tincture())
    elif command == "inventory":
        pawn.inventory_pressed() if hasattr(pawn, "inventory_pressed") else None
    elif command == "act1a":
        # Village: the first quest, take the second from Hesper, leave for the forest.
        quest1(world, pawn); goto(world, pawn, "NPC", "HesperVane"); talk(pawn, 1)
        goto(world, pawn, "Exit", "Scumblewood"); pawn.interact()
    elif command == "act1b":
        # Forest: light the Rib Gate lamp and go into the Vault.
        pawn.dev_set_invulnerable(True); pawn.dev_grant(unreal.Name("LampOil"), 1)
        goto(world, pawn, "Lampstand", "RibGate"); pawn.interact(); say(pawn, "lit the Rib Gate lamp")
        goto(world, pawn, "Exit", "GrisailleVault"); pawn.interact()
    elif command == "act1c":
        # Vault: cross, kill the boss, take the lantern, decide the wall (act1c <ending 1-3>). The Turning follows.
        pawn.dev_set_invulnerable(True)
        pawn.set_actor_location(find(world, "Place", "GalleryFarLedge").get_actor_location(), False, True)
        pawn.dev_grant(unreal.Name("Sanguine"), 5); pawn.dev_grant(unreal.Name("TerreVerte"), 5)
        print("dealt", pawn.dev_hurt_enemies(5000.0))
        goto(world, pawn, "Pickup", "OdilesLantern"); pawn.interact(); say(pawn, "took the lantern")
        goto(world, pawn, "Choice", "MQ_ColdUnderTheRibs"); pawn.interact()
        pawn.menu_number(int(args[1]) if len(args) > 1 else 3); pawn.menu_number(1)
        print("turning time", pawn.get_turning_time() if hasattr(pawn, "get_turning_time") else "?")
    elif command == "turn":
        pawn.dev_start_turning()
    elif command == "at":
        # at <x> <y> <z> <yaw> [pitch]: stand somewhere (metres) looking a given way.
        x, y, z, yaw = (float(n) for n in args[1:5])
        pawn.set_actor_location(unreal.Vector(x * 100.0, y * 100.0, z * 100.0 + 100.0), False, True)
        pawn.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), True)
        pawn.get_controller().set_control_rotation(unreal.Rotator(roll=0.0, pitch=float(args[5]) if len(args) > 5 else 0.0, yaw=yaw))
    elif command == "satchel":
        # satchel <tab 0-3> [rows down]: open the satchel on a tab and move the highlight.
        pawn.dev_open_satchel(int(args[1]))
        if len(args) > 2:
            pawn.dev_move_cursor(int(args[2]))
    elif command == "rebind":
        # rebind <row> <Key>: on the settings tab, go to a row, press Use, then press the key (e.g. rebind 5 C).
        pawn.dev_open_satchel(3); pawn.dev_move_cursor(int(args[1])); pawn.interact()
        key = unreal.Key()
        key.set_editor_property("key_name", unreal.Name(args[2]))
        print("captured:", pawn.dev_press_key(key))
        for action in ("Use", "Dodge", "Flare", "Pack"):
            print("   ", action, "=", pawn.dev_describe_key(unreal.Name(action)))
    elif command == "key":
        # key <Key>: the key the settings screen is waiting for.
        key = unreal.Key()
        key.set_editor_property("key_name", unreal.Name(args[1]))
        print("captured:", pawn.dev_press_key(key))
        for action in ("Use", "Dodge", "Flare", "Pack"):
            print("   ", action, "=", pawn.dev_describe_key(unreal.Name(action)))
    elif command == "talkseq":
        # talkseq <NpcId> <n> <n> ...: talk, pressing these numbers on successive lines.
        goto(world, pawn, "NPC", args[1]); talk(pawn, [int(n) for n in args[2:]])
    elif command == "corin":
        # corin <ending 1-3>: The Unseen Side from first meeting to its ending (1 Terre Verte, 2 Bone Black, 3 leave him).
        pawn.dev_set_invulnerable(True)
        goto(world, pawn, "NPC", "CorinHalfSeen"); talk(pawn, 2); say(pawn, "walked round him")
        goto(world, pawn, "NPC", "CorinHalfSeen"); talk(pawn, 1); say(pawn, "took the quest")
        pawn.set_actor_location(find(world, "Place", "CorinsBack").get_actor_location(), False, True); say(pawn, "saw his back")
        goto(world, pawn, "NPC", "CorinHalfSeen"); talk(pawn, 1); say(pawn, "talked again")
        pawn.dev_grant(unreal.Name("TerreVerte"), 3); pawn.dev_grant(unreal.Name("BoneBlack"), 3)
        goto(world, pawn, "Choice", "SQ_TheUnseenSide"); pawn.interact(); say(pawn, "at his back")
        pawn.menu_number(int(args[1]) if len(args) > 1 else 3); pawn.menu_number(1); say(pawn, "decided")
    elif command == "grind":
        # grind <ending 1-2>: A Fair Grind in the village, after the first quest (1 Ceruse, 2 Ochre).
        goto(world, pawn, "NPC", "BartramSedge"); talk(pawn, 1); say(pawn, "took the quest")
        goto(world, pawn, "NPC", "BartramSedge"); talk(pawn, 1); say(pawn, "talked again")
        pawn.dev_grant(unreal.Name("Ceruse"), 3); pawn.dev_grant(unreal.Name("Ochre"), 4)
        goto(world, pawn, "Choice", "SQ_AFairGrind"); pawn.interact(); say(pawn, "at the wall")
        pawn.menu_number(int(args[1]) if len(args) > 1 else 2); pawn.menu_number(1); say(pawn, "decided")
        goto(world, pawn, "NPC", "BartramSedge"); talk(pawn, 4); say(pawn, "what Bartram says now")
    elif command == "count":
        cls = getattr(unreal, args[1])
        print(args[1], len(unreal.GameplayStatics.get_all_actors_of_class(world, cls)))
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
    elif command == "build":
        # Give the player a pigment build without playing for it: build <Pigment> [<Pigment>]
        for i, pigment in enumerate(args[1:3]):
            pawn.dev_grant(unreal.Name(pigment), 3)
            print("attuned", pigment, pawn.dev_attune(unreal.Name(pigment + "_T1")))
    elif command == "god":
        pawn.dev_set_invulnerable(args[1] == "on")
    elif command == "hurt":
        print("dealt", pawn.dev_hurt_enemies(float(args[1])))
    elif command == "enemies":
        print(pawn.dev_describe_enemies())
    elif command == "where":
        print("player", pawn.get_actor_location())
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UnderpaintingEnemy):
            print(a.get_name(), a.get_actor_location(), "velocity", a.get_velocity())
    elif command == "grant":
        pawn.dev_grant(unreal.Name(args[1]), int(args[2]))
    elif command == "stroke":
        enemy = find(world, "Enemy", args[1] if len(args) > 1 else "Blotter")
        to_me = pawn.get_actor_location() - enemy.get_actor_location()
        to_me.z = 0.0
        direction = to_me.normal() if to_me.length() > 1.0 else unreal.Vector(-1.0, 0.0, 0.0)
        spot = enemy.get_actor_location() + direction * 140.0
        pawn.set_actor_location(unreal.Vector(spot.x, spot.y, pawn.get_actor_location().z), False, True)
        pawn.set_actor_rotation((direction * -1.0).rotator(), True)
        print("stroke started:", pawn.start_stroke())
    elif command == "lock":
        look_at(pawn, find(world, "Enemy", args[1] if len(args) > 1 else "Blotter").get_actor_location())
        pawn.toggle_lock_on()
    elif command == "swap":
        pawn.swap_well()
    elif command == "poultice":
        print("poultice used:", pawn.use_poultice())
    elif command == "lampoil":
        print("lamp oil used:", pawn.use_lamp_oil())
    elif command == "light":
        pawn.start_light_attack()
    elif command == "heavy":
        pawn.start_heavy_attack()
    elif command == "dodge":
        pawn.start_dodge()
    elif command == "flare":
        print("flare lit:", pawn.use_flare())
    elif command == "flask":
        print("flask used:", pawn.use_fixative_flask())
    elif command == "read":
        goto(world, pawn, "Lore", args[1]); pawn.interact(); say(pawn, "reading " + args[1]); pawn.interact()
    elif command == "lookaway":
        # Stand next to something but look the other way: "Use" must not pick it.
        goto(world, pawn, args[1], args[2])
        yaw = pawn.get_actor_rotation().yaw + 180.0
        pawn.get_controller().set_control_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    elif command == "probe":
        solid = [a.get_actor_label() for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UnderpaintingSketchPlatform) if a.is_solid()]
        backs = {a.get_name(): a.is_open() for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UnderpaintingPassableBack)}
        print("solid platforms:", len(solid), "| passable backs open:", sorted(k for k, v in backs.items() if v), "of", len(backs))
    elif command == "wall":
        # Stand in front of, or behind, the lesson wall. args[1] is "front" or "behind".
        wall = [a for a in unreal.GameplayStatics.get_all_actors_with_tag(world, unreal.Name("MainRoute"))][0]
        forward = wall.get_actor_forward_vector()
        sign = 1.0 if args[1] == "front" else -1.0
        spot = wall.get_actor_location() + forward * (300.0 * sign)
        pawn.set_actor_location(unreal.Vector(spot.x, spot.y, wall.get_actor_location().z + 60.0), False, True)
        look_at(pawn, wall.get_actor_location())
    elif command == "walk":
        # Walks forward for one call (movement is applied over the next frames).
        pawn.add_movement_input(pawn.get_actor_forward_vector(), 1.0, True)
    elif command == "save":
        pawn.save_now()
    elif command == "load":
        pawn.load_now()
    elif command == "interact":
        pawn.interact()
    say(pawn, command)


if __name__ == "__main__":
    main(sys.argv[1:])
