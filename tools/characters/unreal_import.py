"""Step 6 of /make-character, run inside Unreal: bring the character in.

    run_script("characters/unreal_import.py", ["d28"])

Reads   art_source/characters/<id>/<id>.fbx and <id>_paint.png
Makes   /Game/Folded/<id>/SK_<id>           the mesh with its bones
        /Game/Folded/<id>/Anims/<id>_*      one animation for each movement
        /Game/Folded/<id>/T_<id>_Paint      the drawing (front on the left half, back on the right)
        /Game/Folded/<id>/MI_<id>_Paint     the drawing as a material
        /Game/Folded/M_FoldPaint, M_FoldInk the two materials every Folded character shares
The paint glows a little by itself: the Folded are ink, and ink holds in the dark.
"""

import os
import sys

import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "/Game/Folded"


def shared_materials():
    paint_path, ink_path = ROOT + "/M_FoldPaint", ROOT + "/M_FoldInk"
    if not eal.does_asset_exist(paint_path):
        paint = asset_tools.create_asset("M_FoldPaint", ROOT, unreal.Material, unreal.MaterialFactoryNew())
        sample = mel.create_material_expression(paint, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
        sample.set_editor_property("parameter_name", "Paint")
        glow = mel.create_material_expression(paint, unreal.MaterialExpressionScalarParameter, -600, 320)
        glow.set_editor_property("parameter_name", "Glow")
        glow.set_editor_property("default_value", 0.45)
        lit = mel.create_material_expression(paint, unreal.MaterialExpressionMultiply, -250, 260)
        mel.connect_material_expressions(sample, "RGB", lit, "A")
        mel.connect_material_expressions(glow, "", lit, "B")
        rough = mel.create_material_expression(paint, unreal.MaterialExpressionConstant, -250, 420)
        rough.set_editor_property("r", 1.0)
        mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        paint.set_editor_property("used_with_skeletal_mesh", True)
        mel.recompile_material(paint)
        eal.save_asset(paint_path)
    if not eal.does_asset_exist(ink_path):
        ink = asset_tools.create_asset("M_FoldInk", ROOT, unreal.Material, unreal.MaterialFactoryNew())
        colour = mel.create_material_expression(ink, unreal.MaterialExpressionConstant3Vector, -400, 0)
        colour.set_editor_property("constant", unreal.LinearColor(0.012, 0.009, 0.008, 1.0))
        rough = mel.create_material_expression(ink, unreal.MaterialExpressionConstant, -400, 200)
        rough.set_editor_property("r", 1.0)
        mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        ink.set_editor_property("used_with_skeletal_mesh", True)
        mel.recompile_material(ink)
        eal.save_asset(ink_path)
    return eal.load_asset(paint_path), eal.load_asset(ink_path)


def main(args):
    char_id = args[0]
    source = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "art_source", "characters", char_id)
    folder = "%s/%s" % (ROOT, char_id)
    paint_parent, ink = shared_materials()

    # The drawing.
    task = unreal.AssetImportTask()
    task.filename = os.path.join(source, char_id + "_paint.png")
    task.destination_path = folder
    task.destination_name = "T_%s_Paint" % char_id
    task.automated, task.replace_existing, task.save = True, True, True
    asset_tools.import_asset_tasks([task])
    texture = eal.load_asset("%s/T_%s_Paint" % (folder, char_id))

    instance_path = "%s/MI_%s_Paint" % (folder, char_id)
    instance = eal.load_asset(instance_path) if eal.does_asset_exist(instance_path) else asset_tools.create_asset(
        "MI_%s_Paint" % char_id, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, paint_parent)
    mel.set_material_instance_texture_parameter_value(instance, "Paint", texture)
    eal.save_asset(instance_path)

    # The mesh, bones and movements.
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)
    options.set_editor_property("automated_import_should_detect_type", False)
    mesh_options = options.get_editor_property("skeletal_mesh_import_data")
    mesh_options.set_editor_property("import_morph_targets", False)
    mesh_options.set_editor_property("use_t0_as_ref_pose", False)
    mesh_options.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    anim_options = options.get_editor_property("anim_sequence_import_data")
    anim_options.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim_options.set_editor_property("remove_redundant_keys", False)

    task = unreal.AssetImportTask()
    task.filename = os.path.join(source, char_id + ".fbx")
    task.destination_path = folder
    task.destination_name = "SK_%s" % char_id
    task.automated, task.replace_existing, task.save = True, True, True
    task.options = options
    asset_tools.import_asset_tasks([task])
    made = [str(p) for p in task.get_editor_property("imported_object_paths")]

    mesh = None
    for path in eal.list_assets(folder, recursive=True):
        asset = eal.load_asset(path)
        if isinstance(asset, unreal.SkeletalMesh):
            mesh = asset
    if mesh is None:
        raise RuntimeError("No skeletal mesh came in. Imported: %s" % made)
    # Slot 0 is the drawing; slot 1 is the ink edge. (Changing a slot in place does not stick: the list has to be rebuilt.)
    old = mesh.get_editor_property("materials")
    materials = []
    for index, slot in enumerate(old):
        fresh = unreal.SkeletalMaterial()
        fresh.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
        fresh.set_editor_property("material_interface", instance if index == 0 else ink)
        materials.append(fresh)
    mesh.set_editor_property("materials", materials)
    eal.save_loaded_asset(mesh)
    assigned = [m.get_editor_property("material_interface").get_name() for m in mesh.get_editor_property("materials")]
    if assigned[0] != instance.get_name():
        raise RuntimeError("The materials did not take: %s" % assigned)

    # The importer names movements after the file and the armature. Give them the names the game looks for.
    for path in list(eal.list_assets(folder, recursive=True)):
        clean = str(path).split(".")[0]
        if "/Anims/" in clean or not isinstance(eal.load_asset(path), unreal.AnimSequence):
            continue
        name = clean.split("/")[-1].split("_%s_" % char_id)[-1]
        target = "%s/Anims/%s_%s" % (folder, char_id, name)
        if eal.does_asset_exist(target):
            eal.delete_asset(target)
        eal.rename_asset(clean, target)
    animations = sorted(str(p).split(".")[0] for p in eal.list_assets(folder + "/Anims", recursive=True))
    bounds = mesh.get_bounds()
    print("IMPORTED", mesh.get_path_name(), "size cm", [round(v * 2) for v in (bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z)],
          "slots", [str(s.get_editor_property("material_slot_name")) for s in materials])
    print("animations:", [a.split("/")[-1] for a in animations])


if __name__ == "__main__":
    main(sys.argv[1:])
