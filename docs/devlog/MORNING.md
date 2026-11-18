# Before you play

Last updated: 2026-11-18, night. This is the page for your first playtest. The loop is stopped; nothing more will change until you say.

**The short version:** Act 1 now has animation, hit feedback, sound, a camera that behaves, prompts that teach
each rule when it first matters, a marker for the next goal, fair checkpoints, and numbers that were measured
by the game playing itself. That last part found faults that would have spoiled your playtest (the main one: the
Cartoon Gallery could not really be crossed) and they are fixed. **No person has played it yet. You are the first.**

Devlog: `docs/devlog/014-feel-and-play.md`. The 24-second before/after fight: `docs/devlog/clips/014-fight-before-after.mp4`.

## How to play it

1. Open **Unreal** (or type `/ready` to Claude). Click its window so it is in front.
2. Content Drawer → Maps → Matriarch → **L_Matriarch_Village_A** → Play. It starts a new game.
3. Turn the sound on. Follow the line at the top right and the small green marker. You should not need this page or the design document again; if you do, that is a finding.
4. Close the game whenever you like. It keeps what you have done and puts you back where you last rested or walked into a zone.

**Estimated play time:** 35 to 45 minutes straight through; about an hour with the three side stories and some
looking around. Every rule is taught in the first 20 minutes or so. (Measured: the game plays its own main path in
11 min 26 s, never lost and never reading a note. The rest is an estimate.)

If something goes wrong: **Tab → Settings → "I am stuck"** puts you back where you last woke. Dying is safe: you
wake at the last Lampstand or zone door, whole, and the first death costs nothing.

## What changed

**How it feels**
- The player has real animations (swings that chain, a dash, flinches, a death). Enemies are still plain shapes, but they wind up, strike, rock and fall.
- Hits land: a freeze, a camera kick, knockback, paint in the enemy's colour, a number.
- 46 sounds, all free. **I cannot hear them. Tell me what is wrong, too loud, or silly.**
- Early presses are remembered, a dodge can cut a swing short, the camera trails, and locking on puts it over your shoulder.
- Menus can be clicked. A mark on the ground shows where an enemy's attack will land. A red flag shows where a hit came from.

**How it plays**
- 22 one-line prompts teach each rule once, when it first matters. (Settings can turn them off.)
- A green marker hangs over the next goal, or over the door that leads to it. (Settings can turn it off.)
- You wake where you last rested **or arrived**, whole, with two poultices. Progress is saved as you go.
- A dodge now protects you from the whole blow. Before, long attacks hit you anyway.
- A Flare can be relit as it gutters (its last three seconds), and sketched platforms hold a moment after the light goes. Before, the gallery could not really be crossed.
- Nothing hostile stands within 15 metres of a Lampstand or a zone door.
- The first enemy is alone. Groups can be drawn apart and take turns. The gallery has no enemies on it. The pit under it is no longer a trap.
- The boss: less Health (600 → 480), about 15% lighter hits, and it only notices you inside its own room.
- Small caches of crafting materials at five dead ends, so nobody needs to find a hidden wall to craft at all.
- Hesper gives two Lamp Oil with the lantern, so nobody reaches the Rib Gate unable to light it.

The measurements and every number: `docs/BALANCE.md`. The act as a sequence, with times: `docs/PACING.md`.

## What to judge

These are the things only you can tell me. In rough order of how much they matter.

1. **Does a hit feel like a hit?** Yours on them, theirs on you. Too much freeze and shake, or too little?
2. **The sound.** Everything about it.
3. **Did you ever not know what to do next?** Where, exactly?
4. **Did any prompt come too late, too early, or tell you something you already knew?** Did you miss one you needed?
5. **The first fight** (one Blotter in the road). Did it teach you to dodge, or just hurt?
6. **The forest's last stretch** (the Descent to the Rib Gate, nine enemies between two Lampstands). Tense, or a slog?
7. **The Cartoon Gallery.** Did you understand you could relight the Flare as it flickers? Is the jump fair? How many falls?
8. **The boss.** How many tries? I aimed for two or three. If one, it is too easy now and I will put some back. If more than four, tell me what kept killing you.
9. **The camera when locked on**, especially against the boss, which fills the screen.
10. **Pigment.** At the end, was there something you wanted and could not afford? (There should be.) Was there ever nothing you could afford? (There should not.)
11. **The walk home after the Turning.** Does it earn its two quiet minutes?
12. **Speed.** Does it hold 60 frames a second for you? I measured about 55 in the editor with everything else running, and could not do better than that from here.

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
24a. *The icons* (devlog 012). Would you want these on the Pack page in place of the text list?
25. *Changing a key.* Change Dodge to something else. Did it work, and was it obvious how?

## What is rough or unproven

**Unproven**
- **Everything by hand.** All of it has been played only by script.
- **The sound.** Never heard.
- **The mouse.** One scripted click on one menu worked. Clicking through every menu has not been tried.
- **The main path in one sitting.** The game played it in nine pieces, with fixes between. Two enemies (one on the Rib Stair, one at the Rib Gate) were moved after the runs that passed them.
- **The boss.** One win in one try at the final numbers. Five losses before the fixes.
- **Endings.** Only the third choice at the Heart wall was replayed in this pass. The others were last run before it (devlog 011).
- Three prompts never came up in the runs: lock-on, low Glim, the second palette well.
- "I am stuck" has not been pressed. The fall rescue worked the once it was needed.
- A changed key working in play; a gamepad; the end card seen clean. (As before.)

**Rough**
- Enemies and people are plain shapes. The player is the engine's grey mannequin.
- The lock-on camera gets very close to the boss.
- Enemies feel their way round pillars; a U-shaped wall can still trap them.
- The Turning is words on black. Only the satchel has the paper look.
- "A Fair Grind": its effects on the Hem's light and the flax press are not built. Three small attunement effects are missing (devlog 008).
- Moving, looking and jumping cannot be rebound.

**The Mac**
- It was short of memory all night with Unreal and Blender both open (about 4.4 GB spilled to disk). **Close Blender and other big apps before you play**: it will run better for you than it did for me.
- A "Message Log" window may sit over the Unreal viewport. Close it with its ✕.

## Decisions I made that you may want to undo

All are in `docs/QUESTIONS.md` (40 to 58 are from this pass). The ones that change a rule:

- **45. A dodged blow is spent.** The alternative was to shorten every long attack instead.
- **46. Being hit knocks you out of your swing** for 0.3 to 0.5 seconds.
- **47. Groups take turns:** two attacking at most, never starting together. The design allowed three.
- **49. A Flare can be relit in its last three seconds.** The alternative is a longer Flare.
- **50. No enemies on the gallery's platforms.** The alternative keeps one on the fourth, which is more dramatic and much harder.
- **53. The boss is easier** than its first numbers.
- **42 and 43. You wake where you arrived, and progress saves itself.** The old rule was Lampstands only.
## Open questions (with what Claude chose)

From before this pass, still open:

- **6. The story's central truth:** the Limner is the god's own unfinished self-portrait.
- **23. You can leave the Vault without Odile's lantern.** Hesper then gets a plainer ending.
- **24. Crafted gear puts itself on.**
- **28. After the Turning you are standing exactly where you stood**, and walk out.
- **29. Closing the Furrow seals the First Well's cache for good** if you had not opened it.
- **31. The Stair of Spines has high walls**, so nobody falls into a pit with no way out.
- **32. The paper look** for menus.
- **7. A seventh pigment, Lampblack,** in Act 3: suggested, not added.
