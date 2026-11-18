# Devlog 003: exploration, first pass

2026-11-18. First half of milestone **M3**. The three zones are now one connected place, and each has something to find.

## What is new

- **The zones connect.** The blue doorways take you village → forest → vault and back. You arrive at the matching doorway with everything you carried.
- **"Use" follows your eyes.** It picks what the character is facing (by where the camera points), not the nearest thing. Stand by the slab with your back to it and there is no prompt.
- **The Flare** (F): 20 Glim for 8 seconds of light in an 8 m circle. It gutters in its last second.
- **Sketched platforms** in the Cartoon Gallery are only drawings until a Flare lights them. A Fixative Flask (G) makes one solid for good.
- **The one-sided forest.** Six fronts with nothing behind them. From the painted side they are walls; from behind, where the pale cross-hatched patch is, you walk through. The main route teaches it once by the Sunward Clearing; the other five hide caches.
- **Things to read.** All 18 pieces of Act 1 lore are placed: Odile's seven notes along her route, three village ledgers, and the Hand's eight margin notes, which cannot be seen until a Flare has been lit beside them.
- **Named places** announce themselves the first time you walk in, and count toward quests.
- **A compass** at the top: the Lamp in the west as the fixed bearing, and a marker for the current quest step when it is in this zone.
- **The pack** (Tab) now counts what you have found.
- **Death** returns you to where you last rested, even in another zone.

## Pictures

![Reading the Hand's margin note](img/003-reading.png)
*A margin note in the Vault, readable after a Flare.*

![The Flare lit in the Cartoon Gallery](img/003-flare.png)
*In the Vault with the compass up and a note in reach.*

## How it was checked

- 7 automated tests pass (a new one covers lore counting, caches, fixing platforms and places).
- A script played it in the running game: finished the first quest with the new "Use", stood by the slab facing away (no prompt), read a ledger, walked out to the forest, read Odile's note at the First Well, opened a cache, stood on each side of the lesson wall (shut from the front, open from behind), went into the Vault, lit a Flare (Glim 100 → 80, one platform turned solid, the margin note appeared) and read it.
- The first version of "Use" was wrong and the script caught it: measured from the camera, it preferred things between the camera and the character.

**Not checked:** nobody has *walked* any of it. The script teleports. Whether the ramps are climbable, the Gallery jumps are makeable, and the lesson wall reads as a lesson are questions for hands and eyes.

## Still to do in M3

Navigation for enemies, signposts, the Nape Overlook's secret, and one style probe.
