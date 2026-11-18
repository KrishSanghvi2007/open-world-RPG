# Decisions

This is the owner's control panel. Claude reads it every session.

## Owner decides (Claude must ask first)
- Creative direction: world, story, characters, tone, names
- Art direction and the overall look
- Scope: adding or cutting features, regions or systems
- Anything that costs money (paid assets, plugins, services)
- Deleting or replacing the owner's own work
- Anything hard to undo (engine upgrades, major restructures)

## Claude decides (then logs it here)
Code, architecture, tools, file layout, bug fixes, balance within GDD ranges, asset production details.

---

## Log
Newest first. Format: `date: decision (who): why`

- 2026-11-18: The owner delegated the six open GDD decisions to Claude and Fable. Settled as follows (details in `docs/GDD.md` §13):
  1. Death loses half of each wet pigment, rounded down, with no retrieval run. The first death of a save costs nothing, and Slumber Blue is never lost (it doesn't regrow).
  2. The one-sided forest stays as the signature idea: about 6 hand-placed walk-through backs with a consistent charcoal cross-hatch mark. A one-day grey-box test comes first, with a fallback if testers miss it.
  3. Finishing choices are permanent; a confirmation shows the cost and the immediate effects.
  4. Four core quests; "A Fair Grind" is a stretch goal (the Fixative Flask recipe now unlocks from Ceruse tier 1).
  5. Lighting: Lumen and ray tracing off. One sun plus a tinted sky light per zone, and painted bounce. Upgrade path once the slice holds 60 fps: screen-space bounce, then Lumen only in small interiors.
  6. Keep "Underpainting" as the working title; test a subtitle or alternative before going public.

- 2026-11-18: Use Fable 5.2 as the main model and creative co-designer (owner).
- 2026-11-18: Owner stays hands-off. Claude handles all code and git; the owner makes high-level calls (owner).
- 2026-11-18: Gameplay in C++ with thin Blueprints, and content in JSON → DataTables, so AI agents can read and change it (Claude).
- 2026-11-18: Working title "Underpainting" (Claude proposal, owner may rename).
- 2026-11-18: World concept: an unfinished painted world on the backs of sleeping colossi, under a dying sun. Pigments are the core system (crafting = character building), the story moves in acts that darken, and regions reshape between acts (owner).
- 2026-11-18: Region shifts are before/after level versions, not real-time moving terrain; darkness is tied to story acts, not a real-time clock. Both choices are for feasibility (Claude, owner agreed).
- 2026-11-18: Stylized-painterly art, weighty and deliberate combat (owner).
- 2026-11-18: Engine: Unreal Engine 5.8 on a Mac M2 Pro with 16 GB RAM; Blender for assets (owner).
