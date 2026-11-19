---
name: make-character
description: Turn one of Krish and Hannah Sanghvi's drawn figures into a rigged, animated 3D character in Underpainting. Use when the owner types /make-character <id> (for example /make-character d24), or asks to "make", "build" or "bring in" one of the Folded, a character from the drawings, or a figure from docs/story/CAST_IN_GAME.md.
---

# /make-character <id>: a drawing becomes a character in the game

`<id>` is a drawing's ID in the owners' drawings repository, such as `d28` or `d40`. The pipeline was proved on
those two (devlog 015). Everything is free and runs on this Mac except one optional online step.

The owner is non-technical. Do all of it yourself and report in plain language with pictures.

## Rules that do not bend

- **Credit:** "Characters by Krish & Hannah Sanghvi" (already in `docs/ASSETS.md`; add a row for each new drawing used).
- **The drawings repository is read-only.** It is cloned next to this project at `../exquisite-corpse`
  (branch `claude/exquisite-corpse-phase1`). Never commit or push there. Its push address is deliberately broken.
  If it is missing: `gh repo clone KrishSanghvi2007/exquisite-corpse ../exquisite-corpse -- --branch claude/exquisite-corpse-phase1 --single-branch`, then `git -C ../exquisite-corpse remote set-url --push origin no-push-read-only`.
- **Copy in only what is used.** `paint_front.py` copies the drawings it reads into `art_source/characters/<id>/source/`. Nothing else comes across.
- **The game is not the films.** Take the figure and its personality from `docs/CAST.md` in the drawings repository. Leave the films' stories there.
- **Free tools only. Never pay, never start a trial.** The one online service is used with no login. If it says the quota is used up, use the offline step instead.
- **Send nothing but the picture** to any online service: no names, no project, no email.
- **Ask the owner first** if the drawing has nudity, gore or anything they should see before it is made (CAST.md flags these in its Notes), and for any new story it would need. Casting already agreed is in `docs/story/CAST_IN_GAME.md`.

## Before you start

1. Read the character's entry in `../exquisite-corpse/docs/CAST.md` (Figure, Personality, Locomotion, Parts, Cut notes, Self-interactions) and its row in `docs/story/CAST_IN_GAME.md` (who it is in the game and what it does).
2. Look at the drawing: `../exquisite-corpse/drawings/redrawn/<id>/redrawn.png` and the painted versions in `drawings/painted/<id>/`.
3. Blender must be open and connected (the `blender` MCP), and Unreal must be open with its MCP server. `/ready` does both.
4. A Python with numpy, scipy, Pillow and gradio_client. If there is none: `python3 -m venv <scratchpad>/venv && <scratchpad>/venv/bin/pip install numpy scipy pillow gradio_client imageio-ffmpeg`. Call it `PY` below.

## The seven steps

Each step is one script in `tools/characters/`. Look at the picture each one makes before going on.

### 1. The plan: `tools/characters/plans/<id>.json`

Copy `d40.json` (a standing person) or `d28.json` (a creature drawn from above) and change it. What matters:

| Key | Meaning |
|---|---|
| `keep` | The part names from CAST.md that belong to the character. Leave out scenery (a web, a moon, a ladder). |
| `erase` | Boxes, as fractions of the sheet `[x0, y0, x1, y1]`, to rub out (drips, a thread, a neighbour). |
| `open`, `regrow` | For solid-black figures tangled with thin lines: how many pixels to wear away, and how far to grow back. 0 for line drawings. |
| `colours`, `regions` | A colour for each part, then extra boxes or ellipses on top (hair, a face). Use the Glaze palette: ochre `#D9A441`, rose `#C4574B`, cream `#F2E3C0`, teal `#2E5E6E`, violet `#4A3558`, bare canvas `#D8CBB0`. |
| `fill_boxes`, `hole_boxes` | Force an area to be part of the figure, or to stay empty, when the lines do not close. |
| `back_regions`, `back_keep_ink`, `back_same` | What changes on the back (the back of a head is hair), which parts keep their drawn lines, or "the same as the front". |
| `shape` | `"inflate"` (offline, keeps every line) or `"hunyuan"` (the online service). Start with `"inflate"`. |
| `depth` | How round each part is, for inflate. 1 is normal; a head is about 1.7. |
| `pose` | `"upright"`, or `"lying"` for a creature drawn from above, with `"head_dir"` saying which way its head points on the sheet. |
| `size_m`, `tris`, `outline_m` | Height (or span, if lying) in metres; triangles to keep; width of the ink edge. |
| `root_part` | The part the model is centred on. |
| `floating` | Parts that hang in the air beside the body (hands). Each owns its whole piece and nothing else. |
| `ink_parts` | Parts that are solid black (a spider's legs): they get the ink material, not the picture. |
| `extra_bones` | Named points to hang things on in the game (a face). |

### 2. Paint the front and back

    PY tools/characters/paint_front.py <id>

Writes `art_source/characters/<id>/front.png`, `back.png` and `front_white.png`. **Look at them.** Fix the plan until the
figure is whole, clean and coloured. Most of the time in this pipeline goes here, and it is worth it: the painting
is what the player sees.

### 3. Give it thickness

    PY tools/characters/inflate.py <id>

Blows the outline up like a balloon. Offline, repeatable, and the drawing's lines stay exactly where they were.

Optional, to compare: `PY tools/characters/image_to_3d.py <id>` asks the free Hunyuan3D-2.1 demo for a shape (about
25 seconds; no login). On the two pilots it re-drew the spider with different proportions and returned the standing
figure as a flat card, so neither was used. If you set `"shape": "hunyuan"`, step 4 measures the match and refuses a
poor one.

### 4. Build it in Blender

Through the Blender MCP, in two calls (the first must be on its own):

    bpy.ops.wm.read_homefile(use_empty=True)

    CHAR_ID = "<id>"; exec(open("<repo>/tools/characters/blender_build.py").read())

It prints `MATCH 0.xx`: how well the shape agrees with the drawing. Above 0.9 is right. It then maps the painting
straight through the shape, front and back; puts one bone at each pivot CAST.md gives; hands every point of the
surface to the nearest part; and adds the ink edge (the same surface pushed out and turned inside out, black).
Look at it from the front, the side and three-quarters. If a part tears when it moves, its points are owned by the
wrong part: use `floating`, `bias` or `tails` in the plan.

### 5. Movements: `tools/characters/anims/<id>.py`

Write this from CAST.md's **Locomotion** and **Self-interactions**, quoting the lines you used at the top of the
file. Copy `d40.py` or `d28.py` for the shape of it. Each movement is a `Take("Name", frames)` with poses set by
`take.turn(frame, bone, tilt, swing, twist)` and `take.shift(frame, bone, (x, y, z))`.

The game looks for these names:

| A creature that fights | A person |
|---|---|
| `Idle`, `Walk`, `Rush`, `Windup`, `Strike`, `Flinch`, `Curl` (struck down or dead) | `Idle`, `Walk`, and any gestures by name; `Still` if they can be set |

Then, in Blender:

    CHAR_ID = "<id>"; exec(open("<repo>/tools/characters/blender_anim.py").read())
    CHAR_ID = "<id>"; exec(open("<repo>/tools/characters/blender_export.py").read())

The export writes `turnaround.png` (front, three-quarter, side, back) and `<id>.fbx`.

### 6. Bring it into Unreal

With Play stopped, through the dev toolset:

    run_script("characters/unreal_import.py", ["<id>"])

It makes `/Game/Folded/<id>/SK_<id>`, one animation per movement under `Anims/`, and the materials.

### 7. Put it in the game, and show the owner

- **A creature:** a row in `data/enemies.json` (family `Folded`), a class in `Source/Underpainting/Combat/UnderpaintingBestiary.*` whose `GetFigureId()` returns the ID (see `AHatcherEnemy`), its name in `ENEMY_CLASSES` in `tools/greybox/build_matriarch_greybox.py`, and its zone's enemy lists in `data/regions.json`. Give it a test room in `tools/greybox/build_combat_room.py`.
- **A person:** an `UnderpaintingNPC` with `figure_id` set (see Lacuna in `build_village`), their lines in `data/dialogue.json`, and their name in `NPCS` in `tools/validate_data.py`.
- Code changes need Unreal closed, a build, and Unreal reopened (see CLAUDE.md). Then rebuild the levels with `greybox/build_matriarch_greybox.py`: it must print `no problems`.
- `python3 tools/validate_data.py` must print `ERRORS: none`. Run the automated tests.
- See it in play. `tools/dev/ghost.py on` hides the player's body; `tools/dev/pilot.py record <name> <seconds> <fps>` saves frames.
- `PY tools/characters/compare_sheet.py <id> docs/devlog/img/<n>-<name>.png <shot>:<crop>:<caption> ...` puts the drawing beside the result.

## Finish

1. Add the drawing to `docs/ASSETS.md` under "Characters by Krish & Hannah Sanghvi".
2. Commit and push (large files go through Git LFS on their own).
3. Tell the owner in a few plain sentences what was made and where to see it, with the comparison picture, and say
   what did not work. A character is not done until they have looked at it.

## What to expect (from the two pilots)

- **Works well:** figures with a clear outline; flat, graphic, front-facing drawings; parts that float free.
- **Works less well:** thin limbs that cross each other (points get owned by the wrong limb, and a leg kinks); figures
  whose side view matters, because every one of these is as deep as a cushion, not as a body.
- **Takes a person's eye:** the colours in the plan, and whether the movements feel like the character.
- **About an hour** for a figure like the pilots once the plan is right; longer for one with many parts.
