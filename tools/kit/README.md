# The environment kit

Every piece of scenery in Act 1 is made by a script here, so any of it can be changed and remade. Nothing is
bought or downloaded.

| File | Runs in | What it does |
|---|---|---|
| `kitlib.py` | Blender | The toolbox: crooked blocks, tapered posts, sagging roofs, lumps, bent limbs, walls with holes. |
| `blender_varnish.py` | Blender | The village: houses, walls, parapets, ramp, stalls, props, trees (53 pieces). |
| `blender_scumblewood.py` | Blender | The forest: her vertebrae, trees, roots, rock, undergrowth, camps, the Rib Gate (38 pieces). |
| `blender_vault.py` | Blender | The Vault: ribs, paper, drawn lines, the heart, the studies (22 pieces). |
| `unreal_kit.py` | Unreal | Brings a zone's pieces in as `/Game/Kit/<Zone>/SM_*` and paints them. |
| `showroom.py` | Unreal | Lays a zone's kit out under its own light: `/Game/Maps/Dev/L_Kit_<Zone>`. |
| `blender_vista.py` | Blender | What lies round the village: her shoulder as ground, the far colossi, the Lamp. |
| `dress.py` | Unreal | **Dresses a zone:** stands the kit over the grey-box layout. Run by the level builder. |
| `shots.json` | | The fixed viewpoints used for before-and-after pictures. |

## To change a piece

1. Edit its function in `blender_<zone>.py`.
2. In Blender (through the Blender MCP), in two calls:
   `bpy.ops.wm.read_homefile(use_empty=True)` then
   `exec(open(R + "/tools/kit/kitlib.py").read()); exec(open(R + "/tools/kit/blender_<zone>.py").read())`
   It writes `art_source/kit/<Zone>/*.fbx` and `kit.json`.
3. In Unreal: `run_script("kit/unreal_kit.py", ["<Zone>"])`, then `run_script("kit/showroom.py", ["<Zone>"])` to look.

## How it is painted

Pieces carry no textures. Each part of a piece has a material slot with a plain name (`Plaster`, `Roof`, `Wood`,
`Stone`, `Foliage`, `Bone`, `Paper`, `Line`...). `unreal_kit.py` gives each name the Glaze material in that zone's
palette from `docs/ART_BIBLE.md`. To recolour a zone, change `PALETTES` there and run it again.

- Dark things (`Dark`, `Line`, `Metal`...) keep their light strokes dark; the standard Glaze would dab them with cream.
- Very large forms (`Cliff`) get broad brushwork, or the strokes read as dots.
- `Glow` is paint that shines by itself (windows, lanterns, embers). It lights nothing, so it costs nothing.

## Things to know

- A piece's front faces **+Y in Unreal** (it is -Y in Blender; the export mirrors it). To face the Lamp in the west, turn it 180.
- Most pieces have **no collision**. The grey-box shapes underneath are what the player touches, so dressing a
  zone cannot change where anyone can walk. Two exceptions, listed at the top of `unreal_kit.py`: furniture-sized
  props (stalls, barrels, benches, trees) are solid, because walking through them looks broken, and are only ever
  placed off the routes; and the houses with a way in, whose walls are solid.
- Foliage is solid shapes (blades, lumps), not see-through cards: the Glaze material is opaque, and it keeps the
  kit to a handful of materials.
- Budget (art bible, section 11): 300 to 1,500 triangles a piece. The largest is 1,068.

## Dressing a zone

`tools/greybox/build_matriarch_greybox.py` builds each level's layout and gameplay, then calls `dress.py`, which:

1. hides how the grey-box shapes look (they stay solid),
2. stands kit pieces over them (houses over house blocks, stone over terrace edges, and so on),
3. gives the things the player uses their finished look (a "skin" on the Lampstand, nodes, stations, notes, caches, doors),
4. adds the ground's detail, the props, the far view and the ambient life.

It never moves anything that is played. To see a level without its dressing, build it with `"grey"`:
`run_script("greybox/build_matriarch_greybox.py", ["village", "A", "grey"])`. To build one zone only, name it
(`village`, `forest`, `vault`) and the version (`A`, `B`).
