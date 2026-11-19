# Underpainting: art bible for Act 1

Written 2026-11-19 by Claude (Fable) for the "make Act 1 look finished" phase. It turns `docs/GDD.md` section 9
and the look test into rules specific enough to build from. If this page and a finished asset disagree, one of
them is wrong: fix it and say which in `docs/DECISIONS.md`.

**Chosen look:** style B, **Glaze** (owner's decision, 2026-11-18). **Tonal anchor:** wabi-sabi.

![The look test, style B](looktest/B_Glaze_Hero.png)
*The target. Everything in Act 1 should sit comfortably in this picture.*

![Looking sunward](looktest/B_Glaze_Sunward.png)
*The same corner, looking at the Lamp. Note how little there is: three values, one warm light, violet shade.*

## 1. The idea in one sentence

**A world painted in thin warm colour over a pale ground, by a hand that stopped, lit by a lamp that is going out.**

Everything follows from three words in that sentence.

- **Thin.** Glaze is transparent paint. The ground shows through the lights; colour is never thick or plastic.
  Surfaces look brushed, not rendered. Nothing is glossy.
- **Stopped.** The picture is at three stages at once: painted (Varnish), half-painted (the Scumblewood), and
  only drawn (the Vault). Where paint ends it ends in a brush edge, and what lies beyond is canvas with the
  drawing still on it. This is the game's signature image and must be visible in every zone.
- **Going out.** The light is low, amber and from the west, always. Shadows are long and violet. After the
  Turning the light is lower still. Light is the most valuable thing on screen, so there is never much of it.

### The five rules (from the design document; they are law)

1. **Three values per view:** light, middle, dark. If a screenshot does not read in greyscale, it is not done.
2. **No black or grey shadows.** Shade leans to the complement of the light: amber light, violet-teal shade.
3. **Saturation is a budget.** Scenery stays under about 70%. Full strength is for pigment, Strokes and enemies.
4. **Pure white is for Erasures.** Nothing else is white. The palest scenery is cream `#F2E3C0`.
5. **Brushwork lives in textures and silhouettes,** never in a screen filter.

### Wabi-sabi, as instructions

- **Nothing is square.** Walls lean up to 3 degrees. Roof ridges sag. Posts taper. Build straight, then bend.
- **Nothing is new.** Every building has one visible mend: a patch of different plaster, a re-set tile, a splinted post.
- **Nothing is symmetrical.** Two windows are never the same size. A pair of anything is one and a slightly different one.
- **The ground shows.** In every lit surface, a little of the pale ground comes through the colour.
- **Unfinished is beautiful, not broken.** Bare canvas is calm, cream and clean, with a fine charcoal line on it. It
  is never rubble, rot or ruin. The world is not decaying; it was never completed.

## 2. The zones

Each zone exists twice: **A** before the Turning, **B** after. B is the same place with the light moved.

### 2.1 Varnish, the village on her shoulder (painted)

| | A: Dusk | B: after the Turning |
|---|---|---|
| Mood | Warm, tended, tired. A lamp trimmed for two hundred years. | The same, a little colder. Half the village has gone into shade and nobody has said so. |
| Light | Amber, low, from the west. | Lower. The Hem is in the shadow of her risen shoulder. |
| Sky | Peach to rose bands, lilac cloud strokes. | The same sky with the warm band narrower and the lilac deeper. |
| Shade | Violet `#4A3558` leaning teal `#2E5E6E`. | Deeper violet; the Hem reads as blue-violet. |
| Haze | Warm, thin, far. | Cooler, nearer. |

**Palette:** ochre `#D9A441`, madder rose `#C4574B`, cream `#F2E3C0`, teal `#2E5E6E`, violet `#4A3558`.
Supporting: timber umber `#6B4F3A`, footing stone lilac-grey `#8F86A6`, bare canvas `#D8CBB0` (B only, in the Hem).

**Value plan:** light = lit plaster and ground. Middle = roofs, timber, shaded plaster. Dark = footings in shade,
doorways, the far parapet. The Lampstand flame is the brightest thing in the square once it is lit.

**In B:** the Hem's paint has thinned. Its houses keep their shapes but their walls are bare canvas with the
drawing showing, roofs dulled to violet. If Bartram whitewashed: one wall is hard, flat, too-bright white (the
only white in Varnish, and it should look wrong). If Ochre-washed: warm cream.

### 2.2 The Scumblewood, the Furrow down her spine (half-painted)

| | A | B |
|---|---|---|
| Mood | A place the painter worked on from one side only. Green and alive where the Lamp reaches; drawn and silent where it does not. | Her spine has arched. The same wood, tilted, with a stair of bone through it. |
| Light | Amber, lower in the sky than in Varnish, broken by trunks. Half the light of the village. | Dimmer again. |
| Sky | A strip between two cliffs: rose above, violet toward the Vault. | The same, more violet. |
| Shade | Deep teal-violet. This is the zone where shade is most coloured. | |
| Haze | Green-grey, in layers down the gorge, so each clearing is paler than the last. | |

**Palette:** terre verte `#6B8F71`, raw umber `#6B4F3A`, bare canvas `#D8CBB0`, charcoal `#3A3A3A`,
sanguine `#9E2B25` (accents only: blood wells, Madderbacks). Supporting: moss light `#9DB28A`, bone `#CFC3A8`.

**The one-sided rule made visible.** The Lamp is in the west. Everything is painted on its west face and bare
on its east. Walking down the Furrow you see paint; turn round and you see canvas and charcoal. Trees, rocks,
the gorge walls and Corin all obey this. It is the zone's whole identity and should never be broken for convenience.

**Value plan:** light = lit moss and the canvas side. Middle = painted bark and leaf. Dark = the gorge's west wall
in its own shade, and the mouth of the Rib Gate.

**In B:** the First Well is shut inside the Furrow. Five vertebrae have risen as a stair: pale bone, clean, freshly
uncovered, with soil and torn roots at their feet. The Nape Overlook shows the horizon for the first time.

### 2.3 The Grisaille Vault, inside her ribs (sketch)

| | A | B |
|---|---|---|
| Mood | The inside of a drawing. Quiet, dry, exact. The hand that drew this was thinking, not showing off. | The heart is warm. Some of the drawing has "taken" and become solid for good. |
| Light | None. Paper is its own light (unlit surfaces). | A warm glow from the Heart reaches the last third. |
| Sky | None. Dark paper overhead, with ruled lines running to a vanishing point. | |
| Shade | There is no shade. Hatching does the work of shadow. | |

**Palette:** paper `#E6E1D6`, greys `#8C8A85` and `#4B4A48`, graphite `#2E2E30`, one blue `#2F4BA8` (sketched
platforms, Pentiments), heart glow `#E8923A`. **Nothing else.** The first warm colour the player has seen for
ten minutes is the Heart, and that is why it matters.

**What makes it a drawing:** flat unlit planes; a dark outline on every edge; hatching where a surface turns away;
construction lines left in (perspective lines to one vanishing point above the Heart, measuring ticks along the
ribs, a circle drawn three times before it was right); margin notes in the Hand's writing; smudges where a palm rested.

**In B:** the last two platforms and the Heart wall are painted in whatever colour the player chose, and hold.

## 3. Architecture

### 3.1 Varnish

Built by people on a slope that breathes. Low, thick, mended.

- **Walls:** limewashed plaster over rubble, cream, 40 cm thick, leaning. A footing course of lilac-grey stone up to
  knee height, darker and rougher. Corners are rounded by two centuries of hands.
- **Roofs:** madder-rose tile, steep (40 to 45 degrees), with a sagging ridge and one or two slipped tiles. Deep eaves.
  A stub of chimney, never smoking straight.
- **Timber:** ochre-brown frames, door and window surrounds, brackets. Hand-adzed: tapered and slightly twisted.
- **Doors:** plank, teal or umber. One door in the Hem is black (Ansel Verdigris set it; it went black). Leave it alone.
- **Windows:** small, deep-set, shuttered, glowing amber from within at dusk. The glow is painted, not lit.
- **Terraces:** her shoulder is cut into three steps held by retaining walls of the same lilac stone, with a
  low parapet. Ramps, never stairs, between them (carts).
- **Signs of work:** drying racks, pigment jars on sills, cloth dyed and hung, a ladder left against a wall.
- **Never:** glass panes larger than a hand, metal roofs, anything machined, anything taller than two storeys.

### 3.2 The Scumblewood

Nothing here was built. It grew, or it is her.

- **The gorge walls** are two rows of her vertebrae: tall, rounded, bark-brown where painted, with moss on
  ledges. Bare canvas on the side away from the Lamp, with charcoal contour lines following the form.
- **Trees:** thick leaning trunks, canopies as three or four overlapping masses of dabbed leaf, ragged at the edge.
  Painted west, drawn east. Roots cross the path and are the forest's "architecture": arches, steps, railings.
- **Clearings** are flat pads of moss and trodden earth, each with one thing that names it (a well, a lamp, a nest).
- **People's marks:** Ilka's chalk tallies and blood wells (dark sanguine pools ringed with scab-brown), Corin's
  lean-to, rope and pegs where someone has been going down for years. Small, low, temporary.
- **The Rib Gate:** the first place bone shows. Two ribs as a pointed arch, pale, with the dark of the Vault behind.

### 3.3 The Grisaille Vault

Not a building: a ribcage, drawn.

- **The Rib Stair:** arches of rib at regular intervals, each drawn with a confident outline and hatched on its
  underside. Regularity matters here: this is the one place in Act 1 that is symmetrical, because it is anatomy.
- **The Cartoon Gallery:** a pit crossed by platforms that are only sketched (blue line, no fill) until lit.
  Named for a painter's cartoon, the full-size drawing made before the painting.
- **The Heart:** a round chamber. A hollow the size of a sleeping person. Three figure studies on the wall, one
  of them a blank. The heart itself is the only round, warm, soft-edged thing in the zone.

## 4. Kit list

Modular pieces, made in Blender by script (so they can be regenerated), in the look-test manner: low, chunky,
slightly crooked, about 300 to 1,500 triangles each, one or two materials.

### 4.1 Varnish

| Piece | Notes |
|---|---|
| Wall, 2 m and 4 m; corner; end | Plaster over footing. Lean built in. |
| Wall with door; wall with window; wall with shuttered window | Openings are real, for interiors. |
| Gable end; roof, 4 m run; roof end; ridge; chimney | Sag built into the ridge. |
| Door (opens); shutter pair | Separate, so doors can swing. |
| Retaining wall, 4 m; parapet, 4 m; parapet end | Lilac stone. |
| Ramp, 4 m wide; cart ruts decal | |
| Step, single (for house thresholds) | |
| Lampstand (final) | The look-test Lampstand, finished: post, bracket, lantern, stone base. |
| Grinding Slab; Armature Bench; flax press | The three working props that are gameplay. |
| Market stall with awning; drying rack; dye line with cloth | Square dressing. |
| Crate, barrel, sack, jar cluster, basket | |
| Bench; trough; handcart; ladder | |
| Fence, wattle, 2 m; gate | |
| Cistern and standpipe | Lacuna's corner of the Hem. |

### 4.2 The Scumblewood

| Piece | Notes |
|---|---|
| Vertebra cliff, 3 sizes | Painted on one side, canvas on the other. |
| Tree, 3 kinds; sapling; stump; fallen trunk | Canopy as dab masses. |
| Root arch; root step; root rail | |
| Rock, 4 sizes; rock with moss cap | |
| Fern clump; grass tuft; moss patch; reed | Foliage: cards with ragged brush edges, instanced. |
| Mushroom shelf; hanging moss | |
| Blood well; chalk tally stone | Ilka. |
| Lean-to; bedroll; cook-ring; rope and peg line | Corin and Ilka. |
| Rib arch (the Rib Gate) | Bone. |
| Vertebra step (the Stair of Spines) | B only. |
| Canvas edge decal; charcoal contour decal | Where paint stops. |

### 4.3 The Grisaille Vault

| Piece | Notes |
|---|---|
| Rib arch; rib post; rib span | Outline and hatching in the texture. |
| Paper floor tile; paper wall; torn paper edge | |
| Construction line; vanishing-point fan; measuring ticks; triple circle | Thin unlit geometry, graphite. |
| Sketched platform (final) | Blue line; fills when lit. |
| Margin note plaque | The Hand's writing. |
| Smudge decal; eraser crumb pile | |
| Heart; hollow; study panel, 3 | |

## 5. Interiors

Small, warm, crowded with work. Each is one or two rooms inside a real house shell, entered through a real door,
with no loading screen. Sunlight does not reach inside (one sun, no bounce), so each is lit by a painted window
glow and one or two small shadowless lamps, with the bounce painted into the walls.

| Interior | Where | Why it is there |
|---|---|---|
| **Hesper's lampworks** | Lampstand square, Varnish | Where the lantern comes from. Wick racks, oil jars, a bench, and on the left a second bench kept exactly as it was eleven years ago. |
| **Bartram's grinding house** | Lampstand square, behind the Grinding Slab | Pigment in jars to the ceiling. The stone jars of Thinner. The receipt for the madder. |
| **Ilka's hide** | The First Well (A); the third vertebra (B) | A tick-warden's shelter: ledgers, bottled samples, a cot. Her count is on the wall. |
| **Corin's hollow** | The Sunward Clearing | Inside a hollow trunk. Painted on the side with the opening. He sits so you cannot see the rest. |
| *Added:* **the cistern house** | The Hem | Lacuna's. One wet room, pipes, a drain. The only interior that drips. |
| *Added:* **the black door** | Top of the Hem | Ansel Verdigris's old house. Locked in Act 1. Seen through one window: a room where nothing has moved in nine years. Sets up Act 2. |

Not an interior: the Vault. The Heart is the room.

## 6. People, creatures and the Folded

Four kinds of being, and each is painted differently. A player should be able to tell which kind something is
from across a clearing.

| Kind | How it is painted | Who |
|---|---|---|
| **People** | Glaze, like the world: three values, warm lights, soft edges. Simple, chunky shapes; faces are two or three strokes. | The Limner, Hesper, Bartram, Corin, Ilka |
| **The Folded** | Ink line kept exactly as drawn, thin colour inside it, a black edge all round. Slightly self-lit. | Lacuna, the Hatcher, and the rest of `docs/story/CAST_IN_GAME.md` |
| **Erasures** | Pure flat white. No brushwork, no shading, no outline. The ground under them is bare. They look cut out of the picture. | Blotter, Old Palimpsest |
| **Parasites and drafts** | One saturated hue each. Parasites are wet and painted thick; drafts are a single coloured line with nothing inside. | Madderback (red), Pentiment (blue) |

### The cast

- **The Limner (player).** A walker's coat in dusty teal, madder scarf, the lantern at the hip, the knife. Seen
  from behind for the whole game: a pack and a rolled blanket cover the back. *(See the open question below.)*
- **Hesper Vane.** Ochre work dress, cream apron, madder shawl pinned badly so it hangs longer on one side, grey
  hair up, a lighting pole. (As in the style probe, devlog 013.)
- **Bartram Sedge.** Wide, short, an apron stained every colour, forearms stained to the elbow, a muller in one hand.
- **Corin Half-Seen.** Painted on the front, bare canvas with charcoal lines behind. Stands so you see the front.
- **Ilka Thrum.** Tall, thin, a long waxed coat the colour of dried blood, a ledger, glass tubes on a bandolier.
- **Blotter.** A tall white figure with cloth arms. No face. Where it stands the ground has faded.
- **Madderback.** A boar-sized tick, sanguine red, glossy only on its swollen back. Legs dark umber.
- **Pentiment.** A person drawn in one blue line: an earlier idea of someone, with a second arm sketched where the
  first one was.
- **Old Palimpsest.** White on white, layered like scraped pages, with the faint outline of a curled figure showing
  through. In its second phase the white comes off in strips and what is under it is madder red.

**Open question for the owner.** The story says the Limner is "painted on the front and bare behind", and the
player learns this in Act 3 from a mirror. In a third-person game the player looks at the Limner's back for hours.
My recommendation: the pack and blanket cover it, so the reveal holds, and the coat's back is painted a little
more thinly than its front for anyone who looks hard. The alternative is to show the bare back from the first
minute and trust nobody to understand it. (`docs/QUESTIONS.md`.)

## 7. Effects

Every effect is paint doing something paint does. No sparks, no smoke puffs, no lens flares.

| Thing | How it looks |
|---|---|
| **Pigment, wet** | Glossy dabs of full-strength colour that sit up from the surface. |
| **Pigment, dry** | The same colour, matte, flat, settled. |
| **Harvesting** | The node's colour lifts off in three or four brush dabs and flies to the satchel in an arc. |
| **Glim** | Small amber motes round the lantern, like dust in a sunbeam. Fewer as it runs down. |
| **Flare** | A ring of warm light that spreads like a wash across wet paper, and dries back from its edge inward. |
| **Strokes** | One brushstroke each, in the pigment's colour, in the manner the stroke is named for: Glaze, a thin transparent sweep; Scumble, dry and broken; Alla Prima, wet and fast; Impasto, a thick ridge; Erase, a white wipe; Sfumato, blue smoke with no edge. |
| **Hits** | Paint knocked off the thing struck, in its own colour. Erasures shed white flakes, like dried gesso. |
| **Erasure** | Where an Erasure stands or strikes, colour leaves: a patch of bare canvas that slowly takes paint again. |
| **Lampstand** | A soft amber pool. Lit ones breathe; dark ones stutter. |
| **The Turning** | The ground tilts; long cracks of bare canvas run through the paint; the light swings. Then stillness, and one eye. |

## 8. Interface

**Paper, ink and one stroke of colour.** The satchel (devlog 010) is the pattern for everything.

- **Surfaces** are cream paper with a faint ruled line, slightly foxed at the edges. Never glass, never a gradient.
- **Text** is dark umber ink, not black. Headings sit on a single brushstroke of the relevant pigment.
- **Bars** (Health, Stamina, Glim) are three brushstrokes that shorten from the right, with a dry-brush tail.
- **The palette wells** are two round wells of wet colour in a wooden palette.
- **Icons** are painted (devlog 012), one or two strokes each.
- **The map** is a drawing of the Matriarch from above, in the Hand's line, with places written in.
- **The title screen** is the easel: a canvas, mostly bare, with the game's name in one stroke of madder.
- **Fonts:** free, with the licence in `docs/ASSETS.md`. A humanist serif for reading; a brush or pen hand for titles.

## 9. Quest things

- **Something to decide** (a choice point) is always bare canvas, cross-hatched: the place that is not finished.
- **Something to read** is paper with handwriting. The Hand's margin notes are in graphite; Odile's are in ink.
- **Something to follow** is the green marker, the journal line, and above all the next lit Lampstand.
- **Something given** arrives as a line of text and an icon, top left. No fanfare.

## 10. The vista

The land is a body. The player should feel that without being told.

- **Under your feet:** Varnish's terraces are the slope of a shoulder. The forest's gorge is the groove of a spine.
  The Vault's arches are ribs. Far edges of every zone fall away in a long curve, never a cliff: skin, not rock.
- **Her:** from the village parapet, the long ridge of her back runs north into haze, with the dark line of the
  Furrow down it. Her head is turned away and is never seen in Act 1.
- **The Lamp:** low in the west, a pale disc in a warm band. It does not move. All light in Act 1 comes from it.
- **The others:** on the horizon, two or three colossi as flat, pale silhouettes, each a different back-line.
  They are part of the sky painting and cost nothing.
- **After the Turning:** one of them is closer and darker and has moved. That is the Wader. From the Nape Overlook
  it is the thing you look at.
- **The edge of the picture:** at the very horizon the sky's paint thins to bare canvas for a hand's breadth. Most
  players will never notice. Act 3 goes there.

## 11. Budgets

The lighting rule is fixed: no Lumen, no ray tracing, one shadow-casting sun, one tinted sky light per zone,
small lights without shadows, bounce painted in. The Vault is unlit.

| | Budget |
|---|---|
| Frame | 60 fps at 1080p on the M2 Pro (16 GB), measured after every zone |
| Triangles in view | under about 1.5 million |
| Draw calls | under about 2,000; foliage and repeated kit pieces instanced |
| Kit piece | 300 to 1,500 triangles, one or two materials |
| Hero prop or character | up to 15,000 triangles |
| Textures | 1K for kit pieces, 2K only for characters and skies; brush masks shared |
| Materials | a handful of masters (Glaze, Sketch, Foliage, Fold); everything else is an instance |
| Small lights | no shadows; no more than about eight in view |
| Translucency | effects only, and briefly |

**How a zone is checked:** the same walk through it before and after, frame times recorded, the slowest spots
looked at. A zone is not finished until it holds 60.

## 12. What is not allowed

- A full-screen paint filter.
- Anything photographic: scanned rock, photo textures, realistic foliage. A free scanned asset may be used only
  if it has been repainted until it sits in the look-test picture.
- Black. Use umber or violet.
- White, except Erasures.
- A straight ridge, a square wall, or a matched pair.
- Anything paid.
