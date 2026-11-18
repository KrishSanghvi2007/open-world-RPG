# Status

Claude keeps this file current. It says what's done, what's next, and what's waiting on the owner.

Last updated: 2026-11-18

## Waiting on the owner
Nothing blocks work. Claude is working through `docs/ROADMAP.md` on its own; see `docs/devlog/MORNING.md`.
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

## Next (Claude does these; see `docs/ROADMAP.md`)
All nine slice milestones are built. No Acts 2-3 and no full art pass until the owner says so. What is left inside the slice, in order:
1. **Proving by script: done.** Routes traced, Madderback and Pentiment fought, two full runs of the act (green wall; red wall with Corin in Bone Black). `tools/dev/play.py` has `act1a`, `corin <n>`, `act1b`, `act1c <n>` to repeat them. (Done: all main walking routes in the six levels traced with `tools/dev/check_route.py <route>`; it found and fixed a pit ramp in the Vault that ended against a wall.)
2. **Leftovers:** signposts (M3); three small attunement effects (devlog 008); the Hem's own light and the flax press bonus from "A Fair Grind". ("A Fair Grind" itself is done: `grind <n>` in `tools/dev/play.py`.) (Done: trading through dialogue with `Give.`/`Get.` in a reply's SetFlags; enemies steer round obstacles in `AUnderpaintingEnemy::MoveTowards`.)
3. **The paper look** on the other menus, if the owner likes the satchel.
4. **Remaining style probes:** player, boss, interior. (Done: item icons, `tools/stylekit/paint_icons.py`, devlog 012; NPC, `build_hesper` in `tools/looktest/blender_build_props.py`, devlog 013. `build_looktest.py view <Camera>` parks the editor view for a clean capture.)
5. Act on the owner's playtest notes as soon as there are any: they outrank everything above.
