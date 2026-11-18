# Devlog 006: enemies and the boss

2026-11-18. Milestone **M5**. All four of the slice's enemies now fight, each teaching its one thing.

## What is new

| Enemy | Teaches | What it does |
|---|---|---|
| **Blotter** (already in) | Dodging | A poke, a sweep, a lunge that drains Glim. |
| **Madderback** | Positioning | Jabs and drinks 10 Health back. Charges 8 m in a straight line; if the charge is stopped short by something solid it is stuck for 2 seconds. Below 30% Health it swells and bursts: a white flash, which cannot be Erased. |
| **Pentiment** | Using light | Thrusts at spear's length, sweeps all round if you circle it, and fades to redraw itself up to 6 m away. Light a Flare while it fades and it is staggered instead. |
| **Old Palimpsest** (boss) | Everything | Five attacks. White at first (weak to Bone Black). At half Health it scrapes itself down over 2.5 seconds, wide open, and comes up red (weak to Terre Verte): the lesson reverses. Its grab drains 30 Glim. Her heart beats every 20 seconds and half-lights the room for 4, making Strokes cheaper on the beat. |

- **Never more than three attack at once.** The rest hold back.
- **Two telegraphs.** Warm glow: dodge it or Erase it. White flash: move.
- **A test room for each enemy** under Maps → Dev, with a Lampstand to rest at.
- The forest and the Vault now hold the real enemies in the numbers the design gives (6 Madderbacks and 6 Blotters; 6 Pentiments, 4 Blotters and the boss).

## Style probe: the first enemy

![The Blotter, style probe](img/006-style-probe-blotter.png)
*A Blotter at the edge of the village square, in the look-test scene. Wound rag, no face, bowed over its work, standing on the bare canvas it has dabbed the ground back to. The only white thing in the picture. About forty minutes' work; it goes no further than this for now.*

In play, enemies are still plain shapes. The probe shows where they are headed.

## How it was checked

- 8 automated tests pass. The enemy test now also checks the boss's two-phase data.
- In the running game, against the boss in its test room:
  - It opened with its long line attack from range, then swept and slammed up close.
  - An Impasto in its weak colour hit for 47.3 and staggered it.
  - Pushed below half Health it entered phase 2, and its weakness changed from Bone Black to Terre Verte. It went on attacking.

Two things were wrong and are fixed:
1. The Contrast bonus was being counted twice in stagger damage.
2. A Bone Black build could stagger the boss with every single hit. It now cannot be staggered again for 8 seconds after a stagger.

**Not checked in play:** the Madderback and the Pentiment have not been fought at all, by script or by hand. The boss's phase-2 attacks (the triple flail and the grab), the heartbeat's effect on cost, and the three-attacker limit have not been seen working. The whole fight from full Health to death has not been played.

## Test it (5 minutes)

Maps → Dev:
- **L_Combat_Madderback**: stand with a pillar behind you, step aside when it charges, and hit it while it is stuck.
- **L_Combat_Pentiment**: when it fades, press **F**.
- **L_Combat_Boss**: you will want a build first. Until M7 the quick way is to ask Claude to give your save one.

## Rough edges

- All enemies are coloured cylinders. The boss is a very large one.
- No path-finding: they walk straight at you, so pillars confuse them.
- The boss does not yet reveal what it is curled around. That scene (Odile, the lantern, the Heart wall) is milestone M6.
