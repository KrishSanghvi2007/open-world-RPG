# Devlog 001: foundations, a look test, first combat, and the story

2026-11-18. Milestones **M0 Foundations** and **M1 Story**.

## What exists now

- **The project.** Unreal 5.8, C++, with Claude connected to the open editor.
- **Game data in code.** Every file in `data/` loads at game start; a strict check fails loudly if the data and the code disagree.
- **Grey-box of the Sleeping Matriarch.** Village, forest and vault, built by script, with a labelled placeholder for every Lampstand, node, enemy, character and exit.
- **A look test.** One corner of the village square in three painterly styles. Working choice: B, Glaze.
- **First combat.** Stamina, sprint, dodge with a safe window, light and heavy attacks, and one Blotter that telegraphs with a warm glow, can be staggered, and drains Glim with its lunge.
- **The story.** `docs/STORY.md`: the whole arc, the Limner, the cast, the Matriarch's secrets, the mysteries, four endings, and the first 24 pieces of written lore.

## Pictures

![The look test in three styles](../looktest/comparison.png)
*Look test. Top: Gouache. Middle: Glaze (the working choice). Bottom: Ink and Wash.*

![Village grey-box](img/001-village-greybox.png)
*Varnish in grey-box: three terraces, twelve houses.*

![Vault grey-box](img/001-vault-greybox.png)
*The Grisaille Vault: Rib Stair, Cartoon Gallery, the Heart.*

![Standing at the top of the Rib Stair](img/001-vault-play.png)
*In play, at the top of the Rib Stair.*

![The fight room](img/001-combat-room.png)
*The fight room with one Blotter.*

## Test it (5 minutes)

1. In Unreal's Content Drawer open **Maps → Dev → L_Combat_Greybox** and press **Play**.
2. Walk toward the tall figure. When it glows warm, press **Left Ctrl** to dodge.
3. Hit it while it recovers: **left mouse** light, **right mouse** heavy. Three light hits in a row stagger it.
4. Watch the three bars at the bottom left: Health, Stamina, Glim.

## Known rough edges

- The Blotter is a white cylinder. There are no attack animations; a thin outline shows where each swing reaches.
- Enemies walk straight at you (no path-finding yet).
- Dying restarts the level, because Lampstands do not work yet.
- The grey-box routes have not been walked end to end.

## Next

Milestone **M2, the walking skeleton**: one connected loop in the village (talk, harvest, craft, attune, fight, finish a quest, save and load).
