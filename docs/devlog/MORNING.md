# Good morning

Last updated: 2026-11-18, late. Claude keeps this page current while you are away.

**The short version:** all nine milestones of the Act 1 slice are built. The act can be played from Hesper's first line to an end card. Almost all of it has been driven by script and automated test, and **almost none of it has been played with real hands**. That is the one thing I cannot do, and it is what today is for.

## What is playable

| | What | Devlog |
|---|---|---|
| M2 | The first loop: talk, harvest, craft, relight, rest, attune | 001–003 |
| M3 | Three connected zones, the Flare, sketched platforms, one-sided walls, 18 notes, a compass | 004 |
| M4 | Two palette wells, six Strokes, the Contrast rule, lock-on, the perfect dodge | 005 |
| M5 | Blotter, Madderback, Pentiment and the boss Old Palimpsest, each with a test room | 006 |
| M6 | Quests that end in a choice that asks twice and changes the world; the Heart scene | 007 |
| M7 | All 10 recipes, wearable gear, potions, all 12 attunement nodes | 008 |
| M8 | The Turning: all three places change, the Stair of Spines, the Nape Overlook, an end card | 009 |
| M9 | The satchel (pack, journal, notes, settings), keys you can change, continue | 010 |

Also: the **style kit** paints every placeholder in the Glaze look, and there are style probes for an environment corner, two props, an enemy (the Blotter) and a UI screen (the satchel).

## Test it in 10 minutes

1. Open **Unreal**. If it is not open, type `/ready` to Claude. Click on the Unreal window so it is in front (it runs very slowly otherwise).
2. Content Drawer → Maps → Matriarch → **L_Matriarch_Village_A** → Play.
3. **Keys.** WASD move, mouse look, Space jump, **E** use or talk, **1–9** choose, **Tab** satchel, **Q** close a menu or lock on, **left mouse** light attack, **right mouse** heavy (hold for a Stroke), **Left Ctrl** dodge, **Left Shift** sprint, **Z** swap well, **F** flare, **G** fix a platform, **H** poultice, **J** lamp oil, **K** draught, **L** tincture. **Esc** stops. You can change most of these on the satchel's Settings page.
4. **The first quest (5 min).** Follow the text at the top right. Talk to Hesper (the figure by the tall lamp post), go up the ramp for Ochre, take linseed from the brown drum, use the grey slab, pour the oil into the lamp post, rest there and press 1, talk to Hesper again.
5. **The satchel (1 min).** Press Tab. Left and Right turn the pages.
6. **A fight (2 min).** Stop. Maps → Dev → **L_Combat_Greybox** → Play. One Blotter.
7. **A build (2 min).** In that room press **F7** (fills your pack), then walk to the lamp post, rest, and attune two colours. Hold right mouse for a Stroke.

To see the end of the act without playing an hour: ask Claude for "a save at the Heart wall".

## Where your feel-feedback matters most

Only hands can judge these. The question to answer is in italics.

**Movement**
1. *Walking and sprinting in the village square.* Is the walk speed right for a place this size? Is sprint worth its Stamina?
2. *The ramps between terraces.* Part of the village, or obstacles?
3. *Jumping in the Vault.* Gravity is 0.6 there. Floaty-pleasant or floaty-annoying?

**Combat**
4. *The dodge.* 0.6 s long, safe for the first 0.3 s. Far enough? Safe when it should be?
5. *Reading the Blotter.* It glows warm before each hit. Is the warning long enough to react to, short enough to be tense?
6. *Your swings.* Light 0.75 s, heavy 1.3 s, and you can only shuffle while swinging. Weighty, or just slow? There are no animations, so judge the timing, not the look.
7. *Stamina.* Do you run out at interesting moments or annoying ones?
8. *Strokes.* Hold right mouse 0.28 s for the Stroke, tap for a heavy. Do you get the one you meant?
9. *Lock-on* (Q). Does the camera help or fight you?
10. *The Madderback* (`L_Combat_Madderback`). Can you bait its charge into a pillar on purpose? That is the whole enemy.
11. *The Pentiment* (`L_Combat_Pentiment`). When it fades, is there time to think "Flare!" and press F?
12. *The boss* (`L_Combat_Boss`). Does the moment it turns red read as "the rule just changed"?
13. *Two builds.* After F7, try two colours, then two others. Different ways to play, or the same fight in another colour?
14. *Heavy against light.* Make the Bone Scraper and compare it with the knife. Is slow fun?

**Exploration**
15. *"Use" follows your eyes.* Stand between Hesper and the Lampstand and turn. Does the prompt switch when you expect?
16. *The lesson wall* in the forest, past the second lamp post: a dark wall with a pale cross-hatched patch. Would you think to walk into it? The design says to drop the idea if testers miss it.
17. *The Flare* in the Vault's gallery. 8 seconds and 8 metres. Tense or stingy?
18. *Finding notes.* Did you find any without being told where?

**Story moments**
19. *The second ask.* At the brood nest, pick an ending. Does "This cannot be painted over" make you hesitate?
20. *The hollow.* After the boss: a lantern and a note, no cutscene. Does the note land?
21. *The Turning.* Nine seconds of shake, dark and three lines. Does it feel like something enormous moved?
22. *The walk home.* Is the changed forest "the same place, moved", or just a new corridor?
23. *The Nape Overlook.* You could see it all game. Is getting there a reward?

**The screens**
24. *The satchel.* Does paper and ink feel like this game? Should every menu look like it?
25. *Changing a key.* Change Dodge to something else. Did it work, and was it obvious how?

## What is broken, rough or unproven

**Unproven (built, tested by script, never by hand)**
- The whole game with a real keyboard and mouse.
- Whether a changed key actually works in play. The setting saves; I cannot press keys.
- Walking any route. The script teleports. The main routes of all three zones and the Stair of Spines have passed a ground trace (no gaps, nothing too steep). That trace found one real fault, now fixed: the ramp out of the Vault's gallery pit ended against a wall. The jumps across the gallery's sketched platforms cannot be traced and are untested.
- The Madderback and the Pentiment have never been fought.
- Only the green ending of the Heart wall was played through the Turning. The red wall, Corin's three endings and "pour the flame into the Lampstand" are built but unseen.
- A gamepad.

**Rough**
- Everyone and everything is a plain shape. No models, no animation, no sound.
- Enemies walk straight at you; pillars confuse them.
- The Turning is words on black.
- Only the satchel has the paper look. Dialogue, crafting, the palette and the fighting HUD are plain panels.
- No trading (Ilka and Corin should trade after their quests).
- "A Fair Grind", the stretch quest, is not built.
- Three small attunement effects are missing (devlog 008).
- Moving, looking and jumping cannot be rebound.

**The Mac**
- It runs short of memory with Unreal open (about 4.3 GB spilled to disk tonight, and the warning showed twice). The game kept running. Closing Blender and other large apps while you play should help.
- A "Message Log" window may sit over the Unreal viewport after Claude runs tests. Close it with its ✕.

## Open questions (with what Claude chose)

All 34 are in `docs/QUESTIONS.md` with the alternatives. The ones most worth your eye:

- **6. The story's central truth:** the Limner is the god's own unfinished self-portrait.
- **23. You can leave the Vault without Odile's lantern.** Hesper then gets a plainer ending.
- **24. Crafted gear puts itself on.**
- **28. After the Turning you are standing exactly where you stood**, and walk out.
- **29. Closing the Furrow seals the First Well's cache for good** if you had not opened it.
- **31. The Stair of Spines has high walls**, so nobody falls into a pit with no way out.
- **32. The paper look** for menus.
- **7. A seventh pigment, Lampblack,** in Act 3: suggested, not added.
