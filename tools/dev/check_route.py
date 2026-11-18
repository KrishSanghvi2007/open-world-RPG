"""Checks that a route in the level open in the editor can be walked: traces the ground every half metre along
a list of waypoints and reports gaps, steps too high to climb and slopes too steep.

    check_route.py stair      the Stair of Spines in L_Matriarch_Scumblewood_B, Rib Gate to the Nape Overlook
    check_route.py x,y,z x,y,z ...   any waypoints, in metres
"""

import math
import sys

import unreal

ROUTES = {
    # L_Matriarch_Scumblewood_A: the Furrow, from the village door down to the Rib Gate.
    "forest": [(-6, 0, 0), (40, 0, -3), (80, 20, -6), (120, 20, -8), (160, 0, -11), (200, 0, -13), (240, 15, -15), (280, 15, -16)],
    # L_Matriarch_Vault_A: the Rib Stair to the gallery's edge; the far ledge to the Heart wall; the way back up out of the pit.
    "vault_in": [(-8, 0, 0), (1, 0, 0), (31, 0, -10), (45, 0, -10)],
    "vault_heart": [(107, 0, -10), (126, 0, -10), (156, 0, -10)],
    "vault_pit": [(60, 0, -18), (69, -10.5, -18), (45, -10.5, -10), (38, 0, -10)],
    # L_Matriarch_Village_A: square up to the terraces; square down to the Hem; square to the Furrow door.
    "village_up": [(-4, 4, 4), (-22, 5, 4), (-22, 19, 8), (0, 30, 8)],
    "village_down": [(-4, 4, 4), (22, -9, 4), (22, -23, 0), (0, -34, 0)],
    "village_door": [(-4, 4, 4), (32, 4, 4)],
    "stair": [(272, 12, -16), (272, 37, -10), (268, 50, -10), (174, 50, 14), (174, 54.5, 14), (174, 50, 14), (150, 51, 14)],
}
MAX_STEP = 0.45       # metres: what the character can step up
MAX_SLOPE = 44.0      # degrees: what the character can walk up


IGNORE = []


def ground(world, x, y, z_guess):
    start = unreal.Vector(x * 100.0, y * 100.0, (z_guess + 2.5) * 100.0)
    end = unreal.Vector(x * 100.0, y * 100.0, (z_guess - 4.0) * 100.0)
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, IGNORE,
                                                 unreal.DrawDebugTrace.NONE, True)
    if not hit:
        return None, None
    data = hit.to_tuple()
    blocking, location, actor = data[0], data[4], data[9]
    return (location.z / 100.0, actor) if blocking else (None, None)


def main(args):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    # Only what you can stand on counts: named-place volumes, exits and other things you walk through are skipped.
    IGNORE[:] = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                 if isinstance(a, (unreal.UnderpaintingPlace, unreal.UnderpaintingInteractable, unreal.UnderpaintingEnemy, unreal.TextRenderActor))]
    points = ROUTES[args[0]] if args and args[0] in ROUTES else [tuple(float(n) for n in a.split(",")) for a in args]
    problems, obstacles, z, walked, steepest, last_actor = [], {}, points[0][2], 0.0, 0.0, None
    for (x0, y0, _), (x1, y1, _) in zip(points, points[1:]):
        length = math.hypot(x1 - x0, y1 - y0)
        steps = max(1, int(length / 0.5))
        run = length / steps
        for i in range(steps + 1):
            x, y = x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps
            height, actor = ground(world, x, y, z)
            # Something standing in the straight line (a rock, a trunk): note it once, then look at the ground under it.
            for _ in range(4):
                if height is None or height - z <= MAX_STEP or actor is None:
                    break
                label = actor.get_actor_label()
                obstacles[label] = obstacles.get(label, 0.0) + run
                IGNORE.append(actor)
                height, actor = ground(world, x, y, z)
            if height is None:
                problems.append("no ground at (%.1f, %.1f) near height %.1f" % (x, y, z))
                continue
            rise = height - z
            if run > 0.01:
                steepest = max(steepest, math.degrees(math.atan2(abs(rise), run)))
            label = actor.get_actor_label() if actor else "?"
            if label != last_actor:
                print("  %6.1f m  onto %-32s height %6.1f" % (walked + length * i / steps, label, height))
                last_actor = label
            z = height
        walked += length
    print("walked %.0f m, steepest slope %.0f degrees, ends at height %.1f" % (walked, steepest, z))
    for label in obstacles:
        print("IN THE WAY (walk round it): %s" % label)
    if steepest > MAX_SLOPE:
        problems.append("slope of %.0f degrees is too steep to walk" % steepest)
    if len(problems) > 6:
        problems = problems[:6] + ["... and %d more" % (len(problems) - 6)]
    print("PROBLEMS: %s" % (problems or "none"))


if __name__ == "__main__":
    main(sys.argv[1:])
