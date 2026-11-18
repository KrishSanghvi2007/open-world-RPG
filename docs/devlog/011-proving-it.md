# Devlog 011: proving it

2026-11-18, late. Not a milestone. After the ninth milestone I stopped building and spent the evening trying to break what was there, by script, since I cannot play it by hand. This is what that found.

## What was checked

| Check | Result |
|---|---|
| Ground traced every half metre along the main routes of all six levels | All continuous, nothing too steep. Forest 298 m, Stair of Spines 166 m. |
| Madderback fought in its room | Jab (17.1, heals it 10), charge (sticks in a pillar), burst (28.5): all seen. |
| Pentiment fought in its room | Redraw (26.6), sweep (22.8), Flare counter: all seen. Thrust: only after a fix. |
| A second full run of the act | Red Heart wall, Corin in Bone Black: 8 Madderbacks, Corin at the village gate, no Blotters in the Hem. |
| Corin's quest | Played start to finish. |
| "A Fair Grind" | Built, then played start to finish. |

## What it found (all fixed)

1. **The ramp out of the Vault's gallery pit ended against a wall.** Fall in, and the way back up was blocked at the top.
2. **A lip of up to 0.9 m on every ramp of the Stair of Spines.** Too high to step up; the new road home would have needed a jump at every vertebra.
3. **The Pentiment never thrust.** It walked in too close and always swept. It now holds at spear's length.
4. **One Flare locked the Pentiment down for eight seconds** (caught four times running). Now once per Flare.
5. **Ending a conversation was counted twice.** A quest with two "talk to them" steps in a row skipped one.
6. **Odile's lantern stayed in your pack** after you gave it to Hesper.
7. **Name labels floated over people and caches that were not there**, giving away Corin's hidden cache.

None of these would have shown in the automated tests. Five of the seven only showed when the thing was actually run.

## Also added

- **Trading**, as a reply in conversation: Ilka (1 Sanguine for 2 Terre Verte) and Corin (1 Rib Splinter for 1 Bone Black).
- **Enemies go round pillars** instead of pushing into them. Not real path-finding.
- **"A Fair Grind"**, the stretch quest: Bartram, the lead white, and the first wall of the Hem.
- `tools/dev/check_route.py` (is this route walkable?) and replayable runs in `tools/dev/play.py` (`act1a`, `corin`, `act1b`, `act1c`, `grind`).

## What scripts cannot tell you

Whether any of it is fun. The list of 25 things only hands can judge is in `MORNING.md`.
