"""Prints facts about a Folded character's mesh in Unreal: its material slots, and which way its faces point."""
import sys

import unreal

char_id = sys.argv[1] if len(sys.argv) > 1 else "d40"
mesh = unreal.EditorAssetLibrary.load_asset("/Game/Folded/%s/SK_%s" % (char_id, char_id))
print("materials:", [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name()) for m in mesh.get_editor_property("materials")])
paint = unreal.EditorAssetLibrary.load_asset("/Game/Folded/%s/MI_%s_Paint" % (char_id, char_id))
print("paint parent:", paint.get_editor_property("parent").get_name(), "texture:", unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(paint, "Paint"))
tex = unreal.EditorAssetLibrary.load_asset("/Game/Folded/%s/T_%s_Paint" % (char_id, char_id))
print("texture size:", tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), "srgb:", tex.get_editor_property("srgb"))
parent = unreal.EditorAssetLibrary.load_asset("/Game/Folded/M_FoldPaint")
print("parent two sided:", parent.get_editor_property("two_sided"), "skeletal:", parent.get_editor_property("used_with_skeletal_mesh"))
print("mesh material methods:", [n for n in dir(mesh) if "material" in n.lower()])
print("skeletal material fields:", [n for n in dir(unreal.SkeletalMaterial) if not n.startswith("_")][:20])
