# Game data: schemas

Content for the vertical slice (the Sleeping Matriarch, Act 1). The design it implements is `docs/GDD.md`; the two must always agree. If you change a number here, change it there.

## Conventions

- Each file is a JSON array of row objects for one Unreal DataTable.
- Every row has `Name`: the unique row ID (PascalCase, no spaces). Unreal's DataTable JSON import uses it as the row key, so it is **not** a member of the row struct.
- Every other key maps to a `UPROPERTY` of the same name in a `USTRUCT` deriving from `FTableRowBase`. Suggested struct names are given per file.
- IDs that point at another table are stored as strings (`FName` in C++). The cross-reference lists below say which table each one points at.
- No booleans are used (avoids the `b` prefix mismatch on import). Yes/no facts are enums or empty strings.
- Enum-like strings are listed with their allowed values. Create a `UENUM` for each, or keep them as `FName`.
- An empty string `""` or empty array `[]` means "none".
- Units: time in seconds, distance in metres (multiply by 100 for Unreal units), `MoveSpeed` already in Unreal units per second, percentages as whole numbers (20 = 20%) unless the field says "0 to 1".
- Three small structs are reused across files:
  - `FIdCount`: `Id` (FName), `Count` (int32).
  - `FDropEntry`: `Id` (FName), `Count` (int32), `Chance` (float, 0 to 1).
  - Nested structs specific to one file are described under that file.
- NPCs (`HesperVane`, `BartramSedge`, `CorinHalfSeen`, `IlkaThrum`), crafting stations (`GrindingSlab`, `ArmatureBench`), world flags and level names do not have tables yet. They are plain IDs for now.

## pigments.json (`FPigmentRow`)

The six pigments. A pigment is a crafting material, the currency for attunement, and a combat style all at once.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Pigment ID. Used by every other file. |
| `DisplayName` | string | Name shown to the player. |
| `Source` | string | Where it comes from, in the fiction. |
| `SourceZones` | FName[] | Zone rows in `regions.json` where it can be obtained. |
| `ColourHex` | string | The pigment's colour, `#RRGGBB`. Drives UI, VFX and weapon tint. |
| `Hue` | enum | `Orange`, `Green`, `Red`, `Black`, `White`, `Blue`. Compared against an enemy's `Hue` for the Contrast rule. |
| `Complement` | FName | The opposite pigment (another row in this file). Pairs: Ochre/SlumberBlue, TerreVerte/Sanguine, BoneBlack/Ceruse. |
| `Rarity` | enum | `Common`, `Uncommon`, `Rare`. |
| `LightCost` | int | Glim spent per Stroke at ambient light 0. Actual cost = `LightCost x (1 - AmbientLight)`, rounded up. |
| `Regrowth` | enum | `OnRest` (nodes in light of 0.5 or more regrow when the player rests), `Never` (nodes are one-time), `DropOnly` (no nodes; enemy drops only). |
| `DeathLoss` | enum | `Half` (on death the player loses half of the wet units carried, rounded down) or `None` (never lost; used for pigments that cannot be regained). The first death of a save loses nothing. |
| `CraftingUses` | FName[] | Items in `items.json` whose recipe uses this pigment. |
| `AttunementPassive` | string | Player-facing text of the tier 1 passive. The numbers live in `attunements.json`. |
| `StrokeName` | FName | ID of the Stroke that replaces the heavy attack while this pigment is the active well. |
| `StrokeDamageMultiplier` | float | Multiplier on the weapon's `HeavyDamage` (per hit, for multi-hit Strokes). |
| `StrokeWindup` | float | Wind-up of the Stroke in seconds. |
| `CombatModifier` | string | Full description of what the Stroke does, with its numbers. |
| `LoreLine` | string | Flavour text. |

## items.json (`FItemRow`)

Everything the player can hold that is not a pigment. All rows share one flat struct; fields that do not apply are 0 or `None`.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Item ID. |
| `DisplayName` | string | Name shown to the player. |
| `Type` | enum | `Weapon`, `Armour`, `Consumable`, `Material`, `Key`, `Upgrade`. |
| `Description` | string | Player-facing text. |
| `Source` | string | How it is obtained (notes for designers). |
| `LightDamage` / `HeavyDamage` | int | Weapons: damage of a light and a heavy attack. |
| `AttackSpeed` | float | Weapons: animation play-rate multiplier (1.0 = baseline). |
| `StaminaCostLight` / `StaminaCostHeavy` | int | Weapons: Stamina per attack. A Stroke costs `StaminaCostHeavy`. |
| `Reach` | float | Weapons: reach in metres. |
| `StaggerMultiplier` | float | Weapons: multiplier on stagger damage dealt. |
| `DamageReduction` | float | Armour: fraction of incoming damage removed, 0 to 1. |
| `EffectType` | enum | `None`, `GlimCapacity`, `RestoreGlim`, `HealOverTime`, `FixTarget`, `DamagePercent`, `GravityScale`, `StaminaRegenPercent`. |
| `EffectValue` | float | Size of the effect (meaning depends on `EffectType`). |
| `EffectDuration` | float | Duration in seconds; 0 = instant or permanent. |
| `MaxStack` | int | Carry limit. |

## recipes.json (`FRecipeRow`)

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Recipe ID, `Recipe_<OutputItem>`. |
| `OutputItem` | FName | Row in `items.json`. |
| `OutputCount` | int | How many are made per craft. |
| `Inputs` | FIdCount[] | Each `Id` is a row in `pigments.json` or `items.json`. |
| `Station` | enum | `GrindingSlab` (consumables) or `ArmatureBench` (gear). Both are in Varnish. |
| `UnlockType` | enum | `Start` (known from the beginning), `Quest` (on completing the quest), `Attunement` (on buying the node). |
| `UnlockId` | FName | Row in `quests.json` or `attunements.json`; empty for `Start`. |
| `Notes` | string | Designer notes. |

## attunements.json (`FAttunementRow`)

The palette: the character-building nodes. Two tiers per pigment in the slice. A pigment's effects apply only while it sits in one of the player's 2 palette wells.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Node ID, `<Pigment>_T<Tier>`. |
| `DisplayName` | string | Name shown to the player. |
| `Pigment` | FName | Row in `pigments.json`. |
| `Tier` | int | 1 or 2. Tier 1 also unlocks placing the pigment in a well, and its Stroke. |
| `EffectType` | FName | Machine-readable ID of the main numeric effect. |
| `EffectValue` | float | The number for `EffectType`. |
| `EffectText` | string | Full player-facing text, including any secondary effect. |
| `CostPigment` | FName | Pigment spent to buy the node (always its own pigment). |
| `CostAmount` | int | 3 for tier 1, 6 for tier 2. |
| `TemperPigment` | FName | Tier 2 only: the complement pigment that must also be paid. |
| `TemperAmount` | int | 2 for tier 2, 0 for tier 1. |
| `Prerequisites` | FName[] | Nodes in this file that must be owned first. |

## enemies.json (`FEnemyRow`)

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Enemy ID. |
| `DisplayName` | string | Name shown to the player. |
| `Family` | enum | `Erasure`, `Parasite`, `Draft`. |
| `Tier` | enum | `Standard` or `Boss`. |
| `Zones` | FName[] | Zone rows in `regions.json`. |
| `Hue` | enum | Same values as pigment `Hue`. The enemy's visible colour. |
| `MaxHP` | int | Health. |
| `Poise` | int | Stagger threshold. Stagger damage equals damage dealt (times modifiers). When the total reaches `Poise` the enemy is staggered for 2.0 s and the total resets. The total also resets after 5 s without a hit. |
| `MoveSpeed` | float | Unreal units per second. |
| `AggroRadius` / `LeashRadius` | float | Metres. Notices the player inside the first; gives up outside the second. |
| `GlimOnKill` | int | Glim returned to the player's lantern on death. |
| `Weaknesses` | FName[] | Pigments whose Strokes deal +30% damage and +50% stagger damage. |
| `Resistances` | FName[] | Pigments whose Strokes deal -30% damage. |
| `Phase2HPPercent` | int | Boss only: HP percentage at which phase 2 begins. 0 = no second phase. |
| `Phase2Hue` | enum | Boss only: hue during phase 2. |
| `Attacks` | FEnemyAttack[] | See below. |
| `Drops` | FDropEntry[] | Each `Id` is a row in `pigments.json` or `items.json`. |
| `BehaviourNotes` | string | AI notes and special rules. |

`FEnemyAttack`: `AttackName` (FName), `Phase` (int: 1, or 2 for boss phase 2 only), `Windup` (float: seconds the tell is visible before the hit can land), `Active` (float: seconds the hitbox is live), `Recovery` (float: seconds the enemy cannot act afterwards; the punish window), `Damage` (int), `Telegraph` (enum: `WarmRim` = can be dodged or cancelled with Erase; `WhiteFlash` = must be dodged), `Notes` (string).

Weakness rule for the boss: in phase 1 only the weakness and resistance that match hue `White` apply (BoneBlack, Ceruse); in phase 2 only those that match `Phase2Hue` apply (TerreVerte, Sanguine). In general, a weakness is always the complement of the enemy's current hue, and a resistance is always the pigment of the same hue.

## quests.json (`FQuestRow`)

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Quest ID. `MQ_` = main, `SQ_` = side. |
| `DisplayName` | string | Name shown to the player. |
| `QuestType` | enum | `Main` or `Side`. |
| `SliceScope` | enum | `Core` (must be built for the slice) or `Stretch` (built only if time allows; nothing else may depend on it). |
| `Act` | int | Story act (all are 1 in the slice). |
| `Giver` | FName | NPC ID. |
| `Zone` | FName | Zone row in `regions.json` where most of the quest happens. |
| `Summary` | string | Journal text. |
| `Steps` | string[] | Objectives, in order, as the journal shows them. |
| `Objectives` | FQuestObjective[] | What the game checks for each step. See below. |
| `Prerequisites` | FName[] | Quests in this file that must be complete first. |
| `Choices` | FQuestChoice[] | The ways the quest can end. Empty = no choice. |
| `RewardItems` | FIdCount[] | Rows in `items.json`, granted whatever the choice. |
| `RewardPigments` | FIdCount[] | Rows in `pigments.json`, granted whatever the choice. |
| `RewardNotes` | string | Recipe unlocks and choice-specific rewards, in words. |

`FQuestObjective` (one per entry in `Steps`, same order): `Event` (FName: `Talk`, `Have`, `Craft`, `Relight`, `Attune`, `Kill`, `Reach`, `Inspect`, `Choose`), `Target` (FName: what the event is about, e.g. an NPC, an item or pigment, a Lampstand, an attunement node, an enemy, a place, or the quest itself for `Choose`), `Count` (int). `Have` is met by what the player is carrying; every other event is counted from the moment its step becomes current.

`FQuestChoice`: `ChoiceId` (FName, unique within the quest), `Label` (string, shown to the player), `CostPigment` (FName, row in `pigments.json`, or empty), `CostAmount` (int), `Consequence` (string: what changes in the world), `WorldFlags` (FName[]: flags set in the save game; level scripts and spawn tables read them), `RewardItems` / `RewardPigments` (FIdCount[]: granted only if this choice is made).

## regions.json (`FRegionRow`)

One `Region` row for the colossus and one `Zone` row per sub-zone.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Region or zone ID. |
| `DisplayName` | string | Name shown to the player. |
| `RowType` | enum | `Region` or `Zone`. |
| `ParentRegion` | FName | For zones: the region row. Empty for the region. |
| `Act` | int | Act in which this version of the place is visited. |
| `PaintState` | enum | `Painted`, `HalfPainted`, `Sketch`; `Mixed` for the region row. |
| `AmbientLight` | float | 0 to 1, before the shift. Multiplies Stroke cost by `(1 - AmbientLight)`. |
| `AmbientLightAfterShift` | float | 0 to 1, after the shift, before quest-flag bonuses. |
| `GravityScale` | float | 1.0 normal; 0.6 in sketch zones. |
| `Palette` | string[] | Key colours, `#RRGGBB`, most dominant first. |
| `Enemies` | FName[] | Rows in `enemies.json` present before the shift. |
| `EnemiesAfterShift` | FName[] | Rows in `enemies.json` present after the shift. |
| `Pigments` | FName[] | Rows in `pigments.json` with harvest nodes here. Ceruse is drop-only, so it appears only on the region row. |
| `HarvestNodeCounts` | int[] | Number of nodes, in the same order as `Pigments`. Every node yields 2. Zeros on the region row (see its zones). |
| `Lampstands` | int | Rest points in the zone. |
| `LevelBefore` / `LevelAfter` | FName | Sublevel shown before and after the shift. |
| `ShiftTrigger` | string | What causes the shift. |
| `ShiftBefore` / `ShiftAfter` | string | Layout and state on each side of the shift. |
| `Notes` | string | Designer notes and zone-specific rules. |

## dialogue.json (`FDialogueRow`)

One row per line of dialogue. A conversation is a chain of rows.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Node ID, `<Npc>_<Topic>`. |
| `Npc` | FName | Who the conversation belongs to (an NPC ID). |
| `Speaker` | string | Name shown above the line. |
| `Text` | string | The line. |
| `Next` | FName | Row shown after this one when there are no choices. Empty ends the conversation. |
| `Entry` | int | 0 = not a starting point. Above 0, this row can start a conversation with `Npc`; when several qualify, the highest number wins. |
| `RequireFlags` / `ForbidFlags` | FName[] | For entry rows: flags that must (or must not) be set for the row to start a conversation. |
| `SetFlags` | FName[] | Flags set when the row is shown. |
| `Choices` | FDialogueChoice[] | Up to 6 replies (only those whose flags hold are offered). Empty = no choice. |

`FDialogueChoice`: `Label` (string), `Next` (FName; empty ends the conversation), `RequireFlags` / `ForbidFlags` (FName[]: the reply is only offered when these hold), `SetFlags` (FName[]), `StartQuest` (FName: a quest to start, or empty).

**Flags.** Any name can be set as a flag. Quest choices set their `WorldFlags`. In a reply's `SetFlags`, `Give.<Id>.<n>` takes that many of an item or pigment from the player and `Get.<Id>.<n>` hands them over, instead of setting a flag (`.<n>` may be left off for 1). A reply with both is a trade; gate it with `Has.<Id>` in `RequireFlags`. The game also provides `Has.<Item or Pigment>` (the player carries at least one) and `Killed.<Enemy>` (one has been killed). The game also provides these automatically from quest progress: `Q.<Quest>.Active`, `Q.<Quest>.Done`, and `Q.<Quest>.Step<N>` (true while step N, counting from 1, is the current one).

A finished conversation counts as the event `Talk` with the NPC as target.

## lore.json (`FLoreRow`)

Things to find and read. The texts are written in `docs/STORY.md` section 9; this file is what the game shows.

| Field | Type | Meaning |
|---|---|---|
| `Name` | row key | Lore ID. |
| `Kind` | enum | `Marginalia` (the Hand's working notes: invisible until a Flare is lit nearby, then readable for good), `FieldNote` (Odile Vane's notes, always visible), `Ledger` (the village's records, always visible). |
| `Title` | string | Heading shown above the text and in the journal. |
| `Text` | string | The words. |
| `Zone` | FName | Zone row in `regions.json` where it is found. |

## Cross-reference checklist

When adding rows, keep these true (a validation script should check them):

- `pigments.Complement`, `attunements.Pigment/CostPigment/TemperPigment`, `enemies.Weaknesses/Resistances`, `quests.Choices.CostPigment`, `quests.RewardPigments.Id`, `regions.Pigments` → `pigments.Name`
- `pigments.CraftingUses`, `recipes.OutputItem`, `quests.RewardItems.Id` → `items.Name`
- `recipes.Inputs.Id`, `enemies.Drops.Id` → `pigments.Name` or `items.Name`
- `recipes.UnlockId` → `quests.Name` or `attunements.Name`, according to `UnlockType`
- `attunements.Prerequisites` → `attunements.Name`; `quests.Prerequisites` → `quests.Name`
- `pigments.SourceZones`, `enemies.Zones`, `quests.Zone`, `regions.ParentRegion` → `regions.Name`
- `regions.Enemies`, `regions.EnemiesAfterShift` → `enemies.Name`
- No recipe may have a `Stretch` quest as its `UnlockId`.
- `pigments.CraftingUses` must list exactly the items whose recipe has that pigment as an input.
- `regions.HarvestNodeCounts` must be the same length as `regions.Pigments`.

## sounds.json

What each thing that happens sounds like. Code asks for a row by name (`swing_light`, `footstep_Scumblewood`, `amb_GrisailleVault`); the row says which files, how loud, and how much the pitch may wander.

| Field | Type | Meaning |
|---|---|---|
| `Name` | FName | What code asks for. `footstep_<Zone>` and `amb_<Zone>` are looked up by zone; `amb_` rows loop. |
| `Assets` | FString[] | Object paths of sound assets. One is picked at random each time. Under `/Game/ThirdParty/Audio/` (free packs, not in git) or `/Game/Audio/Made/` (ours). |
| `Volume` | float | 0 to 1.5. |
| `PitchMin`, `PitchMax` | float | The pitch is random between these. |
| `Source` | FString | Where it came from and its licence. Must not be empty. |

A missing file is not an error: the sound is skipped and the log says so.

## hints.json

The teaching prompts: one short line that appears when the thing it teaches first matters, and never again. Their order in the file is their priority.

| Field | Type | Meaning |
|---|---|---|
| `Name` | FName | Saved as the flag `Hint.<Name>` once shown. |
| `Trigger` | FName | When it appears. The triggers are code (`AUnderpaintingPlayerCharacter::TickHints`); the validator lists them. |
| `Text` | FString | At most 170 characters. `{Use}`, `{Dodge}` and the other action names become the player's own keys; `{Pigment}`, `{Stroke}`, `{Cost}`, `{Enemy}` and `{Weakness}` are filled from what is in front of them. |
| `Seconds` | float | The longest it stays up. It goes sooner if the player does the thing. |

Quest objectives also carry a `Zone` (a zone row in regions.json): where that step happens, so the objective marker can point at the right door when it is somewhere else.

