# Lacuna (d40, the Faceless One). From CAST.md:
#   "Glides upright as the pipe frame shuffles in a parallelogram gait on curl-toed slippers that slosh in puddles.
#    Its hands drift independently around it like satellites. Slow, deliberate, dripping."
#   Self-interactions used: a hand turns its own tap; a hand reaches into the void of its own face and disappears to
#   the wrist; the hands drift back toward the torn sleeves and hover there, almost docking.
# Runs inside blender_anim.py (Take, sheet_point, math, Vector are already there). Directions: x right, y back, z up.

TAU = 2 * math.pi
HAND_L_REST = sheet_point(0.30, 0.45)        # the wrists, where CAST.md pivots the hands
HAND_R_REST = sheet_point(0.64, 0.475)
TAP_R = sheet_point(0.895, 0.555)            # the handle of the right tap
FACE = sheet_point(0.501, 0.163)
SLEEVE_L = sheet_point(0.352, 0.495)         # the torn ends of the sleeves
SLEEVE_R = sheet_point(0.612, 0.5)
FINGERS_L = sheet_point(0.215, 0.33)         # where the left hand's fingertips are


def drift(take, frame, length, amount=1.0, hold_left=None, hold_right=None):
    """The hands as satellites: slow, separate ellipses round where they were drawn."""
    t = frame / length * TAU
    if hold_left is None:
        take.shift(frame, "handL", (0.05 * amount * math.sin(t), 0.04 * amount * math.cos(t), 0.06 * amount * math.sin(2 * t + 0.6)))
        take.turn(frame, "handL", swing=6 * amount * math.sin(t + 1.0))
    if hold_right is None:
        take.shift(frame, "handR", (0.06 * amount * math.cos(t + 0.8), 0.05 * amount * math.sin(t + 0.8), 0.05 * amount * math.sin(2 * t)))
        take.turn(frame, "handR", swing=5 * amount * math.cos(t))


# Waiting: a slow sway, the head a little behind the coat, the hands going round.
take = Take("Idle", 120)
for frame in range(0, 121, 10):
    t = frame / 120 * TAU
    take.turn(frame, "torso", swing=1.4 * math.sin(t))
    take.turn(frame, "head", swing=-2.2 * math.sin(t - 0.5), tilt=1.5 * math.sin(2 * t))
    take.shift(frame, "torso", (0, 0, 0.008 * math.sin(2 * t)))
    drift(take, frame, 120)
take.done()

# Walking: the pipe frame shuffles. One leg rises while the other bears, the slippers' toes flex, and everything above
# rides level and a little late.
take = Take("Walk", 48)
for frame in range(0, 49, 4):
    t = frame / 48 * TAU
    left_up, right_up = max(0.0, math.sin(t)), max(0.0, math.sin(t + math.pi))
    take.shift(frame, "leg_pipeL", (0, -0.03 * left_up, 0.055 * left_up))
    take.shift(frame, "leg_pipeR", (0, -0.03 * right_up, 0.055 * right_up))
    take.turn(frame, "slipL", swing=-9 * left_up)
    take.turn(frame, "slipR", swing=9 * right_up)
    take.turn(frame, "torso", swing=2.0 * math.sin(t))
    take.shift(frame, "torso", (0.012 * math.sin(t), 0, 0.01 * abs(math.sin(t))))
    take.turn(frame, "head", swing=-2.5 * math.sin(t - 0.6))
    drift(take, frame, 48, 0.6)
take.done()

# A hand turns its own tap: out and down to the handle, a quarter-turn and back, and home.
take = Take("Tap", 72, loop=False)
reach = TAP_R - HAND_R_REST + Vector((-0.05, -0.02, 0.07))
for frame in range(0, 73, 6):
    drift(take, frame, 72, 0.5, hold_right=True)
    take.turn(frame, "torso", swing=0.0)
for frame, amount, twist in ((0, 0.0, 0), (16, 1.0, 0), (26, 1.0, 55), (38, 1.0, -10), (48, 1.0, 55), (56, 1.0, 0), (72, 0.0, 0)):
    take.shift(frame, "handR", tuple(reach * amount))
    take.turn(frame, "handR", swing=-35 * amount, twist=twist)
    take.turn(frame, "tapR_arm", twist=twist * 0.2)
    take.turn(frame, "head", swing=-6 * amount, tilt=-4 * amount)
take.done()

# A hand goes to the place where the face was, in to the wrist, and comes out with nothing.
take = Take("Face", 84, loop=False)
to_face = FACE - FINGERS_L + Vector((0, -0.16, 0))            # fingertips at the face, a hand's depth in front of it
inside = to_face + Vector((0.02, 0.24, 0.0))                  # and then through it
for frame in range(0, 85, 6):
    drift(take, frame, 84, 0.4, hold_left=True)
for frame, where, tilt in ((0, Vector((0, 0, 0)), 0), (20, to_face, 0), (34, to_face, 0), (46, inside, -6), (58, inside, -6), (68, to_face, 0), (84, Vector((0, 0, 0)), 0)):
    take.shift(frame, "handL", tuple(where))
    take.turn(frame, "handL", swing=-18 if where.length > 0.01 else 0)
    take.turn(frame, "head", tilt=tilt, swing=3 if where.length > 0.01 else 0)
take.done()

# Set. Finished. The hands have come home to the sleeves and nothing drifts any more. Only the faintest breath.
take = Take("Still", 96)
for frame in range(0, 97, 24):
    breath = 0.004 * math.sin(frame / 96 * TAU)
    take.shift(frame, "torso", (0, 0, breath))
    take.turn(frame, "torso", swing=0.0)
    take.turn(frame, "head", swing=0.0, tilt=0.0)
    take.shift(frame, "handL", tuple(SLEEVE_L - HAND_L_REST + Vector((0.0, 0.0, -0.02))))
    take.turn(frame, "handL", swing=150.0)
    take.shift(frame, "handR", tuple(SLEEVE_R - HAND_R_REST + Vector((0.0, 0.0, -0.03))))
    take.turn(frame, "handR", swing=-88.0)
take.done()
