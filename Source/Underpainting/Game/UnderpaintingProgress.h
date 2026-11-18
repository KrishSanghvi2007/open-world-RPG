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
	UPROPERTY(SaveGame) int32 Deaths = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnProgressNotice, const FString&);
DECLARE_MULTICAST_DELEGATE(FOnProgressChanged);

UCLASS()
class UUnderpaintingProgress : public UObject
{
	GENERATED_BODY()

public:

	/** The content tables the rules read. Must be called before anything else. */
	void SetTables(UDataTable* InPigments, UDataTable* InItems, UDataTable* InRecipes, UDataTable* InAttunements, UDataTable* InQuests, UDataTable* InDialogue);

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
	bool CanAttune(FName NodeId, FString* OutWhyNot = nullptr) const;
	bool Attune(FName NodeId);

	/** Sum of EffectValue over owned nodes with this EffectType (for example "MaxGlim"). */
	float GetAttunementValue(FName EffectType) const;
	TArray<FName> GetAllAttunementNodes() const;

	// --- Quests ---

	bool StartQuest(FName QuestId);
	bool IsQuestActive(FName QuestId) const;
	bool IsQuestComplete(FName QuestId) const;
	const FQuestProgress* GetQuestProgress(FName QuestId) const { return Data.Quests.Find(QuestId); }
	TArray<FName> GetActiveQuests() const;

	/** Journal text of the quest's current step, or empty. */
	FString GetCurrentStepText(FName QuestId) const;

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

	bool bAdvancing = false;
};
