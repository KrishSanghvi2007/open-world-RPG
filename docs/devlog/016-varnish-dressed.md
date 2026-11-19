# Devlog 016: Varnish, dressed

2026-11-19. The first zone of the finishing pass. The village on her shoulder goes from labelled blocks to a place.

Nothing that is played has moved. Every route, every Lampstand, node, station, note and cache is where it was.
The art stands on top of the old shapes, which are still there underneath, unseen, and are still what you touch.

## Before and after

Same four viewpoints each time (they are kept in `tools/kit/shots.json`, so later passes can be compared too).

![The Lampstand square, before and after](img/016-square.png)

![The whole village, before and after](img/016-overview.png)

![The Hem, before and after](img/016-hem.png)

![The Sunward Terraces, before and after](img/016-terraces.png)

And two views that had nothing to compare with, because before there was nothing there:

![Looking west from the square: the Lamp, and the others on the horizon](img/016-west.png)

## What was made

- **The kit** (step 2): 116 pieces across the three zones, every one built by a script in Blender and painted in
  Unreal from the art bible's palettes. Nothing downloaded. `tools/kit/README.md` says how to change any of them.
- **Twelve houses** in seven kinds, none square, each with a sagging ridge, one patch of re-set tiles and a slipped
  one. Windows glow (painted, not lit: it costs nothing). Ansel's door in the Hem is black.
- **The terraces** are held by lilac stone with capstones, edged by gap-toothed parapets, joined by cart ramps with
  timber set across them for grip.
- **Her shoulder.** The village now sits in the slope of a body that falls away in a long curve on every side, rises
  to the ridge of her spine in the east, and has the groove of the Furrow leaving the square's north gate.
- **The far view.** West, over the Hem's roofs: the Lamp as a pale disc, and three other colossi on the horizon as
  flat pale shapes, each with a different back-line. One of them is upright. Remember it.
- **Paths.** The ways people walk are worn pale into the ground, from the Lampstand to each ramp and the gate.
  They are also the three routes the game sends you on, so they double as wayfinding.
- **Work and life:** two market stalls, drying racks beside the ochre, dye lines and a cistern in the Hem, woodpiles
  against the warm walls, a ladder left where someone will want it again. Smoke from five chimneys, pigment drifting
  on the air, birds wheeling out over the slope.
- **Things you use have their real shapes:** the Lampstand (its lantern is dull until you light it), the Grinding
  Slab, the Armature Bench, ochre as flakes of lifting hide, caches as tied bundles, notes on stakes, the gate.
- **No more floating labels.** The grey-box name tags are hidden; the game's own prompts do that job.
- **Motion blur is off** in every zone. Paint does not smear when you turn your head.

## After the Turning (version B)

The same village with the light moved, built by the same script (`shifted=True`), so the two cannot drift apart.

- **The Hem has gone into shade.** Its ground is bare canvas with the drawing showing through, its houses are canvas
  under dulled violet roofs, and their windows are out. The square and the terraces above are as they were.
- **The Furrow is shut.** Two of her vertebrae have risen through the gap in the square's north parapet.
- **The Stair of Spines has come up to the lower gate:** vertebrae stepping away down the slope to the west.
- **The Wader is nearer and darker, and has moved.** From the Hem it now stands over the roofs.

![The Hem after the Turning](img/016-b-hem.png)

![West from the Hem after the Turning: the Wader](img/016-b-west.png)

## Frame rate

Measured with a new tool, `tools/dev/bench.sh`: the game on its own (not inside the editor), drawing exactly
1920 x 1080, the autopilot walking all three village routes for 40 seconds after a warm-up.

| | Average | Slowest tenth of frames |
|---|---|---|
| Bare grey box, the engine's default settings | 47 | 41 |
| Bare grey box, settings changed (below) | **120** | 102 |

**The 60 frames a second the GDD asks for was never being met**, even before any art: the engine's default
anti-aliasing (TSR) alone cost more than half the frame. It is now plain temporal anti-aliasing, and screen-space
ambient occlusion (which lays grey into corners, against the art rules) is off. That more than doubled the speed.

I had not yet measured the dressed village under the new settings in the standalone game when the project was
paused. Inside the editor (a slower, noisier place) it ran at about the same speed as the bare grey box, after the
scenery was welded into a few meshes per area. Measure it first on resuming:
`tools/dev/bench.sh L_Matriarch_Village_A "<walk script>" 40`.

## Choices I made for you (all easy to change; `docs/QUESTIONS.md` 72 to 83)

- Her skin is a dusty ochre-umber, warm where the Lamp reaches it. The grey-box had it violet.
- Furniture-sized props are solid, and only stand where the routes do not go. Small things are not solid.
- Hesper's lampworks, Bartram's grinding house and the cistern house have real doorways, shut for now. They open
  when their insides are furnished (step 4).
- The people are still the plain stand-ins. They, the player and the enemies are step 5.

## What is not right yet

- **The people, the player and the enemies are still stand-ins** (plain cylinders and the grey mannequin). Step 5.
- **No house can be entered yet.** Three have doorways, shut. Step 4.
- **Smoke is solid puffs,** not soft. It reads from across the square and looks crude close up. I can make it softer
  with a see-through material if you would rather.
- **The lilac stone may be too violet** for you, especially in shade. It is one colour in one file
  (`PALETTES["Varnish"]["Stone"]` in `tools/kit/unreal_kit.py`).
- **The lower gate in B stands in front of a solid parapet,** as it did in the grey box. It works; it does not yet
  look like a way through.
- **No person has walked into the props.** The autopilot walked all three routes without touching one, but it walks
  the routes and you will not.
- Nothing here has been heard. Audio is step 7.
