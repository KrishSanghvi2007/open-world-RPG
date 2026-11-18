// Row structs for the game content in data/*.json.
// The field names here must match the JSON keys exactly: see data/README.md for what each one means.
// "Name" is the row key in every file, so it is not a member of any struct.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UnderpaintingDataTypes.generated.h"

// ---------------------------------------------------------------------------
// Enums. Value names match the strings used in the JSON.
// Fields that may be empty in the JSON (for example Phase2Hue) stay FName.
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EPigmentHue : uint8
{
	Orange,
	Green,
	Red,
	Black,
	White,
	Blue
};

UENUM(BlueprintType)
enum class EPigmentRarity : uint8
{
	Common,
	Uncommon,
	Rare
};

UENUM(BlueprintType)
enum class EPigmentRegrowth : uint8
{
	OnRest,
	Never,
	DropOnly
};

UENUM(BlueprintType)
enum class EPigmentDeathLoss : uint8
{
	Half,
	None
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon,
	Armour,
	Consumable,
	Material,
	Key,
	Upgrade
};

UENUM(BlueprintType)
enum class EItemEffectType : uint8
{
	None,
	GlimCapacity,
	RestoreGlim,
	HealOverTime,
	FixTarget,
	DamagePercent,
	GravityScale,
	StaminaRegenPercent
};

UENUM(BlueprintType)
enum class ECraftingStation : uint8
{
	GrindingSlab,
	ArmatureBench
};

UENUM(BlueprintType)
enum class ERecipeUnlockType : uint8
{
	Start,
	Quest,
	Attunement
};

UENUM(BlueprintType)
enum class EEnemyFamily : uint8
{
	Erasure,
	Parasite,
	Draft
};

UENUM(BlueprintType)
enum class EEnemyTier : uint8
{
	Standard,
	Boss
};

UENUM(BlueprintType)
enum class EAttackTelegraph : uint8
{
	/** Can be dodged, or cancelled with Erase. */
	WarmRim,
	/** Must be dodged. */
	WhiteFlash
};

UENUM(BlueprintType)
enum class EQuestType : uint8
{
	Main,
	Side
};

UENUM(BlueprintType)
enum class EQuestSliceScope : uint8
{
	Core,
	Stretch
};

UENUM(BlueprintType)
enum class ERegionRowType : uint8
{
	Region,
	Zone
};

UENUM(BlueprintType)
enum class EPaintState : uint8
{
	Painted,
	HalfPainted,
	Sketch,
	Mixed
};

// ---------------------------------------------------------------------------
// Small structs shared between files
// ---------------------------------------------------------------------------

/** An ID in another table, with a quantity. */
USTRUCT(BlueprintType)
struct FIdCount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	int32 Count = 0;
};

/** One possible drop from an enemy. Chance is 0 to 1. */
USTRUCT(BlueprintType)
struct FDropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	float Chance = 0.0f;
};

// ---------------------------------------------------------------------------
// pigments.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FPigmentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FString Source;

	/** Zone rows in regions.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	TArray<FName> SourceZones;

	/** #RRGGBB. Use UUnderpaintingDataSubsystem::HexToColour to convert. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FString ColourHex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	EPigmentHue Hue = EPigmentHue::Orange;

	/** The opposite pigment (a row in pigments.json). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FName Complement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	EPigmentRarity Rarity = EPigmentRarity::Common;

	/** Glim per Stroke at ambient light 0. Actual cost = LightCost x (1 - AmbientLight), rounded up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	int32 LightCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	EPigmentRegrowth Regrowth = EPigmentRegrowth::OnRest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	EPigmentDeathLoss DeathLoss = EPigmentDeathLoss::Half;

	/** Items in items.json whose recipe uses this pigment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	TArray<FName> CraftingUses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FString AttunementPassive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment|Stroke")
	FName StrokeName;

	/** Multiplier on the weapon's HeavyDamage, per hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment|Stroke")
	float StrokeDamageMultiplier = 1.0f;

	/** Seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment|Stroke")
	float StrokeWindup = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment|Stroke")
	FString CombatModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pigment")
	FString LoreLine;
};

// ---------------------------------------------------------------------------
// items.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType Type = EItemType::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	int32 LightDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	int32 HeavyDamage = 0;

	/** Animation play-rate multiplier; 1.0 is the baseline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	float AttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	int32 StaminaCostLight = 0;

	/** A Stroke also costs this much. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	int32 StaminaCostHeavy = 0;

	/** Metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	float Reach = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon")
	float StaggerMultiplier = 0.0f;

	/** Fraction of incoming damage removed, 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armour")
	float DamageReduction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect")
	EItemEffectType EffectType = EItemEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect")
	float EffectValue = 0.0f;

	/** Seconds; 0 means instant or permanent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect")
	float EffectDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 1;
};

// ---------------------------------------------------------------------------
// recipes.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Row in items.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName OutputItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 OutputCount = 1;

	/** Each Id is a row in pigments.json or items.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FIdCount> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	ECraftingStation Station = ECraftingStation::GrindingSlab;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	ERecipeUnlockType UnlockType = ERecipeUnlockType::Start;

	/** Row in quests.json or attunements.json, by UnlockType. Empty for Start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName UnlockId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FString Notes;
};

// ---------------------------------------------------------------------------
// attunements.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FAttunementRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	FString DisplayName;

	/** Row in pigments.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	FName Pigment;

	/** 1 or 2. Tier 1 also unlocks the pigment's well and Stroke. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	int32 Tier = 1;

	/** Machine-readable ID of the main numeric effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	FName EffectType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	float EffectValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	FString EffectText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement|Cost")
	FName CostPigment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement|Cost")
	int32 CostAmount = 0;

	/** Tier 2 only: the complement pigment that must also be paid. Empty for tier 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement|Cost")
	FName TemperPigment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement|Cost")
	int32 TemperAmount = 0;

	/** Nodes in attunements.json that must be owned first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attunement")
	TArray<FName> Prerequisites;
};

// ---------------------------------------------------------------------------
// enemies.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FEnemyAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName AttackName;

	/** 1, or 2 for a boss's second phase only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 Phase = 1;

	/** Seconds the tell is visible before the hit can land. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Windup = 0.0f;

	/** Seconds the hitbox is live. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Active = 0.0f;

	/** Seconds the enemy cannot act afterwards: the punish window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Recovery = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EAttackTelegraph Telegraph = EAttackTelegraph::WarmRim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FString Notes;
};

USTRUCT(BlueprintType)
struct FEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyFamily Family = EEnemyFamily::Erasure;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyTier Tier = EEnemyTier::Standard;

	/** Zone rows in regions.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<FName> Zones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EPigmentHue Hue = EPigmentHue::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 MaxHP = 0;

	/** Stagger threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 Poise = 0;

	/** Unreal units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	float MoveSpeed = 0.0f;

	/** Metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	float AggroRadius = 0.0f;

	/** Metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	float LeashRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 GlimOnKill = 0;

	/** Pigments whose Strokes deal +30% damage and +50% stagger damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<FName> Weaknesses;

	/** Pigments whose Strokes deal -30% damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<FName> Resistances;

	/** Boss only: HP percentage at which phase 2 begins. 0 means no second phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Boss")
	int32 Phase2HPPercent = 0;

	/** Boss only: hue during phase 2 (an EPigmentHue value name). Empty for everything else. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Boss")
	FName Phase2Hue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<FEnemyAttack> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<FDropEntry> Drops;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FString BehaviourNotes;
};

// ---------------------------------------------------------------------------
// quests.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FQuestChoice
{
	GENERATED_BODY()

	/** Unique within the quest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName ChoiceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FString Label;

	/** Row in pigments.json, or empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName CostPigment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	int32 CostAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FString Consequence;

	/** Flags set in the save game; level scripts and spawn tables read them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	TArray<FName> WorldFlags;
};

/** What the game checks for one quest step. */
USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()

	/** Talk, Have, Craft, Relight, Attune, Kill, Reach, Inspect or Choose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName Event;

	/** What the event is about: an NPC, an item or pigment, a Lampstand, a node, an enemy, a place, or the quest itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FQuestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EQuestType QuestType = EQuestType::Main;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EQuestSliceScope SliceScope = EQuestSliceScope::Core;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 Act = 1;

	/** NPC ID. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName Giver;

	/** Zone row in regions.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName Zone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FString> Steps;

	/** One per step, in the same order: what the game checks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestObjective> Objectives;

	/** Quests that must be complete first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FName> Prerequisites;

	/** The ways the quest can end. Empty means no choice. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestChoice> Choices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Rewards")
	TArray<FIdCount> RewardItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Rewards")
	TArray<FIdCount> RewardPigments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Rewards")
	FString RewardNotes;
};

// ---------------------------------------------------------------------------
// regions.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FRegionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	ERegionRowType RowType = ERegionRowType::Zone;

	/** For zones: the region row. Empty for the region itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName ParentRegion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	int32 Act = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	EPaintState PaintState = EPaintState::Painted;

	/** 0 to 1, before the shift. Stroke cost is multiplied by (1 - AmbientLight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	float AmbientLight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	float AmbientLightAfterShift = 1.0f;

	/** 1.0 is normal; 0.6 in sketch zones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	float GravityScale = 1.0f;

	/** Key colours, #RRGGBB, most dominant first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FString> Palette;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FName> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FName> EnemiesAfterShift;

	/** Pigments with harvest nodes here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FName> Pigments;

	/** Number of nodes, in the same order as Pigments. Every node yields 2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<int32> HarvestNodeCounts;

	/** Rest points in the zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	int32 Lampstands = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region|Shift")
	FName LevelBefore;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region|Shift")
	FName LevelAfter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region|Shift")
	FString ShiftTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region|Shift")
	FString ShiftBefore;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region|Shift")
	FString ShiftAfter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FString Notes;
};

// ---------------------------------------------------------------------------
// dialogue.json
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FString Label;

	/** Row shown next. Empty ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName Next;

	/** The reply is only offered when these flags are set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	TArray<FName> RequireFlags;

	/** The reply is hidden when any of these flags is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	TArray<FName> ForbidFlags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	TArray<FName> SetFlags;

	/** A quest to start, or empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName StartQuest;
};

USTRUCT(BlueprintType)
struct FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The NPC the conversation belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Npc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Text;

	/** Row shown after this one when there are no choices. Empty ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Next;

	/** 0 = not a starting point. Above 0 the row can open a conversation; the highest qualifying number wins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int32 Entry = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FName> RequireFlags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FName> ForbidFlags;

	/** Flags set when the row is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FName> SetFlags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FDialogueChoice> Choices;
};
