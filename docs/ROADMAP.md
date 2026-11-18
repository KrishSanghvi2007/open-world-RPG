# Roadmap

Set by the owner on 2026-11-18. **Build the game, not the art.** Systems, exploration and story come first; art is placeholder (mannequins, simple shapes, plain materials) except for style probes.

Every milestone ends with a "done when" test the owner can do in under 5 minutes, and a devlog entry in `docs/devlog/` with 3 to 5 screenshots.

## Standing rules

- **Gameplay in C++, content in `data/*.json`.** Dialogue and quests are data-driven. Automated tests for core systems where practical (`Underpainting.*` in the editor's test runner).
- **Style probes.** The first time a new kind of thing is built, make one quick probe of it at look-test quality (about 1 hour at most), screenshot it for the devlog, then go back to placeholders. Never polish beyond that.

  | Kind | Probe |
  |---|---|
  | Environment corner | Done: `docs/looktest/` (three styles) |
  | Prop | Done: Lampstand, Grinding Slab (in the look test) |
  | Player character | To do |
  | NPC | To do |
  | Enemy | To do |
  | Boss | To do |
  | Interior | To do |
  | UI screen | To do |
  | Item icon | To do |

- **Autonomy.** Claude does not wait for the owner. Choices that come up are decided (recommended, reversible option), logged in `docs/QUESTIONS.md`, and work continues. Claude stops only for spending money, deleting the owner's work, or anything irreversible.
- Commit and push after each task. Run `python3 tools/validate_data.py` after data changes.
- If Unreal crashes or disconnects, reopen it and continue. If something is blocked, note it and move to the next task.
- `docs/devlog/MORNING.md` is kept ready for the owner's return: what is playable, how to test it in 10 minutes, what is broken, and the open questions.

## Scope and parallel tracks (owner, 2026-11-18)

**Scope:** the Act 1 vertical slice only, M3 to M9 below. Acts 2 and 3 are **not** built, and there is no full art pass yet.

Two tracks run alongside, worked on between milestones:

| Track | What | Where | State |
|---|---|---|---|
| **Story on paper** | Outlines of the Act 2 and 3 regions, quests, characters and key dialogue scenes; foreshadowing added back into Act 1's content. | `docs/story/` | To start after M3 |
| **Style kit** | A reusable Glaze master material, post-process and haze, applied automatically to all placeholder geometry so playtests look coherent and painterly without real models. No new models beyond first-of-kind style probes. | `tools/stylekit/`, `/Game/StyleKit/` | To start after M3 |

## Milestones

### M0. Foundations (done)
Project, MCP connection, data structs and loader, grey-box of the three zones, look test, first combat (stamina, dodge, light and heavy attacks, one Blotter).

**Done when:** open `/Game/Maps/Dev/L_Combat_Greybox`, press Play, walk to the white figure, dodge its warm-lit attack with Left Ctrl and kill it with the mouse buttons.

### M1. Story (done)
`docs/STORY.md`: the main arc across all acts, the Limner's personal story, the major characters and what they want, the Sleeping Matriarch's story and secrets, the central mysteries and how they are revealed, endings driven by what the player finished, and the lore discoverables. Consistent with `docs/GDD.md`; GDD changes suggested, minor ones applied.

**Done when:** the one-page summary at the top of `docs/STORY.md` can be read in 5 minutes and tells the whole story, ending included.

### M2. Walking skeleton (in the village) (done; not yet played by hand)
One thin, connected, playable loop with placeholders. Crude is fine; connected is the point.

1. Move (done) and fight one Blotter (done; bring it into the village level).
2. Inventory, and harvesting an Ochre node.
3. Craft one item at the Grinding Slab.
4. Spend pigment on one attunement node in a skill-tree screen.
5. Talk to one NPC (Hesper Vane) through data-driven dialogue.
6. Finish one short quest.
7. Save and load.

**Done when:** in `L_Matriarch_Village_A`, in one sitting of 5 minutes: talk to Hesper, harvest Ochre, craft Lamp Oil, relight the Lampstand, buy the Ochre attunement, kill the Blotter, see the quest complete, save, quit, load, and find everything as you left it.

### M3. Exploration (high priority)
Traversal, landmarks, secrets, discoverables (lore from `docs/STORY.md`), the sketch-zone rules (low gravity, solid only while lit, Flare, Fixative Flask), the one-sided forest and its passable backs, pentimenti, a compass or signposts, rewards for curiosity, zone exits that connect the three levels.

**Done when:** walk from the village through the Furrow to the Rib Gate and into the Vault without the editor, find one secret cache through a passable back, and cross the Cartoon Gallery using a Flare.

### M4. Combat depth
Glim and light as a resource, Strokes for each pigment, the Contrast rule, lock-on, the Moss Poultice, weapon and armour swapping, death and Lampstand respawn with wet-pigment loss.

**Done when:** in the fight room, kill a Blotter with Bone Black Strokes and see the +30% damage number; die once and wake at the Lampstand.

### M5. Enemies and the boss
Madderback, Pentiment, the three-attackers limit, and Old Palimpsest with its two phases and the heartbeat.

**Done when:** each enemy can be fought in its own test room, and the boss can be beaten or lost to in the Heart.

### M6. Quests and dialogue
All slice quests from JSON, branching dialogue, permanent choices with confirmation, world flags and their consequences in the levels.

**Done when:** play "The Tick-Warden's Ledger" twice with different choices and see different results in the world.

### M7. Crafting and attunement depth
All 10 recipes, both stations, all 12 attunement nodes with their effects, palette wells, wet and dry pigment.

**Done when:** craft every recipe from a debug-filled inventory and feel two different two-pigment builds.

### M8. The region shift
The Turning: "B" versions of the three levels, the Stair of Spines, the Nape Overlook, the changed village, quest results visible.

**Done when:** finish the boss, watch the Turning, and walk the new route back to a changed village.

### M9. The full UI
Menus, HUD, inventory, journal, palette, map or compass, settings with rebinding, one UI style probe.

**Done when:** every system above can be used without debug keys.
