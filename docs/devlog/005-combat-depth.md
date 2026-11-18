# Devlog 005: combat depth

2026-11-18. Milestone **M4**. The fight now has the game's own idea in it: colour is the damage system, and light is what it costs.

## What is new

- **The palette's two wells.** The first pigment you attune goes into the first well. **Z** swaps the active well. A pigment's passives only count while it sits in a well.
- **Strokes.** Hold the heavy button and the swing becomes the active well's Stroke. All six are in, with their numbers from `data/pigments.json`:

  | Pigment | Stroke | What it does |
  |---|---|---|
  | Hide Ochre | Glaze | Marks the enemy: it takes +15% from everything for 6 s. |
  | Slumber Blue | Sfumato | A 4 m dash through the enemy that cannot be hit; slows it 40% for 3 s. |
  | Terre Verte | Scumble | Gives back 15 Stamina on a hit. |
  | Sanguine | Alla Prima | Three quick hits, each healing 4. |
  | Bone Black | Impasto | A slow slam: 1.4x damage, double stagger, slows the enemy. |
  | Ceruse | Erase | A fast swipe that wipes out an attack the enemy has started. |

- **Light is the price.** A Stroke costs Glim: the pigment's light cost x (1 - the light where you stand). Free in the village, half in the forest, full in the Vault. A Flare makes any place count as at least half-lit.
- **The Contrast rule.** A Stroke in the enemy's complementary colour: +30% damage, +50% stagger. In its own colour: -30%, "muddy". The game says which when it happens.
- **Lock-on** (Q): the camera and the Limner settle on the enemy in front.
- **Moss Poultice** (H): 40 Health over 4 seconds. **Lamp Oil** (J): +40 Glim.
- **Perfect dodge:** a hit that passes through the safe part of a dodge gives back 5 Glim.
- **Passives that now work:** Warm Ground (+20 Glim), Green Earth (+20% Stamina regen), First Blush (heal 8 on a kill), Dead Colouring (+15% stagger), Half-Dream (+20% dodge distance), Lucid (longer safe window), and Ceruse's lead toll (-15 maximum Health while it is in a well).
- Enemy bars only show for enemies within 25 m.

## Picture

![Two Blotters in the Scumblewood](img/005-combat-hud.png)
*The forest in play, with the style kit: two Blotters, one winding up a Dab, one a Wipe.*

## How it was checked

- 8 automated tests pass. The new one checks the Contrast numbers, that every enemy in the data is weak to its complement, and what an Impasto costs in each zone (0, 5, 10, and 5 in the Vault under a Flare).
- In the running game, in the forest, with a Bone Black and Ceruse build:
  - **Impasto on a Blotter: 47.3 damage** (26 heavy x 1.4 x 1.3 contrast), and it staggered at once. It cost 5 Glim, as the forest's half light says.
  - **Erase on a Blotter: 14.6** (muddy, as it should be for white on white), and it cancelled the second Blotter's attack mid-swing.
  - Carrying Ceruse dropped maximum Health to 85.
  - Lock-on found its target.

**Not checked in play:** the other four Strokes, the Moss Poultice and Lamp Oil (the test character had none), and the perfect dodge. Their rules are simple and compile, but nobody has seen them work.

## Test it (5 minutes)

1. Play the village loop until you have rested and attuned Warm Ground (see devlog 002). Ochre is now in your first well.
2. Go down to the Blotter by the lower gate. Press **Q** to lock on.
3. **Hold right mouse**: that is Glaze. It is free here because the village is fully lit. The Blotter's bar says "glazed"; your next hits do more.
4. Dodge through its warm-lit swing with **Left Ctrl**. If you time it well: "Perfect dodge."

## Rough edges

- No animations: a Stroke is a thicker outline in the pigment's colour.
- Scumble's backstep and Impasto's ground smear are simplified to their numbers.
- Two Blotters at once will kill a careless tester quickly. That may be right.
