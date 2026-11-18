# Good morning

Last updated: 2026-11-18, night. Claude keeps this page current while you are away.

## What is playable

- **The walking skeleton (M2):** one connected loop in the village. Talk to Hesper, harvest, craft, relight the Lampstand, rest, attune, finish the quest, fight a Blotter, save and load.
- **Exploration, first pass (M3):** the three zones connect through blue doorways; the Flare; sketched platforms; one-sided walls with caches behind them; 18 notes to find; a compass.
- **The style kit:** every grey-box level is now painted in the Glaze look (sky, haze, violet shadows, brush strokes) with no models.
- **A fight room** with one Blotter, and a **look test** in the chosen art style (Glaze).

## Test it in 10 minutes

1. Open **Unreal**. If it is not open, type `/ready` to Claude.
2. **The loop (6 min).** Content Drawer → Maps → Matriarch → **L_Matriarch_Village_A** → Play.
   Keys: **WASD** move, **mouse** look, **E** use or talk, **1-9** choose, **Q** close a menu, **Tab** pack, **left / right mouse** attack, **Left Ctrl** dodge, **Left Shift** sprint, **F** flare, **G** fix a sketched platform, **F5** save, **F9** load, **F8** new game. **Esc** stops.
   Follow the quest text at the top right: talk to Hesper (the teal figure by the tall lamp post), go up the ramp for Ochre, take linseed from the brown drum, use the grey slab, pour the oil into the lamp post, rest there and press 1, talk to Hesper again. Then go down the other ramp and fight the white figure.
   Press **F5**, stop, press Play again: you are where you were, with what you had.
   Then walk through the blue doorway at the north end of the square into the forest, and on to the Vault.
3. **The look (2 min).** Maps under LookTest → **L_LookTest_Glaze** → Play.
4. **The story (2 min).** Read section 1 of `docs/STORY.md`. It changed this evening on your advisor's note: Act 1 is now about Hesper's wife. Outlines for Acts 2 and 3 are in `docs/story/`.
   After the quest, talk to Hesper again and ask her things; talk to Bartram; in the forest, talk to Corin and Ilka.

## Where your feel-feedback matters most

These are the things only hands can judge. For each, the question to answer is in italics.

**Movement**
1. *Walking and sprinting in the village square.* Is the walk speed right for a place this size? Is sprint (Left Shift) worth its Stamina?
2. *The two ramps between terraces.* Do they feel like part of the village or like obstacles?
3. *Jumping in the Vault* (`L_Matriarch_Vault_A`). Gravity is 0.6 there. Is it floaty-pleasant or floaty-annoying?

**Combat** (the Blotter by the village's lower gate, or `L_Combat_Greybox`)
4. *The dodge* (Left Ctrl). It lasts 0.6 s and cannot be hit for the first 0.3 s. Does it go far enough? Does it feel safe when it should?
5. *Reading the Blotter.* It glows warm before each hit. Is the warning long enough to react to, and too long to be tense? Its three attacks should feel different: a poke, a sweep, a lunge.
6. *Your swings.* Light takes 0.75 s, heavy 1.3 s, and you can only shuffle while swinging. Is that "weighty and deliberate" or just slow? There are no animations yet, only a thin outline where the swing reaches, so judge the timing, not the look.
7. *Stamina.* 100, with 12 per light attack and 20 per dodge. Do you run out at interesting moments or annoying ones?
8. *Stagger.* Three light hits in a row stagger the Blotter for 2 seconds. Does that feel earned?

**Exploration**
9. *"Use" follows your eyes.* Stand between Hesper and the Lampstand and turn: does the prompt switch when you expect it to?
10. *The lesson wall* in the forest (`L_Matriarch_Scumblewood_A`, past the second lamp post). Coming down the gorge you meet a dark wall with a pale cross-hatched patch. Would you think to walk into it? This is the game's signature idea and the design says to drop it if testers miss it.
11. *The Flare* (F) in the Vault's gallery. 8 seconds and 8 metres: enough to cross two platforms, not three. Tense or stingy?
12. *Finding notes.* Odile's notes are cream slips; the Hand's only show after a Flare. Did you find any without being told where?
13. *The compass* at the top. Useful, or noise?

## What is broken or rough

- **The Mac is short of memory with Unreal open.** Unreal showed a memory warning during a test (about 4.5 GB had spilled to disk). The game still ran. Closing Blender and the Epic Games Launcher while playtesting should help; tell Claude if the game stutters.
- **Nobody has played the loop by hand yet.** Claude drove it from a script and it passed; real keys and a real mouse may still find problems.
- A "Message Log" window may sit on top of the Unreal viewport after Claude runs tests. Close it with its ✕.
- Menus are plain text with number keys.
- Nobody has walked the forest or the vault: the script teleports. Ramps, jumps and the lesson wall are untested by hand.
- Enemies walk straight at you and their health bars show through walls.
- Enemies have no models or animations and no path-finding.
- Dying restarts the level.
- The **Diagnostics** button in Unreal's bottom bar is no longer red. It was Unreal's own start-up report; the one real error it listed is fixed.

## Open questions (with what Claude chose)

See `docs/QUESTIONS.md`. The ones most worth your eye:

6. **The story's central truth:** the Limner is the god's own unfinished self-portrait.
10. **A Blotter in the village** for the test loop only.
12. **Saving:** on rest and on F5; one save.
7. **A seventh pigment, Lampblack,** in Act 3: suggested, not added.
