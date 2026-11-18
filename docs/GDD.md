# Underpainting: Game Design Document

Working title. Version 0.1, covering the whole game in outline and the first playable region in detail. The matching game data is in `data/`; the numbers here and there are the same numbers.

## 1. Pitch

A god painted the world onto the backs of sleeping colossi, using the sun as its lamp, and walked away before the picture was finished. Now the lamp is dying. Paint only holds its shape in light, so wherever the dark spreads, the world is unmade by pale things called Erasures, and the cold colossi stir beneath it. You are a Limner, a painter-restorer with a lantern and a knife. You harvest pigment from the living canvas, grind it into tools, and take it into yourself. There is never enough, so every unit forces the same choice: spend it on making yourself stronger, or on finishing a piece of the world so that it survives the night.

## 2. Pillars, and the one system they all feed

The owner wants exploration, combat, character building and questing/crafting. Four separate systems is too much for one person. So there is one system, **pigment**, and each pillar is a different way of touching it.

| Pillar | What it does to pigment |
|---|---|
| Exploration | **Finds** it. Each pigment grows in one kind of place, so wanting a colour sends you somewhere. |
| Combat | **Tests** it. Your two chosen pigments decide your moves; enemy colour decides which pigment hurts them. |
| Character building | **Absorbs** it. There is no experience bar. You grow by spending pigment on your palette. |
| Crafting | **Shapes** it. The same pigment becomes weapons, armour and consumables. |
| Questing | **Gives it away.** Quests end by painting something into the world for good, with pigment you could have kept. |

```
        EXPLORE ──finds──▶ PIGMENT ◀──drops── FIGHT
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
         ATTUNE             CRAFT            FINISH
     (your palette:     (gear, oils,     (quest choices:
      passives and       poultices)       a person, a wall,
      combat Strokes)                     a wound)
            │                 │                 │
            └── stronger, better lit ──▶ go further into the dark
```

Light is the second, smaller resource. It decides how expensive it is to *use* pigment where you stand.

## 3. World and lore

**Three layers, one idea.** The world is paint (form), lit by a lamp (energy), on a living canvas (life). All three are running down together. That is the theme: creation is not a single act, it has to be kept up, and nobody is keeping it up.

- **The Unfinished World.** The god worked the way a painter does: a rough drawing first, then thin colour, then finished detail. It stopped partway. Some places are finished, some are half-painted, and some are still only drawings.
- **The Dying Sun.** The lamp is going out. This happens in story acts, never on a real-time clock.
- **The Walking Land.** Each region is one colossus. As they feel the cold they shift in their sleep, and the land on their backs changes shape between acts.

**Erasures** are not evil. They are what happens to a painted world in the dark: the return of the blank canvas. They are the only pure-white things in the game.

**The story** (the full arc, the characters, the mysteries and the endings) is in `docs/STORY.md`. The people of this world call the god **the Hand** and its leaving **the Lifting**; an unfinished thing is **open** and a finished thing is **set**.

**The Limner.** "Limner" is an old word for a painter of illuminated manuscripts; it shares a root with "illuminate". Limners carry a lantern lit from a village Lampstand and are trusted to decide what gets finished.

**The rule that gives the game its ending.** A *finished* thing no longer needs the lamp. It holds its form in the dark. The sun will go out whatever you do. The last act takes place in a world made only of the things you chose to finish.

## 4. The pigment system

### 4.1 The six pigments

Six pigments on three complementary axes (colour-wheel opposites). Each is a real historical pigment, used roughly the way painters used it.

| Pigment | Hex | Found | Rarity | Light cost | Stroke (replaces heavy attack) | Passive at tier 1 |
|---|---|---|---|---|---|---|
| **Hide Ochre** | #D9A441 | Sun-baked skin flakes, village terraces | Common | 6 | **Glaze**: thrust; target takes +15% damage for 6 s and glows | Max Glim +20 |
| **Slumber Blue** | #2F4BA8 | Dream-dew inside her ribs. Never regrows | Rare | 15 | **Sfumato**: 4 m blurred dash through the enemy; slows it 40% for 3 s | Dodge distance +20% |
| **Terre Verte** | #6B8F71 | Moss on her spine | Common | 8 | **Scumble**: low sweep ending in a backstep; refunds 15 Stamina on hit | Stamina regen +20% |
| **Sanguine** | #9E2B25 | Blood wells; Madderbacks | Uncommon | 12 | **Alla Prima**: three-hit flurry, each hit heals 4 HP | Kills restore 8 HP |
| **Bone Black** | #2A2826 | Charred bone spurs | Common | 10 | **Impasto**: slow slam, 1.4x damage, double stagger, leaves a slowing smear | Stagger damage +15% |
| **Ceruse** | #EDE8DC | Dropped by Erasures only | Uncommon | 20 | **Erase**: fast swipe that cancels an enemy's attack and deletes projectiles | Harmful effects 50% shorter, but max HP -15 (lead is poison) |

The pairs: **Ochre ↔ Slumber Blue**, **Terre Verte ↔ Sanguine**, **Bone Black ↔ Ceruse**.

### 4.2 Harvesting (sources)

- Pigment comes from **nodes** (2 units each) and from **enemy drops**.
- Fresh pigment is **wet**. Resting at a **Lampstand** (the rest point: a village lamp, or a pilot flame in the wild) **dries** it. Dry pigment is safe. If you die, you wake at the last Lampstand and lose half of each wet pigment you were carrying, rounded down, for good. There is no body to run back to. Two mercies: the first death of a game costs nothing (Hesper explains the rule instead), and Slumber Blue is never lost, because it cannot be found again.
- Resting also brings back enemies and regrows nodes, but **only nodes standing in light of 0.5 or more**. Paint does not hold in the dark. So Ochre, Terre Verte and Sanguine renew, Slumber Blue never does, and Ceruse must be taken from Erasures.
- First pass through the slice, without farming, yields about 60 units: Ochre 8, Terre Verte 10, Bone Black 12, Sanguine 9, Ceruse 10, Slumber Blue 10.

### 4.3 The three sinks

1. **Attunement** (section 4.4): 3 units for tier 1; 6 units plus 2 of the complement for tier 2.
2. **Crafting**: 1 to 4 units per recipe, plus materials (hide, bone, chitin, graphite, and linseed oil as the binder). Two stations in the village: the **Grinding Slab** (consumables) and the **Armature Bench** (gear). There are no coins; where people trade, they trade pigment.
3. **Finishing**: 3 to 5 units per quest choice.

Buying everything would cost roughly three times what one pass provides. The scarcity is intended: it is what makes the choices real.

### 4.4 Attunement: the palette (character building)

- The Limner has a **palette with 2 wells** (a third opens in Act 2).
- Spend dry pigment at a Lampstand to buy a node. Each pigment has 2 tiers in the slice (12 nodes total; a third tier is reserved for the full game).
- **Tier 1** (3 units) lets you place that pigment in a well, unlocks its Stroke, and grants its passive.
- **Tier 2** (6 units + 2 of its complement) deepens it. The complement cost is "tempering": painters mix a little of the opposite colour in to control a strong one. It means no build can ignore the other end of its axis.
- Passives and Strokes apply **only for the 2 pigments currently in wells**. Wells can be swapped freely at any Lampstand. Your build is which two colours you carry.

| Node | Effect |
|---|---|
| Ochre T2, Golden Hour | Perfect dodge restores 10 Glim (was 5); Glaze glow doubles |
| Terre Verte T2, Verdaccio | Poultices heal 60 (was 40); Scumble refunds 25 Stamina |
| Sanguine T2, Wet-on-Wet | Alla Prima heals 6 per hit; a kill with it refunds its Glim |
| Bone Black T2, Lamp-Black Weight | Smear lasts 5 s; Impasto cannot be interrupted |
| Ceruse T2, Clean Ground | A successful Erase restores 10 Glim; lead toll falls to -10 HP |
| Slumber Blue T2, Lucid | Dodge invulnerability 0.36 s (was 0.30); Sfumato slow lasts 5 s |

### 4.5 Light as a resource: Glim

The lantern holds **Glim** (100 at the start). The word is deliberately not "Lumen", which is the name of an Unreal lighting feature and would confuse the code.

- Every zone has an **ambient light** value from 0 (dark) to 1 (full dusk light).
- **Stroke cost = the pigment's light cost x (1 - ambient light).** In the village Strokes are free. In the half-lit forest they cost half. In the dark they cost full price. The dark is not a damage-over-time punishment; it simply makes your colours expensive.
- **Flare** (20 Glim, always full price): 8 seconds of light in an 8 m radius. Inside it, ambient light counts as at least 0.5, sketched objects become solid, hidden outlines appear, and some enemies falter.
- **Getting Glim back:** resting (full), killing an Erasure (+10; they are full of swallowed light), a perfect dodge (+5), Lamp Oil (+40).

Killing Erasures returns light and Ceruse; nothing else does. The player is pulled toward the dangerous, dark places because that is where the light is hiding.

## 5. Combat

**Feel:** weighty and deliberate. Every swing commits you. You win by reading, not by reflexes.

**Player numbers:** 100 HP, 100 Stamina, 100 Glim. Stamina regenerates at 30 per second after a 0.8 s pause. Dodge costs 20 Stamina, lasts 0.6 s and cannot be hit for the first 0.30 s. Sprinting costs 10 per second. Healing is the **Moss Poultice** (40 HP over 4 s); resting tops you up to 2 for free and more must be crafted (carry up to 5).

**Moves:** light attack (never changes, so the basics stay learnable), heavy attack, dodge, Flare, use item, swap active well. **Holding heavy performs the Stroke of the active well**, costing the weapon's heavy Stamina plus Glim. That is the whole move list.

**Weapons are painter's tools:**

| Weapon | Light / Heavy damage | Speed | Stamina L / H | Reach | Note |
|---|---|---|---|---|---|
| Limner's Knife (start) | 14 / 26 | 1.0 | 12 / 25 | 1.6 m | Palette knife |
| Mahlstick | 16 / 30 | 0.9 | 14 / 28 | 2.4 m | Painter's steadying staff |
| Bone Scraper | 22 / 40 | 0.75 | 18 / 34 | 2.0 m | 1.3x stagger |

**The Contrast rule (colour theory as the damage system).** Every enemy is visibly one hue. Hit it with a Stroke of the **complementary** pigment: +30% damage and +50% stagger. Hit it with its **own** hue: -30% ("muddy"). The player never needs a bestiary; the enemy's colour is the tooltip.

**Stagger.** Each enemy has Poise. Damage dealt builds toward it; reaching it staggers the enemy for 2 s.

**Enemy design principles**

1. **No wind-up shorter than 0.6 s** for standard enemies. Recovery is always at least as long as the wind-up, so every dodge earns a clear opening.
2. **Two telegraphs only.** A **warm rim** of light along the attacking limb means "dodge it, or Erase it". A **white flash** with a scraping sound means "this cannot be cancelled: move".
3. **Every enemy teaches one thing.** Three attacks each, at most.
4. **Never more than 3 enemies attacking at once**, and at most 6 active (also a performance limit).
5. **White is reserved for Erasures** and full saturation for things that matter in a fight. The background never competes.

**The slice's enemies**

| Enemy | Zone | HP / Poise | Hue → weak to | Teaches | Drops |
|---|---|---|---|---|---|
| **Blotter** (Erasure) | Forest, Vault | 60 / 30 | White → Bone Black | Dodging; fearing the dark. Its lunge drains Glim. It will not enter bright light. | Ceruse, +10 Glim |
| **Madderback** (giant blood-tick) | Forest | 90 / 50 | Red → Terre Verte | Positioning: its charge gets stuck in trees for 2 s. | Sanguine, Tick Chitin |
| **Pentiment** (a discarded draft of a person) | Vault | 75 / 35 | Blue → Ochre | Using light: a Flare cancels its teleport and staggers it. | Graphite Shard, Slumber Blue |
| **Old Palimpsest** (boss) | Vault, the Heart | 600 / 120 | Phase 1 White → Bone Black. Phase 2 Red → Terre Verte | Everything above | Ceruse 4, Slumber Blue 2 |

**The boss** is an Erasure wearing layers of everything it has rubbed out, curled against the Matriarch's heart. At 50% HP it scrapes its own surface away and turns from white to blood red, which reverses its weakness. It is the exam for the Contrast rule: a player who has only invested in one axis must fall back on the basics for half the fight. The heart beats every 20 seconds, lighting the arena to 0.5 for 4 seconds, so Strokes are half price on the beat. Full attack timings are in `data/enemies.json`.

## 6. Exploration

Three states of world, three sets of rules. Each is a small, fixed list, not a physics simulation.

**Painted** (the village). Solid, safe, saturated. Ambient light 1.0: Strokes are free. Nodes regrow. This is what the player is fighting for, so it has to be the most beautiful place in the game.

**Half-painted** (the forest). The signature idea: **the god only painted what it could see from where it stood.** Everything is finished on the side facing the sun and left as bare canvas and charcoal line on the far side.
- Some unpainted backs are **not solid**. Looking back toward the sun shows you which cliffs and trunks are only a front, and you can walk through them from behind.
- To keep this fair, passable backs are hand-placed and always carry the same mark (a charcoal cross-hatch patch). There are about 6 in the forest. The main route uses the trick exactly once, next to a Lampstand, as its lesson; every other use hides something optional.
- Ambient light 0.5. Nodes regrow in sunlit patches only.
- This also saves work: the far side of most assets needs no texture.

**Sketch** (the dungeon). Grey paper and graphite line.
- **Gravity is 0.6x and there is no fall damage.** Jumps are long and floaty.
- **Sketched objects are solid only while lit.** A Flare makes platforms and bridges real for 8 seconds. A Fixative Flask makes one real permanently.
- Ambient light 0. Nothing regrows. Slumber Blue is found only here.

**Lore discoverables** come in four kinds, written in `docs/STORY.md` section 9: the Hand's marginalia (in sketch zones, revealed by a Flare), Odile Vane's field notes (along her route from the village to the Heart), the village's ledgers and logs, and pentimenti.

**Pentimenti.** A pentimento is the ghost of an earlier idea showing through a finished painting. In this world they are faint outlines of what used to be, or was nearly, there. A Flare reveals them. They point to caches, and after a region shift they mark where things stood.

## 7. Questing

Quests are short (3 to 8 steps), text only, and almost all end the same way: **something in the world is unfinished, and you decide what colour it becomes, or whether it is finished at all.** The cost comes out of the same pocket as your own growth. "Leave it" is always a real option with its own reward, never a failure.

**Choices are permanent.** There is one save and no undo for the world. Before a choice is locked in, a confirmation shows its cost and its immediate consequences in plain words (later-act consequences stay unseen). Builds remain forgiving: palette wells can be swapped at any Lampstand.

The slice has four core quests and one stretch quest:

1. **Dusk on the Shoulder** (main; Hesper Vane, lampwright). The tutorial: harvest Ochre, craft Lamp Oil, relight the village Lampstand, attune for the first time, receive the lantern.
2. **Cold Under the Ribs** (main; Hesper). Descend the spine, enter the Vault, defeat Old Palimpsest. Beside the boss's hollow the wall shows two charcoal studies of a figure with a lantern and a knife, and a blank where a third was (see `docs/STORY.md`). Then fill that blank on the Heart wall: **Sanguine (5)** makes her blood run hot (richer blood wells, 2 more Madderbacks); **Terre Verte (5)** thickens the moss (forest light 0.55 instead of 0.4, 2 fewer Blotters); **leave it bare** and keep your pigment plus 4 Ceruse, at a cost that arrives in Act 2.
3. **The Unseen Side** (side; Corin Half-Seen). Corin was painted from the front only and has spent his life walking backwards to face the sun. Finish him in **Terre Verte (3)** and he becomes the forest's warden (moss nodes yield +1). Finish him in **Bone Black (3)** and he becomes the village's grim gatekeeper (no Blotters reach the village after the shift). **Leave him** and, because only an unfinished man can see the sketch beneath the paint, he shows you a hidden cache of 2 Slumber Blue.
4. **The Tick-Warden's Ledger** (side; Ilka Thrum). The village wants the Madderback nest burned. Ilka's records show the blood wells only stay open because the ticks bite. **Burn it**: half the Madderbacks, but wells yield 1 Sanguine instead of 2. **Dress the wells with Terre Verte (3)**: same danger, wells yield 3, and Ilka trades. The parasite is the supply chain.
5. **A Fair Grind** (side, **stretch goal**; Bartram Sedge, colourman). He wants to whitewash the lowest terrace so it reflects more light. **Ceruse (3)** is brilliant (+0.25 light there, more linseed oil) and will poison the families living in it by Act 2. **Ochre (4)** is duller (+0.1) and safe. Built only after the other four are finished; nothing else depends on it.

Choices set named flags in the save. Finished things persist into later acts.

## 8. Acts and region shifts

| Act | Name | Light ceiling | What it adds |
|---|---|---|---|
| 1 | Dusk | 1.0 | The Sleeping Matriarch. **The vertical slice.** |
| 2 | Afterglow | 0.7 | A second colossus that is awake and wading. Third palette well. Earlier choices come due. |
| 3 | Lampblack | 0.4 | The lamp itself. Half-painted places decay to sketch unless finished. |
| Finale | Varnish | 0 | Varnish is the last coat that seals a painting as it is. Only what you finished remains. |

**Region shifts.** Between acts, and once inside Act 1 to prove the idea, a colossus moves. Technically this is a swap between an "A" and a "B" version of each zone's level. The camera never sees the land move: the screen shakes, the Limner grabs hold, the view cuts to her vast eye half-opening, and then the B levels are loaded.

**The Turning (the slice's shift)**, triggered by finishing *Cold Under the Ribs*:

- **Forest:** her spine arches. The gorge that was the way down closes. Five vertebrae rise into the **Stair of Spines**, a direct route from the dungeon door to the village, with a branch to the **Nape Overlook**, which was visible but unreachable all game. There: a cache, and the first sight of the next colossus on the horizon.
- **Village:** her shoulder drops. The lowest terrace, the Hem, falls into shadow and becomes half-painted. Blotters appear there unless Corin guards it.
- **Vault:** her heart is warm. A first amber glaze of colour spreads over the grey.

**Light is moved, not made.** Before the shift the three zones hold 1.0 + 0.5 + 0.0 = 1.5 light. After, 0.85 + 0.4 + 0.25 = 1.5. Warming the heart darkened the village. Each act lowers the total. Only finishing things gets around that rule.

## 9. Art direction

**The look:** hand-painted, visible brushwork, strong coloured light. Closer to a gouache illustration than to a film.

**Rules**

1. **Three values per view** (light, middle, dark), as in a painter's tonal study. If a screenshot does not read in greyscale, it is not finished.
2. **Shadows are never black or grey.** They are tinted toward the complement of the light: warm dusk light, violet-teal shadow.
3. **Saturation is a budget.** Scenery stays under about 70% saturation. Full saturation is for pigment nodes, Strokes and enemies.
4. **Pure white belongs to Erasures.** Nothing else in the game is white.
5. **Brushwork is in the textures and silhouettes**, not in an expensive screen filter: hand-painted textures at 1K to 2K, ragged brush-shaped edges on foliage and cloth, simple shading.

**Palette by zone**

| Zone | State | Colours | Technique reference |
|---|---|---|---|
| Varnish (village) | Painted | Ochre #D9A441, madder rose #C4574B, cream #F2E3C0, teal shadow #2E5E6E, violet shadow #4A3558 | Glazing: thin warm layers over a light ground |
| The Scumblewood (forest) | Half-painted | Terre verte #6B8F71, raw umber #6B4F3A, bare canvas #D8CBB0, charcoal #3A3A3A, sanguine accents #9E2B25 | Scumbling: broken, dry colour that lets the layer beneath show |
| The Grisaille Vault (dungeon) | Sketch | Paper #E6E1D6, greys #8C8A85 and #4B4A48, graphite #2E2E30, one blue #2F4BA8, heart glow #E8923A | Grisaille: the monochrome underpainting itself |

**How "unfinished" looks.** Half-painted: where paint stops, it stops in a ragged brush edge; beyond is woven canvas with the charcoal drawing still on it. Sketch: flat unlit paper surfaces with dark outlines, hatching instead of shadow, and the god's working marks left in view (perspective lines running to a vanishing point, measuring ticks, a figure drawn three times in slightly different poses). The dungeon's greatest asset is that it is cheap: almost no textures.

**Lighting rule (decided).** Lumen and ray tracing are switched off for the whole project. Lighting is built from four cheap parts:
1. One moving light, the sun, which is the only light that casts real-time shadows.
2. One sky light per zone for soft fill, tinted with that zone's shadow colour.
3. Bounce light and glow are painted into textures and materials, the way a painter would do it. No baked lightmaps in the slice: with an A and a B version of every level they would double build time for little gain.
4. A light post-process per zone: colour grade, shadow tint, and ink outlines in sketch zones. No full-screen "oil paint" filter.

Lanterns, Lampstands and Flares are small lights without shadows. The Vault uses unlit materials, so it costs almost nothing.

*Later upgrade path, only once the finished slice holds 60 fps with room to spare:* first try a screen-space bounce-light effect if the engine version offers one (cheap, but can flicker); then, at most, Lumen switched on inside single small rooms such as house interiors in later acts. Neither is to be attempted during the slice, and each must be measured on the M2 Pro before it is kept.

**Performance (M2 Pro, 16 GB).** Target 60 fps at 1080p. No more than 6 active enemies. Three small levels streamed separately, never one large open map.

## 10. Audio direction

- **The Matriarch breathes.** A slow, very low swell about every 12 seconds, everywhere in the region. Before the Turning it shudders; after, it is steady.
- **Music thins with the paint.** Village: small warm ensemble. Forest: the same melody on two instruments. Vault: one instrument, then only pencil-scratch and paper sounds.
- **Every enemy attack has its own sound in the wind-up**, so fights can be read by ear. White-flash attacks share one dry scraping sound.
- **Glim below 20** is signalled by the lantern's hiss dropping out.
- No voice acting. Text, with a short vocal sound per character.

## 11. Vertical slice specification

**Goal:** prove the loop (harvest → craft/attune → fight → finish → shift) in **45 to 90 minutes**.

| Beat | Place | Minutes | Contents |
|---|---|---|---|
| 1 | Varnish | 10 to 15 | Quest 1. First harvest, craft, attune. Both stations. 1 Lampstand. 4 Ochre nodes. No combat. |
| 2 | The Scumblewood | 20 to 30 | 6 Madderbacks, 6 Blotters. Corin, Ilka. One-sided walls. 2 Lampstands. Nodes: 5 Terre Verte, 3 Sanguine, 3 Bone Black. |
| 3 | The Grisaille Vault | 15 to 25 | 6 Pentiments, 4 Blotters. Flare platforming in the Cartoon Gallery. 1 Lampstand. Nodes: 3 Bone Black, 3 Slumber Blue. Boss. The Heart choice. |
| 4 | The Turning and the return | 5 to 15 | Shift sequence. Stair of Spines, Nape Overlook, changed village. Quest results visible. End card. |

**Exact contents:** 1 region, 3 zones, each in an A and B version. 6 pigments. 12 attunement nodes. 2 palette wells. 3 weapons, 3 armours, 5 consumables, 1 lantern upgrade, 5 materials. 10 recipes. 3 enemy types and 1 two-phase boss. 4 core quests (2 main, 2 side) plus 1 stretch side quest. 4 named characters. 4 Lampstands. 1 region shift.

**Build order (each step is playable):** (1) a grey-box room with movement, Stamina, dodge, light and heavy attacks and one Blotter; (2) pigments, the palette, Glim and two Strokes; (3) harvesting, crafting, Lampstands; (4) the three zones in grey-box with their rules; (5) the remaining enemies and the boss; (6) quests and flags; (7) the shift; (8) art and audio.

## 12. Out of scope for the slice

Acts 2 and 3, any second colossus, the third palette well and tier 3 nodes, mounts, swimming, climbing, stealth, ranged weapons, blocking or parrying beyond the Erase stroke, branching dialogue trees, companions, voice acting, a world map screen (the region is small enough for signposts), weather, a real-time day and night cycle, real-time moving terrain, multiplayer, difficulty settings, and any respec of world choices.

## 13. Risks and settled decisions

**Where I pushed back on the concept**

- **"Open world" should mean an open region.** A seamless open world is beyond one person and this machine. Each colossus is a compact place of three or four zones with several routes through it. The colossi are the world map.
- **"Looser physics" in sketch zones is a trap** if it means simulation: it would be unpredictable and hard to tune. It is exactly three authored rules instead (section 6).
- **The familiar retrieval-run death rule is gone.** Dropping your currency where you die and running back for it is another game's signature. Wet and dry pigment gives the same tension with less to build.
- **Two versions of every level doubles the art** if done naively. Only the forest truly changes layout (about 30% of its pieces move). The village and vault change lighting, materials and a few props.
- **Darkness as a punishment is not fun.** Light changes prices rather than hurting you, and the dark places are where Glim and the rarest pigments are.

**Decisions (settled 2026-11-18; the owner delegated these six)**

1. **Dying costs half of each wet pigment, rounded down, with no retrieval run.** The first death costs nothing and Slumber Blue is never lost. *Why:* the plain rule had two flaws when tested: a first death before the player understands "wet" feels arbitrary, and permanently losing a pigment that never regrows could lock a player out of a build. Rounding down also means carrying a single unit is always safe.
2. **The one-sided forest is in, as the game's signature idea.** Passable backs are hand-placed, always marked, about 6 in the forest, and the main route needs the trick only once (section 6). *Why:* unmarked walk-through walls would read as a bug, so the idea is kept but fenced. It still gets a one-day grey-box test first; if testers cannot find the marked lesson wall, it falls back to ordinary half-painted patches and nothing else in the design changes.
3. **Finishing choices are permanent.** One save, no repainting; a confirmation shows cost and immediate consequences first (section 7). *Why:* permanence is the theme, and a paid undo would turn every choice into a loan. The confirmation protects a beginner from mis-clicks without softening the choice.
4. **Four core quests; *A Fair Grind* is a stretch goal.** *Why:* it adds flavour but no new mechanic. Checking the cut found one dependency: it used to unlock the Fixative Flask recipe. That unlock now comes from attuning Ceruse (tier 1), so the quest can be dropped without touching anything else.
5. **Lumen is off; lighting follows the rule in section 9.** *Why:* the painterly look comes from painted colour, not simulated bounce, and 16 GB of shared memory is the tightest limit on this machine. Baked lighting was also rejected for the slice because of the A and B levels. A measured upgrade path is named but postponed.
6. **The title stays "Underpainting" as a working title.** *Why:* it names the theme and the mechanic. Its weakness is that it is a common art term, so searches will find painting tutorials first; before any public page, test a subtitle or an alternative. Nothing in the project depends on the name.

**Remaining risks to watch**

- **Readability of six colours for colour-blind players.** Each pigment and enemy hue needs a second cue (a symbol on the well, a pattern on the enemy). Plan it from the first UI mock-up.
- **Economy tuning.** The "about 60 units, about three times that in costs" ratio is a first guess. Expect to adjust node counts after the first full playthrough.
- **Animation is the real cost of weighty combat.** Six Strokes across three weapons is 18 animations if each is unique. Start with one animation per Stroke shared across all weapons.
