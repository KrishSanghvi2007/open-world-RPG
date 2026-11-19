"""Step 4 of /make-character, run inside Blender after blender_build.py: the character's movements.

    CHAR_ID = "d28"; exec(open("<repo>/tools/characters/blender_anim.py").read())

This file is the toolbox (poses, keys, loops). What each character actually does is in tools/characters/anims/<id>.py,
written from the "Locomotion" and "Self-interactions" notes of that character in the drawings repository's docs/CAST.md.
Each movement becomes one action named <id>_<Name>, 24 frames a second, and is exported as its own animation.
"""

import math
import os

import bpy
from mathutils import Euler, Matrix, Vector

REPO = "/Users/user/Documents/GitHub/open-world-RPG"
char_id = CHAR_ID            # noqa: F821
rig = bpy.data.objects[char_id + "_Rig"]
body = bpy.data.objects[char_id + "_Body"]
FPS = 24
bpy.context.scene.render.fps = FPS
for pose_bone in rig.pose.bones:
    pose_bone.rotation_mode = "XYZ"          # tilt (x) first, then swing in the drawing's plane (z)
if rig.animation_data is None:
    rig.animation_data_create()
for old in [a for a in bpy.data.actions if a.name.startswith(char_id + "_")]:
    bpy.data.actions.remove(old)


class Take:
    """One movement. Set poses with turn() and shift() at a frame; every bone touched is keyed at that frame."""

    def __init__(self, name, length, loop=True):
        self.name, self.length, self.loop = char_id + "_" + name, length, loop
        self.action = bpy.data.actions.new(self.name)
        self.action.use_fake_user = True
        rig.animation_data.action = self.action
        self.rest()

    def rest(self):
        for pose_bone in rig.pose.bones:
            pose_bone.rotation_euler = (0, 0, 0)
            pose_bone.location = (0, 0, 0)
            pose_bone.scale = (1, 1, 1)

    def turn(self, frame, bone, tilt=0.0, swing=0.0, twist=0.0):
        """Degrees. tilt: toward (+) or away from the viewer of the drawing. swing: in the drawing's plane. twist: about the bone."""
        if bone not in rig.pose.bones:
            return
        pose_bone = rig.pose.bones[bone]
        pose_bone.rotation_euler = Euler((math.radians(tilt), math.radians(twist), math.radians(swing)), "XYZ")
        pose_bone.keyframe_insert("rotation_euler", frame=frame)

    def shift(self, frame, bone, offset):
        """Move a bone by metres in the model's own directions (x right, y back, z up for a standing figure)."""
        if bone not in rig.pose.bones:
            return
        pose_bone = rig.pose.bones[bone]
        rest = pose_bone.bone.matrix_local.to_3x3()
        if pose_bone.parent:
            rest = pose_bone.parent.bone.matrix_local.to_3x3().inverted() @ rest
            world_to_parent = pose_bone.parent.bone.matrix_local.to_3x3().inverted()
            pose_bone.location = rest.inverted() @ (world_to_parent @ Vector(offset))
        else:
            pose_bone.location = rest.inverted() @ Vector(offset)
        pose_bone.keyframe_insert("location", frame=frame)

    def size(self, frame, bone, factor):
        if bone not in rig.pose.bones:
            return
        rig.pose.bones[bone].scale = (factor, factor, factor) if not isinstance(factor, tuple) else factor
        rig.pose.bones[bone].keyframe_insert("scale", frame=frame)

    def done(self):
        self.action.frame_range = (0, self.length)
        self.action.use_frame_range = True
        try:
            for curve in self.action.fcurves:
                for point in curve.keyframe_points:
                    point.interpolation = "BEZIER"
        except AttributeError:
            pass            # newer Blender keeps curves in layers; the default interpolation is already smooth
        print("  take", self.name, self.length, "frames")


def sheet_point(fx, fy, front_m=0.0):
    """A point given as a fraction of the drawing sheet (as CAST.md gives them) -> where that is on the model, in metres."""
    import json
    front = json.load(open(os.path.join(REPO, "art_source", "characters", char_id, "front.json")))
    root_u, root_v, fig_v0, scale, lying, flip, mid_depth = rig["sheet_to_model"]
    sheet_w, sheet_h = front["sheet_px"]
    cx0, cy0, cx1, cy1 = front["crop_px"]
    side = front["square_px"]
    u = (fx * sheet_w - cx0 + (side - (cx1 - cx0)) / 2.0) / side
    v = 1.0 - (fy * sheet_h - cy0 + (side - (cy1 - cy0)) / 2.0) / side
    if lying:
        return Vector(((u - root_u) * scale * flip, (v - root_v) * scale * flip, mid_depth * scale + front_m))
    return Vector(((u - root_u) * scale, -mid_depth * scale - front_m, (v - fig_v0) * scale))


def bone_length(name):
    return rig.data.bones[name].length if name in rig.data.bones else 0.0


def bone_reach(name):
    """Where a bone's tip is relative to its head, in the model's directions, at rest."""
    bone = rig.data.bones[name]
    return bone.tail_local - bone.head_local


exec(open(os.path.join(REPO, "tools", "characters", "anims", char_id + ".py")).read())
rig.animation_data.action = bpy.data.actions.get(char_id + "_Idle")
bpy.context.scene.frame_start = 0
bpy.context.scene.frame_end = 48
bpy.ops.wm.save_mainfile()
print("ANIMATED", char_id, [a.name for a in bpy.data.actions if a.name.startswith(char_id + "_")])
