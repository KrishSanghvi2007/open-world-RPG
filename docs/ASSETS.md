# Third-party assets

**Rule (owner, 2026-11-18, permanent): free assets only, never anything paid.** Prefer CC0 or a similarly permissive licence. Every outside file used by the game is listed here with its source and licence.

Re-downloadable packs (Fab, Marketplace, Poly Haven) live in `Content/ThirdParty/`, which git ignores. List each one here so it can be restored on another machine.

| Pack | Source / URL | Licence | Installed to | Used for |
|---|---|---|---|---|
| Mannequins (Manny and Quinn, with animations) | Ships with Unreal Engine 5.8: `Templates/TemplateResources/High/Characters/Content`. Restore with `tools/restore_engine_content.sh` | Unreal Engine EULA | `Content/Characters/` (ignored by git; it must stay at this path because the template Blueprints point at it) | Placeholder player and enemy bodies until our own characters exist |
