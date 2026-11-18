# Status

Claude keeps this file current. It says what's done, what's next, and what's waiting on the owner.

Last updated: 2026-11-18

## Waiting on the owner
- [ ] Restart Claude Code so `CLAUDE.md` and the Fable model load.

## Decisions waiting on you
None right now. The six GDD decisions are settled (see `docs/DECISIONS.md`).

## Done
- Claude Code installed on the Mac; Blender MCP connected and tested (telemetry off: "Allow Telemetry" unticked in the add-on, server registered at user scope with `DISABLE_TELEMETRY=true`).
- Unreal Engine 5.8.4 installed at `/Users/Shared/Epic Games/UE_5.8`. It includes the experimental "Unreal MCP" plugin (`Engine/Plugins/Experimental/ModelContextProtocol`), off by default; enable it per project under Edit → Plugins.
- Full Xcode installed and selected (Xcode 27.2, build 27B52; UE 5.8 accepts 15.2–27.9), license accepted.
- Git LFS and the GitHub CLI installed; `git lfs install` run; GitHub login done, so Claude can push to `main`.
- Unreal MCP server registered at user scope (`unreal-mcp`, http://127.0.0.1:8000/mcp). It connects only while an Unreal project with the plugin is open.
- Design document (`docs/GDD.md`) and vertical-slice data (`data/`) drafted.
- Repo set up for hands-off work: `CLAUDE.md`, Unreal `.gitignore`, Git LFS rules, project settings.

## Next (Claude does these)
1. Create the Unreal project **Underpainting** in this repo (Third Person template), and enable the MCP and AllToolsets plugins with server auto-start.
2. Add a C++ module and the DataTable structs for `data/*.json`.
3. Greybox the Sleeping Matriarch region: village, forest and ribcage dungeon.
