# Good morning

Last updated: 2026-11-18, night. Claude keeps this page current while you are away.

## What is playable

- **The walking skeleton (M2):** one connected loop in the village. Talk to Hesper, harvest, craft, relight the Lampstand, rest, attune, finish the quest, fight a Blotter, save and load.
- **A fight room** with one Blotter.
- **Three grey-box zones** to walk around: village, forest, vault (the vault has its low gravity).
- **A look test** in the chosen art style (Glaze).

## Test it in 10 minutes

1. Open **Unreal**. If it is not open, type `/ready` to Claude.
2. **The loop (6 min).** Content Drawer → Maps → Matriarch → **L_Matriarch_Village_A** → Play.
   Keys: **WASD** move, **mouse** look, **E** use or talk, **1-9** choose, **Q** close a menu, **Tab** pack, **left / right mouse** attack, **Left Ctrl** dodge, **Left Shift** sprint, **F5** save, **F9** load, **F8** new game. **Esc** stops.
   Follow the quest text at the top right: talk to Hesper (the teal figure by the tall lamp post), go up the ramp for Ochre, take linseed from the brown drum, use the grey slab, pour the oil into the lamp post, rest there and press 1, talk to Hesper again. Then go down the other ramp and fight the white figure.
   Press **F5**, stop, press Play again: you are where you were, with what you had.
3. **The look (2 min).** Maps under LookTest → **L_LookTest_Glaze** → Play.
4. **The story (2 min).** Read section 1 of `docs/STORY.md`. It changed this evening on your advisor's note: Act 1 is now about Hesper's wife.

## What is broken or rough

- **Nobody has played the loop by hand yet.** Claude drove it from a script and it passed; real keys and a real mouse may still find problems.
- A "Message Log" window may sit on top of the Unreal viewport after Claude runs tests. Close it with its ✕.
- Menus are plain text with number keys; "use" picks the nearest thing, not the one you look at.
- Enemies have no models or animations and no path-finding.
- Dying restarts the level.
- The **Diagnostics** button in Unreal's bottom bar is no longer red. It was Unreal's own start-up report; the one real error it listed is fixed.

## Open questions (with what Claude chose)

See `docs/QUESTIONS.md`. The ones most worth your eye:

6. **The story's central truth:** the Limner is the god's own unfinished self-portrait.
10. **A Blotter in the village** for the test loop only.
12. **Saving:** on rest and on F5; one save.
7. **A seventh pigment, Lampblack,** in Act 3: suggested, not added.
