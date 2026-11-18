# Devlog 002: the walking skeleton

2026-11-18. Milestone **M2**. One thin loop, connected end to end, in the village. Everything is placeholder art on purpose.

## What you can do now

In `L_Matriarch_Village_A`, in one sitting:

1. **Talk** to Hesper Vane. Her lines come from `data/dialogue.json` and change with the quest; you can answer, or say nothing.
2. **Harvest** Hide Ochre from the four nodes on the top terrace. Fresh pigment is *wet*.
3. **Take** linseed oil from the flax press.
4. **Craft** Lamp Oil at Bartram's Grinding Slab.
5. **Relight** the village Lampstand with it.
6. **Rest** at the Lampstand. Your pigment dries, the nodes regrow, and the game saves.
7. **Attune**: spend 3 dry Ochre on *Warm Ground* in the palette. Your Glim bar grows from 100 to 120.
8. **Talk** to Hesper again and receive the lantern. The quest completes and teaches four recipes.
9. **Fight** the Blotter by the lower gate. It drops Ceruse.
10. **Save and load.** Quit and come back: everything is as you left it, including where you stood.

Dying costs nothing the first time, then half of your wet pigment; you wake where you last rested.

## Pictures

![Talking to Hesper](img/002-dialogue.png)
*Dialogue. Hesper's line after the quest.*

![The Grinding Slab](img/002-crafting.png)
*Crafting. The second recipe was taught by finishing the quest.*

![The palette at the Lampstand](img/002-palette.png)
*The palette. Warm Ground is owned; the rest say what they still need.*

## How it was checked

- **Automated tests** (6 pass): the first quest played through by rule, save and load, the death rule, stamina, attack timing, and the enemy design rules.
- **A scripted play-through in the running game** (`tools/dev/play.py quest1`): walks the player to each thing and presses the same "use" the E key does. It completed the quest, saved, changed something, loaded, and got the saved state back. A second script fought the Blotter: three hits staggered it, five killed it.
- That script found a real problem, since fixed: Bartram stood so close to his slab that "use" talked to him instead.

Nobody has played it by hand with the keyboard yet. That is the first thing to do in the morning.

## Test it (5 minutes)

1. Unreal → Content Drawer → Maps → Matriarch → **L_Matriarch_Village_A** → **Play**.
2. Keys: **WASD** move, **mouse** look, **E** use or talk (and continue), **1-9** choose, **Q** close a menu, **Tab** your pack, **left / right mouse** attack, **Left Ctrl** dodge, **Left Shift** sprint, **F5** save, **F9** load, **F8** new game.
3. Follow the quest text at the top right. The Ochre nodes are up the ramp on the top terrace; the flax press, the slab and the Lampstand are in the square; the Blotter is down the other ramp by the lower gate.

## Rough edges

- Menus are plain text panels with number keys. The real UI is milestone M9.
- Everything is a coloured shape. The Blotter is a white cylinder.
- The village Blotter is there only for this test; the design has no enemies in the village yet.
- "Use" picks the nearest thing, not the thing you are looking at.
- There is no Glim use yet and no healing item use (M4).
