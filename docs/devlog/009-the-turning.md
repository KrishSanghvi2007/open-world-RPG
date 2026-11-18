# Devlog 009: the Turning

2026-11-18. Milestone **M8**. Act 1 now runs from the first conversation to an end card. Decide the Heart wall and the Matriarch turns over: all three places change, the old road shuts, and a new one rises.

## What is new

- **The Turning.** When you decide the Heart wall, the ground shakes, the picture goes dark, three lines come up, and you are standing in the same spot in a changed world. It takes 9 seconds and you cannot move during it.
- **Three changed levels.** Each zone has a "B" version that the game uses from then on.
  - **Vault:** warmer paper, light up from 0 to 0.25, the two drawn platforms nearest the Heart are now solid, and nothing hostile is left. The Heart wall keeps the colour you gave it.
  - **Forest:** the top of the Furrow is shut, with the First Well and its cache sealed inside. Five vertebrae rise from the Rib Gate into the **Stair of Spines**. Ilka has moved her camp onto the third. Light down from 0.5 to 0.4.
  - **Village:** the Furrow gap is a wall. A **lower gate** in the Hem is where the Stair arrives. The Hem is in shadow, its houses back to bare canvas, and two Blotters walk it. Light down from 1.0 to 0.85.
- **The Nape Overlook**, visible all game and now reachable: a cache (2 Slumber Blue, 3 Shed Hide) and the first sight of the next colossus on the horizon.
- **Your earlier choices show up here.**
  - Green Heart wall: the forest is brighter (0.55) and has two fewer Blotters.
  - Red Heart wall: two more Madderbacks.
  - Corin finished in Terre Verte waits at the foot of the Stair; in Bone Black he guards the village gate and the Hem has no Blotters; unfinished, he stays where he was.
  - A burned nest still halves the Madderbacks.
- **The end.** Hesper asks if you found her. Answer, and the end card comes up. You can keep walking after it.

![The Turning](img/009-turning-1.png)
*Mid-Turning. The lines come up one at a time.*

![The Vault, warm](img/009-vault-warm.png)
*The same spot a moment later. The Heart wall was finished in Terre Verte, and has stayed that way.*

![The Nape Overlook](img/009-nape-overlook.png)
*The Nape Overlook. The grey blocks on the horizon are the next colossus, wading. It is a shape and nothing more; Act 2 is not built.*

![The Hem after the Turning](img/009-village-after.png)
*The Hem in shadow: bare-canvas houses and Blotters where there were none.*

![End card](img/009-end-card.png)
*The end card. (In this picture the notices and the Talk prompt sit on top of it. That is fixed, but I have not looked at it again since.)*

## How it was checked

One scripted run of the whole act in the running game, with the Heart wall finished in Terre Verte:

- The Turning played and loaded the changed Vault with the player still at the wall. Light read 0.25.
- Changed forest: 4 Blotters instead of 6, light 0.55. Reaching the Stair moved the quest to its last step.
- The Nape Overlook cache opened.
- The village exit arrived at the lower gate. Two Blotters in the Hem, light 0.85.
- Hesper took the lantern, the quest completed with its rewards, and the end card showed.

The Stair of Spines was checked for walkability by tracing the ground every half metre from the Rib Gate to the Overlook: 166 m, steepest slope 25°, no gaps. The first build failed that check (a lip of up to 0.9 m where each ramp met its vertebra) and was fixed.

11 automated tests pass. The new one covers when she turns, that she turns once, and how the light moves.

**Not checked:**
- Nobody has *walked* the Stair. The script teleports; the ground trace says it can be walked.
- Only the green-wall ending was played. The red wall's extra Madderbacks, Corin's three positions and "pour the flame into the Lampstand" are built but unseen.
- Loading a save made before the Turning, after it.

## Rough edges

- The Turning has no picture of her eye opening, only words on black. No sound.
- The village Hem is not tilted 4° as the design says, and the houses change colour rather than flake.
- Your Glim goes back to 100 whenever a level loads, even if your maximum is higher.
- Ilka's "trades after the wells are dressed" and Corin's splinter trade are not built: there is no trading yet.
- Pentimenti (ghost outlines of what stood before) are a text label, not an effect.

## Test it (5 minutes, with help)

The honest 5-minute version needs a save near the end. Ask Claude for "a save at the Heart wall". Then: press **E** at the pale panel, choose, confirm, hold on, and walk home. Out of the Vault, up the pale ramp behind the Rib Gate lamp, along the spine to the gate at the top.
