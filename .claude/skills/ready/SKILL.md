---
name: ready
description: Get the Mac ready for a work session. Checks and opens everything Underpainting needs (git sync, Unreal + MCP, Blender + MCP, keep-awake) and fixes what it can. Use when the owner types /ready, "get ready", "check everything", or "I'm leaving, keep working".
---

# /ready: prepare the Mac for a work session

The owner is non-technical. Do every check and fix yourself. Only ask them for a physical click when there is no other way, and then give **one** short instruction. Finish with a short checklist report.

Run these steps in order, and fix problems as you go:

1. **Git sync.** Run `git pull --ff-only`, then merge any unmerged `origin/claude/*` branches (see CLAUDE.md). Report "up to date" or what came in.

2. **Keep the Mac awake.** If no `caffeinate` process is running, start one for 12 hours: `nohup caffeinate -dimsu -t 43200 >/dev/null 2>&1 &`. Check `pmset -g batt`. If the Mac is on battery, tell the owner to plug it in.

3. **Unreal.**
   - Find the `.uproject` in the repo (`git ls-files '*.uproject'`, or search the repo).
   - If port 8000 isn't listening (`lsof -iTCP:8000 -sTCP:LISTEN`), open the project: `open -a "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app" --args "<abs path to .uproject>"`. Then poll port 8000 every 5 s for up to 10 minutes, running in the background so you can keep checking the other steps.
   - When it's listening, confirm it with a read-only `unreal-mcp` call (for example, list the actors in the current level).
   - If the port never opens: check whether Unreal is running (`pgrep -f UnrealEditor`). Then tell the owner the one likely fix, such as clicking "Allow" on the network popup or "Yes" to rebuild modules.

4. **Blender** (only if the owner wants art work this session; if unclear, check it but don't launch it).
   - If port 9876 is listening, confirm with a read-only `blender` call (scene info).
   - If Blender isn't running and art work is wanted, run `open -a Blender`. The add-on still needs one click, so tell the owner: "In Blender, press N over the 3D view, open the BlenderMCP tab, and click Connect to Claude."

5. **Claude tools.** Run `claude mcp list` and confirm `unreal-mcp` and `blender` are registered. Re-register any that are missing, using the commands in `docs/STATUS.md`.

6. **Phone control.** Remind the owner in one line: "To control this from your phone, type /remote-control now." (You can't run it for them.)

7. **Report** as a checklist, with nothing else around it:
   ```
   ✅ Git: up to date
   ✅ Mac: will stay awake 12 h (plugged in)
   ✅ Unreal: open, connected
   ⏸ Blender: not needed today
   👉 Your one action: type /remote-control
   ```
   Then say what you plan to work on next (from `docs/STATUS.md`), and start once the owner says go.
