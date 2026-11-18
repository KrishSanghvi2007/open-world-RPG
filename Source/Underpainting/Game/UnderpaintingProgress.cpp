#include "UnderpaintingProgress.h"

#include "Underpainting.h"
#include "Engine/DataTable.h"

const FName UUnderpaintingProgress::FirstQuest(TEXT("MQ_DuskOnTheShoulder"));

namespace
{
	const FName EVENT_Talk(TEXT("Talk"));
	const FName EVENT_Have(TEXT("Have"));
	const FName EVENT_Craft(TEXT("Craft"));
	const FName EVENT_Relight(TEXT("Relight"));
	const FName EVENT_Attune(TEXT("Attune"));
	const FName ITEM_LampOil(TEXT("LampOil"));
	const FName ITEM_MossPoultice(TEXT("MossPoultice"));
}

template <typename RowType>
const RowType* UUnderpaintingProgress::Find(const UDataTable* Table, FName Id) const
{
	return Table && !Id.IsNone() ? Table->FindRow<RowType>(Id, TEXT("Progress"), false) : nullptr;
}

void UUnderpaintingProgress::SetTables(UDataTable* InPigments, UDataTable* InItems, UDataTable* InRecipes, UDataTable* InAttunements, UDataTable* InQuests, UDataTable* InDialogue, UDataTable* InLore)
{
	Pigments = InPigments;
	ItemsTable = InItems;
	Recipes = InRecipes;
	AttunementsTable = InAttunements;
	QuestsTable = InQuests;
	Dialogue = InDialogue;
	LoreTable = InLore;
}

void UUnderpaintingProgress::Notice(const FString& Text)
{
	UE_LOG(LogUnderpainting, Log, TEXT("Notice: %s"), *Text);
	OnNotice.Broadcast(Text);
}

void UUnderpaintingProgress::NewGame()
{
	Data = FUnderpaintingProgressData();
	StartQuest(FirstQuest);
	Changed();
}

void UUnderpaintingProgress::SetData(const FUnderpaintingProgressData& InData)
{
	Data = InData;
	Changed();
}

// --- Pigment and items ---

bool UUnderpaintingProgress::IsPigment(FName Id) const
{
	return Find<FPigmentRow>(Pigments, Id) != nullptr;
}

FString UUnderpaintingProgress::GetDisplayName(FName Id) const
{
	if (const FPigmentRow* Pigment = Find<FPigmentRow>(Pigments, Id))
	{
		return Pigment->DisplayName;
	}
	if (const FItemRow* Item = Find<FItemRow>(ItemsTable, Id))
	{
		return Item->DisplayName;
	}
	if (const FQuestRow* Quest = Find<FQuestRow>(QuestsTable, Id))
	{
		return Quest->DisplayName;
	}
	return Id.ToString();
}

void UUnderpaintingProgress::AddPigment(FName Id, int32 Count, bool bWet)
{
	if (Count <= 0 || !IsPigment(Id))
	{
		return;
	}
	(bWet ? Data.WetPigment : Data.DryPigment).FindOrAdd(Id) += Count;
	Notice(FString::Printf(TEXT("+%d %s%s"), Count, *GetDisplayName(Id), bWet ? TEXT(" (wet)") : TEXT("")));
	AdvanceQuests();
	Changed();
}

bool UUnderpaintingProgress::SpendPigment(FName Id, int32 Count, bool bDryOnly)
{
	const int32 Available = bDryOnly ? GetDryPigment(Id) : GetPigment(Id);
	if (Count <= 0 || Available < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	if (!bDryOnly)
	{
		int32& Wet = Data.WetPigment.FindOrAdd(Id);
		const int32 FromWet = FMath::Min(Wet, Remaining);
		Wet -= FromWet;
		Remaining -= FromWet;
	}
	Data.DryPigment.FindOrAdd(Id) -= Remaining;
	Changed();
	return true;
}

int32 UUnderpaintingProgress::AddItem(FName Id, int32 Count)
{
	const FItemRow* Row = Find<FItemRow>(ItemsTable, Id);
	if (!Row || Count <= 0)
	{
		return 0;
	}

	int32& Held = Data.Items.FindOrAdd(Id);
	const int32 Added = FMath::Clamp(Count, 0, FMath::Max(0, FMath::Max(1, Row->MaxStack) - Held));
	Held += Added;
	if (Added > 0)
	{
		Notice(FString::Printf(TEXT("+%d %s"), Added, *Row->DisplayName));
	}
	else
	{
		Notice(FString::Printf(TEXT("You cannot carry more %s."), *Row->DisplayName));
	}
	AdvanceQuests();
	Changed();
	return Added;
}

bool UUnderpaintingProgress::RemoveItem(FName Id, int32 Count)
{
	int32* Held = Data.Items.Find(Id);
	if (!Held || *Held < Count || Count <= 0)
	{
		return false;
	}
	*Held -= Count;
	Changed();
	return true;
}

// --- Crafting ---

bool UUnderpaintingProgress::IsRecipeUnlocked(FName RecipeId) const
{
	const FRecipeRow* Recipe = Find<FRecipeRow>(Recipes, RecipeId);
	if (!Recipe)
	{
		return false;
	}
	switch (Recipe->UnlockType)
	{
	case ERecipeUnlockType::Quest: return IsQuestComplete(Recipe->UnlockId);
	case ERecipeUnlockType::Attunement: return HasAttunement(Recipe->UnlockId);
	default: return true;
	}
}

bool UUnderpaintingProgress::CanCraft(FName RecipeId, FString* OutWhyNot) const
{
	const FRecipeRow* Recipe = Find<FRecipeRow>(Recipes, RecipeId);
	if (!Recipe || !IsRecipeUnlocked(RecipeId))
	{
		if (OutWhyNot) { *OutWhyNot = TEXT("You have not learned this."); }
		return false;
	}

	for (const FIdCount& Input : Recipe->Inputs)
	{
		const int32 Have = IsPigment(Input.Id) ? GetPigment(Input.Id) : GetItemCount(Input.Id);
		if (Have < Input.Count)
		{
			if (OutWhyNot) { *OutWhyNot = FString::Printf(TEXT("Needs %d %s (you have %d)."), Input.Count, *GetDisplayName(Input.Id), Have); }
			return false;
		}
	}

	if (const FItemRow* Output = Find<FItemRow>(ItemsTable, Recipe->OutputItem))
	{
		if (GetItemCount(Recipe->OutputItem) >= FMath::Max(1, Output->MaxStack))
		{
			if (OutWhyNot) { *OutWhyNot = FString::Printf(TEXT("You cannot carry more %s."), *Output->DisplayName); }
			return false;
		}
	}
	return true;
}

bool UUnderpaintingProgress::Craft(FName RecipeId)
{
	if (!CanCraft(RecipeId))
	{
		return false;
	}

	const FRecipeRow* Recipe = Find<FRecipeRow>(Recipes, RecipeId);
	for (const FIdCount& Input : Recipe->Inputs)
	{
		if (IsPigment(Input.Id))
		{
			SpendPigment(Input.Id, Input.Count);
		}
		else
		{
			RemoveItem(Input.Id, Input.Count);
		}
	}
	AddItem(Recipe->OutputItem, Recipe->OutputCount);
	NotifyEvent(EVENT_Craft, Recipe->OutputItem, 1);
	return true;
}

TArray<FName> UUnderpaintingProgress::GetRecipesForStation(ECraftingStation Station) const
{
	TArray<FName> Result;
	if (Recipes)
	{
		for (const TPair<FName, uint8*>& Pair : Recipes->GetRowMap())
		{
			const FRecipeRow* Recipe = reinterpret_cast<const FRecipeRow*>(Pair.Value);
			if (Recipe->Station == Station && IsRecipeUnlocked(Pair.Key))
			{
				Result.Add(Pair.Key);
			}
		}
	}
	return Result;
}

// --- Attunement ---

bool UUnderpaintingProgress::CanAttune(FName NodeId, FString* OutWhyNot) const
{
	const FAttunementRow* Node = Find<FAttunementRow>(AttunementsTable, NodeId);
	if (!Node)
	{
		return false;
	}
	if (HasAttunement(NodeId))
	{
		if (OutWhyNot) { *OutWhyNot = TEXT("Already yours."); }
		return false;
	}
	for (const FName& Prerequisite : Node->Prerequisites)
	{
		if (!HasAttunement(Prerequisite))
		{
			if (OutWhyNot) { *OutWhyNot = TEXT("Needs the tier before it."); }
			return false;
		}
	}
	// Attunement takes dry pigment only: what you take into yourself has to have set first.
	if (GetDryPigment(Node->CostPigment) < Node->CostAmount)
	{
		if (OutWhyNot) { *OutWhyNot = FString::Printf(TEXT("Needs %d dry %s (you have %d dry)."), Node->CostAmount, *GetDisplayName(Node->CostPigment), GetDryPigment(Node->CostPigment)); }
		return false;
	}
	if (Node->TemperAmount > 0 && GetDryPigment(Node->TemperPigment) < Node->TemperAmount)
	{
		if (OutWhyNot) { *OutWhyNot = FString::Printf(TEXT("Needs %d dry %s to temper it."), Node->TemperAmount, *GetDisplayName(Node->TemperPigment)); }
		return false;
	}
	return true;
}

bool UUnderpaintingProgress::Attune(FName NodeId)
{
	if (!CanAttune(NodeId))
	{
		return false;
	}

	const FAttunementRow* Node = Find<FAttunementRow>(AttunementsTable, NodeId);
	SpendPigment(Node->CostPigment, Node->CostAmount, true);
	if (Node->TemperAmount > 0)
	{
		SpendPigment(Node->TemperPigment, Node->TemperAmount, true);
	}
	Data.Attunements.Add(NodeId);
	Notice(FString::Printf(TEXT("Attuned: %s"), *Node->DisplayName));
	NotifyEvent(EVENT_Attune, NodeId, 1);
	Changed();
	return true;
}

float UUnderpaintingProgress::GetAttunementValue(FName EffectType) const
{
	float Total = 0.0f;
	for (const FName& NodeId : Data.Attunements)
	{
		const FAttunementRow* Node = Find<FAttunementRow>(AttunementsTable, NodeId);
		if (Node && Node->EffectType == EffectType)
		{
			Total += Node->EffectValue;
		}
	}
	return Total;
}

TArray<FName> UUnderpaintingProgress::GetAllAttunementNodes() const
{
	return AttunementsTable ? AttunementsTable->GetRowNames() : TArray<FName>();
}

// --- Quests ---

bool UUnderpaintingProgress::StartQuest(FName QuestId)
{
	const FQuestRow* Row = Find<FQuestRow>(QuestsTable, QuestId);
	if (!Row || Data.Quests.Contains(QuestId))
	{
		return false;
	}
	for (const FName& Prerequisite : Row->Prerequisites)
	{
		if (!IsQuestComplete(Prerequisite))
		{
			return false;
		}
	}

	Data.Quests.Add(QuestId, FQuestProgress());
	Notice(FString::Printf(TEXT("New quest: %s"), *Row->DisplayName));
	AdvanceQuests();
	Changed();
	return true;
}

bool UUnderpaintingProgress::IsQuestActive(FName QuestId) const
{
	const FQuestProgress* Progress = Data.Quests.Find(QuestId);
	return Progress && !Progress->bComplete;
}

bool UUnderpaintingProgress::IsQuestComplete(FName QuestId) const
{
	const FQuestProgress* Progress = Data.Quests.Find(QuestId);
	return Progress && Progress->bComplete;
}

TArray<FName> UUnderpaintingProgress::GetActiveQuests() const
{
	TArray<FName> Result;
	for (const TPair<FName, FQuestProgress>& Pair : Data.Quests)
	{
		if (!Pair.Value.bComplete)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

FString UUnderpaintingProgress::GetCurrentStepText(FName QuestId) const
{
	const FQuestRow* Row = Find<FQuestRow>(QuestsTable, QuestId);
	const FQuestProgress* Progress = Data.Quests.Find(QuestId);
	return Row && Progress && Row->Steps.IsValidIndex(Progress->Step) ? Row->Steps[Progress->Step] : FString();
}

bool UUnderpaintingProgress::IsObjectiveMet(const FQuestObjective& Objective, const FQuestProgress& Progress) const
{
	if (Objective.Event == EVENT_Have)
	{
		const int32 Have = IsPigment(Objective.Target) ? GetPigment(Objective.Target) : GetItemCount(Objective.Target);
		return Have >= Objective.Count;
	}
	return Progress.StepCount >= Objective.Count;
}

void UUnderpaintingProgress::NotifyEvent(FName Event, FName Target, int32 Count)
{
	for (TPair<FName, FQuestProgress>& Pair : Data.Quests)
	{
		const FQuestRow* Row = Find<FQuestRow>(QuestsTable, Pair.Key);
		if (!Row || Pair.Value.bComplete || !Row->Objectives.IsValidIndex(Pair.Value.Step))
		{
			continue;
		}
		const FQuestObjective& Objective = Row->Objectives[Pair.Value.Step];
		if (Objective.Event == Event && Objective.Target == Target)
		{
			Pair.Value.StepCount += Count;
		}
	}
	AdvanceQuests();
	Changed();
}

void UUnderpaintingProgress::AdvanceQuests()
{
	// Completing a quest hands out rewards, which can satisfy another step: guard against re-entry and loop instead.
	if (bAdvancing)
	{
		return;
	}
	bAdvancing = true;

	bool bMoved = true;
	while (bMoved)
	{
		bMoved = false;
		TArray<FName> QuestIds;
		Data.Quests.GetKeys(QuestIds);
		for (const FName& QuestId : QuestIds)
		{
			const FQuestRow* Row = Find<FQuestRow>(QuestsTable, QuestId);
			FQuestProgress& Progress = Data.Quests[QuestId];
			if (!Row || Progress.bComplete)
			{
				continue;
			}

			bool bStepped = false;
			if (Row->Objectives.IsValidIndex(Progress.Step) && IsObjectiveMet(Row->Objectives[Progress.Step], Progress))
			{
				Progress.Step++;
				Progress.StepCount = 0;
				bStepped = true;
				bMoved = true;
			}

			if (Progress.Step >= Row->Objectives.Num())
			{
				CompleteQuest(QuestId, *Row);
				bMoved = true;
			}
			else if (bStepped && !IsObjectiveMet(Row->Objectives[Progress.Step], Progress))
			{
				Notice(FString::Printf(TEXT("%s: %s"), *Row->DisplayName, *Row->Steps[Progress.Step]));
			}
		}
	}

	bAdvancing = false;
}

void UUnderpaintingProgress::CompleteQuest(FName QuestId, const FQuestRow& Row)
{
	Data.Quests[QuestId].bComplete = true;
	Notice(FString::Printf(TEXT("Quest complete: %s"), *Row.DisplayName));
	for (const FIdCount& Reward : Row.RewardItems)
	{
		AddItem(Reward.Id, Reward.Count);
	}
	for (const FIdCount& Reward : Row.RewardPigments)
	{
		AddPigment(Reward.Id, Reward.Count, false);
	}
}

// --- Flags ---

bool UUnderpaintingProgress::HasFlag(FName Flag) const
{
	if (Data.Flags.Contains(Flag))
	{
		return true;
	}

	// Automatic quest flags: Q.<Quest>.Active, Q.<Quest>.Done, Q.<Quest>.Step<N> (N counts from 1).
	const FString Text = Flag.ToString();
	TArray<FString> Parts;
	if (Text.ParseIntoArray(Parts, TEXT(".")) == 3 && Parts[0] == TEXT("Q"))
	{
		const FName QuestId(*Parts[1]);
		const FQuestProgress* Progress = Data.Quests.Find(QuestId);
		if (!Progress)
		{
			return false;
		}
		if (Parts[2] == TEXT("Active")) { return !Progress->bComplete; }
		if (Parts[2] == TEXT("Done")) { return Progress->bComplete; }
		if (Parts[2].StartsWith(TEXT("Step")))
		{
			return !Progress->bComplete && Progress->Step == FCString::Atoi(*Parts[2].RightChop(4)) - 1;
		}
	}
	return false;
}

void UUnderpaintingProgress::SetFlag(FName Flag)
{
	if (!Flag.IsNone())
	{
		Data.Flags.AddUnique(Flag);
		Changed();
	}
}

bool UUnderpaintingProgress::PassesFlags(const TArray<FName>& Require, const TArray<FName>& Forbid) const
{
	for (const FName& Flag : Require)
	{
		if (!HasFlag(Flag)) { return false; }
	}
	for (const FName& Flag : Forbid)
	{
		if (HasFlag(Flag)) { return false; }
	}
	return true;
}

// --- Dialogue ---

FName UUnderpaintingProgress::FindDialogueEntry(FName NpcId) const
{
	FName Best = NAME_None;
	int32 BestPriority = 0;
	if (Dialogue)
	{
		for (const TPair<FName, uint8*>& Pair : Dialogue->GetRowMap())
		{
			const FDialogueRow* Row = reinterpret_cast<const FDialogueRow*>(Pair.Value);
			if (Row->Npc == NpcId && Row->Entry > BestPriority && PassesFlags(Row->RequireFlags, Row->ForbidFlags))
			{
				Best = Pair.Key;
				BestPriority = Row->Entry;
			}
		}
	}
	return Best;
}

const FDialogueRow* UUnderpaintingProgress::GetDialogueRow(FName RowId) const
{
	return Find<FDialogueRow>(Dialogue, RowId);
}

TArray<int32> UUnderpaintingProgress::GetOfferedChoices(const FDialogueRow& Row) const
{
	TArray<int32> Result;
	for (int32 Index = 0; Index < Row.Choices.Num(); ++Index)
	{
		if (PassesFlags(Row.Choices[Index].RequireFlags, Row.Choices[Index].ForbidFlags))
		{
			Result.Add(Index);
		}
	}
	return Result;
}

void UUnderpaintingProgress::EnterDialogueRow(FName RowId)
{
	if (const FDialogueRow* Row = GetDialogueRow(RowId))
	{
		for (const FName& Flag : Row->SetFlags)
		{
			Data.Flags.AddUnique(Flag);
		}
	}
}

FName UUnderpaintingProgress::ChooseDialogue(FName RowId, int32 ChoiceIndex)
{
	const FDialogueRow* Row = GetDialogueRow(RowId);
	if (!Row || !Row->Choices.IsValidIndex(ChoiceIndex))
	{
		return NAME_None;
	}

	const FDialogueChoice& Choice = Row->Choices[ChoiceIndex];
	for (const FName& Flag : Choice.SetFlags)
	{
		Data.Flags.AddUnique(Flag);
	}
	if (!Choice.StartQuest.IsNone())
	{
		StartQuest(Choice.StartQuest);
	}
	return Choice.Next;
}

void UUnderpaintingProgress::EndDialogue(FName NpcId)
{
	NotifyEvent(EVENT_Talk, NpcId, 1);
}

// --- The world ---

bool UUnderpaintingProgress::RelightLampstand(FName LampstandId)
{
	if (IsLampstandLit(LampstandId))
	{
		return false;
	}
	if (!RemoveItem(ITEM_LampOil, 1))
	{
		Notice(TEXT("The Lampstand is guttering. It needs Lamp Oil."));
		return false;
	}
	Data.LitLampstands.Add(LampstandId);
	Notice(TEXT("The Lampstand burns clean."));
	NotifyEvent(EVENT_Relight, LampstandId, 1);
	return true;
}

int32 UUnderpaintingProgress::HarvestNode(FName NodeKey, FName PigmentId, int32 Yield)
{
	if (IsNodeDepleted(NodeKey) || !IsPigment(PigmentId))
	{
		return 0;
	}
	Data.DepletedNodes.Add(NodeKey);
	AddPigment(PigmentId, Yield, true);
	return Yield;
}

void UUnderpaintingProgress::Rest()
{
	// Wet pigment dries and is safe from now on.
	for (TPair<FName, int32>& Wet : Data.WetPigment)
	{
		if (Wet.Value > 0)
		{
			Data.DryPigment.FindOrAdd(Wet.Key) += Wet.Value;
			Wet.Value = 0;
		}
	}

	// Nodes regrow. (Per-pigment regrowth rules, such as Slumber Blue never regrowing, arrive with the forest and vault.)
	Data.DepletedNodes.Reset();

	// Resting tops Moss Poultices up to 2 for free, once you know how to make them.
	if (IsQuestComplete(FirstQuest) && GetItemCount(ITEM_MossPoultice) < 2)
	{
		Data.Items.FindOrAdd(ITEM_MossPoultice) = 2;
	}

	Notice(TEXT("You rest. Your pigment dries."));
	AdvanceQuests();
	Changed();
}

void UUnderpaintingProgress::ApplyDeathPenalty()
{
	Data.Deaths++;
	if (Data.Deaths == 1)
	{
		Notice(TEXT("The first time, the dark takes nothing."));
		Changed();
		return;
	}

	for (TPair<FName, int32>& Wet : Data.WetPigment)
	{
		// Rounded down, so a single wet unit is always safe. Slumber Blue is never lost: it cannot be found again.
		const FPigmentRow* Pigment = Find<FPigmentRow>(Pigments, Wet.Key);
		if (Pigment && Pigment->DeathLoss == EPigmentDeathLoss::Half)
		{
			Wet.Value -= Wet.Value / 2;
		}
	}
	Notice(TEXT("Half of your wet pigment is gone."));
	Changed();
}

// --- Exploration ---

const FLoreRow* UUnderpaintingProgress::GetLore(FName LoreId) const
{
	return Find<FLoreRow>(LoreTable, LoreId);
}

void UUnderpaintingProgress::CountLore(ELoreKind Kind, int32& OutFound, int32& OutTotal) const
{
	OutFound = 0;
	OutTotal = 0;
	if (LoreTable)
	{
		for (const TPair<FName, uint8*>& Pair : LoreTable->GetRowMap())
		{
			if (reinterpret_cast<const FLoreRow*>(Pair.Value)->Kind == Kind)
			{
				OutTotal++;
				OutFound += Data.FoundLore.Contains(Pair.Key) ? 1 : 0;
			}
		}
	}
}

bool UUnderpaintingProgress::FindLore(FName LoreId)
{
	const FLoreRow* Row = GetLore(LoreId);
	if (!Row || Data.FoundLore.Contains(LoreId))
	{
		return false;
	}

	Data.FoundLore.Add(LoreId);
	int32 Found = 0, Total = 0;
	CountLore(Row->Kind, Found, Total);
	const TCHAR* KindName = Row->Kind == ELoreKind::Marginalia ? TEXT("Marginalia") : (Row->Kind == ELoreKind::FieldNote ? TEXT("Odile's notes") : TEXT("Ledgers"));
	Notice(FString::Printf(TEXT("%s: %d of %d"), KindName, Found, Total));
	NotifyEvent(TEXT("Inspect"), LoreId, 1);
	return true;
}

void UUnderpaintingProgress::ReachPlace(FName PlaceId, const FString& DisplayName)
{
	if (PlaceId.IsNone())
	{
		return;
	}
	if (!Data.DiscoveredPlaces.Contains(PlaceId))
	{
		Data.DiscoveredPlaces.Add(PlaceId);
		if (!DisplayName.IsEmpty())
		{
			Notice(DisplayName);
		}
	}
	NotifyEvent(TEXT("Reach"), PlaceId, 1);
}

bool UUnderpaintingProgress::OpenCache(FName CacheKey, const TArray<FIdCount>& Contents)
{
	if (IsCacheOpened(CacheKey))
	{
		return false;
	}
	Data.OpenedCaches.Add(CacheKey);
	for (const FIdCount& Entry : Contents)
	{
		if (IsPigment(Entry.Id)) { AddPigment(Entry.Id, Entry.Count, true); }
		else { AddItem(Entry.Id, Entry.Count); }
	}
	Changed();
	return true;
}

bool UUnderpaintingProgress::FixPlatform(FName PlatformKey)
{
	if (IsPlatformFixed(PlatformKey))
	{
		return false;
	}
	if (!RemoveItem(TEXT("FixativeFlask"), 1))
	{
		Notice(TEXT("You have no Fixative Flask."));
		return false;
	}
	Data.FixedPlatforms.Add(PlatformKey);
	Notice(TEXT("Fixed. It will hold now, lit or not."));
	Changed();
	return true;
}

FName UUnderpaintingProgress::GetCurrentObjectiveTarget(FName QuestId) const
{
	const FQuestRow* Row = Find<FQuestRow>(QuestsTable, QuestId);
	const FQuestProgress* Progress = Data.Quests.Find(QuestId);
	return Row && Progress && Row->Objectives.IsValidIndex(Progress->Step) ? Row->Objectives[Progress->Step].Target : NAME_None;
}
