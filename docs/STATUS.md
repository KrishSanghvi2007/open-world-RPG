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
- The three zones are separate levels; the "B" versions come with the shift (M8).

## Next (Claude does these; see `docs/ROADMAP.md`)
Scope is the Act 1 slice only. In order:
1. Finish **M3**: navigation for enemies, signposts, the Nape Overlook secret, one style probe.
2. **M4, combat depth:** Glim costs, Strokes, the Contrast rule, lock-on, the Moss Poultice, Lampstand respawn.
3. Then M5 to M9.
