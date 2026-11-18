"""Plays the golden path of Act 1 with the autopilot, one zone at a time, at a person's pace, and notes the time of each beat.

    golden_path.py village            the first quest, then out by the Furrow
    golden_path.py forest             across the Scumblewood to the Rib Gate, fighting what is in the way
    golden_path.py vault              the Rib Stair, the Cartoon Gallery, the boss, the lantern, the choice
    golden_path.py trials <Enemy> [n] fight a fresh one n times at each of three skills (run it in Maps/Dev/L_Combat_Greybox)
    golden_path.py status             where it has got to and the times so far

Start Play-In-Editor in the zone's level first. The autopilot walks with real input, waits as long as a person
takes to read each line, and fights with the given skill (0 = never dodges, 1 = always). Times are in the log
as "AUTOPILOT <seconds> <label>" and in the status. docs/PACING.md is written from them.
"""

import sys

import unreal

READ = "wait 2.5"      # a person taking in a new screen

VILLAGE = "; ".join([
    "mark start", "wait 4",                                       # looking about
    "walkto NPC HesperVane", "talk", "mark talked to Hesper",
    "walk -22 5", "walk -22 19",                                 # the ramp up to the Sunward Terraces
    "walkto Node Ochre 1", "use", "wait 1", "walkto Node Ochre 2", "use", "wait 1",
    "walkto Node Ochre 3", "use", "wait 1", "walkto Node Ochre 4", "use", "mark four Ochre",
    "walk -22 19", "walk -22 5", "walkto Note FlaxPress", "use", "wait 1", "mark linseed oil",
    "walkto Station GrindingSlab", "use", READ, "menu 1", "wait 1.5", "close", "mark Lamp Oil made",
    "walkto Lampstand Varnish", "use", "wait 2", "mark Lampstand lit",
    "use", READ, "wait 4", "menu 1", "wait 2", "close", "mark attuned Ochre",
    "walkto NPC HesperVane", "talk", "mark first quest done",
    "talk 1", "mark second quest taken",
    "walkto NPC BartramSedge", "talk 1", "mark met Bartram",
    "walkto Exit Scumblewood", "mark at the Furrow", "use",
])

# The Furrow runs one way: the First Well, the Bend, the Sunward Clearing, the Descent, the Hollow Vertebra, the Approach, the Rib Gate.
FOREST = "; ".join([
    "guard on {skill}", "mark start", "wait 3",
    "walkto Node TerreVerte 1", "use", "wait 1",
    "walk 40 0", "fight {skill} 90", "mark First Well: first fight over",
    "walkto NPC IlkaThrum", "talk 1", "mark met Ilka",
    "walk 62 10", "walk 80 20", "fight {skill} 90", "mark the Bend",
    "walk 120 20", "walkto Lampstand SunwardClearing", "use", READ, "wait 5", "close", "mark rested at the Sunward Clearing",
    "walkto NPC CorinHalfSeen", "talk 1", "mark met Corin",
    "walk 120 20", "walk 160 0", "fight {skill} 90", "mark the Descent",
    "walk 200 0", "fight {skill} 150", "mark the Hollow Vertebra",
    "walk 240 15", "fight {skill} 120", "mark the Approach",
    "walk 272 15", "fight {skill} 90", "mark reached the Rib Gate",
    "walkto Lampstand RibGate", "use", "wait 2", "use", READ, "wait 5", "close", "mark Rib Gate Lampstand lit",
    "walkto Exit GrisailleVault", "mark at the Rib Gate door", "use",
])

VAULT = "; ".join([
    "guard on {skill}", "mark start", "wait 4",
    "walk 1 0", "walk 31 0", "walk 38 -3", "mark down the Rib Stair",   # the lone Pentiment at the foot comes to you: the guard fights it
    "walk 44.5 0", "wait 3", "mark at the Gallery edge",
    # Six sketched platforms over the pit: light a Flare, run to the edge, jump, land, and again.
    "flare", "wait 0.3", "sprint 45.6 0.0", "jump", "sprint 51.0 0.0", "wait 0.3", "flare", "walk 49.7 -0.4", "sprint 52.5 0.5", "jump", "sprint 61.0 3.0", "wait 0.3", "flare", "walk 59.7 3.6", "sprint 62.4 2.3", "jump", "sprint 71.0 -2.0", "wait 0.3", "flare", "walk 69.7 -2.6", "sprint 72.4 -1.3", "jump", "sprint 81.0 3.0", "wait 0.3", "flare", "walk 79.7 3.5", "sprint 82.5 2.4", "jump", "sprint 91.0 -1.0", "wait 0.3", "flare", "walk 89.6 -1.1", "sprint 92.6 -0.8", "jump", "sprint 101.0 0.0", "wait 0.3", "flare", "walk 99.6 0.0", "sprint 102.6 0.0", "jump", "sprint 108.5 0.0", "wait 0.3",
    "walk 112 0", "wait 1", "mark across the Gallery",
    "walkto Lampstand RibStairHeart", "use", READ, "wait 5", "close", "mark rested before the Heart",
    "walk 126 0", "mark boss begins", "fight {skill} 300", "mark boss over",
    "walkto Pickup OdilesLantern", "use", "wait 2", "mark took the lantern",
    "walkto Choice MQ_ColdUnderTheRibs", "use", READ, "wait 6", "menu 3", "wait 2", "menu 1", "mark chose",
])

# After the Turning: back across the Gallery (two platforms have taken and are solid now), up the Rib Stair, up the Stair
# of Spines that has risen in the Scumblewood, in by the new lower gate, and to Hesper.
VAULT_HOME = "; ".join([
    "guard on {skill}", "mark start for home", "wait 4", "walk 126 0", "walk 110 0",
    "flare", "walk 109.9 0.0", "sprint 106.1 0.0", "jump", "sprint 101.0 0.0", "wait 0.3", "flare", "walk 102.4 0.1", "sprint 99.4 -0.2", "jump", "sprint 91.0 -1.0", "wait 0.3", "flare", "walk 92.3 -1.5", "sprint 89.5 -0.4", "jump", "sprint 81.0 3.0", "wait 0.3", "flare", "walk 82.3 3.6", "sprint 79.6 2.3", "jump", "sprint 71.0 -2.0", "wait 0.3", "flare", "walk 72.3 -2.6", "sprint 69.6 -1.3", "jump", "sprint 61.0 3.0", "wait 0.3", "flare", "walk 62.3 3.4", "sprint 59.5 2.5", "jump", "sprint 51.0 0.0", "wait 0.3", "flare", "walk 52.4 0.0", "sprint 49.4 0.0", "jump", "sprint 44.0 0.0", "wait 0.3",
    "walk 31 0", "walk 1 0", "walkto Exit Scumblewood", "mark out of the Vault", "use",
])
FOREST_HOME = "; ".join([
    "guard on {skill}", "mark the Rib Gate, turned", "wait 4",
    "walk 272 12", "walk 272 37", "walk 268 50", "walk 222 50", "walk 176 50", "mark top of the Stair of Spines",
    "walkto Exit VarnishVillage", "use",
])
VILLAGE_HOME = "; ".join([
    "guard on {skill}", "mark the lower gate", "wait 4",
    "walk 22 -23", "walk 22 -9", "walk 6 2", "walkto NPC HesperVane", "talk 1", "mark told Hesper", "wait 6", "mark end",
])

# The second halves, for carrying on after a death: from the Sunward Clearing Lampstand, and from the one before the Heart.
FOREST_2 = "guard on {skill}; mark woke at the Sunward Clearing; wait 3; " + FOREST.split("mark met Corin; ")[1]
VAULT_2 = ("guard on {skill}; mark woke before the Heart; wait 3; walkto Lampstand RibStairHeart; use; wait 4; close; "
           + VAULT.split("mark rested before the Heart; ")[1])

SEGMENTS = {"village": VILLAGE, "forest": FOREST, "vault": VAULT, "forest2": FOREST_2, "vault2": VAULT_2,
            "vault_home": VAULT_HOME, "forest_home": FOREST_HOME, "village_home": VILLAGE_HOME}

# How hard each enemy is: the same fight five times at three levels of skill (the share of attacks dodged), in a test room.
# The player cannot die in a trial; the damage taken is what is measured (docs/BALANCE.md).
def trials(enemy, rounds="5"):
    return "; ".join("trial %s %s %s" % (enemy, skill, rounds) for skill in ("0.3", "0.6", "0.9"))


def main(args):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError("Play-In-Editor is not running.")
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    command = args[0] if args else "status"
    if command == "trials":
        player.dev_autopilot(trials(args[1], args[2] if len(args) > 2 else "5"))
    elif command in SEGMENTS:
        skill = args[1] if len(args) > 1 else "0.6"
        player.dev_autopilot(SEGMENTS[command].replace("{skill}", skill))
    print(player.dev_autopilot_status())


if __name__ == "__main__":
    main(sys.argv[1:])
