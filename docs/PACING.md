# Pacing: Act 1 as a sequence

Written 2026-11-18. The act is planned as beats that alternate: **calm, explore, fight, story, reward, rest**.
No new story, models or places were added to make this; what changed is where things stand and what the game
tells you, and when.

Times in the "autopilot" column are measured: the game playing itself along the main path at middling skill,
reading every line of talk at a person's reading speed, never exploring, never lost, never dying
(`tools/dev/golden_path.py`). A person takes two and a half to three and a half times as long on a first visit.

## The sequence

| # | Where | Beat | What happens | What it teaches | Autopilot clock |
|---|---|---|---|---|---|
| 1 | Varnish, the Lampstand square | calm | You wake by a dark Lampstand. Nothing can hurt you here. | moving, looking | 0:00 |
| 2 | | story | Hesper Vane. | Use; the journal | 0:33 |
| 3 | The Sunward Terraces | explore | Up the ramp for four Hide Ochre. *Small find: a cache at the far end of the terrace.* | sprint; harvesting; wet and dry pigment | 0:56 |
| 4 | The flax press, the Grinding Slab | explore | Linseed oil; make Lamp Oil. | crafting | 1:25 |
| 5 | The Lampstand | reward, rest | Pour the oil. The square lights up. Rest; attune Ochre. | Lampstands; resting; the palette; Strokes | 1:37 |
| 6 | | story, reward | Hesper again: the lantern, poultices, two Lamp Oil, the second quest. | | 2:30 |
| 7 | (optional) Bartram; the Hem | story, explore | "A Fair Grind". *Small find: a cache in the Hem's far corner.* | | 2:55 |
| 8 | The Furrow, top | fight | One Blotter, alone in the road, seen from the door. | Glim and the price of light; attack, dodge, lock on; the stagger bar; the colour rule | 3:15 |
| 9 | The First Well | story, fight | Ilka Thrum and her ledger. A second Blotter. Terre Verte. | one-sided walls (a hidden cache here) | 3:50 |
| 10 | The Bend | fight | The first Madderback, alone. *Small find: a cache in the corner.* | a charging enemy; pillars | 4:10 |
| 11 | The Sunward Clearing | rest, story | A lit Lampstand. Corin Half-Seen. The painted cliff you walk through. | one-sided walls, on the main path | 4:25 |
| 12 | The Descent | fight | Two Blotters together. *Small find: a cache in the corner.* | fighting a pair | 5:05 |
| 13 | The Hollow Vertebra | fight (the forest's peak), story | Three Madderbacks, standing apart. The brood nest and Ilka's choice. | drawing enemies one at a time | 5:40 |
| 14 | The Approach, the last slope | fight | Two Madderbacks; then two Blotters, one at a time. | spending poultices | 6:15 |
| 15 | The Rib Gate | reward, rest | A dark Lampstand: your Lamp Oil lights it. | | 6:35 |
| 16 | The Rib Stair | calm (dread) | Down into the dark. One Blotter on the stair, one Pentiment at its foot, each alone. | the Flare; a new enemy | 7:00 |
| 17 | The Cartoon Gallery | explore (skill) | Six sketched platforms over a pit, no enemies on them. Fall and you walk back up a ramp. | keeping a Flare alive; the long jump | 7:10 |
| 18 | The far ledge | rest | The last Lampstand. Empty and safe. | | 7:55 |
| 19 | The Heart | fight (the act's peak) | Old Palimpsest. | everything | 8:00 |
| 20 | | story, reward | Odile's lantern. The blank on the Heart wall. | | 8:45 |
| 21 | | story | The Turning. | | 9:00 |
| 22 | The way home | explore | Back across the gallery (two platforms have taken), the Stair of Spines, the new lower gate. The world has changed. | | 9:10 |
| 23 | Varnish | story | Hesper. The end card. | | 11:25 |

## Play time

| Stretch | Autopilot | A first-time player, estimated |
|---|---|---|
| Varnish: the first quest, meeting Bartram | 3:04 | 8 to 10 min |
| The Scumblewood, door to door | 3:35 | 10 to 14 min |
| The Vault to the last Lampstand | 1:12 | 5 to 8 min |
| The boss | 0:53 a try | 3 to 6 min (two or three tries) |
| The lantern, the choice, the Turning | 0:26 | 2 to 3 min |
| Home | 2:16 | 5 to 7 min |
| **The main path** | **11:26** | **33 to 48 min** |
| With the three side stories, the notes, the caches and some crafting | | **about an hour, to 75 min** |

Every rule the owner listed is taught by minute 7 of the autopilot's clock, which is about minute 20 for a person:
movement, Use, harvesting, crafting, Glim, the colour rule, attunement, Strokes, the Flare, one-sided walls.

The estimate is an estimate. The autopilot walks straight at things it already knows about, and the two halves of
the forest and the Vault were timed in separate runs while faults were being fixed between them.

## What was dull or unfair, and what was done

- **Before a fight had been learned, the forest opened with two enemies at once.** Now one, then one, then one of a new kind, then a pair.
- **Three fights in a row could arrive as one.** The Hollow Vertebra and the Approach are spread out.
- **The Vault piled its fights onto its jumping puzzle.** Now the stair is two duels, the gallery is only the gallery, the ledge is a rest, the Heart is the boss.
- **Dying sent you back to the last Lampstand you had rested at**, which could be a zone away. Now walking into a zone is a place to wake.
- **Dead ends had nothing in them.** Five small caches (materials, linseed oil) at the far corners of the terraces, the Hem, the Bend, the Descent and the pit.
- **The lit Lampstand is the landmark.** Lit ones now shine more than twice as far and breathe; dark ones stutter, which catches the eye. In the Vault you can see the last one glowing across the whole gallery. In the forest the turns of the Furrow still hide each clearing until you are nearly in it: opening those sightlines is level art, which this pass was told to leave alone, so there the objective marker does the leading.
- **The walk home after the Turning** is long (2 min at autopilot pace) and quiet. It is left alone on purpose: it is where you see what your choices changed. If it drags in the owner's hands, that is the first thing to cut.

## Not changed

- No new story, people, enemies, places or models.
- The three side stories are where they were.
- The long descent of the Furrow is one road by design. Its turns hide each next clearing until you are nearly in it, and that was kept.
