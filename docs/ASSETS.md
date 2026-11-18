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

## Made by us (no outside source)

| What | Made by | Where | Used for |
|---|---|---|---|
| 22 sounds: swings, the Flare, chimes, the heartbeat, the Turning's rumble, the air of each zone | `tools/audio/make_sounds.py` (noise and sine waves; needs numpy) | `art_source/audio/*.wav`, imported to `Content/Audio/Made/` (both in git) | Everything no free pack had |
| Item icons, brush textures, painted skies, props | `tools/stylekit/`, `tools/looktest/` | `art_source/`, `Content/StyleKit/`, `Content/LookTest/` | The Glaze look and the style probes |

## Restoring the sound packs on another machine

1. `python3 tools/audio/fetch_kenney.py` downloads the three Kenney packs into `Saved/ThirdParty/kenney/` (it stops if a page no longer says CC0).
2. In Unreal, run `tools/audio/import_audio.py` through the dev toolset. It imports exactly the sounds `data/sounds.json` names.

If the packs are missing the game still runs; it is just quieter, and the log says which sounds it could not find.

`data/sounds.json` is the list of every sound the game plays, with the source of each in its `Source` field.

