# Devlog 004: the style kit

2026-11-18. Parallel track, not a milestone. Playtests now look like the game without any models.

## What it is

- **One material, one colour.** `M_Glaze` takes a single colour and paints three values of it in brush strokes: a shadow that leans toward the zone's shadow tint (never grey), the colour itself, and a light that leans toward warm ground showing through. Warm bounce is painted in on downward faces. A placeholder only ever needs a colour.
- **Applied automatically.** The level builders pick the stroke pattern from what a piece is (ground, stone, wood, plaster, roof). The C++ placeholders (people, nodes, Lampstands, enemies) use the same material. The Blotter keeps its exact, flat white: pure white belongs to Erasures.
- **A look per zone.** `VillageDusk`: painted sky, low warm sun, violet shadows, golden haze. `ForestHalfLight`: dimmer, cooler, greener. `VaultSketch`: flat paper, no sun, a drawn line round everything.
- It follows the lighting rule: one shadow-casting sun, one tinted sky light, no full-screen paint filter, Lumen off.

## Pictures

![The village square in grey-box, with the style kit](img/004-stylekit-village.png)
*The same boxes as before. Shapes that look black here are the working objects, which take their colour when the game starts.*

![The Vault as paper and line](img/004-stylekit-vault.png)
*The Vault from above: paper, graphite, the one blue.*

## Where it lives

`tools/stylekit/stylekit.py` and `/Game/StyleKit`. To restyle everything: change the numbers in `LOOKS` or the recipe in `GLAZE`, then rebuild the levels.

## Rough edges

- The village ground reads a little pink under the violet sky light.
- The forest and the fight room have not been looked at with the kit yet.
- Performance has not been measured.
