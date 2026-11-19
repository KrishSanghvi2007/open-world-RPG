# Status

## PAUSED by owner on 2026-11-19

The owner has put Underpainting on hold. Nothing was deleted. Everything is committed and pushed to `main`.
No loop is running and the Mac is no longer kept awake. Unreal and Blender were closed.

**Where things stand.** Act 1 (the Sleeping Matriarch) is fully playable end to end, from the first quest to the
Turning, in a grey-box layout with the style kit's painted materials. The "make Act 1 look finished" pass had begun:

- Done: step 1, the art bible (`docs/ART_BIBLE.md`); step 2, the environment kit (116 scripted pieces, `tools/kit/`);
  step 3a, **Varnish dressed**, both A and B (devlog 016). The dressed village plays through by autopilot to the
  Furrow, 12 of 12 automated tests pass, and the data validates.
- Found and fixed on the way: the engine's default rendering settings held even the bare grey box to 47 frames a
  second at 1080p. Now 120 (`docs/devlog/016-varnish-dressed.md`, Frame rate).
- Not done: 3b the Scumblewood, 3c the Grisaille Vault, 4 interiors, 5 characters, 6 effects and interface, 7 audio,
  8 final polish, and the finish (gallery, showcase video, MORNING.md). The checklist is below, unchanged.
- One measurement is owed: the dressed village in the standalone game under the new settings (only the bare
  grey box was measured there before the pause).
- The owner has not yet playtested anything. `docs/devlog/MORNING.md` covers the game-feel pass and the characters;
  it does not yet mention the village's new look.

**How to resume.**
1. Open Claude Code in this folder and say what you want, for example "Resume Underpainting where you paused."
   Typing `/ready` first opens Unreal and Blender, connects them, and keeps the Mac awake while you are away.
2. Claude will: pull `main`; reopen Unreal; run `run_script("dev/unthrottle.py", [])` (the editor otherwise idles
   when it is not the front window); measure the dressed village with `tools/dev/bench.sh` (Unreal must be closed
   for that); then pick up the first unticked line below, **3b. The Scumblewood**, with
   `tools/kit/dress.py` (add a `dress_forest`, the way `dress_village` is written) and the same before/after
   viewpoints, devlog and frame-rate check.
3. To work autonomously again, ask for the loop: "Continue the finishing pass with /loop, usual rules."
4. To play the game as it is: open Unreal, open `Maps/Matriarch/L_Matriarch_Village_A`, press Play.

If a decision should change first, `docs/QUESTIONS.md` 69 to 83 lists every choice made in this pass, each with
the alternatives.

---

Claude keeps this file current. It says what's done, what's next, and what's waiting on the owner.

Last updated: 2026-11-19

## Waiting on the owner
- [ ] **Your first playtest of Act 1** (whenever you like; the art pass is running meanwhile and changes no layouts). Open `docs/devlog/MORNING.md`: how to start, what changed, the twelve things to judge, about 35 to 60 minutes of play.
- [ ] Optional, one time: to have phone control on in every session, type `/config` in Claude Code and switch on "Enable Remote Control for all sessions". (Claude leaves this switch to you because it opens remote access to this Mac.)

## Decisions waiting on you
None right now. The six GDD decisions are settled (see `docs/DECISIONS.md`).

## Done
- Claude Code installed on the Mac; Blender MCP connected and tested (telemetry off: "Allow Telemetry" unticked in the add-on, server registered at user scope with `DISABLE_TELEMETRY=true`).
- Unreal Engine 5.8.4 installed at `/Users/Shared/Epic Games/UE_5.8`, with full Xcode 27.2 and its Metal Toolchain (Unreal will not start without the Metal Toolchain: `xcodebuild -downloadComponent MetalToolchain`).
- Git LFS and the GitHub CLI installed; GitHub login done, so Claude can push to `main`.
- Unreal MCP server registered at user scope (`unreal-mcp`, http://127.0.0.1:8000/mcp).
- Design document (`docs/GDD.md`) and vertical-slice data (`data/`) drafted.
- Repo set up for hands-off work: `CLAUDE.md`, Unreal `.gitignore`, Git LFS rules, project settings, the `/ready` skill.
- Terminal shortcut `rpg` added to `~/.zshrc` (opens the project and resumes Claude). It works in new Terminal windows.
- **Unreal project created** (`Underpainting.uproject` at the repo root, C++, from the Third Person template with the Combat variant). MCP and AllToolsets plugins are on and the MCP server starts with the editor, so Claude connects as soon as the project is open. Verified: Claude can read and change the open level.
- **Data structs in C++** (`Source/Underpainting/Data/`): a row struct for each file in `data/`, and `UUnderpaintingDataSubsystem`, which loads the JSON when the game starts. The automated test `Underpainting.Data.LoadAll` passes.
- **Look test** (`/Game/LookTest/Maps/`, pictures in `docs/looktest/`): one corner of the village square in three painterly styles, built by `tools/looktest/`. Working choice: B, Glaze.
- **First combat** (`Source/Underpainting/Combat/`): stamina, sprint, dodge, light and heavy attacks, and a data-driven Blotter, in `/Game/Maps/Dev/L_Combat_Greybox`. Tests: `Underpainting.Combat.*`.
- **Story** written: `docs/STORY.md`.
- **The UI (M9)**: the satchel (`DrawInventory` and its four tab functions in `UnderpaintingHUD.cpp`), `Game/UnderpaintingSettings.*` (rebindable keys, text size, own save file), key capture in `AUnderpaintingPlayerController::InputKey`, continue from the saved zone, Health and Glim carried between zones. Devlog: `docs/devlog/010-the-satchel.md`.
- **The region shift (M8)**: the Turning sequence, `World.Turned`, "B" levels for all three zones (`build_matriarch_greybox.py B`), Stair of Spines, Nape Overlook, after-shift consequences, end card. `tools/dev/check_route.py` checks a route can be walked. Devlog: `docs/devlog/009-the-turning.md`.
- **Crafting and attunement depth (M7)**: all 10 recipes, saved gear (`Equip`, Tab menu), Braided Wick, Sanguine Draught (K), Blue Tincture (L), tier-2 attunement effects, F7 debug fill. Devlog: `docs/devlog/008-crafting-and-attunement.md`.
- **Quests and choices (M6)**: quest endings made at a place (`AUnderpaintingChoicePoint`), asked twice, setting permanent world flags; inspectables; things that appear or vanish by flag; dialogue for three more quests. Devlog: `docs/devlog/007-quests-and-choices.md`.
- **Enemies and the boss (M5)**: `Source/Underpainting/Combat/UnderpaintingBestiary.*` (Madderback, Pentiment, Old Palimpsest with two phases and the heartbeat), three-attacker limit, a test room per enemy in `/Game/Maps/Dev`. First enemy style probe done. Devlog: `docs/devlog/006-enemies-and-boss.md`.
- **Combat depth (M4)**: palette wells, six Strokes with Glim cost by light, the Contrast rule, lock-on, Moss Poultice, Lamp Oil, perfect dodge, attunement passives. Rules in `Source/Underpainting/Combat/UnderpaintingCombatRules.h`. Devlog: `docs/devlog/005-combat-depth.md`.
- **Story on paper** (parallel track): `docs/story/` has outlines of Act 2 (the Wader), Act 3 (the Studio) and the Finale, and `FORESHADOWING.md`. Hesper, Bartram, Corin and Ilka have new lines in `data/dialogue.json` that plant what those acts need.
- **Style kit** (parallel track): `tools/stylekit/stylekit.py` and `/Game/StyleKit`. One Glaze material that paints three values from one colour, and a look per zone, applied automatically to the grey-box and to the C++ placeholders. Devlog: `docs/devlog/004-style-kit.md`.
- **Exploration, first pass (M3)**: zone exits, gaze-based "Use", the Flare, sketched platforms, one-sided walls and caches, 18 lore notes from `data/lore.json`, named places, a compass. Code in `Source/Underpainting/Game/UnderpaintingExploration.*`. Devlog: `docs/devlog/003-exploration-first-pass.md`.
- **Walking skeleton (M2)** in `L_Matriarch_Village_A`: dialogue from `data/dialogue.json`, harvesting, crafting, the Lampstand (relight, rest, save), attunement, the first quest, a Blotter, save and load. Code in `Source/Underpainting/Game/`. Tests: `Underpainting.Game.*`. Scripted play: `tools/dev/play.py`. Devlog: `docs/devlog/002-walking-skeleton.md`.
- **Roadmap** set by the owner: `docs/ROADMAP.md`. Choices made while the owner is away are in `docs/QUESTIONS.md`.
- **Grey-box of the Sleeping Matriarch, "A" versions** (`/Game/Maps/Matriarch/`): `L_Matriarch_Village_A`, `L_Matriarch_Scumblewood_A`, `L_Matriarch_Vault_A`. Built by `tools/greybox/build_matriarch_greybox.py`. Each has its layout, a player start, lighting by the GDD rule, and labelled markers for every Lampstand, node, enemy, NPC, station and exit, with counts checked against `data/regions.json`. The Vault has 0.6x gravity. Opened and play-tested in the editor.

## Known gaps
- The grey-box routes have not been walked end to end.
- Nobody has walked the forest or the vault by hand; the scripted play teleports.
- Enemies have no models, animations or path-finding.
- Dying restarts the level.
- The three zones are separate levels, each with an A (before) and B (after the Turning) version.

## IN PROGRESS: make Act 1 look and feel finished (set by the owner 2026-11-19)
The owner's brief, in order. Tick each as it is done; the loop resumes from the first unticked line.
Ground rules: gameplay layouts are LOCKED (art goes on top; move nothing unless it is broken); 60 fps at 1080p on
the M2 Pro under the GDD lighting rule, measured after every zone; style B Glaze, wabi-sabi; free assets only.

- [x] 1. **Art bible:** `docs/ART_BIBLE.md`.
- [x] 2. **Environment kit:** 113 pieces made in Blender by script and painted in Unreal by zone palette (`tools/kit/README.md`). To look: open `Maps/Dev/L_Kit_Varnish`, `L_Kit_Scumblewood` or `L_Kit_Vault`. Each zone's ground and cliffs are shaped when that zone is dressed (step 3).
- [ ] 3. **Zones,** A then B, one at a time, with before/after shots in a devlog each, and a frame-rate check each:
  - [x] 3a. Varnish (square, terraces, the Hem, house exteriors): devlog 016. Owed: a standalone frame-rate measurement of the dressed level.
  - [ ] 3b. The Scumblewood
  - [ ] 3c. The Grisaille Vault
- [ ] 4. **Interiors:** Hesper's lampworks, Bartram's grinding house, Ilka's hide, Corin's hollow; added: the cistern house, the black door.
- [ ] 5. **Characters:** the Limner, Hesper, Bartram, Corin, Ilka; Blotter, Madderback, Pentiment, Old Palimpsest; Lacuna and the Hatcher in real 3D; then any other Folded the cast puts in Act 1 (Piecemeal; the Vigil waits for the owner).
- [ ] 6. **Effects and interface:** pigment, Glim, Flare, Strokes, Erasures, hits, harvesting, the Turning; painted HUD, satchel, dialogue, crafting, palette, journal, map; title screen and main menu.
- [ ] 7. **Audio:** free music per zone, boss and the Turning; ambience; footstep surfaces. All credited.
- [ ] 8. **Final polish:** a full scripted playthrough of A and B; fix collisions, floating props, seams, stuck spots, frame dips.
- [ ] **Finish:** devlog with a gallery, a 60 to 90 second showcase video, `MORNING.md`, stop the loop.

## Done, waiting on the owner: characters from the drawings (set 2026-11-19)
- [x] 1. Nine figures cast into the game: `docs/story/CAST_IN_GAME.md` (the Folded).
- [x] 2. Two pilots made end to end and placed: the Hatcher (d28) at the Bend, and Lacuna (d40) in the Hem with her side story. Test room `Maps/Dev/L_Combat_Hatcher`.
- [x] 3. Each beside its drawing, and what worked and what did not: devlog 015.
- [x] 4. The pipeline as a skill: `/make-character <id>`.
- Not done: Stipple (the small spider); the Vigil (waits for the owner's say on the drawing's nudity); the other six are on paper only.

## Done, waiting on the owner: "feel good and play well" before the playtest (set 2026-11-18)
Ten steps from the owner, then a devlog with a before/after fight clip, then stop and wait. Tick them off here so work can resume after any break.

**Part A, game feel**
- [x] 1. Animations: the engine pack's attacks, dash, hit reactions and deaths on the player; enemies gather, strike, flinch and fall by moving their placeholder (`AUnderpaintingEnemy::UpdateBody`).
- [x] 2. Hit feedback: hit-stop, camera shake, enemy flinch and knockback, paint splashes, floating damage, bar trails and refusals (`Game/UnderpaintingFeel.*`).
- [x] 3. Sound: 46 events in `data/sounds.json`; three CC0 packs from Kenney plus 22 sounds made by `tools/audio/make_sounds.py`. Sources in `docs/ASSETS.md`.
- [x] 4. Responsiveness: input buffer, cancel windows, combo chaining, aim assist, ledge grace, camera lag, over-the-shoulder lock-on.
- [x] 5. Clarity: every menu row can be clicked, ground marks show where enemy attacks land, a hit-direction indicator. (Mouse checked by scripted click only.)

**Part B, playable design**
- [x] 6. Onboarding: 22 one-line prompts in `data/hints.json`, each shown once when the thing it teaches first matters (`AUnderpaintingPlayerCharacter::TickHints`). Seen working through the village and into the forest.
- [x] 7. Wayfinding: a marker over the next goal (or the door toward it), brighter Lampstands, five caches at dead ends. Sightlines in the forest were left alone (level art).
- [x] 8. Pacing: 23 beats in `docs/PACING.md`; enemies moved so each beat is what it says.
- [x] 9. Difficulty and balance: measured by trials and by simulation; written up in `docs/BALANCE.md`.
- [x] 10. Robustness: wake where you rested or arrived, autosave, fall rescue, safe Lampstands and doors; the autopilot played the main path to "Quest complete" in nine pieces (11 min 26 s).

**Done:** devlog 014, the before/after clip (`docs/devlog/clips/014-fight-before-after.mp4`), `MORNING.md`. **The loop is stopped. Waiting for the owner's playtest.**

Tools made for this: the autopilot (`Game/UnderpaintingAutopilot.*`, `tools/dev/pilot.py`), the frame recorder and `tools/dev/make_clip.py`.

## Next (Claude does these; see `docs/ROADMAP.md`)
All nine slice milestones are built. No Acts 2-3 and no full art pass until the owner says so. What is left inside the slice, in order:
1. **Proving by script: done.** Routes traced, Madderback and Pentiment fought, two full runs of the act (green wall; red wall with Corin in Bone Black). `tools/dev/play.py` has `act1a`, `corin <n>`, `act1b`, `act1c <n>` to repeat them. (Done: all main walking routes in the six levels traced with `tools/dev/check_route.py <route>`; it found and fixed a pit ramp in the Vault that ended against a wall.)
2. **Leftovers:** signposts (M3); three small attunement effects (devlog 008); the Hem's own light and the flax press bonus from "A Fair Grind". ("A Fair Grind" itself is done: `grind <n>` in `tools/dev/play.py`.) (Done: trading through dialogue with `Give.`/`Get.` in a reply's SetFlags; enemies steer round obstacles in `AUnderpaintingEnemy::MoveTowards`.)
3. **The paper look** on the other menus, if the owner likes the satchel.
4. **Remaining style probes:** player, boss, interior. (Done: item icons, `tools/stylekit/paint_icons.py`, devlog 012; NPC, `build_hesper` in `tools/looktest/blender_build_props.py`, devlog 013. `build_looktest.py view <Camera>` parks the editor view for a clean capture.)
5. Act on the owner's playtest notes as soon as there are any: they outrank everything above.
