# Good morning

Last updated: 2026-11-18, evening. Claude keeps this page current while you are away.

## What is playable

- **A fight.** One Blotter in a grey-box room, with stamina, dodge, light and heavy attacks.
- **Three grey-box zones** you can walk around: village, forest, vault (the vault has its low gravity).
- **A look test** in three art styles (pictures in `docs/looktest/`).

Not yet playable: harvesting, crafting, attunement, dialogue, quests, saving. Those are milestone M2, in progress.

## Test it in 10 minutes

1. Open **Unreal**. If it is not open, type `/ready` to Claude.
2. **The fight (4 min).** Content Drawer → Maps → Dev → **L_Combat_Greybox** → Play.
   Keys: **WASD** move, **mouse** look, **Space** jump, **Left Shift** sprint, **Left Ctrl** dodge, **left mouse** light attack, **right mouse** heavy attack. Press **Esc** to stop.
   Dodge when the figure glows warm, then hit it while it recovers.
3. **The zones (4 min).** Maps → Matriarch → **L_Matriarch_Village_A**, then **L_Matriarch_Vault_A**. Jump in the vault to feel the low gravity.
4. **The look (2 min).** Maps under LookTest → **L_LookTest_Glaze** → Play and walk around the Lampstand.
5. **The story.** Read section 1 of `docs/STORY.md` (5 minutes).

## What is broken or rough

- A "Message Log" window may sit on top of the Unreal viewport after Claude runs tests. Close it with its ✕.
- Enemies have no models or animations and no path-finding.
- Dying restarts the level.
- The red **Diagnostics** button in Unreal's bottom bar is Unreal's own performance report (slow start-up while its caches warm up), not a project error. The one real start-up error it listed is fixed.

## Open questions (with what Claude chose)

See `docs/QUESTIONS.md`. The ones most worth your eye:

6. **The story's central truth:** the Limner is the god's own unfinished self-portrait.
7. **A seventh pigment, Lampblack,** in Act 3: suggested, not added.
