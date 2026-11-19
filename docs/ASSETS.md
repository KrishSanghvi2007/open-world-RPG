# Third-party assets

**Rule (owner, 2026-11-18, permanent): free assets only, never anything paid.** Prefer CC0 or a similarly permissive licence. Every outside file used by the game is listed here with its source and licence.

Re-downloadable packs (Fab, Marketplace, Poly Haven) live in `Content/ThirdParty/`, which git ignores. List each one here so it can be restored on another machine.

| Pack | Source / URL | Licence | Installed to | Used for |
|---|---|---|---|---|
| Mannequins (Manny and Quinn, with animations) | Ships with Unreal Engine 5.8: `Templates/TemplateResources/High/Characters/Content`. Restore with `tools/restore_engine_content.sh` | Unreal Engine EULA | `Content/Characters/` (ignored by git; it must stay at this path because the template Blueprints point at it) | Placeholder player and enemy bodies until our own characters exist |
| Mannequin animations: three attacks, a charged attack, a dash, hit reactions, deaths, jump and land | The same engine pack as above (`Content/Characters/Mannequins/Anims`) | Unreal Engine EULA | `Content/Characters/` (ignored by git) | The player's swings, Strokes, dodge, flinches and death (`AUnderpaintingPlayerCharacter::LoadAnimations`) |
| Impact Sounds 1.0 | Kenney, https://kenney.nl/assets/impact-sounds | Creative Commons Zero (CC0) | `Content/ThirdParty/Audio/Kenney/ImpactSounds/` (ignored by git) | Hits, footsteps per zone, landings, stagger |
| RPG Audio | Kenney, https://kenney.nl/assets/rpg-audio | Creative Commons Zero (CC0) | `Content/ThirdParty/Audio/Kenney/RPGAudio/` (ignored by git) | Cloth, pages, harvest, pick-up, crafting, doors, enemy wind-up creaks, default footsteps |
| Interface Sounds 1.0 | Kenney, https://kenney.nl/assets/interface-sounds | Creative Commons Zero (CC0) | `Content/ThirdParty/Audio/Kenney/InterfaceSounds/` (ignored by git) | Menu open, close, select, move, refuse; the quest-choice bell |

## Characters by Krish & Hannah Sanghvi

The Folded (the Hatcher, Lacuna, and the seven others cast in `docs/story/CAST_IN_GAME.md`) are drawn from
Krish and Hannah Sanghvi's own folded-paper ("exquisite corpse") drawings, used here with both artists' approval.
The drawings live in their own repository, which this project only reads. Only the files a character is
actually built from are copied in, under `art_source/characters/<id>/source/`.

| What | From | Licence | Where | Used for |
|---|---|---|---|---|
| d28, the Spider: ink redraw, original and painted sheet | The owners' drawings repository (`drawings/redrawn/d28`, `originals`, `painted`) | The owners' own work | `art_source/characters/d28/source/` | The Hatcher |
| d40, the Faceless One: ink redraw, original and painted sheet | The same (`d40`) | The owners' own work | `art_source/characters/d40/source/` | Lacuna |
| The models, materials and movements made from them | `tools/characters/` (our scripts) | Ours, from the owners' drawings | `art_source/characters/<id>/`, `Content/Folded/<id>/` | The Hatcher and Lacuna in the game |
| Part boxes, pivots and movement notes for both | The same (`docs/CAST.md`, `tools/film/cast.json`) | The owners' own work | read in place, not copied | Rigging and animation |

Free tools used to turn a drawing into a model (nothing paid, no account):

| Tool | From | Licence or terms | Used for |
|---|---|---|---|
| Hunyuan3D-2.1, through its public demo | https://huggingface.co/spaces/tencent/Hunyuan3D-2.1 (anonymous, free quota) | Tencent Hunyuan 3D 2.1 Community License | Tried on both pilots and **not used in the game**: the two shapes it returned are kept in `art_source/characters/*/raw/hunyuan.glb` only as a record (devlog 015) |
| Blender 5.2 | blender.org | GPL (the program; what is made with it is ours) | Building, rigging, animating and exporting the characters |
| gradio_client, numpy, scipy, Pillow | PyPI | Apache 2.0, BSD, BSD, HPND | Talking to the demo; painting the front view |

## Made by us (no outside source)

| What | Made by | Where | Used for |
|---|---|---|---|
| 22 sounds: swings, the Flare, chimes, the heartbeat, the Turning's rumble, the air of each zone | `tools/audio/make_sounds.py` (noise and sine waves; needs numpy) | `art_source/audio/*.wav`, imported to `Content/Audio/Made/` (both in git) | Everything no free pack had |
| Item icons, brush textures, painted skies, props | `tools/stylekit/`, `tools/looktest/` | `art_source/`, `Content/StyleKit/`, `Content/LookTest/` | The Glaze look and the style probes |

### The environment kit (made 2026-11-19)

All 113 scenery pieces (village, forest, Vault) are made by script, with no outside models or textures:
`tools/kit/blender_varnish.py`, `blender_scumblewood.py`, `blender_vault.py` (shapes, in Blender) and
`tools/kit/unreal_kit.py` (paint, in Unreal). Sources are in `art_source/kit/`; the game's copies in `Content/Kit/`.
How to remake them: `tools/kit/README.md`.

## Restoring the sound packs on another machine

1. `python3 tools/audio/fetch_kenney.py` downloads the three Kenney packs into `Saved/ThirdParty/kenney/` (it stops if a page no longer says CC0).
2. In Unreal, run `tools/audio/import_audio.py` through the dev toolset. It imports exactly the sounds `data/sounds.json` names.

If the packs are missing the game still runs; it is just quieter, and the log says which sounds it could not find.

`data/sounds.json` is the list of every sound the game plays, with the source of each in its `Source` field.

