"""The style kit: makes placeholder geometry look like the game (look-test style B, Glaze) without any models.

Used by the level builders (tools/greybox/*.py). Run on its own inside the editor to (re)create the assets:
    stylekit.py

What it provides, all under /Game/StyleKit:
    M_Glaze        one master material. Give it ONE colour and it paints three values of it (dark, middle,
                   light) in brush strokes, with tinted shadow and warm bounce. So a placeholder only ever
                   needs a colour. The game's C++ placeholders use it too.
    M_Sketch       the unlit paper-and-graphite version for sketch zones (the Vault).
    glaze(hex, surface) / sketch(hex)     material instances, made on demand and cached.
    apply_look(preset)                    sky, sun, tinted sky light, haze and post-process for a zone.

Lighting follows docs/GDD.md section 9: one shadow-casting sun, one tinted sky light, small lights without
shadows, a light post-process, no full-screen paint filter. Lumen and ray tracing stay off.
"""

import unreal

ROOT = "/Game/StyleKit"
MAT = ROOT + "/Materials"
TEX = "/Game/LookTest/Textures"          # the brush-stroke masks and painted skies made for the look test

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Which brush-stroke mask and stroke size (cm per repeat) each kind of surface gets.
SURFACES = {
    "Plaster": ("T_Stroke_Plaster", 260.0),
    "Ground": ("T_Stroke_Ground", 420.0),
    "Stone": ("T_Stroke_Stone", 340.0),
    "Wood": ("T_Stroke_Wood", 240.0),
    "Roof": ("T_Stroke_Roof", 260.0),
    "Dab": ("T_Stroke_Dab", 160.0),
}

TRIPLANAR = """
float3 w = pow(abs(N), 6.0);
w /= (w.x + w.y + w.z);
float3 p = P / max(Scale, 1.0);
float4 x = Texture2DSample(Tex, TexSampler, p.yz);
float4 y = Texture2DSample(Tex, TexSampler, p.xz);
float4 z = Texture2DSample(Tex, TexSampler, p.xy);
return x * w.x + y * w.y + z * w.z;
"""

# One colour in, three painted values out (GDD art rule 1). Shadows lean toward the zone's shadow tint and are
# never grey (rule 2); lights lean toward a warm ground showing through, which is what a glaze is.
# Purity 1 keeps the colour flat and exact: pure white is reserved for Erasures (rule 4).
GLAZE = """
float3 dark = lerp(Colour * 0.52, ShadowTint, 0.28);
float3 light = lerp(Colour, LightTint, 0.45) * 1.04;
float3 accent = lerp(Colour, AccentTint, 0.55);
float t1 = smoothstep(0.34 - Sharp, 0.34 + Sharp, S.r);
float t2 = smoothstep(0.68 - Sharp, 0.68 + Sharp, S.r);
float3 c = lerp(lerp(dark, Colour, t1), light, t2);
c = lerp(c, accent, saturate(S.g * AccentAmount));
c *= lerp(1.0, 0.82 + 0.36 * S.b, Bristle);
return lerp(c, Colour, Purity);
"""

BOUNCE = "return C * BounceColour * Bounce * saturate(0.75 - N.z * 0.75);"

SKETCH = "return Colour * (0.94 + 0.10 * S.r) * lerp(1.0, 0.90 + 0.2 * S.b, 0.5);"


def lin(hex_colour, scale=1.0):
    h = hex_colour.lstrip("#")
    r, g, b = (pow(int(h[i:i + 2], 16) / 255.0, 2.2) * scale for i in (0, 2, 4))
    return unreal.LinearColor(r, g, b, 1.0)


def _custom(material, code, output_type, inputs, x, y, description):
    node = mel.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code.strip())
    node.set_editor_property("output_type", output_type)
    node.set_editor_property("description", description)
    custom_inputs = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        custom_inputs.append(entry)
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


def _strokes(material, x, y):
    tex = mel.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, x, y)
    tex.set_editor_property("parameter_name", "StrokeMask")
    tex.set_editor_property("texture", eal.load_asset(TEX + "/T_Stroke_Plaster"))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    pos = mel.create_material_expression(material, unreal.MaterialExpressionWorldPosition, x, y + 200)
    normal = mel.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, x, y + 350)
    tri = _custom(material, TRIPLANAR, unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["Tex", "P", "N", "Scale"], x + 350, y, "Triplanar strokes")
    mel.connect_material_expressions(tex, "", tri, "Tex")
    mel.connect_material_expressions(pos, "", tri, "P")
    mel.connect_material_expressions(normal, "", tri, "N")
    mel.connect_material_expressions(_scalar(material, "Tiling", 260.0, x, y + 500), "", tri, "Scale")
    return tri, normal


def ensure_materials(rebuild=False):
    """Creates M_Glaze and M_Sketch if they do not exist (or always, with rebuild=True)."""
    if rebuild or not eal.does_asset_exist(MAT + "/M_Glaze"):
        if eal.does_asset_exist(MAT + "/M_Glaze"):
            eal.delete_asset(MAT + "/M_Glaze")
        m = asset_tools.create_asset("M_Glaze", MAT, unreal.Material, unreal.MaterialFactoryNew())
        tri, normal = _strokes(m, -1500, 0)
        ramp = _custom(m, GLAZE, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                       ["S", "Colour", "ShadowTint", "LightTint", "AccentTint", "Sharp", "AccentAmount", "Bristle", "Purity"], -500, 0, "Glaze: three values from one colour")
        mel.connect_material_expressions(tri, "", ramp, "S")
        vectors = {"Colour": lin("#D9B77A"), "ShadowTint": lin("#4A3558"), "LightTint": lin("#F8EBC8"), "AccentTint": lin("#D9A441")}
        for i, (name, value) in enumerate(vectors.items()):
            mel.connect_material_expressions(_vector(m, name, value, -950, 250 + i * 200), "", ramp, name)
        for i, (name, value) in enumerate((("Sharp", 0.20), ("AccentAmount", 0.35), ("Bristle", 0.6), ("Purity", 0.0))):
            mel.connect_material_expressions(_scalar(m, name, value, -950, 1100 + i * 110), "", ramp, name)
        mel.connect_material_property(ramp, "", unreal.MaterialProperty.MP_BASE_COLOR)

        bounce = _custom(m, BOUNCE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["C", "N", "BounceColour", "Bounce"], -100, 500, "Painted bounce light")
        mel.connect_material_expressions(ramp, "", bounce, "C")
        mel.connect_material_expressions(normal, "", bounce, "N")
        mel.connect_material_expressions(_vector(m, "BounceColour", lin("#E8923A"), -500, 700), "", bounce, "BounceColour")
        mel.connect_material_expressions(_scalar(m, "Bounce", 0.28, -500, 900), "", bounce, "Bounce")
        mel.connect_material_property(bounce, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        for prop, value, y in ((unreal.MaterialProperty.MP_ROUGHNESS, 1.0, 1100), (unreal.MaterialProperty.MP_SPECULAR, 0.0, 1200)):
            const = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -100, y)
            const.set_editor_property("r", value)
            mel.connect_material_property(const, "", prop)
        mel.recompile_material(m)
        eal.save_asset(MAT + "/M_Glaze")

    if rebuild or not eal.does_asset_exist(MAT + "/M_Sketch"):
        if eal.does_asset_exist(MAT + "/M_Sketch"):
            eal.delete_asset(MAT + "/M_Sketch")
        k = asset_tools.create_asset("M_Sketch", MAT, unreal.Material, unreal.MaterialFactoryNew())
        k.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        tri, _ = _strokes(k, -1200, 0)
        paper = _custom(k, SKETCH, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["S", "Colour"], -400, 0, "Paper with a little tooth")
        mel.connect_material_expressions(tri, "", paper, "S")
        mel.connect_material_expressions(_vector(k, "Colour", lin("#E6E1D6"), -800, 300), "", paper, "Colour")
        mel.connect_material_property(paper, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.recompile_material(k)
        eal.save_asset(MAT + "/M_Sketch")


_cache = {}


def _instance(name, parent, setup):
    if name in _cache:
        return _cache[name]
    path = "%s/Instances/%s" % (MAT, name)
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        ensure_materials()
        mi = asset_tools.create_asset(name, MAT + "/Instances", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, eal.load_asset(MAT + "/" + parent))
        setup(mi)
        mel.update_material_instance(mi)
        eal.save_asset(path)
    _cache[name] = mi
    return mi


def glaze(hex_colour, surface="Plaster", shadow_tint=None):
    """The painted version of a colour for one kind of surface."""
    mask, tiling = SURFACES.get(surface, SURFACES["Plaster"])
    tint = (shadow_tint or "#4A3558").lstrip("#").upper()
    name = "MI_Glaze_%s_%s_%s" % (hex_colour.lstrip("#").upper(), surface, tint)

    def setup(mi):
        mel.set_material_instance_vector_parameter_value(mi, "Colour", lin(hex_colour))
        mel.set_material_instance_vector_parameter_value(mi, "ShadowTint", lin("#" + tint))
        mel.set_material_instance_texture_parameter_value(mi, "StrokeMask", eal.load_asset("%s/%s" % (TEX, mask)))
        mel.set_material_instance_scalar_parameter_value(mi, "Tiling", tiling)
    return _instance(name, "M_Glaze", setup)


def sketch(hex_colour, surface="Plaster"):
    """The paper-and-graphite version of a colour, for sketch zones."""
    mask, tiling = SURFACES.get(surface, SURFACES["Plaster"])
    name = "MI_Sketch_%s_%s" % (hex_colour.lstrip("#").upper(), surface)

    def setup(mi):
        mel.set_material_instance_vector_parameter_value(mi, "Colour", lin(hex_colour))
        mel.set_material_instance_texture_parameter_value(mi, "StrokeMask", eal.load_asset("%s/%s" % (TEX, mask)))
        mel.set_material_instance_scalar_parameter_value(mi, "Tiling", tiling * 1.6)
    return _instance(name, "M_Sketch", setup)


# --------------------------------------------------------------------------------------
# Zone looks. Values for the village come straight from the look test's Glaze style.
# --------------------------------------------------------------------------------------

LOOKS = {
    # Painted, full dusk light. Warm sun low in the west, violet shadows, golden haze.
    "VillageDusk": dict(sky="T_Sky_Glaze", sky_brightness=2.6, sun=("#FFDCAE", 8.0, -20.0), sky_light=("#8E86E0", 2.4),
                        fog=("#F2B880", 0.0050, 0.18), ev=1.25, saturation=1.05, contrast=0.98, bloom=0.6, vignette=0.35,
                        shadow_tint="#E4D4F4", ao=0.35, ink=None),
    # Half-painted, half the light. Cooler and greener, the sun raking in from one side only.
    "ForestHalfLight": dict(sky="T_Sky_Glaze", sky_brightness=1.5, sun=("#FFD2A0", 5.0, -28.0), sky_light=("#7C8AC8", 1.9),
                            fog=("#A8B090", 0.0085, 0.16), ev=0.55, saturation=0.98, contrast=1.02, bloom=0.5, vignette=0.42,
                            shadow_tint="#D2DCEC", ao=0.4, ink=None),
    # Sketch, no light at all: flat paper, a drawn line round everything, the one blue.
    "VaultSketch": dict(sky=None, sky_brightness=0.0, sun=None, sky_light=None, fill=("#E6E1D6", 1.6),
                        fog=("#B4B0A8", 0.0040, 0.05), ev=0.0, saturation=0.9, contrast=1.04, bloom=0.2, vignette=0.5,
                        shadow_tint="#E0E0EA", ao=0.0, ink=("#2E2E30", 1.3, 0.85)),
    # After the Turning. The same three places with the light moved: 0.85, 0.4 and 0.25 (GDD section 8).
    "VillageAfter": dict(sky="T_Sky_Glaze", sky_brightness=2.1, sun=("#FFCF9C", 6.2, -14.0), sky_light=("#8478D8", 2.3),
                         fog=("#D89C84", 0.0060, 0.18), ev=1.05, saturation=1.0, contrast=1.0, bloom=0.6, vignette=0.4,
                         shadow_tint="#D8C8F0", ao=0.35, ink=None),
    "ForestAfter": dict(sky="T_Sky_Glaze", sky_brightness=1.25, sun=("#FFC690", 4.2, -22.0), sky_light=("#7480C4", 1.8),
                        fog=("#9CA890", 0.0095, 0.16), ev=0.42, saturation=0.96, contrast=1.03, bloom=0.5, vignette=0.45,
                        shadow_tint="#CCD6EA", ao=0.4, ink=None),
    # The Heart is warm: a first thin amber glaze over the grey paper.
    "VaultWarm": dict(sky=None, sky_brightness=0.0, sun=None, sky_light=None, fill=("#F4D6A4", 2.0),
                      fog=("#C8A470", 0.0050, 0.05), ev=0.0, saturation=1.0, contrast=1.04, bloom=0.35, vignette=0.45,
                      shadow_tint="#F0DCC0", ao=0.0, ink=("#3A2E26", 1.3, 0.85)),
}

SUN_YAW = 90.0      # the Lamp hangs in the west (-Y), so its light travels toward +Y


def apply_look(preset):
    """Adds the sky, lights, haze and post-process of a zone to the level being built."""
    look = LOOKS[preset]
    ensure_materials()

    if look["sky"]:
        sky_name = "MI_Sky_%s" % preset
        sky_path = "%s/Instances/%s" % (MAT, sky_name)
        sky_mi = eal.load_asset(sky_path) if eal.does_asset_exist(sky_path) else asset_tools.create_asset(
            sky_name, MAT + "/Instances", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(sky_mi, eal.load_asset("/Game/LookTest/Materials/M_PaintedSky"))
        mel.set_material_instance_texture_parameter_value(sky_mi, "Sky", eal.load_asset("%s/%s" % (TEX, look["sky"])))
        mel.set_material_instance_scalar_parameter_value(sky_mi, "Brightness", look["sky_brightness"])
        mel.update_material_instance(sky_mi)
        eal.save_asset(sky_path)
        sky = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -20000), unreal.Rotator(roll=0.0, pitch=0.0, yaw=SUN_YAW))
        comp = sky.static_mesh_component
        comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
        comp.set_material(0, sky_mi)
        comp.set_cast_shadow(False)
        comp.set_collision_profile_name("NoCollision")
        sky.set_actor_scale3d(unreal.Vector(30000, 30000, 30000))
        sky.set_actor_label("PaintedSky")
        sky.set_folder_path("Lighting")

    if look["sun"]:
        colour, lux, pitch = look["sun"]
        sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(roll=0.0, pitch=pitch, yaw=SUN_YAW))
        sun.set_actor_label("Sun")
        sun.set_folder_path("Lighting")
        sc = sun.light_component
        sc.set_mobility(unreal.ComponentMobility.MOVABLE)
        sc.set_intensity(lux)
        sc.set_light_color(lin(colour))
        sc.set_editor_property("dynamic_shadow_distance_movable_light", 14000.0)
        sc.set_editor_property("dynamic_shadow_cascades", 3)

    if look["sky_light"]:
        tint, intensity = look["sky_light"]
        sky_light = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 4000), unreal.Rotator(0, 0, 0))
        sky_light.set_actor_label("SkyLight_ZoneTint")
        sky_light.set_folder_path("Lighting")
        kc = sky_light.light_component
        kc.set_mobility(unreal.ComponentMobility.MOVABLE)
        kc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
        kc.set_intensity(intensity)
        kc.set_light_color(lin(tint))
        kc.set_editor_property("cast_shadows", False)
        kc.recapture_sky()

    if look.get("fill"):
        colour, lux = look["fill"]
        fill = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(roll=0.0, pitch=-60.0, yaw=20.0))
        fill.set_actor_label("Fill_NoShadow")
        fill.set_folder_path("Lighting")
        fc = fill.light_component
        fc.set_mobility(unreal.ComponentMobility.MOVABLE)
        fc.set_intensity(lux)
        fc.set_light_color(lin(colour))
        fc.set_cast_shadows(False)

    colour, density, falloff = look["fog"]
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    fog.set_folder_path("Lighting")
    gc = fog.component
    gc.set_fog_density(density)
    gc.set_fog_height_falloff(falloff)
    gc.set_fog_inscattering_color(lin(colour, 0.55))
    gc.set_editor_property("fog_cutoff_distance", 1200000.0)
    gc.set_editor_property("start_distance", 1500.0)
    gc.set_editor_property("fog_max_opacity", 0.85)

    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    volume.set_actor_label("PostProcess_Zone")
    volume.set_folder_path("Lighting")
    volume.set_editor_property("unbound", True)
    pp = volume.get_editor_property("settings")

    def setting(name, value):
        pp.set_editor_property("override_" + name, True)
        pp.set_editor_property(name, value)
    setting("auto_exposure_min_brightness", look["ev"])      # fixed exposure: a painting does not re-expose itself
    setting("auto_exposure_max_brightness", look["ev"])
    setting("auto_exposure_bias", 0.0)
    setting("motion_blur_amount", 0.0)                        # paint does not smear when you turn your head
    setting("bloom_intensity", look["bloom"])
    setting("vignette_intensity", look["vignette"])
    setting("color_saturation", unreal.Vector4(look["saturation"], look["saturation"], look["saturation"], 1.0))
    setting("color_contrast", unreal.Vector4(look["contrast"], look["contrast"], look["contrast"], 1.0))
    tint = lin(look["shadow_tint"])
    setting("color_gain_shadows", unreal.Vector4(tint.r ** (1 / 2.2), tint.g ** (1 / 2.2), tint.b ** (1 / 2.2), 1.0))
    setting("ambient_occlusion_intensity", look["ao"])
    setting("ambient_occlusion_radius", 60.0)
    if look["ink"]:
        ink_colour, width, opacity = look["ink"]
        ink_path = "%s/Instances/MI_Ink_%s" % (MAT, preset)
        ink_mi = eal.load_asset(ink_path) if eal.does_asset_exist(ink_path) else asset_tools.create_asset(
            "MI_Ink_%s" % preset, MAT + "/Instances", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(ink_mi, eal.load_asset("/Game/LookTest/Materials/M_PP_InkLine"))
        mel.set_material_instance_vector_parameter_value(ink_mi, "Ink", lin(ink_colour))
        mel.set_material_instance_scalar_parameter_value(ink_mi, "Width", width)
        mel.set_material_instance_scalar_parameter_value(ink_mi, "Opacity", opacity)
        mel.update_material_instance(ink_mi)
        eal.save_asset(ink_path)
        pp.set_editor_property("weighted_blendables", unreal.WeightedBlendables(array=[unreal.WeightedBlendable(weight=1.0, object=ink_mi)]))
    volume.set_editor_property("settings", pp)


if __name__ == "__main__":
    ensure_materials(rebuild=True)
    eal.save_directory(ROOT)
    print("style kit materials rebuilt")
