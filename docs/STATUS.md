# Status

Claude keeps this file current. It says what's done, what's next, and what's waiting on the owner.

## Waiting on the owner
- [ ] Finish installing Xcode (App Store), open it once, and accept the prompts.
- [ ] One-time GitHub login in the browser when the Mac Claude asks (so it can push for you).

## Decisions waiting on you (from the GDD, section 13; recommendation listed first)
1. Death cost: lose half of your unbanked ("wet") pigment, with no corpse-run? Or no loss / classic retrieval.
2. Keep the one-sided forest (only sun-facing sides are painted) as a signature idea, after a one-day test?
3. Are "finishing" choices permanent? Recommended: yes, with palette slots freely swappable.
4. Build 4 quests, with the 5th ("A Fair Grind") as a stretch goal?
5. Lighting: simple stylized lighting with Lumen off, for 60 fps on 16 GB? Or low Lumen and a lower frame-rate target.
6. Keep the title "Underpainting" for now?

## Done
- Claude Code installed on the Mac; Blender MCP connected and tested (telemetry off).
- Unreal Engine 5.8.4 installed; it includes the MCP plugin.
- Design document (`docs/GDD.md`) and vertical-slice data (`data/`) drafted.
- Repo set up for hands-off work: `CLAUDE.md`, Unreal `.gitignore`, Git LFS rules, project settings.

## Next (Claude does these)
1. Mac setup: install Git LFS and the GitHub CLI, log in once, and set git to push to `main`.
2. Register the Unreal MCP server (`unreal-mcp`, http://127.0.0.1:8000/mcp).
3. Create the Unreal project **Underpainting** in this repo (Third Person template, starting Blueprint-only; C++ gets added once Xcode is ready), and enable the MCP and AllToolsets plugins with server auto-start.
4. Once Xcode is ready, add a C++ module and the DataTable structs for `data/*.json`.
5. Greybox the Sleeping Matriarch region: village, forest and ribcage dungeon.
