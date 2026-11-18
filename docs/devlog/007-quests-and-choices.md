# Devlog 007: quests, choices, and the Heart

2026-11-18. Milestone **M6**. Quests can now end in a choice that costs something, asks you twice, and changes the world for good.

## What is new

- **Choices happen at a place, not in a menu you open.** You walk up to the brood nest, Corin's bare back, or the blank on the Heart wall, and press Use.
- **Every choice asks twice.** First the list, with each cost and what it will do. Then one ending on its own, the line "This cannot be painted over", and Do it / Not yet.
- **The world remembers.** Each ending sets permanent marks in the save that the levels read.
- **Three quests are playable after the first one:**
  - *Cold Under the Ribs* (main): Hesper sends you down; Rib Gate, Vault, boss, the Heart wall.
  - *The Tick-Warden's Ledger*: read Ilka's chalk at three blood wells, then burn the nest or dress the wells.
  - *The Unseen Side*: Corin asks to be finished. Terre Verte, Bone Black, or left as he is.
- **The Heart scene.** While Old Palimpsest lives there is nothing in the hollow. When it dies, Odile's lantern and her last note are there.
- **Hesper's return is written**: hold out the lantern, and choose whether she keeps the flame or pours it into the village Lampstand.

![Odile's last note](img/007-odiles-note.png)
*In the hollow, after the boss. The note Hesper has waited eleven years for.*

![The Heart wall](img/007-heart-wall-choice.png)
*The blank on the Heart wall. Two endings are greyed out because this save has no Sanguine or Terre Verte. (The cut-off text in this picture is fixed.)*

![Asked again](img/007-confirm.png)
*The second ask, before anything is spent.*

## The milestone test: one quest, two endings

"The Tick-Warden's Ledger", played twice in the running game:

| | Burn the nest | Dress the wells |
|---|---|---|
| Cost | nothing | 3 Terre Verte (spent) |
| A blood well then gives | 1 Sanguine | 3 Sanguine |
| Madderbacks in the wood on return | 3 (was 6) | 6 |

The Heart scene was also played: no lantern before the boss dies, lantern and note after, the wall left bare for 4 Ceruse, and the quest moved on to the Turning.

10 automated tests pass. The new one plays the Ledger twice, Corin's quest once, and checks Hesper's return.

## Not done, or not checked

- **Corin's quest and Hesper's return have not been played in the running game**, only by the automated test.
- **Cold Under the Ribs stops at "the Turning"**, which is milestone M8. So Hesper's return cannot be reached in play yet.
- **Most consequences wait for M8**, because they are written as "after the shift": extra or fewer enemies, brighter wood, Corin moving to the village gate, Ilka trading. Working now: well yields, halved Madderbacks, Corin's hidden cache.
- **"A Fair Grind" (Bartram's whitewash) is not built.** The design marks it as a stretch quest.
- The Heart wall and Corin's back are flat panels that change colour. There is no painting animation.
- One crash while building this (a test of mine read a quest that had not started). Fixed.

## Test it (5 minutes)

1. Play the first quest in the village, then go down to the Scumblewood.
2. Talk to Ilka twice. Use each of the three dark red discs (the blood wells).
3. Walk to the tall pale panel in the Hollow Vertebra and press **E**. Pick an ending, then **1** to confirm.
4. Harvest a blood well and count what you get.
