"""Step 5 of /make-character, run inside Blender after blender_anim.py: the turnaround sheet and the file for Unreal.

    CHAR_ID = "d28"; exec(open("<repo>/tools/characters/blender_export.py").read())

Writes  art_source/characters/<id>/turnaround.png    front, three-quarter, side and back, as it will look in the game
        art_source/characters/<id>/<id>.fbx          the mesh, its bones and every movement (one animation each)
"""

import math
import os

import bpy
import numpy as np
from mathutils import Vector

REPO = "/Users/user/Documents/GitHub/open-world-RPG"
char_id = CHAR_ID            # noqa: F821
folder = os.path.join(REPO, "art_source", "characters", char_id)
rig = bpy.data.objects[char_id + "_Rig"]
body = bpy.data.objects[char_id + "_Body"]
scene = bpy.context.scene

# ---------------------------------------------------------------- the turnaround
idle = bpy.data.actions.get(char_id + "_Idle")
if idle:
    rig.animation_data.action = idle
scene.frame_set(0)
bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
evaluated = body.evaluated_get(depsgraph)
corners = [evaluated.matrix_world @ Vector(c) for c in evaluated.bound_box]
low = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
high = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
centre, span = (low + high) / 2.0, max((high - low)[:]) * 1.18

camera_data = bpy.data.cameras.new("Turnaround")
camera_data.type = "ORTHO"
camera_data.ortho_scale = span
camera = bpy.data.objects.new("Turnaround", camera_data)
scene.collection.objects.link(camera)
scene.camera = camera
scene.render.engine = "BLENDER_EEVEE" if "BLENDER_EEVEE" in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items] else scene.render.engine
scene.render.resolution_x = scene.render.resolution_y = 640
scene.render.film_transparent = False
scene.view_settings.view_transform = "Standard"
tiles = []
for name, direction in (("front", (0, -1, 0.12)), ("three_quarter", (0.75, -1, 0.3)), ("side", (1, 0, 0.12)), ("back", (0, 1, 0.12))):
    eye = Vector(direction).normalized()
    camera.location = centre + eye * (span * 3.0)
    camera.rotation_euler = (-eye).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = os.path.join(folder, "raw", "turn_" + name + ".png")
    bpy.ops.render.render(write_still=True)
    image = bpy.data.images.load(scene.render.filepath)
    tiles.append(np.array(image.pixels[:], dtype=np.float32).reshape(640, 640, 4))
    bpy.data.images.remove(image)
sheet = bpy.data.images.new("turnaround", 640 * len(tiles), 640, alpha=False)
sheet.pixels = np.concatenate(tiles, axis=1).ravel()
sheet.filepath_raw = os.path.join(folder, "turnaround.png")
sheet.file_format = "PNG"
sheet.save()
bpy.data.objects.remove(camera)

# ---------------------------------------------------------------- the file for Unreal
for other in bpy.data.objects:
    other.select_set(False)
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
old_name = rig.name
rig.name = "Armature"                 # Unreal adds a bone named after this object unless it has exactly this name
bpy.ops.export_scene.fbx(
    filepath=os.path.join(folder, char_id + ".fbx"), use_selection=True, object_types={"ARMATURE", "MESH"},
    add_leaf_bones=False, use_armature_deform_only=False, mesh_smooth_type="FACE",
    bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False, bake_anim_use_all_actions=True,
    bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
    apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Z", axis_up="Y")
rig.name = old_name
bpy.ops.wm.save_mainfile()
print("EXPORTED", char_id, "size m", [round(d, 2) for d in body.dimensions], "bones", len(rig.data.bones),
      "actions", len([a for a in bpy.data.actions if a.name.startswith(char_id + "_")]))
