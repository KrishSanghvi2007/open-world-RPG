// Automated check of the walking skeleton's rules, played through as the first quest: talk, harvest, craft,
// relight, rest, attune, talk again, then save and load. Run with: Automation RunTests Underpainting.Game

#include "Misc/AutomationTest.h"

#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingProgress.h"
#include "Engine/DataTable.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUnderpaintingProgress* MakeProgress(FAutomationTestBase& Test)
	{
		TArray<FString> Problems;
		UObject* Outer = GetTransientPackage();
		UDataTable* Pigments = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("pigments"), FPigmentRow::StaticStruct(), Problems);
		UDataTable* Items = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("items"), FItemRow::StaticStruct(), Problems);
		UDataTable* Recipes = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("recipes"), FRecipeRow::StaticStruct(), Problems);
		UDataTable* Attunements = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("attunements"), FAttunementRow::StaticStruct(), Problems);
		UDataTable* Quests = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("quests"), FQuestRow::StaticStruct(), Problems);
		UDataTable* Dialogue = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("dialogue"), FDialogueRow::StaticStruct(), Problems);
		for (const FString& Problem : Problems)
		{
			Test.AddError(Problem);
		}
		UUnderpaintingProgress* Progress = NewObject<UUnderpaintingProgress>();
		UDataTable* Lore = UUnderpaintingDataSubsystem::LoadTableFromJson(Outer, TEXT("lore"), FLoreRow::StaticStruct(), Problems);
		Progress->SetTables(Pigments, Items, Recipes, Attunements, Quests, Dialogue, Lore);
		Progress->NewGame();
		return Progress;
	}

	/** Plays a conversation to its end, always taking the first reply. Returns the rows visited. */
	TArray<FName> Talk(UUnderpaintingProgress* Progress, FName Npc)
	{
		TArray<FName> Visited;
		FName Row = Progress->FindDialogueEntry(Npc);
		while (!Row.IsNone() && Visited.Num() < 20)
		{
			Visited.Add(Row);
			Progress->EnterDialogueRow(Row);
			const FDialogueRow* Data = Progress->GetDialogueRow(Row);
			const TArray<int32> Offered = Progress->GetOfferedChoices(*Data);
			Row = Offered.Num() > 0 ? Progress->ChooseDialogue(Row, Offered[0]) : Data->Next;
		}
		Progress->EndDialogue(Npc);
		return Visited;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingFirstQuestTest, "Underpainting.Game.FirstQuestPlaythrough", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingFirstQuestTest::RunTest(const FString& Parameters)
{
	UUnderpaintingProgress* Progress = MakeProgress(*this);
	const FName Quest = UUnderpaintingProgress::FirstQuest;
	const FName Ochre(TEXT("Ochre")), Linseed(TEXT("LinseedOil")), LampOil(TEXT("LampOil")), Varnish(TEXT("Varnish"));

	TestTrue(TEXT("A new game starts the first quest"), Progress->IsQuestActive(Quest));
	TestEqual(TEXT("Step 1"), Progress->GetQuestProgress(Quest)->Step, 0);
	TestTrue(TEXT("Step flag"), Progress->HasFlag(TEXT("Q.MQ_DuskOnTheShoulder.Step1")));

	// 1. Talk to Hesper.
	TestEqual(TEXT("Hesper opens with her introduction"), Talk(Progress, TEXT("HesperVane"))[0], FName(TEXT("Hesper_Intro")));
	TestEqual(TEXT("Talking moves to step 2"), Progress->GetQuestProgress(Quest)->Step, 1);
	TestEqual(TEXT("Hesper now only reminds you"), Progress->FindDialogueEntry(TEXT("HesperVane")), FName(TEXT("Hesper_Waiting")));

	// 2. Harvest: two nodes of 2 make the 4 Ochre the step asks for. A node cannot be taken twice.
	TestEqual(TEXT("First node"), Progress->HarvestNode(TEXT("Village.Node1"), Ochre), 2);
	TestEqual(TEXT("Same node again"), Progress->HarvestNode(TEXT("Village.Node1"), Ochre), 0);
	TestEqual(TEXT("Still on the harvest step"), Progress->GetQuestProgress(Quest)->Step, 1);
	Progress->HarvestNode(TEXT("Village.Node2"), Ochre);
	Progress->HarvestNode(TEXT("Village.Node3"), Ochre);
	Progress->HarvestNode(TEXT("Village.Node4"), Ochre);
	TestEqual(TEXT("8 wet Ochre"), Progress->GetWetPigment(Ochre), 8);
	TestEqual(TEXT("Harvest step done"), Progress->GetQuestProgress(Quest)->Step, 2);

	// 3. Linseed oil. 4. Craft Lamp Oil.
	FString Why;
	TestFalse(TEXT("Cannot craft without linseed"), Progress->CanCraft(TEXT("Recipe_LampOil"), &Why));
	Progress->AddItem(Linseed, 2);
	TestEqual(TEXT("Linseed step done"), Progress->GetQuestProgress(Quest)->Step, 3);
	TestFalse(TEXT("Poultices are not learned yet"), Progress->IsRecipeUnlocked(TEXT("Recipe_MossPoultice")));
	TestTrue(TEXT("Craft Lamp Oil"), Progress->Craft(TEXT("Recipe_LampOil")));
	TestEqual(TEXT("Lamp Oil in the pack"), Progress->GetItemCount(LampOil), 1);
	TestEqual(TEXT("Crafting spent 1 Ochre"), Progress->GetPigment(Ochre), 7);
	TestEqual(TEXT("Craft step done"), Progress->GetQuestProgress(Quest)->Step, 4);

	// 5. Relight the Lampstand.
	TestFalse(TEXT("Unlit at first"), Progress->IsLampstandLit(Varnish));
	TestTrue(TEXT("Relight"), Progress->RelightLampstand(Varnish));
	TestEqual(TEXT("The oil is used up"), Progress->GetItemCount(LampOil), 0);
	TestEqual(TEXT("Relight step done"), Progress->GetQuestProgress(Quest)->Step, 5);

	// 6. Attunement takes dry pigment, so it needs a rest first.
	TestFalse(TEXT("Wet pigment cannot be attuned"), Progress->CanAttune(TEXT("Ochre_T1"), &Why));
	Progress->Rest();
	TestEqual(TEXT("Resting dries pigment"), Progress->GetDryPigment(Ochre), 7);
	TestFalse(TEXT("Nodes regrow on rest"), Progress->IsNodeDepleted(TEXT("Village.Node1")));
	TestFalse(TEXT("Tier 2 needs tier 1"), Progress->CanAttune(TEXT("Ochre_T2"), &Why));
	TestTrue(TEXT("Attune Warm Ground"), Progress->Attune(TEXT("Ochre_T1")));
	TestEqual(TEXT("Attunement cost 3 dry Ochre"), Progress->GetDryPigment(Ochre), 4);
	TestEqual(TEXT("Warm Ground gives +20 Glim"), Progress->GetAttunementValue(TEXT("MaxGlim")), 20.0f);
	TestEqual(TEXT("Attune step done"), Progress->GetQuestProgress(Quest)->Step, 6);

	// 7. Back to Hesper for the lantern.
	TestEqual(TEXT("Hesper gives the lantern"), Talk(Progress, TEXT("HesperVane"))[0], FName(TEXT("Hesper_Lantern")));
	TestTrue(TEXT("Quest complete"), Progress->IsQuestComplete(Quest));
	TestEqual(TEXT("Pilot Lantern rewarded"), Progress->GetItemCount(TEXT("PilotLantern")), 1);
	TestEqual(TEXT("Moss Poultices rewarded"), Progress->GetItemCount(TEXT("MossPoultice")), 3);
	TestTrue(TEXT("Finishing the quest teaches the Moss Poultice"), Progress->IsRecipeUnlocked(TEXT("Recipe_MossPoultice")));
	TestTrue(TEXT("Done flag"), Progress->HasFlag(TEXT("Q.MQ_DuskOnTheShoulder.Done")));
	TestEqual(TEXT("Hesper's line after the quest"), Progress->FindDialogueEntry(TEXT("HesperVane")), FName(TEXT("Hesper_After")));

	// Save and load: the saved data survives being written out and read back.
	FUnderpaintingProgressData Saved = Progress->GetData();
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes, true);
		FObjectAndNameAsStringProxyArchive Archive(Writer, true);
		FUnderpaintingProgressData::StaticStruct()->SerializeItem(Archive, &Saved, nullptr);
	}
	FUnderpaintingProgressData Loaded;
	{
		FMemoryReader Reader(Bytes, true);
		FObjectAndNameAsStringProxyArchive Archive(Reader, true);
		FUnderpaintingProgressData::StaticStruct()->SerializeItem(Archive, &Loaded, nullptr);
	}
	UUnderpaintingProgress* Restored = MakeProgress(*this);
	Restored->SetData(Loaded);
	TestTrue(TEXT("Loaded: quest complete"), Restored->IsQuestComplete(Quest));
	TestEqual(TEXT("Loaded: dry Ochre"), Restored->GetDryPigment(Ochre), 4);
	TestTrue(TEXT("Loaded: attunement"), Restored->HasAttunement(TEXT("Ochre_T1")));
	TestTrue(TEXT("Loaded: Lampstand lit"), Restored->IsLampstandLit(Varnish));
	TestEqual(TEXT("Loaded: lantern"), Restored->GetItemCount(TEXT("PilotLantern")), 1);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingDeathRuleTest, "Underpainting.Game.DeathRule", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingDeathRuleTest::RunTest(const FString& Parameters)
{
	UUnderpaintingProgress* Progress = MakeProgress(*this);
	const FName Ochre(TEXT("Ochre")), Blue(TEXT("SlumberBlue"));
	Progress->AddPigment(Ochre, 5, true);
	Progress->AddPigment(Ochre, 3, false);
	Progress->AddPigment(Blue, 4, true);

	Progress->ApplyDeathPenalty();
	TestEqual(TEXT("The first death costs nothing"), Progress->GetWetPigment(Ochre), 5);

	Progress->ApplyDeathPenalty();
	TestEqual(TEXT("Half of wet pigment is lost, rounded down (5 loses 2)"), Progress->GetWetPigment(Ochre), 3);
	TestEqual(TEXT("Dry pigment is safe"), Progress->GetDryPigment(Ochre), 3);
	TestEqual(TEXT("Slumber Blue is never lost"), Progress->GetWetPigment(Blue), 4);

	Progress->ApplyDeathPenalty();
	Progress->ApplyDeathPenalty();
	TestEqual(TEXT("A single wet unit is always safe"), Progress->GetWetPigment(Ochre), 1);
	Progress->ApplyDeathPenalty();
	TestEqual(TEXT("Still one"), Progress->GetWetPigment(Ochre), 1);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingExplorationTest, "Underpainting.Game.Exploration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingExplorationTest::RunTest(const FString& Parameters)
{
	UUnderpaintingProgress* Progress = MakeProgress(*this);

	// Lore is counted by kind and only once.
	int32 Found = 0, Total = 0;
	Progress->CountLore(ELoreKind::Marginalia, Found, Total);
	TestEqual(TEXT("Eight marginalia in Act 1"), Total, 8);
	Progress->CountLore(ELoreKind::FieldNote, Found, Total);
	TestEqual(TEXT("Seven of Odile's notes"), Total, 7);
	TestTrue(TEXT("First reading counts"), Progress->FindLore(TEXT("Odile_01")));
	TestFalse(TEXT("Second reading does not"), Progress->FindLore(TEXT("Odile_01")));
	Progress->CountLore(ELoreKind::FieldNote, Found, Total);
	TestEqual(TEXT("One found"), Found, 1);
	TestFalse(TEXT("Unknown lore is refused"), Progress->FindLore(TEXT("NoSuchNote")));

	// A cache gives its contents once: pigment arrives wet, items go to the pack.
	TArray<FIdCount> Contents;
	Contents.Add({ TEXT("SlumberBlue"), 2 });
	Contents.Add({ TEXT("ShedHide"), 3 });
	TestTrue(TEXT("Cache opens"), Progress->OpenCache(TEXT("Forest.Cache1"), Contents));
	TestEqual(TEXT("Slumber Blue from the cache"), Progress->GetWetPigment(TEXT("SlumberBlue")), 2);
	TestEqual(TEXT("Shed Hide from the cache"), Progress->GetItemCount(TEXT("ShedHide")), 3);
	TestFalse(TEXT("A cache cannot be emptied twice"), Progress->OpenCache(TEXT("Forest.Cache1"), Contents));

	// Fixing a sketched platform costs a Fixative Flask and lasts.
	TestFalse(TEXT("No flask, no fix"), Progress->FixPlatform(TEXT("Vault.Platform1")));
	Progress->AddItem(TEXT("FixativeFlask"), 1);
	TestTrue(TEXT("Fix with a flask"), Progress->FixPlatform(TEXT("Vault.Platform1")));
	TestTrue(TEXT("It stays fixed"), Progress->IsPlatformFixed(TEXT("Vault.Platform1")));
	TestEqual(TEXT("The flask is used up"), Progress->GetItemCount(TEXT("FixativeFlask")), 0);

	// Reaching a named place is remembered, and counts for quests every time.
	TestFalse(TEXT("Not yet discovered"), Progress->HasDiscovered(TEXT("RibGate")));
	Progress->ReachPlace(TEXT("RibGate"), TEXT("The Rib Gate"));
	TestTrue(TEXT("Discovered"), Progress->HasDiscovered(TEXT("RibGate")));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
