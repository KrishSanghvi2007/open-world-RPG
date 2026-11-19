# Devlog 015: the Folded. Two drawings walk into the game

2026-11-19, overnight. Characters by **Krish & Hannah Sanghvi**: nine of their folded-paper figures cast into
Underpainting, and two of them built all the way to playable.

**Watch first:** [`clips/015-hatcher.mp4`](clips/015-hatcher.mp4), 21 seconds. A Flare shows a web, a strand is
touched anyway, and the Hatcher comes down her thread.

## 1. The casting

`docs/story/CAST_IN_GAME.md`. The short of it:

The story already says the Hand left the world unfinished because *a set thing cannot surprise you*. Before it
learned that, it tried a simpler trick: it folded the sheet, drew a head, folded it away, drew a body without
looking, folded again, drew legs. Then it opened the paper and met someone it had not planned. Those are
**the Folded**. It is exactly how the real drawings were made, so nothing had to be bent to fit.

They are ink, not paint. Ink does not need light, so they live where people cannot, and in a game about light
running out they are the ones who are not afraid of the dark.

| Drawing | In the game | When |
|---|---|---|
| d40 The Faceless One | **Lacuna**, keeper of the Hem's water, with a choice about a missing face | In Act 1 now |
| d28 The Spider | **the Hatcher**, whose web only shows in light | In Act 1 now |
| d24 The Summoned | **Piecemeal**, keeper of the last Lampstand (and the reason it is lit) | Slice, next |
| d05 The Mother | **the Vigil**, a silent witness at the door of the Heart | Slice, later; needs your say first |
| d25 The Darter | **the Unpinned**, still and then not | Act 2 |
| d01 The Icebox | **the Larder**, which keeps your wet pigment cold and safe, for a share | Act 2 |
| d27 The Quarrel | **the Quarrel**, the argument a perfect town put outside its gate | Act 2 |
| d29 The Pole and the Keep | **the Lineman and the Keep**, a place to rest that walks | Act 2 |
| d21 The Puppeteer | **the Lay Hand**, who works the Hand's own posing figure | Act 3, in reserve |

## 2. Lacuna (d40)

![Lacuna beside the drawing](img/015-lacuna.png)
*Top: the drawing; the front view painted in the Glaze colours by script; the back view. Middle: the model from
four sides. Bottom: in the Hem, mid-gesture, a hand going to where the face was; and after "Finish the face".*

A *lacuna* is what picture restorers call a place where the paint is lost. Lacuna keeps the water in the Hem.
The pipes are their legs, the taps drip, the hands float a little way off and do the work. They have no voice, so
the conversation is written as what their hands do.

**In play:** walk down into the Hem after the first quest. Their hands drift round them; now and then one turns
their own tap, or goes to the bare place and stops. Ask about the face and the side story *A Face for Lacuna* begins.
It ends at the bare place itself: **finish the face** (3 Hide Ochre: they are set, stand straight, the hands come
home to the sleeves, a plain warm face like a small moon) or **leave it open** (nothing spent; a hand closes over
yours). It is the game's whole question at the size of one neighbour, and it comes before the Heart wall.

## 3. The Hatcher (d28)

![The Hatcher beside the drawing](img/015-hatcher.png)
*Top: the drawing; the spider alone, front and underside. Middle: the model standing on its angular legs. Bottom:
hanging over the cache at the Bend with no web to be seen; the same view with a Flare burning; down and fighting.*

*Hatching* is shading with lines. Her web is the cross-hatching the forest's shadow is made of. She hangs head-down
on a thread over the corner of the Bend, exactly as drawn, turning slowly.

**In play:** there is a cache under her. In the half-light you see a spider on a thread and nothing else. Light a
Flare and the strands stand out pale, with one gap: the way in. Walk into a strand you did not see and you are
held for two seconds while she comes down and does everything at once. Leave the web alone and she never moves.
The first time you come near, one line of text tells you something is strung there that light would show.

She has her own room for trying her out: **Maps → Dev → L_Combat_Hatcher**.

## 4. How it was done, and what did not work

The pipeline is seven scripts in `tools/characters/`, and it is now a skill: **`/make-character <id>`**.

1. **Paint.** The ink redraw is kept exactly; everything the ink encloses is coloured by part, using the boxes the
   drawings repository already has for each part. Front and back.
2. **Thickness.** The outline is blown up like a balloon, on this Mac.
3. **Build.** In Blender: the painting is laid straight through the shape, a black inside-out shell makes the ink
   edge, a bone goes at each pivot from the cast file, and each point of the surface goes to the nearest part.
4. **Move.** Movements are written from the cast file's own notes ("walks four legs at a time, slow placing, a
   quick last tap, then a sudden rush"; "hands drift like satellites").
5. **Game.** Unreal takes the mesh, the bones and one animation for each movement.

**What worked**

- **The line survives.** The front of each model *is* the drawing. This was the thing you asked for and it is the
  thing I would keep whatever else changes.
- **The cast file did the rigging.** Part boxes and pivots written for the films put the bones in the right
  places with no hand work. Floating hands and plumbing legs came across as drawn.
- **The roles came from the personalities.** "It does nothing until something touches its web, and then it does
  everything" is the Hatcher's whole design.

**What did not**

![What the online service returned](img/015-service-spider.png)
*The free online service's spider: a good spider, but not this one. The legs are redrawn, so the painting no
longer sits on it.*

- **The free image-to-3D service.** You asked for it, so it was tried first, with no login, and it answered in 25
  seconds each time. It gave the Spider real volume but changed its proportions, and it gave the Faceless One
  back as a card two centimetres thick. Neither was used. It is still in the pipeline as an option, and the build
  step now measures how well a shape matches the drawing and refuses a bad one. The other two services you named
  could not be reached from here tonight.

![The Faceless One from the service](img/015-service-faceless.png)
*And its Faceless One: the right outline, with no depth at all (the thin line at the bottom left is the side view).*
- **Depth.** Both characters are as deep as a cushion, not as a body. From the side Lacuna is a thin wedge. For
  figures that are drawings in the story this half works as a look; you will know at once whether you like it.
- **A true turnaround.** The front and back are painted from the drawing. The side views are pictures of the
  finished model, because nothing free that I could reach will draw a new side view of someone else's figure.
- **One of the Hatcher's legs kinks.** Where thin legs cross in the drawing, a few points follow the wrong leg.
  Because her legs are solid black it reads as a spiky leg, not a mistake, but it is one.
- **Stipple, the small spider, is not made.** Nor is the web that is in the drawing; the web in the game is built
  by the game.
- **Lacuna only stands.** The shuffle is animated but they do not walk anywhere yet.

## 5. Not checked

- By hand: nothing. All of this was played by script.
- "Leave it open" for Lacuna was not played, only "Finish the face".
- The Hatcher in the forest was lit and looked at, but touched and fought only in her test room.
- How either looks after the Turning.
- The frame rate with them in the level.

## 6. Yours to decide

In `docs/QUESTIONS.md`, 59 to 68. The four that matter:

1. **The Folded** as the idea that holds them together. Yes, or something else?
2. **The cushion depth and the black edge**: is this the look for all nine?
3. **Lacuna in the Finale**: only someone left open can finish you. Should Lacuna be one of them?
4. **The Vigil** (d05): the drawing has nudity and an unborn child. I have not made it. How do you want her?

Free tools only: Blender, numpy, scipy, Pillow, and one anonymous try of Hunyuan3D-2.1. Nothing paid. Only the
drawings used were copied in, and the drawings repository was never written to.
