// Everything the player has done and carries: pigment, items, recipes, attunement, quests, flags, and the
// state of the world's Lampstands and harvest nodes. The rules for changing it live here too, so they can be
// tested without a level, and so one object is all a save file needs.
//
// Rules: docs/GDD.md sections 4 (pigment, crafting, attunement) and 7 (quests). Content: data/*.json.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingProgress.generated.h"

class UDataTable;

/** Where one quest stands. */
USTRUCT(BlueprintType)
struct FQuestProgress
{
	GENERATED_BODY()

	/** Index of the current step. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	int32 Step = 0;

	/** Matching events counted since this step became current. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	int32 StepCount = 0;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	bool bComplete = false;

	/** The ending chosen, if the quest has choices. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	FName Choice;
};

/** The saved part of UUnderpaintingProgress. */
USTRUCT(BlueprintType)
struct FUnderpaintingProgressData
{
	GENERATED_BODY()

	/** Fresh pigment. Half of each is lost on death until it has dried at a Lampstand. */
	UPROPERTY(SaveGame) TMap<FName, int32> WetPigment;
	UPROPERTY(SaveGame) TMap<FName, int32> DryPigment;
	UPROPERTY(SaveGame) TMap<FName, int32> Items;
	UPROPERTY(SaveGame) TArray<FName> Attunements;
	UPROPERTY(SaveGame) TMap<FName, FQuestProgress> Quests;
	UPROPERTY(SaveGame) TArray<FName> Flags;
	UPROPERTY(SaveGame) TArray<FName> LitLampstands;
	UPROPERTY(SaveGame) TArray<FName> DepletedNodes;
	/** The two palette wells: which pigments are active. None means an empty well. */
	UPROPERTY(SaveGame) TArray<FName> Wells;
	UPROPERTY(SaveGame) int32 ActiveWell = 0;
	UPROPERTY(SaveGame) TArray<FName> FoundLore;
	UPROPERTY(SaveGame) TArray<FName> DiscoveredPlaces;
	UPROPERTY(SaveGame) TArray<FName> OpenedCaches;
	UPROPERTY(SaveGame) TArray<FName> FixedPlatforms;
	UPROPERTY(SaveGame) int32 Deaths = 0;
	/** What is worn and carried. None means the starting knife and smock. */
	UPROPERTY(SaveGame) FName EquippedWeapon;
	UPROPERTY(SaveGame) FName EquippedArmour;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnProgressNotice, const FString&);
DECLARE_MULTICAST_DELEGATE(FOnProgressChanged);

UCLASS()
class UUnderpaintingProgress : public UObject
{
	GENERATED_BODY()

public:

	/** The content tables the rules read. Must be called before anything else. */
	void SetTables(UDataTable* InPigments, UDataTable* InItems, UDataTable* InRecipes, UDataTable* InAttunements, UDataTable* InQuests, UDataTable* InDialogue, UDataTable* InLore = nullptr);

	/** Wipes everything and starts the first main quest. */
	void NewGame();

	// --- Pigment and items ---

	int32 GetPigment(FName Id) const { return GetWetPigment(Id) + GetDryPigment(Id); }
	int32 GetWetPigment(FName Id) const { return Data.WetPigment.FindRef(Id); }
	int32 GetDryPigment(FName Id) const { return Data.DryPigment.FindRef(Id); }
	void AddPigment(FName Id, int32 Count, bool bWet = true);

	/** Spends pigment, wet first (it is the pigment at risk). Fails without spending if there is not enough. */
	bool SpendPigment(FName Id, int32 Count, bool bDryOnly = false);

	int32 GetItemCount(FName Id) const { return Data.Items.FindRef(Id); }

	/** Adds items up to the item's carry limit. Returns how many were actually added. */
	int32 AddItem(FName Id, int32 Count);
	bool RemoveItem(FName Id, int32 Count);

	/** True if Id is a pigment row (rather than an item). */
	bool IsPigment(FName Id) const;

	/** Name to show the player for a pigment, an item or a quest. */
	FString GetDisplayName(FName Id) const;

	// --- Crafting ---

	bool IsRecipeUnlocked(FName RecipeId) const;
	bool CanCraft(FName RecipeId, FString* OutWhyNot = nullptr) const;
	bool Craft(FName RecipeId);
	TArray<FName> GetRecipesForStation(ECraftingStation Station) const;

	// --- Attunement ---

	bool HasAttunement(FName NodeId) const { return Data.Attunements.Contains(NodeId); }

	// --- Gear ---

	static const FName StartingWeapon;
	static const FName StartingArmour;

	FName GetEquippedWeapon() const { return Data.EquippedWeapon.IsNone() ? StartingWeapon : Data.EquippedWeapon; }
	FName GetEquippedArmour() const { return Data.EquippedArmour.IsNone() ? StartingArmour : Data.EquippedArmour; }

	/** Every weapon and armour the Limner has: the starting two, then anything crafted. */
	TArray<FName> GetGear() const;

	/** Puts on a weapon or armour from GetGear. */
	bool Equip(FName ItemId);

	/** The item's row in data/items.json, or null. */
	const FItemRow* GetItemRow(FName ItemId) const;

	/** Extra maximum Glim from one-off upgrades in the pack (the Braided Wick). */
	float GetUpgradeGlim() const;
	bool CanAttune(FName NodeId, FString* OutWhyNot = nullptr) const;
	bool Attune(FName NodeId);

	/** Sum of EffectValue over owned nodes with this EffectType (for example "MaxGlim"). */
	float GetAttunementValue(FName EffectType) const;
	TArray<FName> GetAllAttunementNodes() const;

	// --- The palette ---

	static constexpr int32 NumWells = 2;

	/** Pigment in a well, or None. */
	FName GetWell(int32 Index) const { return Data.Wells.IsValidIndex(Index) ? Data.Wells[Index] : NAME_None; }

	/** The pigment of the well in use: it decides the Stroke. None if that well is empty. */
	FName GetActivePigment() const { return GetWell(Data.ActiveWell); }
	int32 GetActiveWellIndex() const { return Data.ActiveWell; }

	/** A pigment can sit in a well once its tier 1 node is owned. */
	bool CanPlaceInWell(FName PigmentId) const;
	bool SetWell(int32 Index, FName PigmentId);

	/** Switches to the other well (if it holds something). */
	void SwapActiveWell();

	/** True if this pigment is in either well: its passives only apply then. */
	bool IsInWell(FName PigmentId) const { return Data.Wells.Contains(PigmentId); }



	bool StartQuest(FName QuestId);
	bool IsQuestActive(FName QuestId) const;
	bool IsQuestComplete(FName QuestId) const;
	const FQuestProgress* GetQuestProgress(FName QuestId) const { return Data.Quests.Find(QuestId); }
	TArray<FName> GetActiveQuests() const;

	/** What the quest's current step is about (the objective's Target), or None. */
	FName GetCurrentObjectiveTarget(FName QuestId) const;

	/** Journal text of the quest's current step, or empty. */
	FString GetCurrentStepText(FName QuestId) const;

	/** The quest's row, or null. */
	const FQuestRow* GetQuestRow(FName QuestId) const;

	/** True when the quest is waiting for its ending to be chosen. */
	bool IsAwaitingChoice(FName QuestId) const;

	/** Whether an ending can be paid for right now. */
	bool CanChoose(FName QuestId, FName ChoiceId, FString* OutWhyNot = nullptr) const;

	/**
	 * Makes a quest's ending. Permanent: it spends the pigment, sets the choice's world flags for good, and
	 * moves the quest on. The caller is expected to have shown the cost and consequence and asked first.
	 */
	bool MakeChoice(FName QuestId, FName ChoiceId);

	/** How much a harvest node gives here, after quest consequences ("<Zone>_<Pigment>YieldPlus1" and "...Minus1" flags). */
	int32 GetNodeYield(FName ZoneId, FName PigmentId, int32 BaseYield) const;

	/** Tells the quests that something happened (see FQuestObjective for the event names). */
	void NotifyEvent(FName Event, FName Target, int32 Count = 1);

	// --- Flags ---

	/** True for flags that were set, and for the automatic quest flags Q.<Quest>.Active, .Done and .Step<N>. */
	bool HasFlag(FName Flag) const;
	void SetFlag(FName Flag);
	bool PassesFlags(const TArray<FName>& Require, const TArray<FName>& Forbid) const;

	// --- Dialogue ---

	/** The row that opens a conversation with this NPC right now, or None. */
	FName FindDialogueEntry(FName NpcId) const;
	const FDialogueRow* GetDialogueRow(FName RowId) const;

	/** Indices into the row's Choices that may be offered right now. */
	TArray<int32> GetOfferedChoices(const FDialogueRow& Row) const;

	/** Applies what showing a row does (its SetFlags). */
	void EnterDialogueRow(FName RowId);

	/** Applies a reply and returns the next row, or None if the conversation ends. */
	FName ChooseDialogue(FName RowId, int32 ChoiceIndex);

	/** Call when a conversation ends: counts as the Talk event. */
	void EndDialogue(FName NpcId);

	// --- The world ---

	bool IsLampstandLit(FName LampstandId) const { return Data.LitLampstands.Contains(LampstandId); }

	/** Pours 1 Lamp Oil into an unlit Lampstand. */
	bool RelightLampstand(FName LampstandId);

	bool IsNodeDepleted(FName NodeKey) const { return Data.DepletedNodes.Contains(NodeKey); }

	/** Takes the pigment from a node. Returns the amount gained (0 if it was already empty). */
	int32 HarvestNode(FName NodeKey, FName PigmentId, int32 Yield = 2);

	/** Resting at a lit Lampstand: wet pigment dries, nodes that regrow do, and poultices are topped up to 2. */
	void Rest();

	// --- Exploration ---

	const FLoreRow* GetLore(FName LoreId) const;
	bool HasFoundLore(FName LoreId) const { return Data.FoundLore.Contains(LoreId); }

	/** Records a piece of lore as read. Returns true the first time. */
	bool FindLore(FName LoreId);

	/** How many pieces of this kind exist, and how many have been found. */
	void CountLore(ELoreKind Kind, int32& OutFound, int32& OutTotal) const;

	bool HasDiscovered(FName PlaceId) const { return Data.DiscoveredPlaces.Contains(PlaceId); }

	/** Arriving somewhere with a name. Counts as the Reach event every time; announces the place the first time. */
	void ReachPlace(FName PlaceId, const FString& DisplayName);

	bool IsCacheOpened(FName CacheKey) const { return Data.OpenedCaches.Contains(CacheKey); }

	/** Takes everything from a cache, once. */
	bool OpenCache(FName CacheKey, const TArray<FIdCount>& Contents);

	bool IsPlatformFixed(FName PlatformKey) const { return Data.FixedPlatforms.Contains(PlatformKey); }

	/** Spends 1 Fixative Flask to make a sketched object solid for good. */
	bool FixPlatform(FName PlatformKey);

	/** Death: half of each wet pigment is lost, rounded down. The first death of a save costs nothing. */
	void ApplyDeathPenalty();

	// --- Saving ---

	const FUnderpaintingProgressData& GetData() const { return Data; }
	void SetData(const FUnderpaintingProgressData& InData);

	/** Short messages for the player ("+2 Hide Ochre", "Quest complete"). */
	FOnProgressNotice OnNotice;

	/** Fires after anything changes, so the HUD and world actors can refresh. */
	FOnProgressChanged OnChanged;

	/** The quest every new game begins with. */
	static const FName FirstQuest;

private:

	void Notice(const FString& Text);
	void Changed() { OnChanged.Broadcast(); }
	bool IsObjectiveMet(const FQuestObjective& Objective, const FQuestProgress& Progress) const;
	void AdvanceQuests();
	void CompleteQuest(FName QuestId, const FQuestRow& Row);

	template <typename RowType>
	const RowType* Find(const UDataTable* Table, FName Id) const;

	UPROPERTY() FUnderpaintingProgressData Data;

	UPROPERTY(Transient) TObjectPtr<UDataTable> Pigments;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ItemsTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> Recipes;
	UPROPERTY(Transient) TObjectPtr<UDataTable> AttunementsTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> QuestsTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> Dialogue;
	UPROPERTY(Transient) TObjectPtr<UDataTable> LoreTable;

	bool bAdvancing = false;
};
