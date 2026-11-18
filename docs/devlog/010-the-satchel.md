# Devlog 010: the satchel, the journal and your own keys

2026-11-18. Milestone **M9**, the last of the Act 1 slice. Everything can now be reached without a debug key.

## What is new

- **The satchel (Tab).** One sheet of paper with four pages. Left and Right arrows turn the page.
  - **Pack:** pigment as dabs of its own colour with wet and dry counts, what you carry, and your gear. Press a gear number to put it on.
  - **Journal:** each quest in hand with the steps you have done and the one you are on. Finished quests show what you chose, in red, and what it did.
  - **Notes:** everything you have found to read, with counts for Odile's notes, the Hand's marginalia and the ledgers. Press a number to read one again.
  - **Settings:** text size (four steps), key hints on or off, **14 keys you can change**, put all keys back, and New game (asks twice).
- **Changing a key.** Go to its row with Up and Down, press Use, then press the key you want. If another action had that key, the two swap, so nothing is ever left without one. Your keys and text size are kept in their own file and survive a new game.
- **The hints follow your keys.** The line along the bottom, the Use prompt and the item counts all show whatever you have set.
- **Continue works.** Starting the game opens the place you last rested, whichever zone that was, in its changed form if the Matriarch has turned since.
- **Glim and Health carry between zones.** Walking through a doorway no longer tops you up or takes light away.
- **The game saves itself after the Turning.**
- **Notices move aside.** While a menu is open they tuck into the top left corner instead of sitting on it.

## Style probe: a UI screen

![The satchel, Pack page](img/010-satchel-pack.png)
*The Pack page. Paper, ruled lines, ink, and one loaded brush stroke under the open tab in that page's pigment. Worn gear is washed in lantern amber.*

![The Journal](img/010-satchel-journal.png)
*The Journal: in hand on the left, done on the right.*

![Settings, waiting for a key](img/010-satchel-settings.png)
*Settings, part way through changing the Dodge key.*

This is the probe for "a UI screen" and it stops here. The fighting HUD, dialogue, crafting and the palette are still the plain violet panels.

## How it was checked

- 12 automated tests pass. The new one checks that every action starts with its own key, that a free key is simply taken, that a key in use is swapped, and that reset restores the defaults.
- In the running game:
  - All three filled pages drew correctly with real content (a finished quest, a quest in hand, crafted gear).
  - Dodge was changed to C through the settings page, and was still C after Unreal was closed and reopened.
  - Glim stayed at 140 of 140 across a zone doorway (it used to drop to 100).
  - With a save made at the forest Lampstand, pressing Play in the village opened the forest, with gear, pigment and quest intact.

**Not checked:**
- **Whether C actually dodges.** The setting changes and is saved; I cannot press a real key from a script. This is the first thing to try by hand.
- The Notes page with more than nine notes (paging), and reading a note from it.
- New game from the settings page.
- Text sizes other than normal.
- A gamepad, at all.

## What you cannot change yet

Moving (W A S D), looking (mouse) and jumping (Space) are on the engine's own input system and are not in the list. The number keys, arrows and the debug keys (F5, F7, F8, F9) are kept for menus.

## Test it (5 minutes)

1. Play in the village. Press **Tab**. Use **Left** and **Right** to see all four pages.
2. On Settings, press **Down** to "Dodge", press **E**, then press the key you would rather have. Close with Tab and try it.
3. Finish the first quest, then open the Journal again.
