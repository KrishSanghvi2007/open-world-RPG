// Automated check that every file in data/ loads cleanly into its row struct.
// Run from the editor's Session Frontend, or headless with: Automation RunTests Underpainting.Data

#include "Misc/AutomationTest.h"

#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingDataTypes.h"
#include "Engine/DataTable.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingDataLoadTest, "Underpainting.Data.LoadAll", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingDataLoadTest::RunTest(const FString& Parameters)
{
	struct FFileSpec
	{
		const TCHAR* Stem;
		UScriptStruct* RowStruct;
	};

	const FFileSpec Files[] = {
		{ TEXT("pigments"), FPigmentRow::StaticStruct() },
		{ TEXT("items"), FItemRow::StaticStruct() },
		{ TEXT("recipes"), FRecipeRow::StaticStruct() },
		{ TEXT("attunements"), FAttunementRow::StaticStruct() },
		{ TEXT("enemies"), FEnemyRow::StaticStruct() },
		{ TEXT("quests"), FQuestRow::StaticStruct() },
		{ TEXT("regions"), FRegionRow::StaticStruct() },
	};

	TMap<FString, UDataTable*> Tables;

	for (const FFileSpec& File : Files)
	{
		TArray<FString> Problems;
		UDataTable* Table = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), File.Stem, File.RowStruct, Problems);

		for (const FString& Problem : Problems)
		{
			AddError(Problem);
		}

		if (TestNotNull(*FString::Printf(TEXT("%s table"), File.Stem), Table))
		{
			AddInfo(FString::Printf(TEXT("%s: %d rows"), File.Stem, Table->GetRowMap().Num()));
			Tables.Add(File.Stem, Table);
		}
	}

	// Spot checks that values (not just row counts) arrive: an enum, a nested struct array and a float.
	if (UDataTable** Pigments = Tables.Find(TEXT("pigments")))
	{
		const FPigmentRow* Ochre = (*Pigments)->FindRow<FPigmentRow>(TEXT("Ochre"), TEXT("Test"), false);
		if (TestNotNull(TEXT("Ochre row"), Ochre))
		{
			TestEqual(TEXT("Ochre hue"), Ochre->Hue, EPigmentHue::Orange);
			TestEqual(TEXT("Ochre complement"), Ochre->Complement, FName(TEXT("SlumberBlue")));
			TestTrue(TEXT("Ochre stroke multiplier is set"), Ochre->StrokeDamageMultiplier > 0.0f);
		}
	}

	if (UDataTable** Enemies = Tables.Find(TEXT("enemies")))
	{
		const FEnemyRow* Boss = (*Enemies)->FindRow<FEnemyRow>(TEXT("ThePalimpsest"), TEXT("Test"), false);
		if (TestNotNull(TEXT("ThePalimpsest row"), Boss))
		{
			TestEqual(TEXT("Boss tier"), Boss->Tier, EEnemyTier::Boss);
			TestTrue(TEXT("Boss has attacks"), Boss->Attacks.Num() > 0);
			TestTrue(TEXT("Boss has drops"), Boss->Drops.Num() > 0);
			TestEqual(TEXT("Boss phase 2 hue"), Boss->Phase2Hue, FName(TEXT("Red")));
		}
	}

	if (UDataTable** Regions = Tables.Find(TEXT("regions")))
	{
		const FRegionRow* Vault = (*Regions)->FindRow<FRegionRow>(TEXT("GrisailleVault"), TEXT("Test"), false);
		if (TestNotNull(TEXT("GrisailleVault row"), Vault))
		{
			TestEqual(TEXT("Vault paint state"), Vault->PaintState, EPaintState::Sketch);
			TestEqual(TEXT("Vault node counts match pigments"), Vault->HarvestNodeCounts.Num(), Vault->Pigments.Num());
		}
	}

	TestEqual(TEXT("Hex colour"), UUnderpaintingDataSubsystem::HexToColour(TEXT("#FF0000")), FLinearColor(1.0f, 0.0f, 0.0f));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
