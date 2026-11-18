#include "UnderpaintingDataSubsystem.h"

#include "Underpainting.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	template <typename RowType>
	const RowType* FindRowIn(const UDataTable* Table, FName RowName)
	{
		return Table ? Table->FindRow<RowType>(RowName, TEXT("UnderpaintingData"), /*bWarnIfRowMissing*/ false) : nullptr;
	}
}

void UUnderpaintingDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadAll();
}

FString UUnderpaintingDataSubsystem::GetDataDirectory()
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("data"));
}

UDataTable* UUnderpaintingDataSubsystem::LoadTableFromJson(UObject* Outer, const FString& FileStem, UScriptStruct* RowStruct, TArray<FString>& OutProblems)
{
	const FString Path = FPaths::Combine(GetDataDirectory(), FileStem + TEXT(".json"));

	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		OutProblems.Add(FString::Printf(TEXT("%s: could not read %s"), *FileStem, *Path));
		return nullptr;
	}

	UDataTable* Table = NewObject<UDataTable>(Outer ? Outer : GetTransientPackage(), FName(*FString::Printf(TEXT("DT_%s"), *FileStem)), RF_Transient);
	Table->RowStruct = RowStruct;

	// Strict on purpose: a key in the JSON with no matching field (or the reverse) is a mistake we want to hear about.
	Table->bIgnoreExtraFields = false;
	Table->bIgnoreMissingFields = false;

	for (const FString& Problem : Table->CreateTableFromJSONString(Json))
	{
		OutProblems.Add(FString::Printf(TEXT("%s: %s"), *FileStem, *Problem));
	}

	if (Table->GetRowMap().Num() == 0)
	{
		OutProblems.Add(FString::Printf(TEXT("%s: no rows loaded"), *FileStem));
	}

	return Table;
}

int32 UUnderpaintingDataSubsystem::ReloadAll()
{
	TArray<FString> Problems;

	Pigments = LoadTableFromJson(this, TEXT("pigments"), FPigmentRow::StaticStruct(), Problems);
	Items = LoadTableFromJson(this, TEXT("items"), FItemRow::StaticStruct(), Problems);
	Recipes = LoadTableFromJson(this, TEXT("recipes"), FRecipeRow::StaticStruct(), Problems);
	Attunements = LoadTableFromJson(this, TEXT("attunements"), FAttunementRow::StaticStruct(), Problems);
	Enemies = LoadTableFromJson(this, TEXT("enemies"), FEnemyRow::StaticStruct(), Problems);
	Quests = LoadTableFromJson(this, TEXT("quests"), FQuestRow::StaticStruct(), Problems);
	Regions = LoadTableFromJson(this, TEXT("regions"), FRegionRow::StaticStruct(), Problems);
	Dialogue = LoadTableFromJson(this, TEXT("dialogue"), FDialogueRow::StaticStruct(), Problems);
	Lore = LoadTableFromJson(this, TEXT("lore"), FLoreRow::StaticStruct(), Problems);

	for (const FString& Problem : Problems)
	{
		UE_LOG(LogUnderpainting, Error, TEXT("Game data: %s"), *Problem);
	}

	if (Problems.IsEmpty())
	{
		UE_LOG(LogUnderpainting, Log, TEXT("Game data loaded from %s"), *GetDataDirectory());
	}

	return Problems.Num();
}

const FPigmentRow* UUnderpaintingDataSubsystem::FindPigment(FName RowName) const { return FindRowIn<FPigmentRow>(Pigments, RowName); }
const FItemRow* UUnderpaintingDataSubsystem::FindItem(FName RowName) const { return FindRowIn<FItemRow>(Items, RowName); }
const FRecipeRow* UUnderpaintingDataSubsystem::FindRecipe(FName RowName) const { return FindRowIn<FRecipeRow>(Recipes, RowName); }
const FAttunementRow* UUnderpaintingDataSubsystem::FindAttunement(FName RowName) const { return FindRowIn<FAttunementRow>(Attunements, RowName); }
const FEnemyRow* UUnderpaintingDataSubsystem::FindEnemy(FName RowName) const { return FindRowIn<FEnemyRow>(Enemies, RowName); }
const FQuestRow* UUnderpaintingDataSubsystem::FindQuest(FName RowName) const { return FindRowIn<FQuestRow>(Quests, RowName); }
const FRegionRow* UUnderpaintingDataSubsystem::FindRegion(FName RowName) const { return FindRowIn<FRegionRow>(Regions, RowName); }

FLinearColor UUnderpaintingDataSubsystem::HexToColour(const FString& Hex)
{
	FString Digits = Hex;
	Digits.RemoveFromStart(TEXT("#"));

	if (Digits.Len() != 6)
	{
		return FLinearColor(1.0f, 0.0f, 1.0f);
	}

	for (const TCHAR Char : Digits)
	{
		if (!FChar::IsHexDigit(Char))
		{
			return FLinearColor(1.0f, 0.0f, 1.0f);
		}
	}

	// The data stores colours as they look on screen (sRGB), so convert to linear for rendering.
	return FLinearColor::FromSRGBColor(FColor::FromHex(Digits));
}

FName UUnderpaintingDataSubsystem::FindZoneForLevel(const FString& LevelName) const
{
	if (Regions)
	{
		for (const TPair<FName, uint8*>& Pair : Regions->GetRowMap())
		{
			const FRegionRow* Row = reinterpret_cast<const FRegionRow*>(Pair.Value);
			if (Row->RowType == ERegionRowType::Zone && (Row->LevelBefore.ToString() == LevelName || Row->LevelAfter.ToString() == LevelName))
			{
				return Pair.Key;
			}
		}
	}
	return NAME_None;
}
