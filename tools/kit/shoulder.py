"""The shape of her shoulder round Varnish: one function, used both to build the ground (in Blender,
tools/kit/blender_vista.py) and to stand things on it (in Unreal, tools/kit/dress.py). Plain arithmetic, no imports
beyond math, so both can read it.
"""

import math

# The terraces' footprint in the level, in metres (tools/greybox/build_matriarch_greybox.py, build_village).
FOOT_X, FOOT_Y0, FOOT_Y1 = 35.0, -39.0, 43.0


def _smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def _noise(x, y):
    return (math.sin(x * 0.043 + 1.3) * math.cos(y * 0.037 - 0.4) + 0.5 * math.sin(x * 0.11 + y * 0.09) + 0.25 * math.sin(x * 0.27 - y * 0.23 + 2.0)) / 1.75


def shoulder_height(x, y):
    """Height of her shoulder at a point (metres, level coordinates)."""
    dx = max(0.0, abs(x) - FOOT_X)
    dy_w = max(0.0, FOOT_Y0 - y)             # how far west of the Hem (toward the Lamp)
    dy_e = max(0.0, y - FOOT_Y1)             # how far east of the top terrace (toward her spine)
    d = math.sqrt(dx * dx + (dy_w + dy_e) ** 2)
    if d <= 0.0:
        return -3.0                           # under the terraces: out of sight
    yc = min(max(y, FOOT_Y0), FOOT_Y1)
    # Beside the terraces the slope steps up a little before each terrace does, so their sides are always buried.
    level = 4.0 * _smooth((yc + 16.0) / 5.0) + 4.0 * _smooth((yc - 12.0) / 5.0)
    near = level + 0.25
    # The long curve: steep toward the Lamp, gentle along her back (north and south), and up to the ridge of
    # her spine in the east before it falls again on the far side.
    fall = dy_w ** 2 / (420.0 + 1.2 * dy_w) + dx ** 2 / (2200.0 + 2.0 * dx)
    over = max(0.0, dy_e - 150.0)
    ridge = 16.0 * _smooth(dy_e / 110.0) - over ** 2 / (600.0 + 1.5 * over)
    rough = _noise(x, y) * 2.2 * _smooth(d / 25.0)
    h = near - fall + ridge + rough
    # The Furrow: the groove of her spine begins at the gap in the square's north parapet and runs north, deepening.
    if x > FOOT_X:
        along = x - FOOT_X
        centre = 4.0 + 9.0 * math.sin(along * 0.018) * _smooth(along / 60.0)
        width = 4.5 + along * 0.05
        t = abs(y - centre) / width
        if t < 2.2:
            floor = 3.9 - 0.2 * along - along ** 2 / 2200.0
            wgt = 1.0 - _smooth((t - 0.9) / 1.3)
            h = h * (1.0 - wgt) + min(h, floor) * wgt
    edge = _smooth(d / 0.6)
    return -3.0 * (1.0 - edge) + h * edge
