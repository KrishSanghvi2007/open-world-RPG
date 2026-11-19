# The Hatcher (d28, the Spider). From CAST.md:
#   "Descends and rises on its thread (the thread stretches; the body turns slowly as it drops). On the web or floor it
#    walks in alternating tetrapods (legs 1 and 3 of one side with 2 and 4 of the other), slow deliberate placement with
#    a quick final tap of each foot, then a sudden 3-4 frame rush when something is caught."
#   Self-interactions used: grooming (front legs across the fangs), the death-curl bluff.
# Runs inside blender_anim.py (Take, bone_length, bone_reach, math are already there).

LEGS = ["L1", "L2", "L3", "L4", "R1", "R2", "R3", "R4"]
GROUP_A = {"L1", "L3", "R2", "R4"}            # these four step together; the other four step half a cycle later
HEIGHT = 0.42                                 # how high the body rides, metres


def stance(leg, lift=0.0):
    """Angles that put a leg's foot on the ground with the body at HEIGHT: knee up, shin down. lift raises the foot."""
    femur, tibia = bone_length(leg + "_femur"), bone_length(leg + "_tibia")
    if tibia < 0.05:                                    # a leg with no lower half drawn: just angle it down
        return (-math.degrees(math.asin(min(0.95, HEIGHT / max(0.2, femur)))) + lift, 0.0)
    up = 32.0
    need = -(HEIGHT + femur * math.sin(math.radians(up))) / tibia          # sine of the shin's angle to the ground
    while need < -0.97 and up > -30.0:                  # shin too short to reach: lower the knee until it can
        up -= 4.0
        need = -(HEIGHT + femur * math.sin(math.radians(up))) / tibia
    shin = math.degrees(math.asin(max(-0.97, need)))
    return (up + lift, shin - up - lift * 0.6)


def forward_sign(leg):
    """Which way to swing this leg in the drawing's plane to carry its foot toward the head (-y)."""
    return -1.0 if bone_reach(leg + "_femur").x > 0 else 1.0


def stand(take, frame, lifts=None, swings=None, height=HEIGHT, body_tilt=0.0):
    take.shift(frame, "abdomen", (0, 0, height))
    take.turn(frame, "abdomen", tilt=body_tilt)
    for leg in LEGS:
        femur_tilt, tibia_tilt = stance(leg, (lifts or {}).get(leg, 0.0))
        take.turn(frame, leg + "_femur", tilt=femur_tilt, swing=(swings or {}).get(leg, 0.0))
        take.turn(frame, leg + "_tibia", tilt=tibia_tilt)


def gait(name, length, reach, lift, tap):
    take = Take(name, length)
    for frame in range(0, length + 1, max(1, length // 16)):
        lifts, swings = {}, {}
        for leg in LEGS:
            phase = (frame / length + (0.0 if leg in GROUP_A else 0.5)) % 1.0
            if phase < 0.5:                               # foot on the ground, carried back under the body
                along = 1.0 - 4.0 * phase
                up = 0.0
            else:                                         # foot in the air, reaching forward: slow, then a quick last tap
                t = (phase - 0.5) / 0.5
                along = -1.0 + 2.0 * (t * t * (3 - 2 * t))
                up = math.sin(min(1.0, t / 0.8) * math.pi) * lift
                if 0.84 < t < 0.97:
                    up += tap
            swings[leg] = along * reach * forward_sign(leg)
            lifts[leg] = up
        bob = math.sin(frame / length * 4 * math.pi) * 0.012
        stand(take, frame, lifts, swings, HEIGHT + bob)
        take.turn(frame, "abdomen", swing=math.sin(frame / length * 2 * math.pi) * 2.5)
        take.turn(frame, "head", swing=-math.sin(frame / length * 2 * math.pi) * 3.0)
    take.done()


# Standing still: breathing, and now and then the front legs draw across the fangs.
take = Take("Idle", 96)
for frame in range(0, 97, 8):
    breathe = math.sin(frame / 96 * 4 * math.pi) * 0.015
    lifts, swings = {}, {}
    if 40 <= frame <= 72:                                 # grooming: L4 then R4 lift and cross toward the fangs
        lifts["L4"] = 26.0 if frame in (48, 56) else 10.0
        swings["L4"] = 18.0 * forward_sign("L4") if frame in (48, 56) else 0.0
        lifts["R4"] = 26.0 if frame in (56, 64) else (10.0 if frame > 48 else 0.0)
        swings["R4"] = 18.0 * forward_sign("R4") if frame in (56, 64) else 0.0
    stand(take, frame, lifts, swings, HEIGHT + breathe)
    take.turn(frame, "head", swing=math.sin(frame / 96 * 2 * math.pi) * 4.0)
take.done()

gait("Walk", 40, reach=13.0, lift=20.0, tap=7.0)          # slow, deliberate, with the tap
gait("Rush", 8, reach=20.0, lift=16.0, tap=0.0)           # "a sudden 3-4 frame rush": each leg group moves in four frames

# Hanging on the thread, head down, legs half gathered, turning slowly. The game hangs the body; this is the legs.
take = Take("Hang", 96)
for frame in range(0, 97, 12):
    sway = math.sin(frame / 96 * 2 * math.pi)
    take.shift(frame, "abdomen", (0, 0, 0))
    take.turn(frame, "abdomen", twist=sway * 6.0)
    for index, leg in enumerate(LEGS):
        curl = 38.0 + 6.0 * math.sin(frame / 96 * 2 * math.pi + index)
        take.turn(frame, leg + "_femur", tilt=curl * 0.7)
        take.turn(frame, leg + "_tibia", tilt=-curl * 1.5)
take.done()

# The moment before a strike: rears, front legs high, fangs open. Held as long as the game's wind-up lasts.
take = Take("Windup", 14, loop=False)
stand(take, 0)
for frame, amount in ((6, 0.7), (10, 1.0), (14, 1.0)):
    stand(take, frame, {"L4": 46 * amount, "R4": 46 * amount, "L3": 22 * amount, "R3": 22 * amount}, None, HEIGHT + 0.10 * amount, body_tilt=14 * amount)
    take.turn(frame, "head", tilt=18 * amount)
take.done()

# The strike itself: down and forward in three frames, then gathering back.
take = Take("Strike", 16, loop=False)
stand(take, 0, {"L4": 46, "R4": 46, "L3": 22, "R3": 22}, None, HEIGHT + 0.10, body_tilt=14)
stand(take, 3, {"L4": -6, "R4": -6}, {"L4": 24 * forward_sign("L4"), "R4": 24 * forward_sign("R4")}, HEIGHT - 0.14, body_tilt=-12)
take.shift(3, "abdomen", (0, -0.35, HEIGHT - 0.14))
take.turn(3, "head", tilt=-20)
stand(take, 7, None, None, HEIGHT - 0.08, body_tilt=-6)
take.shift(7, "abdomen", (0, -0.2, HEIGHT - 0.08))
stand(take, 16)
take.done()

# Struck: a quick shrink back.
take = Take("Flinch", 8, loop=False)
stand(take, 0)
stand(take, 2, {leg: 12 for leg in LEGS}, None, HEIGHT - 0.1, body_tilt=8)
take.shift(2, "abdomen", (0, 0.12, HEIGHT - 0.1))
stand(take, 8)
take.done()

# The death-curl (also its bluff): all eight legs fold in under the body, which drops on its back edge and is still.
take = Take("Curl", 22, loop=False)
stand(take, 0)
stand(take, 4, {leg: 18 for leg in LEGS}, None, HEIGHT + 0.06)
for frame, amount in ((12, 0.8), (18, 1.0), (22, 1.0)):
    take.shift(frame, "abdomen", (0, 0, 0.16 + (1 - amount) * 0.2))
    take.turn(frame, "abdomen", tilt=6 * amount)
    for leg in LEGS:
        take.turn(frame, leg + "_femur", tilt=58 * amount, swing=10 * amount * forward_sign(leg))
        take.turn(frame, leg + "_tibia", tilt=-128 * amount)
take.done()
