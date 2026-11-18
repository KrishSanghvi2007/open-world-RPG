"""Builds the Varnish "look test": one corner of the village square at near-final painterly quality,
in three style variations for the creative director to choose from.

Run inside the Unreal editor:
    build_looktest.py assets            import textures and meshes, (re)create the parent materials
    build_looktest.py level <Style>     build /Game/LookTest/Maps/L_LookTest_<Style>  (Gouache, Glaze or Wash)
    build_looktest.py all               everything

The look follows docs/GDD.md section 9: brushwork in textures, three values per surface, tinted shadows,
one shadow-casting sun plus one tinted sky light, small lights without shadows, a light post-process and
no full-screen paint filter. Lumen and ray tracing stay off.
"""

import math
import os
import sys
import traceback

import unreal

ROOT = "/Game/LookTest"
TEX, MESH, MAT, MAPS = ROOT + "/Textures", ROOT + "/Meshes", ROOT + "/Materials", ROOT + "/Maps"
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "art_source", "looktest")

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MASKS = ["T_Stroke_Plaster", "T_Stroke_Roof", "T_Stroke_Wood", "T_Stroke_Stone", "T_Stroke_Ground", "T_Stroke_Dab"]
SKIES = ["T_Sky_Gouache", "T_Sky_Glaze", "T_Sky_Wash"]
PROPS = ["SM_House_A", "SM_House_B", "SM_Lampstand", "SM_GrindingSlab", "SM_Tree", "SM_Blotter", "SM_Hesper"]


def lin(hex_colour, scale=1.0):
    h = hex_colour.lstrip("#")
    r, g, b = (pow(int(h[i:i + 2], 16) / 255.0, 2.2) * scale for i in (0, 2, 4))
    return unreal.LinearColor(r, g, b, 1.0)


# --------------------------------------------------------------------------------------
# Import
# --------------------------------------------------------------------------------------

def import_assets():
    tasks = []
    for name in MASKS + SKIES:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(SRC, "textures", name + ".png")
        task.destination_path = TEX
        task.automated, task.replace_existing, task.save = True, True, True
        tasks.append(task)
    for name in PROPS:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(SRC, "props", name + ".fbx")
        task.destination_path = MESH
        task.automated, task.replace_existing, task.save = True, True, True
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_as_skeletal = False
        options.import_materials = False
        options.import_textures = False
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.auto_generate_collision = True
        task.options = options
        tasks.append(task)
    asset_tools.import_asset_tasks(tasks)

    for name in MASKS:
        tex = eal.load_asset("%s/%s" % (TEX, name))
        tex.set_editor_property("srgb", False)                     # data, not colour
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        eal.save_asset("%s/%s" % (TEX, name))
    for name in SKIES:
        tex = eal.load_asset("%s/%s" % (TEX, name))
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        eal.save_asset("%s/%s" % (TEX, name))
    print("imported %d textures, %d meshes" % (len(MASKS + SKIES), len(PROPS)))


# --------------------------------------------------------------------------------------
# Parent materials
# --------------------------------------------------------------------------------------

TRIPLANAR = """
float3 w = pow(abs(N), 6.0);
w /= (w.x + w.y + w.z);
float3 p = P / max(Scale, 1.0);
float4 x = Texture2DSample(Tex, TexSampler, p.yz);
float4 y = Texture2DSample(Tex, TexSampler, p.xz);
float4 z = Texture2DSample(Tex, TexSampler, p.xy);
return x * w.x + y * w.y + z * w.z;
"""

# Three values per surface (GDD rule 1). Sharp near 0 gives flat gouache shapes; larger gives soft glazes.
RAMP = """
float t1 = smoothstep(0.34 - Sharp, 0.34 + Sharp, S.r);
float t2 = smoothstep(0.68 - Sharp, 0.68 + Sharp, S.r);
float3 c = lerp(lerp(Dark, Mid, t1), Light, t2);
c = lerp(c, Accent, saturate(S.g * AccentAmount));
c *= lerp(1.0, 0.80 + 0.40 * S.b, Bristle);
return c;
"""

# Bounce light painted into the material (GDD lighting rule 3): surfaces facing down or sideways pick up warm ground light.
BOUNCE = "return C * BounceColour * Bounce * saturate(0.75 - N.z * 0.75);"


def _new_material(name):
    path = "%s/%s" % (MAT, name)
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return asset_tools.create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())


def _custom(material, code, output_type, inputs, x, y, description):
    node = mel.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code.strip())
    node.set_editor_property("output_type", output_type)
    node.set_editor_property("description", description)
    custom_inputs = []
    for input_name in inputs:
        custom_input = unreal.CustomInput()
        custom_input.set_editor_property("input_name", input_name)
        custom_inputs.append(custom_input)
    node.set_editor_property("inputs", custom_inputs)
    return node


def _scalar(material, name, default, x, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def _vector(material, name, default, x, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def _stroke_sample(material, x, y):
    """Texture object parameter + world position + normal -> triplanar sample of the stroke mask."""
    tex = mel.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, x, y)
    tex.set_editor_property("parameter_name", "StrokeMask")
    tex.set_editor_property("texture", eal.load_asset(TEX + "/T_Stroke_Plaster"))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    pos = mel.create_material_expression(material, unreal.MaterialExpressionWorldPosition, x, y + 200)
    normal = mel.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, x, y + 350)
    tiling = _scalar(material, "Tiling", 300.0, x, y + 500)
    tri = _custom(material, TRIPLANAR, unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["Tex", "P", "N", "Scale"], x + 350, y, "Triplanar strokes")
    mel.connect_material_expressions(tex, "", tri, "Tex")
    mel.connect_material_expressions(pos, "", tri, "P")
    mel.connect_material_expressions(normal, "", tri, "N")
    mel.connect_material_expressions(tiling, "", tri, "Scale")
    return tri, normal


def create_materials():
    # --- M_Painterly: lit, matte, three painted values ---
    m = _new_material("M_Painterly")
    tri, normal = _stroke_sample(m, -1400, 0)
    ramp = _custom(m, RAMP, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                   ["S", "Dark", "Mid", "Light", "Accent", "Sharp", "AccentAmount", "Bristle"], -500, 0, "Three-value ramp")
    mel.connect_material_expressions(tri, "", ramp, "S")
    defaults = {"Dark": lin("#8C6A4E"), "Mid": lin("#D9B77A"), "Light": lin("#F2E3C0"), "Accent": lin("#D9A441")}
    for i, name in enumerate(("Dark", "Mid", "Light", "Accent")):
        mel.connect_material_expressions(_vector(m, name, defaults[name], -900, 250 + i * 200), "", ramp, name)
    for i, (name, value) in enumerate((("Sharp", 0.06), ("AccentAmount", 0.5), ("Bristle", 0.5))):
        mel.connect_material_expressions(_scalar(m, name, value, -900, 1100 + i * 110), "", ramp, name)
    mel.connect_material_property(ramp, "", unreal.MaterialProperty.MP_BASE_COLOR)

    bounce = _custom(m, BOUNCE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["C", "N", "BounceColour", "Bounce"], -100, 500, "Painted bounce light")
    mel.connect_material_expressions(ramp, "", bounce, "C")
    mel.connect_material_expressions(normal, "", bounce, "N")
    mel.connect_material_expressions(_vector(m, "BounceColour", lin("#E8923A"), -500, 700), "", bounce, "BounceColour")
    mel.connect_material_expressions(_scalar(m, "Bounce", 0.0, -500, 900), "", bounce, "Bounce")
    mel.connect_material_property(bounce, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    for prop, value in ((unreal.MaterialProperty.MP_ROUGHNESS, 1.0), (unreal.MaterialProperty.MP_SPECULAR, 0.0)):
        const = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -100, 1100 if value else 1200)
        const.set_editor_property("r", value)
        mel.connect_material_property(const, "", prop)
    mel.recompile_material(m)
    eal.save_asset(MAT + "/M_Painterly")

    # --- M_PainterlyGlow: unlit, for flames and lit windows ---
    g = _new_material("M_PainterlyGlow")
    g.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    tri, _ = _stroke_sample(g, -1200, 0)
    glow = _custom(g, "return lerp(Colour, Hot, S.r * S.r) * Intensity * (0.75 + 0.5 * S.b);", unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                   ["S", "Colour", "Hot", "Intensity"], -400, 0, "Glow")
    mel.connect_material_expressions(tri, "", glow, "S")
    mel.connect_material_expressions(_vector(g, "Colour", lin("#E8923A"), -800, 300), "", glow, "Colour")
    mel.connect_material_expressions(_vector(g, "Hot", lin("#FFE9A8"), -800, 500), "", glow, "Hot")
    mel.connect_material_expressions(_scalar(g, "Intensity", 4.0, -800, 700), "", glow, "Intensity")
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(g)
    eal.save_asset(MAT + "/M_PainterlyGlow")

    # --- M_PaintedSky: unlit panorama on the inside of a sphere ---
    s = _new_material("M_PaintedSky")
    s.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    s.set_editor_property("two_sided", True)
    sky = mel.create_material_expression(s, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    sky.set_editor_property("parameter_name", "Sky")
    sky.set_editor_property("texture", eal.load_asset(TEX + "/T_Sky_Glaze"))
    brightness = _scalar(s, "Brightness", 1.0, -700, 300)
    mul = mel.create_material_expression(s, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(sky, "RGB", mul, "A")
    mel.connect_material_expressions(brightness, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(s)
    eal.save_asset(MAT + "/M_PaintedSky")

    # --- M_PP_InkLine: a thin drawn line where depth or surface direction changes (ink-and-wash style only) ---
    p = _new_material("M_PP_InkLine")
    p.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    nodes = {}
    for i, (key, scene_id) in enumerate((("Scene", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),
                                         ("Depth", unreal.SceneTextureId.PPI_SCENE_DEPTH),
                                         ("Norm", unreal.SceneTextureId.PPI_WORLD_NORMAL))):
        node = mel.create_material_expression(p, unreal.MaterialExpressionSceneTexture, -900, i * 250)
        node.set_editor_property("scene_texture_id", scene_id)
        nodes[key] = node
    ink_code = """
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float2 px = View.ViewSizeAndInvSize.zw * Width;
float d = Depth.r;
float dEdge = 0.0;
float nEdge = 0.0;
float3 n = Norm.rgb;
const float2 o[4] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1) };
for (int i = 0; i < 4; i++)
{
    float di = SceneTextureLookup(uv + o[i] * px, 1, false).r;
    float3 ni = SceneTextureLookup(uv + o[i] * px, 8, false).rgb;
    dEdge += abs(di - d);
    nEdge += 1.0 - saturate(dot(n, ni));
}
float fade = saturate(1.0 - d / FadeDistance);
float edge = saturate(max(dEdge / (d * 0.06 + 1.0) - 0.4, nEdge * 1.4 - 0.5)) * fade * Opacity;
return lerp(Scene.rgb, Ink, edge);
"""
    ink = _custom(p, ink_code, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["Scene", "Depth", "Norm", "Ink", "Width", "Opacity", "FadeDistance"], -300, 0, "Ink line")
    mel.connect_material_expressions(nodes["Scene"], "Color", ink, "Scene")
    mel.connect_material_expressions(nodes["Depth"], "Color", ink, "Depth")
    mel.connect_material_expressions(nodes["Norm"], "Color", ink, "Norm")
    mel.connect_material_expressions(_vector(p, "Ink", lin("#2E2E30"), -700, 800), "", ink, "Ink")
    mel.connect_material_expressions(_scalar(p, "Width", 1.4, -700, 1000), "", ink, "Width")
    mel.connect_material_expressions(_scalar(p, "Opacity", 0.85, -700, 1100), "", ink, "Opacity")
    mel.connect_material_expressions(_scalar(p, "FadeDistance", 9000.0, -700, 1200), "", ink, "FadeDistance")
    mel.connect_material_property(ink, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(p)
    eal.save_asset(MAT + "/M_PP_InkLine")
    print("materials created")


# --------------------------------------------------------------------------------------
# Styles. Colours come from the Varnish palette (GDD section 9):
# ochre #D9A441, madder rose #C4574B, cream #F2E3C0, teal shadow #2E5E6E, violet shadow #4A3558.
# Each surface: (mask, tiling cm, dark, mid, light, accent, accent amount)
# --------------------------------------------------------------------------------------

STYLES = {
    # A. Gouache: flat, opaque shapes with hard edges between the three values. Bold and graphic.
    "Gouache": dict(
        sharp=0.025, bristle=0.25, bounce=0.10, tiling_scale=1.35, sky="T_Sky_Gouache", sky_brightness=2.6,
        surfaces={
            "Plaster": ("T_Stroke_Plaster", 320, "#C9925E", "#EBCB93", "#F7E8C4", "#D9A441", 0.55),
            "Roof": ("T_Stroke_Roof", 260, "#7A2B3A", "#C4574B", "#E07E5E", "#8C3558", 0.5),
            "Wood": ("T_Stroke_Wood", 240, "#3A2638", "#6B4F3A", "#9A6E44", "#4A3558", 0.5),
            "Stone": ("T_Stroke_Stone", 340, "#6E5E7E", "#948898", "#C9B9A0", "#2E5E6E", 0.5),
            "Ground": ("T_Stroke_Ground", 520, "#CC9A6C", "#E4C084", "#F6E2B0", "#D9A441", 0.3),
            "Metal": ("T_Stroke_Dab", 120, "#1F1C2E", "#34304A", "#5A5878", "#2E5E6E", 0.3),
            "Dark": ("T_Stroke_Dab", 160, "#1E1830", "#2A2240", "#3A3050", "#4A3558", 0.3),
            "PigmentOchre": ("T_Stroke_Dab", 60, "#B07A20", "#D9A441", "#F2C860", "#E8923A", 0.5),
            "PigmentGreen": ("T_Stroke_Dab", 60, "#3E5E46", "#6B8F71", "#93B592", "#2E5E6E", 0.4),
            "PigmentRed": ("T_Stroke_Dab", 60, "#6E1A1A", "#9E2B25", "#C4574B", "#4A3558", 0.4),
            "Foliage": ("T_Stroke_Dab", 200, "#2E5E6E", "#6B8F71", "#C9B45A", "#D9A441", 0.45),
            "Erasure": ("T_Stroke_Dab", 220, "#BFC3D6", "#F4F4F6", "#FFFFFF", "#FFFFFF", 0.0),
            "Canvas": ("T_Stroke_Plaster", 300, "#B8AA90", "#D8CBB0", "#E8DEC8", "#D8CBB0", 0.0),
        },
        sun=dict(colour="#FFE0BC", lux=9.0, pitch=-17.0), sky_light=dict(tint="#7480E0", intensity=2.8),
        fog=dict(colour="#E9A070", density=0.0030, falloff=0.25), glow=5.0,
        post=dict(ev=1.1, saturation=1.12, contrast=1.10, bloom=0.25, vignette=0.25, shadow_tint="#C8D0FF", ao=0.45),
        ink=None,
    ),
    # B. Glaze: thin warm layers over a light ground, soft transitions, light seems to come from inside the paint.
    "Glaze": dict(
        sharp=0.20, bristle=0.65, bounce=0.30, tiling_scale=0.8, sky="T_Sky_Glaze", sky_brightness=2.6,
        surfaces={
            "Plaster": ("T_Stroke_Plaster", 320, "#D3A070", "#F0D8A4", "#FBF0D2", "#E4B050", 0.6),
            "Roof": ("T_Stroke_Roof", 260, "#8A3A3A", "#CC6650", "#EE9C74", "#A04A5E", 0.45),
            "Wood": ("T_Stroke_Wood", 240, "#4A3238", "#7C5A40", "#B08454", "#6A4458", 0.4),
            "Stone": ("T_Stroke_Stone", 340, "#94849A", "#B4A6A4", "#DCCBAC", "#6E94A0", 0.35),
            "Ground": ("T_Stroke_Ground", 420, "#CFA070", "#E8C890", "#F8E6BC", "#DDA070", 0.25),
            "Metal": ("T_Stroke_Dab", 120, "#2A2638", "#443E56", "#6E6A84", "#4A7684", 0.3),
            "Dark": ("T_Stroke_Dab", 160, "#2A2038", "#3A2C48", "#4E3E5A", "#6A4458", 0.3),
            "PigmentOchre": ("T_Stroke_Dab", 60, "#B8842A", "#E0AE4C", "#F8D478", "#E8923A", 0.5),
            "PigmentGreen": ("T_Stroke_Dab", 60, "#4A6A52", "#78987C", "#A4C29E", "#4A7684", 0.4),
            "PigmentRed": ("T_Stroke_Dab", 60, "#7A2420", "#AE3A30", "#D46A56", "#6A4458", 0.4),
            "Foliage": ("T_Stroke_Dab", 200, "#4A6E6A", "#84A47C", "#DCCB7C", "#E4B050", 0.4),
            "Erasure": ("T_Stroke_Dab", 220, "#BFC3D6", "#F4F4F6", "#FFFFFF", "#FFFFFF", 0.0),
            "Canvas": ("T_Stroke_Plaster", 300, "#B8AA90", "#D8CBB0", "#E8DEC8", "#D8CBB0", 0.0),
        },
        sun=dict(colour="#FFDCAE", lux=8.0, pitch=-20.0), sky_light=dict(tint="#8E86E0", intensity=2.4),
        fog=dict(colour="#F2B880", density=0.0050, falloff=0.18), glow=6.5,
        post=dict(ev=1.25, saturation=1.05, contrast=0.98, bloom=0.6, vignette=0.35, shadow_tint="#E4D4F4", ao=0.35),
        ink=None,
    ),
    # C. Ink and wash: muted washes on a paper ground, colour saved for roofs, pigment and flame, and a drawn line.
    "Wash": dict(
        sharp=0.09, bristle=0.45, bounce=0.12, tiling_scale=1.0, sky="T_Sky_Wash", sky_brightness=2.4,
        surfaces={
            "Plaster": ("T_Stroke_Plaster", 320, "#B8A084", "#E0D2B4", "#F2EAD4", "#CDA860", 0.35),
            "Roof": ("T_Stroke_Roof", 260, "#74343A", "#B4564C", "#D48A70", "#7E3E58", 0.5),
            "Wood": ("T_Stroke_Wood", 240, "#3A3038", "#62524A", "#8E7A62", "#4A3E52", 0.3),
            "Stone": ("T_Stroke_Stone", 340, "#787486", "#9A989E", "#C8C2B2", "#4E6E78", 0.35),
            "Ground": ("T_Stroke_Ground", 520, "#9A8468", "#C8B490", "#E6D8B6", "#B07868", 0.2),
            "Metal": ("T_Stroke_Dab", 120, "#22222C", "#3A3A48", "#62647A", "#4E6E78", 0.3),
            "Dark": ("T_Stroke_Dab", 160, "#22202C", "#2E2C3A", "#403E4C", "#4A3E52", 0.3),
            "PigmentOchre": ("T_Stroke_Dab", 60, "#B07A20", "#D9A441", "#F2C860", "#E8923A", 0.5),
            "PigmentGreen": ("T_Stroke_Dab", 60, "#3E5E46", "#6B8F71", "#93B592", "#2E5E6E", 0.4),
            "PigmentRed": ("T_Stroke_Dab", 60, "#6E1A1A", "#9E2B25", "#C4574B", "#4A3558", 0.4),
            "Foliage": ("T_Stroke_Dab", 200, "#4E6260", "#84948A", "#C2C09A", "#B0A060", 0.3),
            "Erasure": ("T_Stroke_Dab", 220, "#BFC3D6", "#F4F4F6", "#FFFFFF", "#FFFFFF", 0.0),
            "Canvas": ("T_Stroke_Plaster", 300, "#B8AA90", "#D8CBB0", "#E8DEC8", "#D8CBB0", 0.0),
        },
        sun=dict(colour="#FFE2C0", lux=7.5, pitch=-22.0), sky_light=dict(tint="#9AA8D8", intensity=2.3),
        fog=dict(colour="#DCC4A8", density=0.0055, falloff=0.2), glow=5.0,
        post=dict(ev=1.35, saturation=0.92, contrast=1.04, bloom=0.3, vignette=0.2, shadow_tint="#D8DCF0", ao=0.4),
        ink=dict(colour="#2E2E30", width=1.3, opacity=0.8),
    ),
}

# Light comes from behind the viewer's left shoulder: it travels toward +X and +Y.
SUN_YAW = 37.0
SKY_YAW_OFFSET = 0.0     # calibrated so the painted sun sits where the light comes from

CAMERAS = {
    "Blotter": ((7.4, -6.6, 1.5), (9.0, 42.0)),
    "Hesper": ((10.2, -5.6, 1.35), (4.0, 129.0)),
    "Hero": ((7.6, -8.6, 1.7), (7.0, 124.0)),        # (position m), (pitch, yaw)
    "Sunward": ((6.0, 8.0, 2.0), (6.0, -128.0)),
}


def instance(style_name, surface, glow=False):
    style = STYLES[style_name]
    name = "MI_%s_%s" % (style_name, surface)
    path = "%s/%s/%s" % (MAT, style_name, name)
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = asset_tools.create_asset(name, "%s/%s" % (MAT, style_name), unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if glow:
        mel.set_material_instance_parent(mi, eal.load_asset(MAT + "/M_PainterlyGlow"))
        mel.set_material_instance_texture_parameter_value(mi, "StrokeMask", eal.load_asset(TEX + "/T_Stroke_Dab"))
        mel.set_material_instance_scalar_parameter_value(mi, "Tiling", 90.0)
        mel.set_material_instance_scalar_parameter_value(mi, "Intensity", style["glow"])
    else:
        mask, tiling, dark, mid, light, accent, accent_amount = style["surfaces"][surface]
        mel.set_material_instance_parent(mi, eal.load_asset(MAT + "/M_Painterly"))
        mel.set_material_instance_texture_parameter_value(mi, "StrokeMask", eal.load_asset("%s/%s" % (TEX, mask)))
        mel.set_material_instance_scalar_parameter_value(mi, "Tiling", tiling * style["tiling_scale"])
        for key, value in (("Dark", dark), ("Mid", mid), ("Light", light), ("Accent", accent)):
            mel.set_material_instance_vector_parameter_value(mi, key, lin(value))
        mel.set_material_instance_scalar_parameter_value(mi, "AccentAmount", accent_amount)
        mel.set_material_instance_scalar_parameter_value(mi, "Sharp", style["sharp"])
        mel.set_material_instance_scalar_parameter_value(mi, "Bristle", style["bristle"])
        mel.set_material_instance_scalar_parameter_value(mi, "Bounce", style["bounce"])
    mel.update_material_instance(mi)
    eal.save_asset(path)
    return mi


def v(p):
    return unreal.Vector(p[0] * 100.0, p[1] * 100.0, p[2] * 100.0)


def build_level(style_name):
    style = STYLES[style_name]
    path = "%s/L_LookTest_%s" % (MAPS, style_name)
    if eal.does_asset_exist(path):
        levels.load_level(path)
        for actor in actors.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings) and actor.get_class().get_name() != "Brush":
                actors.destroy_actor(actor)
    else:
        levels.new_level(path)

    mats = {s: instance(style_name, s) for s in style["surfaces"]}
    mats["Glow"] = instance(style_name, "Glow", glow=True)

    def block(name, centre, size, surface, yaw=0.0, pitch=0.0, roll=0.0, folder="Set", shadow=True):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, v(centre), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
        comp = actor.static_mesh_component
        comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
        comp.set_material(0, mats[surface])
        comp.set_cast_shadow(shadow)
        actor.set_actor_scale3d(unreal.Vector(*size))
        actor.set_actor_label(name)
        actor.set_folder_path(folder)
        return actor

    def prop(name, mesh_name, position, yaw=0.0, scale=1.0):
        mesh = eal.load_asset("%s/%s" % (MESH, mesh_name))
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, v(position), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            comp.set_material(index, mats.get(slot_name, mats["Stone"]))
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        actor.set_actor_label(name)
        actor.set_folder_path("Props")
        return actor

    # --- The corner of the Lampstand square ---
    block("Ground_Square", (0, 0, -0.5), (44, 40, 1), "Ground")
    block("Terrace_Wall", (0, 16.5, 2.0), (44, 5, 4), "Stone")                       # the retaining wall of the upper terrace
    block("Terrace_Top", (0, 30, 3.75), (44, 24, 0.5), "Ground")
    block("Terrace_Coping", (0, 13.85, 4.15), (44.4, 0.7, 0.3), "Plaster")
    block("Steps_Ramp", (15.5, 9.0, 1.75), (4.2, 11.0, 0.5), "Ground", roll=-21.0)   # the way up, at the right of frame
    block("Steps_Side", (13.2, 9.5, 1.2), (0.5, 9.5, 2.9), "Stone", roll=-3.0)
    block("Parapet_West", (-14.5, -2, 0.55), (0.7, 26, 1.1), "Stone", yaw=2.0)
    block("Parapet_Cap", (-14.5, -2, 1.15), (0.95, 26.2, 0.14), "Plaster", yaw=2.0)

    prop("House_Square", "SM_House_A", (-5.0, 8.2, 0.0), yaw=174.0)
    prop("House_UpperTerrace", "SM_House_B", (7.0, 22.0, 4.0), yaw=188.0)
    prop("House_UpperTerrace_Far", "SM_House_A", (-12.0, 25.0, 4.0), yaw=10.0, scale=0.95)
    prop("Lampstand", "SM_Lampstand", (4.6, 0.3, 0.0), yaw=-20.0, scale=1.15)
    prop("Tree", "SM_Tree", (9.6, 10.6, 0.0), yaw=40.0, scale=1.25)
    prop("Tree_Terrace", "SM_Tree", (-2.5, 27.0, 4.0), yaw=160.0, scale=1.0)
    prop("GrindingSlab", "SM_GrindingSlab", (0.4, -3.6, 0.0), yaw=150.0)

    # Style probe, first person: Hesper Vane beside her Lampstand, lighting pole in hand.
    prop("Hesper_StyleProbe", "SM_Hesper", (7.6, -2.4, 0.0), yaw=200.0, scale=1.0)

    # Style probe, first enemy: a Blotter at the edge of the square, and the bare canvas it has dabbed the ground back to.
    prop("Blotter_StyleProbe", "SM_Blotter", (11.5, -2.5, 0.0), yaw=215.0, scale=1.1)
    for i, (dx, dy, sx, sy, yaw) in enumerate(((0, 0, 3.4, 2.6, 20), (1.1, -0.9, 1.8, 1.4, -35), (-1.3, 0.8, 1.5, 1.1, 60))):
        block("BareCanvas_%d" % i, (11.5 + dx, -3.3 + dy, -0.003), (sx, sy, 0.05), "Canvas", yaw=yaw, folder="Set/Paving", shadow=False)

    # Worn paving: a ring round the Lampstand and a path to the door.
    for i in range(9):
        angle = i / 9.0 * 2.0 * math.pi
        block("Paving_%02d" % i, (4.6 + math.cos(angle) * 2.1, 0.3 + math.sin(angle) * 2.1, -0.005), (1.25, 0.95, 0.05), "Stone",
              yaw=math.degrees(angle) + 90.0, folder="Set/Paving", shadow=False)
    for i, (x, y, yaw) in enumerate(((2.1, 1.7, 30), (0.6, 2.9, 38), (-0.9, 3.9, 22), (-2.3, 4.7, 30), (-3.4, 5.1, 12))):
        block("Path_%02d" % i, (x, y, -0.005), (1.3, 0.9, 0.05), "Stone", yaw=yaw, folder="Set/Paving", shadow=False)

    # Set dressing from simple shapes: crates and a stack of flax bales by the wall.
    for i, (x, y, z, s, yaw) in enumerate(((7.4, 12.6, 0.45, 0.9, 12), (8.5, 12.9, 0.4, 0.8, -20), (7.9, 12.7, 1.2, 0.7, 30),
                                            (-10.6, 6.0, 0.4, 0.8, 8), (-11.6, 5.3, 0.35, 0.7, -14))):
        block("Crate_%02d" % i, (x, y, z), (s, s, s), "Wood", yaw=yaw, folder="Props/Dressing")
    block("Bench", (0.6, 11.4, 0.45), (2.4, 0.5, 0.12), "Wood", yaw=-4.0, folder="Props/Dressing")
    for i, x in enumerate((-0.4, 1.6)):
        block("Bench_Leg_%d" % i, (x, 11.4, 0.2), (0.14, 0.42, 0.4), "Wood", folder="Props/Dressing")

    # --- Sky: a painted panorama ---
    sky_mi_path = "%s/%s/MI_%s_Sky" % (MAT, style_name, style_name)
    sky_mi = eal.load_asset(sky_mi_path) if eal.does_asset_exist(sky_mi_path) else asset_tools.create_asset(
        "MI_%s_Sky" % style_name, "%s/%s" % (MAT, style_name), unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(sky_mi, eal.load_asset(MAT + "/M_PaintedSky"))
    mel.set_material_instance_texture_parameter_value(sky_mi, "Sky", eal.load_asset("%s/%s" % (TEX, style["sky"])))
    mel.set_material_instance_scalar_parameter_value(sky_mi, "Brightness", style["sky_brightness"])
    mel.update_material_instance(sky_mi)
    eal.save_asset(sky_mi_path)
    sky = actors.spawn_actor_from_class(unreal.StaticMeshActor, v((0, 0, -200)), unreal.Rotator(roll=0.0, pitch=0.0, yaw=SUN_YAW + SKY_YAW_OFFSET))
    sky.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    sky.static_mesh_component.set_material(0, sky_mi)
    sky.static_mesh_component.set_cast_shadow(False)
    sky.static_mesh_component.set_collision_profile_name("NoCollision")
    sky.set_actor_scale3d(unreal.Vector(30000, 30000, 30000))
    sky.set_actor_label("PaintedSky")
    sky.set_folder_path("Lighting")

    # --- Lighting rule: one shadow-casting sun, one tinted sky light, small lights without shadows ---
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, v((0, 0, 20)), unreal.Rotator(roll=0.0, pitch=style["sun"]["pitch"], yaw=SUN_YAW))
    sun.set_actor_label("Sun")
    sun.set_folder_path("Lighting")
    sc = sun.light_component
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_intensity(style["sun"]["lux"])
    sc.set_light_color(lin(style["sun"]["colour"]))
    sc.set_editor_property("dynamic_shadow_distance_movable_light", 12000.0)
    sc.set_editor_property("dynamic_shadow_cascades", 3)
    sc.set_editor_property("shadow_sharpen", 0.6)

    sky_light = actors.spawn_actor_from_class(unreal.SkyLight, v((0, 0, 30)), unreal.Rotator(0, 0, 0))
    sky_light.set_actor_label("SkyLight_ZoneTint")
    sky_light.set_folder_path("Lighting")
    kc = sky_light.light_component
    kc.set_mobility(unreal.ComponentMobility.MOVABLE)
    kc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
    kc.set_intensity(style["sky_light"]["intensity"])
    kc.set_light_color(lin(style["sky_light"]["tint"]))
    kc.set_editor_property("cast_shadows", False)
    kc.recapture_sky()

    def small_light(name, position, colour, intensity, radius):
        light = actors.spawn_actor_from_class(unreal.PointLight, v(position), unreal.Rotator(0, 0, 0))
        light.set_actor_label(name)
        light.set_folder_path("Lighting/Small")
        lc = light.light_component
        lc.set_mobility(unreal.ComponentMobility.MOVABLE)
        lc.set_intensity(intensity)
        lc.set_light_color(lin(colour))
        lc.set_attenuation_radius(radius * 100.0)
        lc.set_cast_shadows(False)                                  # GDD: small lights cast no shadows
        lc.set_editor_property("use_inverse_squared_falloff", False)
        lc.set_editor_property("light_falloff_exponent", 3.0)
    small_light("Lampstand_Light", (5.5, -0.1, 3.0), "#E8923A", 9.0, 9.0)
    small_light("Window_Glow", (-6.2, 4.6, 2.3), "#F2B45A", 3.0, 4.5)

    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, v((0, 0, 0)), unreal.Rotator(0, 0, 0))
    fog.set_folder_path("Lighting")
    fc = fog.component
    fc.set_fog_density(style["fog"]["density"])
    fc.set_fog_height_falloff(style["fog"]["falloff"])
    fc.set_fog_inscattering_color(lin(style["fog"]["colour"], 0.55))
    fc.set_editor_property("fog_cutoff_distance", 1200000.0)      # leave the painted sky clear of fog
    fc.set_editor_property("start_distance", 1500.0)
    fc.set_editor_property("fog_max_opacity", 0.85)

    # --- Post-process: exposure, a colour grade and tinted shadows. No full-screen paint filter. ---
    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, v((0, 0, 0)), unreal.Rotator(0, 0, 0))
    volume.set_actor_label("PostProcess_Zone")
    volume.set_folder_path("Lighting")
    volume.set_editor_property("unbound", True)
    post, pp = style["post"], volume.get_editor_property("settings")

    def setting(name, value):
        pp.set_editor_property("override_" + name, True)
        pp.set_editor_property(name, value)
    # Fixed exposure: a painting does not re-expose itself when you turn your head. Values are EV100.
    setting("auto_exposure_min_brightness", post["ev"])
    setting("auto_exposure_max_brightness", post["ev"])
    setting("auto_exposure_bias", 0.0)
    setting("bloom_intensity", post["bloom"])
    setting("vignette_intensity", post["vignette"])
    setting("color_saturation", unreal.Vector4(post["saturation"], post["saturation"], post["saturation"], 1.0))
    setting("color_contrast", unreal.Vector4(post["contrast"], post["contrast"], post["contrast"], 1.0))
    tint = lin(post["shadow_tint"])
    setting("color_gain_shadows", unreal.Vector4(tint.r ** (1 / 2.2), tint.g ** (1 / 2.2), tint.b ** (1 / 2.2), 1.0))
    setting("ambient_occlusion_intensity", post["ao"])
    setting("ambient_occlusion_radius", 60.0)
    setting("film_grain_intensity", 0.0)
    if style["ink"]:
        ink_path = "%s/%s/MI_%s_InkLine" % (MAT, style_name, style_name)
        ink_mi = eal.load_asset(ink_path) if eal.does_asset_exist(ink_path) else asset_tools.create_asset(
            "MI_%s_InkLine" % style_name, "%s/%s" % (MAT, style_name), unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(ink_mi, eal.load_asset(MAT + "/M_PP_InkLine"))
        mel.set_material_instance_vector_parameter_value(ink_mi, "Ink", lin(style["ink"]["colour"]))
        mel.set_material_instance_scalar_parameter_value(ink_mi, "Width", style["ink"]["width"])
        mel.set_material_instance_scalar_parameter_value(ink_mi, "Opacity", style["ink"]["opacity"])
        mel.update_material_instance(ink_mi)
        eal.save_asset(ink_path)
        pp.set_editor_property("weighted_blendables", unreal.WeightedBlendables(array=[unreal.WeightedBlendable(weight=1.0, object=ink_mi)]))
    volume.set_editor_property("settings", pp)

    for name, (position, (pitch, yaw)) in CAMERAS.items():
        camera = actors.spawn_actor_from_class(unreal.CameraActor, v(position), unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
        camera.set_actor_label("Camera_" + name)
        camera.set_folder_path("Cameras")

    start = actors.spawn_actor_from_class(unreal.PlayerStart, v((11.0, -13.0, 1.2)), unreal.Rotator(roll=0.0, pitch=0.0, yaw=125.0))
    start.set_folder_path("Gameplay")

    levels.save_current_level()
    levels.editor_set_game_view(True)      # hide editor icons so screenshots show only the picture
    print("built", path)


def main(args):
    command = args[0] if args else "all"
    if command in ("assets", "all"):
        import_assets()
        create_materials()
    if command == "import":          # meshes and textures only; leaves the materials (the style kit shares two of them)
        import_assets()
    if command == "materials":
        create_materials()
    if command == "view":            # view <CameraName>: put the editor viewport where that camera is (a still frame has no motion blur)
        position, (pitch, yaw) = CAMERAS[args[1]]
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(v(position), unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
        return
    if command == "gameview":
        levels.editor_set_game_view(True)
    if command == "level":
        build_level(args[1])
    if command == "all":
        for style in STYLES:
            build_level(style)
    eal.save_directory(ROOT)


if __name__ == "__main__":
    try:
        main(sys.argv[1:])
    except Exception:
        # The MCP tool only reports the last line of an error, so keep the full trace where it can be read.
        with open(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "looktest_error.txt"), "w") as handle:
            handle.write(traceback.format_exc())
        raise
