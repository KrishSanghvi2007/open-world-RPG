"""Brings a zone's kit into Unreal and paints it. Run inside the editor:

    run_script("kit/unreal_kit.py", ["Varnish"])

Reads   art_source/kit/<Zone>/*.fbx and kit.json (made by tools/kit/blender_<zone>.py)
Makes   /Game/Kit/<Zone>/SM_*    one static mesh per piece, each material slot painted with the Glaze material in the
                                 zone's palette (docs/ART_BIBLE.md, section 2), by the slot's NAME.
Pieces carry no collision of their own: the grey-box layout underneath is what the player touches, so dressing a
zone can never change where anyone can walk. Houses with a way in are the exception (their walls are the collision).
"""

import json
import os
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "tools", "stylekit"))
import stylekit                     # noqa: E402

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Slot name -> (colour, kind of brushwork). Scenery stays under about 70% saturation; pigment is allowed full strength.
COMMON = {
    "Wood": ("#6B4F3A", "Wood"), "Bark": ("#5E4634", "Wood"), "Metal": ("#35313F", "Stone"), "Dark": ("#2A1F2E", "Plaster"),
    "Black": ("#1B1620", "Wood"), "Teal": ("#2E5E6E", "Wood"), "Cloth": ("#C4574B", "Dab"), "ClothTeal": ("#2E5E6E", "Dab"),
    "ClothOchre": ("#D9A441", "Dab"), "Canvas": ("#D8CBB0", "Plaster"), "Charcoal": ("#3A3A3A", "Plaster"), "Bone": ("#CFC3A8", "Stone"),
    "Paper": ("#E6E1D6", "Plaster"), "Graphite": ("#2E2E30", "Plaster"), "Blue": ("#2F4BA8", "Plaster"), "Water": ("#3F7480", "Ground"),
    "Cliff": ("#6B4F3A", "Stone"), "Line": ("#2E2E30", "Plaster"), "BlueLine": ("#2F4BA8", "Plaster"), "Scab": ("#5A2A22", "Dab"),
    "PigmentOchre": ("#D9A441", "Dab"), "PigmentGreen": ("#6B8F71", "Dab"), "PigmentRed": ("#9E2B25", "Dab"),
    "PigmentBlack": ("#2A2826", "Dab"), "PigmentBlue": ("#2F4BA8", "Dab"), "Sanguine": ("#9E2B25", "Dab"),
}
PALETTES = {
    "Varnish": dict(COMMON, Plaster=("#F2E3C0", "Plaster"), Roof=("#C4574B", "Roof"), Stone=("#8F86A6", "Stone"), Ground=("#E3C39A", "Ground"),
                    Foliage=("#6B8F71", "Dab"), FoliageLight=("#9DB28A", "Dab"), Moss=("#9DB28A", "Dab"),
                    Paving=("#CDBBB2", "Stone"), Path=("#F3DAB4", "Ground")),
    "Scumblewood": dict(COMMON, Plaster=("#D8CBB0", "Plaster"), Roof=("#6B4F3A", "Roof"), Stone=("#7A6A58", "Stone"), Ground=("#7E8A66", "Ground"),
                        Foliage=("#6B8F71", "Dab"), FoliageLight=("#9DB28A", "Dab"), Moss=("#9DB28A", "Dab")),
    "Vault": dict(COMMON, Plaster=("#E6E1D6", "Plaster"), Stone=("#8C8A85", "Stone"), Ground=("#E6E1D6", "Ground")),
}
PALETTES["Vista"] = dict(COMMON, Hide=("#B4905F", "Ground"))
SHADOW = {"Varnish": "#4A3558", "Scumblewood": "#2E5E6E", "Vault": "#4B4A48", "Vista": "#4A3558"}

# How each piece meets the player. Most scenery is not solid: the grey-box shapes underneath are what is touched, so
# dressing a zone cannot change where anyone can walk. Furniture-sized things are the exception: walking through a
# barrel looks broken, so these are solid, and the dressing pass keeps them off the routes (checked by a playthrough).
SOLID_BOX = ("SM_Stall_Madder", "SM_Stall_Ochre", "SM_Crates", "SM_Barrel", "SM_Sacks", "SM_Bench", "SM_Handcart", "SM_Trough", "SM_DryingRack",
             "SM_Cistern", "SM_FlaxPress", "SM_Rocks_B", "SM_Boulder_M", "SM_Boulder_L", "SM_Boulder_Moss", "SM_Stump", "SM_FallenTrunk",
             "SM_TallyStone", "SM_LeanTo", "SM_RibPost", "SM_Woodpile")
SOLID_EXACT = ("SM_Tree_A", "SM_Tree_B", "SM_DyeLine", "SM_Fence", "SM_ForestTree_A", "SM_ForestTree_B", "SM_ForestTree_C", "SM_RootArch",
               "SM_RootArch_Wide", "SM_RibGate", "SM_RibArch", "SM_RibArch_Wide")
WALK_IN = ("SM_House_Lampworks", "SM_House_Grinding", "SM_House_Cistern")


def glow_material():
    """Window light and lantern flame: paint that shines by itself, because sunlight is the only real light there is."""
    path = "/Game/Kit/M_KitGlow"
    if not eal.does_asset_exist(path):
        material = asset_tools.create_asset("M_KitGlow", "/Game/Kit", unreal.Material, unreal.MaterialFactoryNew())
        colour = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
        colour.set_editor_property("parameter_name", "Colour")
        colour.set_editor_property("default_value", unreal.LinearColor(1.0, 0.48, 0.12, 1.0))
        strength = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 220)
        strength.set_editor_property("parameter_name", "Strength")
        strength.set_editor_property("default_value", 3.0)
        lit = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -220, 100)
        mel.connect_material_expressions(colour, "", lit, "A")
        mel.connect_material_expressions(strength, "", lit, "B")
        mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mel.recompile_material(material)
        eal.save_asset(path)
    return eal.load_asset(path)


def glow(name, hex_colour, strength):
    """A dimmer or differently coloured glow: the heart, embers."""
    path = "/Game/Kit/%s" % name
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    parent = glow_material()
    mi = asset_tools.create_asset(name, "/Game/Kit", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    mel.set_material_instance_vector_parameter_value(mi, "Colour", stylekit.lin(hex_colour))
    mel.set_material_instance_scalar_parameter_value(mi, "Strength", strength)
    mel.update_material_instance(mi)
    eal.save_asset(path)
    return mi


# Slots that are dark by nature. The Glaze material lifts its lightest strokes toward cream, which is right for plaster
# and wrong for ink, iron and shadow: these keep their lights close to their own colour.
INKY = ("Dark", "Black", "Line", "Graphite", "Charcoal", "Metal", "PigmentBlack", "Scab", "BlueLine")
# Slots on very large forms get broader brushwork, or the strokes read as a pattern of dots: (mask, cm per repeat, light).
BROAD = {"Cliff": ("T_Stroke_Stone", 1500.0, 0.22), "Hide": ("T_Stroke_Ground", 3200.0, 0.2)}


def _lift(hex_colour, amount, toward="#F2E3C0"):
    a = [int(hex_colour.lstrip("#")[i:i + 2], 16) for i in (0, 2, 4)]
    b = [int(toward.lstrip("#")[i:i + 2], 16) for i in (0, 2, 4)]
    return "#%02X%02X%02X" % tuple(int(round(x + (y - x) * amount)) for x, y in zip(a, b))


def glaze_tuned(colour, surface, shadow, light_lift, tiling=None, mask=None):
    """A Glaze instance whose light strokes stay near its own colour."""
    base_mask, base_tiling = stylekit.SURFACES.get(surface, stylekit.SURFACES["Plaster"])
    mask, tiling = mask or base_mask, tiling or base_tiling
    name = "MI_KitGlaze_%s_%s_%s_%d_%d" % (colour.lstrip("#").upper(), mask.replace("T_Stroke_", ""), shadow.lstrip("#").upper(), int(tiling), int(light_lift * 100))

    def setup(mi):
        mel.set_material_instance_vector_parameter_value(mi, "Colour", stylekit.lin(colour))
        mel.set_material_instance_vector_parameter_value(mi, "ShadowTint", stylekit.lin(shadow))
        # The material mixes 45% of LightTint into the colour for its lightest strokes; choose LightTint so that mix lands
        # only light_lift of the way to cream.
        mel.set_material_instance_vector_parameter_value(mi, "LightTint", stylekit.lin(_lift(colour, min(1.0, light_lift / 0.45))))
        mel.set_material_instance_vector_parameter_value(mi, "AccentTint", stylekit.lin(_lift(colour, light_lift * 0.5, "#D9A441")))
        mel.set_material_instance_texture_parameter_value(mi, "StrokeMask", eal.load_asset("%s/%s" % (stylekit.TEX, mask)))
        mel.set_material_instance_scalar_parameter_value(mi, "Tiling", tiling)
    return stylekit._instance(name, "M_Glaze", setup)


def paint(zone, slot, palette=None):
    palette = palette or PALETTES[zone]
    if slot == "Glow" and "Glow" in palette and palette["Glow"] is None:
        return glaze_tuned("#3A3344", "Plaster", SHADOW[zone], 0.06)      # a window with nobody behind it
    if slot in BROAD and slot in palette:
        mask, tiling, lift = BROAD[slot]
        return glaze_tuned(palette[slot][0], palette[slot][1], SHADOW[zone], lift, tiling, mask)
    if slot in INKY and slot in palette and not (zone == "Vault" and slot in ("Line", "BlueLine", "Graphite")):
        return glaze_tuned(palette[slot][0], palette[slot][1], SHADOW[zone], 0.06)
    if zone == "Vault" and slot in ("Line", "BlueLine", "Graphite"):
        return stylekit.sketch(palette[slot][0], "Plaster")          # drawn lines do not take light: they are on the paper
    if slot == "Glow":
        return glow_material()
    if slot == "Silhouette":
        if palette.get("SilhouetteNear"):
            return glow("MI_KitGlow_SilhouetteNear", "#8E6F7C", 1.25)   # the Wader, after the Turning: closer, and darker for it
        return glow("MI_KitGlow_Silhouette", "#C7A2A6", 1.5)        # a far colossus: paler than anything near, a little cooler than the sky
    if slot == "Plain":
        return glow("MI_KitGlow_Plain", "#D9B99C", 1.7)             # the land far below: all haze
    if slot == "Lamp":
        return glow("MI_KitGlow_Lamp", "#FFF0CC", 7.0)
    if slot == "Heart":
        return glow("MI_KitGlow_Heart", "#E8923A", 1.6)
    if slot == "Erasure":
        return stylekit.glaze("#FFFFFF", "Plaster", SHADOW[zone])
    colour, surface = palette.get(slot, ("#B0A090", "Plaster"))
    if zone == "Vault" and slot in ("Plaster", "Stone", "Ground", "Paper", "Bone"):
        return stylekit.sketch(colour, surface)
    return stylekit.glaze(colour, surface, SHADOW[zone])


def main(args):
    zone = args[0]
    source = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "art_source", "kit", zone)
    listing = json.load(open(os.path.join(source, "kit.json")))
    folder = "/Game/Kit/%s" % zone
    tasks = []
    for piece in listing:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(source, piece["name"] + ".fbx")
        task.destination_path = folder
        task.automated, task.replace_existing, task.save = True, True, False
        options = unreal.FbxImportUI()
        options.import_mesh, options.import_as_skeletal, options.import_materials, options.import_textures = True, False, False, False
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.auto_generate_collision = False
        options.static_mesh_import_data.generate_lightmap_u_vs = False
        task.options = options
        tasks.append(task)
    asset_tools.import_asset_tasks(tasks)

    painted = 0
    for piece in listing:
        mesh = eal.load_asset("%s/%s" % (folder, piece["name"]))
        if not mesh:
            print("MISSING", piece["name"])
            continue
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            mesh.set_material(index, paint(zone, str(slot.get_editor_property("material_slot_name"))))
        body = mesh.get_editor_property("body_setup")
        if body and piece["name"] in WALK_IN + SOLID_EXACT:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        elif piece["name"] in SOLID_BOX:
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(mesh)
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
        eal.save_loaded_asset(mesh)
        painted += 1
    print("KIT IMPORTED", zone, painted, "of", len(listing), "pieces into", folder)


if __name__ == "__main__":
    main(sys.argv[1:])
