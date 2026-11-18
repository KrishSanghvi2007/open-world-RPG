# Status

Claude keeps this file current. It says what's done, what's next, and what's waiting on the owner.

Last updated: 2026-11-18

## Waiting on the owner
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
- **Grey-box of the Sleeping Matriarch, "A" versions** (`/Game/Maps/Matriarch/`): `L_Matriarch_Village_A`, `L_Matriarch_Scumblewood_A`, `L_Matriarch_Vault_A`. Built by `tools/greybox/build_matriarch_greybox.py`. Each has its layout, a player start, lighting by the GDD rule, and labelled markers for every Lampstand, node, enemy, NPC, station and exit, with counts checked against `data/regions.json`. The Vault has 0.6x gravity. Opened and play-tested in the editor.

## Known gaps in the grey-box
- Nobody has walked each route end to end yet: the layouts were checked from above and with one play start in the Vault.
- Markers are placeholders: no enemies, harvesting, Lampstands or zone exits work yet.
- The three zones are separate levels. The persistent level that streams them (`L_Matriarch_Persistent`) and the "B" versions come with the shift (build step 7).
- Passable backs are walls with no collision from either side; the real one-sided rule needs code.
- Sketch platforms are always solid for now; "solid only while lit" needs the Flare.
- Lighting is a first pass and reads too bright in the forest.

## Next (Claude does these)
Following the build order in `docs/GDD.md` section 11:
1. Build step 1: the player character in C++ (movement, Stamina, dodge, light and heavy attacks) and one Blotter, driven by `data/items.json` and `data/enemies.json`, playable in the village grey-box.
2. Turn markers into working actors: Lampstands (rest and respawn), harvest nodes, enemy spawners, zone exits.
3. Walk all three routes and fix what the grey-box gets wrong (jump distances in the Cartoon Gallery, sightlines in the Furrow).
