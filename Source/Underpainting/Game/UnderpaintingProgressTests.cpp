// Automated check of the walking skeleton's rules, played through as the first quest: talk, harvest, craft,
// relight, rest, attune, talk again, then save and load. Run with: Automation RunTests Underpainting.Game

#include "Misc/AutomationTest.h"
#include "UnderpaintingSettings.h"

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

	// The first pigment you open goes into the first well and becomes the active one; passives need a well.
	TestEqual(TEXT("Ochre is in the first well"), Progress->GetWell(0), Ochre);
	TestEqual(TEXT("Ochre is the active pigment"), Progress->GetActivePigment(), Ochre);
	TestTrue(TEXT("The second well is empty"), Progress->GetWell(1).IsNone());
	TestFalse(TEXT("A pigment you have not opened cannot be placed"), Progress->SetWell(1, TEXT("Sanguine")));
	Progress->SetWell(0, NAME_None);
	TestEqual(TEXT("Out of its well, Warm Ground gives nothing"), Progress->GetAttunementValue(TEXT("MaxGlim")), 0.0f);
	Progress->SetWell(0, Ochre);
	TestEqual(TEXT("Back in its well, +20 again"), Progress->GetAttunementValue(TEXT("MaxGlim")), 20.0f);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingChoiceTest, "Underpainting.Game.QuestChoices", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingChoiceTest::RunTest(const FString& Parameters)
{
	// "The Tick-Warden's Ledger", played twice with different endings: the world must come out different.
	const FName Quest(TEXT("SQ_TickWardensLedger")), Forest(TEXT("Scumblewood")), Sanguine(TEXT("Sanguine"));
	const auto FinishFirstQuest = [](UUnderpaintingProgress* Progress)
	{
		// Side quests open once "Dusk on the Shoulder" is done.
		FUnderpaintingProgressData Data = Progress->GetData();
		FQuestProgress Done;
		Done.bComplete = true;
		Data.Quests.Add(UUnderpaintingProgress::FirstQuest, Done);
		Progress->SetData(Data);
	};
	const auto PlayToChoice = [&](UUnderpaintingProgress* Progress)
	{
		TestFalse(TEXT("A side quest cannot start before the first quest is done"), Progress->StartQuest(Quest));
		FinishFirstQuest(Progress);
		if (!TestTrue(TEXT("Then it starts"), Progress->StartQuest(Quest)))
		{
			return;
		}
		Progress->EndDialogue(TEXT("IlkaThrum"));
		TestFalse(TEXT("No deciding before the wells are read"), Progress->IsAwaitingChoice(Quest));
		Progress->NotifyEvent(TEXT("Inspect"), TEXT("BloodWell"), 1);
		Progress->NotifyEvent(TEXT("Inspect"), TEXT("BloodWell"), 1);
		TestEqual(TEXT("Two wells are not three"), Progress->GetQuestProgress(Quest) ? Progress->GetQuestProgress(Quest)->Step : -1, 1);
		Progress->NotifyEvent(TEXT("Inspect"), TEXT("BloodWell"), 1);
		Progress->ReachPlace(TEXT("BroodNest"), TEXT("The Hollow Vertebra"));
		TestTrue(TEXT("At the nest, the quest waits for a decision"), Progress->IsAwaitingChoice(Quest));
	};

	UUnderpaintingProgress* Burn = MakeProgress(*this);
	PlayToChoice(Burn);
	TestEqual(TEXT("Wells give 2 before anything is decided"), Burn->GetNodeYield(Forest, Sanguine, 2), 2);
	TestTrue(TEXT("Burning costs nothing"), Burn->MakeChoice(Quest, TEXT("BurnNest")));
	TestTrue(TEXT("The quest is over"), Burn->IsQuestComplete(Quest));
	TestTrue(TEXT("Flag: the ticks are culled"), Burn->HasFlag(TEXT("Scumblewood_MadderbackHalved")));
	TestEqual(TEXT("Burned: the wells scab and give 1"), Burn->GetNodeYield(Forest, Sanguine, 2), 1);
	TestFalse(TEXT("A decision cannot be made twice"), Burn->MakeChoice(Quest, TEXT("TendWells")));
	TestTrue(TEXT("Either ending teaches the Chitin Jerkin"), Burn->IsRecipeUnlocked(TEXT("Recipe_ChitinJerkin")));

	UUnderpaintingProgress* Tend = MakeProgress(*this);
	PlayToChoice(Tend);
	FString Why;
	TestFalse(TEXT("Dressing the wells needs 3 Terre Verte"), Tend->CanChoose(Quest, TEXT("TendWells"), &Why));
	Tend->AddPigment(TEXT("TerreVerte"), 3, true);
	TestTrue(TEXT("With the pigment, it can be done"), Tend->MakeChoice(Quest, TEXT("TendWells")));
	TestEqual(TEXT("The pigment is spent"), Tend->GetPigment(TEXT("TerreVerte")), 0);
	TestEqual(TEXT("Tended: the wells run clean and give 3"), Tend->GetNodeYield(Forest, Sanguine, 2), 3);
	TestFalse(TEXT("Tended: the ticks are left alone"), Tend->HasFlag(TEXT("Scumblewood_MadderbackHalved")));
	TestTrue(TEXT("Tended: Ilka trades"), Tend->HasFlag(TEXT("Ilka_Trades")));

	// Leaving the Heart wall bare is a real option with its own reward.
	UUnderpaintingProgress* Heart = MakeProgress(*this);
	FUnderpaintingProgressData Data = Heart->GetData();
	FQuestProgress Done; Done.bComplete = true;
	Data.Quests.Add(UUnderpaintingProgress::FirstQuest, Done);
	FQuestProgress AtWall; AtWall.Step = 5;
	Data.Quests.Add(TEXT("MQ_ColdUnderTheRibs"), AtWall);
	Heart->SetData(Data);
	TestTrue(TEXT("At the Heart wall"), Heart->IsAwaitingChoice(TEXT("MQ_ColdUnderTheRibs")));
	TestFalse(TEXT("Sanguine needs 5"), Heart->CanChoose(TEXT("MQ_ColdUnderTheRibs"), TEXT("FillSanguine"), &Why));
	TestTrue(TEXT("Leave it bare"), Heart->MakeChoice(TEXT("MQ_ColdUnderTheRibs"), TEXT("LeaveBlank")));
	TestEqual(TEXT("Bare: 4 Ceruse scraped from the scar"), Heart->GetPigment(TEXT("Ceruse")), 4);
	TestTrue(TEXT("Bare: the wall is recorded as blank"), Heart->HasFlag(TEXT("Heart_Blank")));

	// A place already found does not have to be found again for a quest.
	UUnderpaintingProgress* Corin = MakeProgress(*this);
	FinishFirstQuest(Corin);
	Corin->ReachPlace(TEXT("CorinsBack"), FString());
	Corin->StartQuest(TEXT("SQ_TheUnseenSide"));
	Corin->EndDialogue(TEXT("CorinHalfSeen"));
	TestEqual(TEXT("His back was already seen, so the quest moves straight to the second talk"), Corin->GetQuestProgress(TEXT("SQ_TheUnseenSide")) ? Corin->GetQuestProgress(TEXT("SQ_TheUnseenSide"))->Step : -1, 2);
	Corin->EndDialogue(TEXT("CorinHalfSeen"));
	TestTrue(TEXT("Leaving him unfinished is free"), Corin->MakeChoice(TEXT("SQ_TheUnseenSide"), TEXT("LeaveUnfinished")));
	TestTrue(TEXT("Unfinished: he reveals the cache"), Corin->HasFlag(TEXT("Scumblewood_PentimentoCacheRevealed")));
	TestEqual(TEXT("Unfinished: he greets you as himself"), Corin->FindDialogueEntry(TEXT("CorinHalfSeen")), FName(TEXT("Corin_Unfinished")));

	// Hesper's return: with the lantern she is offered it; without, she is not.
	UUnderpaintingProgress* Return = MakeProgress(*this);
	FUnderpaintingProgressData ReturnData = Return->GetData();
	ReturnData.Quests.Add(UUnderpaintingProgress::FirstQuest, Done);
	FQuestProgress LastStep; LastStep.Step = 7;
	ReturnData.Quests.Add(TEXT("MQ_ColdUnderTheRibs"), LastStep);
	Return->SetData(ReturnData);
	TestEqual(TEXT("Hesper asks if you found her"), Return->FindDialogueEntry(TEXT("HesperVane")), FName(TEXT("Hesper_Return")));
	if (const FDialogueRow* Ask = Return->GetDialogueRow(TEXT("Hesper_Return")))
	{
		const TArray<int32> Without = Return->GetOfferedChoices(*Ask);
		TestTrue(TEXT("Without the lantern, only the plain answer"), Without.Num() == 1 && Without[0] == 1);
		Return->AddItem(TEXT("OdilesLantern"), 1);
		const TArray<int32> With = Return->GetOfferedChoices(*Ask);
		TestTrue(TEXT("With the lantern, only holding it out"), With.Num() == 1 && With[0] == 0);
	}
	// Pour the flame into the Lampstand: the village gets her, and the lantern leaves your pack.
	TestEqual(TEXT("Holding it out leads to the flame"), Return->ChooseDialogue(TEXT("Hesper_Return"), 0), FName(TEXT("Hesper_Return_Flame")));
	TestEqual(TEXT("Pouring leads to her answer"), Return->ChooseDialogue(TEXT("Hesper_Return_Flame2"), 1), FName(TEXT("Hesper_Return_Pour")));
	TestTrue(TEXT("The flame is in the Lampstand"), Return->HasFlag(TEXT("Varnish_FlameInLamp")));
	TestFalse(TEXT("And was not kept"), Return->HasFlag(TEXT("Varnish_FlameKept")));
	TestEqual(TEXT("The lantern has been given"), Return->GetItemCount(TEXT("OdilesLantern")), 0);

	// Flags the game provides by itself.
	TestTrue(TEXT("Has.<pigment>"), Heart->HasFlag(TEXT("Has.Ceruse")));
	TestFalse(TEXT("Has.<item> not carried"), Heart->HasFlag(TEXT("Has.OdilesLantern")));
	Heart->NotifyEvent(TEXT("Kill"), TEXT("ThePalimpsest"), 1);
	TestTrue(TEXT("Killed.<enemy>"), Heart->HasFlag(TEXT("Killed.ThePalimpsest")));

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingCraftAllTest, "Underpainting.Game.CraftEverything", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingCraftAllTest::RunTest(const FString& Parameters)
{
	UUnderpaintingProgress* Progress = MakeProgress(*this);

	// Nothing but Lamp Oil is known at the start (a station lists only what you know).
	const auto Known = [Progress]()
	{
		TArray<FName> Result = Progress->GetRecipesForStation(ECraftingStation::GrindingSlab);
		Result.Append(Progress->GetRecipesForStation(ECraftingStation::ArmatureBench));
		return Result;
	};
	TestEqual(TEXT("One recipe known at the start"), Known().Num(), 1);

	// A full pack, and every recipe taught (what the F7 debug key does).
	for (const TCHAR* Pigment : { TEXT("Ochre"), TEXT("TerreVerte"), TEXT("Sanguine"), TEXT("BoneBlack"), TEXT("Ceruse"), TEXT("SlumberBlue") })
	{
		Progress->AddPigment(Pigment, 20, false);
	}
	for (const TCHAR* Material : { TEXT("LinseedOil"), TEXT("ShedHide"), TEXT("RibSplinter"), TEXT("TickChitin"), TEXT("GraphiteShard") })
	{
		Progress->AddItem(Material, 20);
	}
	Progress->SetFlag(TEXT("Dev.AllRecipes"));
	const TArray<FName> All = Known();
	TestEqual(TEXT("Ten recipes across the two stations"), All.Num(), 10);

	TestEqual(TEXT("Starts with the knife"), Progress->GetEquippedWeapon(), FName(TEXT("LimnersKnife")));
	TestEqual(TEXT("No extra Glim before the Wick"), Progress->GetUpgradeGlim(), 0.0f);
	for (const FName& Recipe : All)
	{
		FString Why;
		TestTrue(*FString::Printf(TEXT("Can craft %s (%s)"), *Recipe.ToString(), *Why), Progress->CanCraft(Recipe, &Why) && Progress->Craft(Recipe));
	}
	TestEqual(TEXT("Poultices come in twos"), Progress->GetItemCount(TEXT("MossPoultice")), 2);
	TestEqual(TEXT("The Braided Wick adds 20 Glim"), Progress->GetUpgradeGlim(), 20.0f);
	TestEqual(TEXT("Six pieces of gear: two to start, four made"), Progress->GetGear().Num(), 6);

	// Made gear goes on at once, and can be changed back.
	TestTrue(TEXT("A crafted weapon is in hand"), Progress->GetEquippedWeapon() != FName(TEXT("LimnersKnife")));
	TestTrue(TEXT("Back to the knife"), Progress->Equip(TEXT("LimnersKnife")));
	TestEqual(TEXT("The knife is in hand"), Progress->GetEquippedWeapon(), FName(TEXT("LimnersKnife")));
	TestTrue(TEXT("Put on the Chitin Jerkin"), Progress->Equip(TEXT("ChitinJerkin")));
	TestEqual(TEXT("Wearing the Jerkin"), Progress->GetEquippedArmour(), FName(TEXT("ChitinJerkin")));
	TestFalse(TEXT("Cannot equip a consumable"), Progress->Equip(TEXT("LampOil")));

	// All twelve attunement nodes can be bought from a full palette, tier 1 before tier 2.
	UUnderpaintingProgress* Palette = MakeProgress(*this);
	for (const TCHAR* Pigment : { TEXT("Ochre"), TEXT("TerreVerte"), TEXT("Sanguine"), TEXT("BoneBlack"), TEXT("Ceruse"), TEXT("SlumberBlue") })
	{
		Palette->AddPigment(Pigment, 20, false);
	}
	TestFalse(TEXT("Tier 2 needs tier 1 first"), Palette->Attune(TEXT("Ochre_T2")));
	int32 Bought = 0;
	for (const TCHAR* Tier : { TEXT("_T1"), TEXT("_T2") })
	{
		for (const TCHAR* Pigment : { TEXT("Ochre"), TEXT("TerreVerte"), TEXT("Sanguine"), TEXT("BoneBlack"), TEXT("Ceruse"), TEXT("SlumberBlue") })
		{
			Bought += Palette->Attune(FName(*(FString(Pigment) + Tier))) ? 1 : 0;
		}
	}
	TestEqual(TEXT("Twelve nodes attuned"), Bought, 12);
	TestEqual(TEXT("Golden Hour: perfect dodge gives 10"), Palette->GetAttunementValue(TEXT("PerfectDodgeGlim")), 10.0f);
	TestEqual(TEXT("Ochre tier 2 is tempered with 2 Slumber Blue: 20 - 3 - 6 - 2"), Palette->GetPigment(TEXT("SlumberBlue")), 9);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingTurningTest, "Underpainting.Game.TheTurning", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingTurningTest::RunTest(const FString& Parameters)
{
	const FName Forest(TEXT("Scumblewood")), Quest(TEXT("MQ_ColdUnderTheRibs"));
	UUnderpaintingProgress* Progress = MakeProgress(*this);
	FUnderpaintingProgressData Data = Progress->GetData();
	FQuestProgress Done; Done.bComplete = true;
	Data.Quests.Add(UUnderpaintingProgress::FirstQuest, Done);
	FQuestProgress AtWall; AtWall.Step = 5;
	Data.Quests.Add(Quest, AtWall);
	Progress->SetData(Data);

	// Before: nothing is due, and the light is the "before" light whatever the flags say.
	TestFalse(TEXT("She does not turn before the wall is decided"), Progress->IsTurningDue());
	TestEqual(TEXT("Forest light before"), Progress->GetZoneLight(Forest, 0.5f, 0.4f), 0.5f);

	// Deciding the wall makes the Turning due. The quest waits for the Stair of Spines, which does not exist yet.
	Progress->AddPigment(TEXT("TerreVerte"), 5, false);
	TestTrue(TEXT("Paint the wound green"), Progress->MakeChoice(Quest, TEXT("FillTerreVerte")));
	TestTrue(TEXT("Now she turns"), Progress->IsTurningDue());
	TestEqual(TEXT("The quest waits on the Turning"), Progress->GetQuestProgress(Quest)->Step, 6);
	TestEqual(TEXT("Still the old light until she has turned"), Progress->GetZoneLight(Forest, 0.5f, 0.4f), 0.5f);

	// Turned: it is not due again, and the light has moved. The green wall made the wood 0.15 brighter.
	Progress->SetFlag(UUnderpaintingProgress::TurnedFlag);
	TestTrue(TEXT("The world is turned"), Progress->HasTurned());
	TestFalse(TEXT("She turns once"), Progress->IsTurningDue());
	TestEqual(TEXT("Forest light after, with the green wall"), Progress->GetZoneLight(Forest, 0.5f, 0.4f), 0.55f);
	TestEqual(TEXT("Village light after"), Progress->GetZoneLight(TEXT("VarnishVillage"), 1.0f, 0.85f), 0.85f);
	TestEqual(TEXT("Vault light after"), Progress->GetZoneLight(TEXT("GrisailleVault"), 0.0f, 0.25f), 0.25f);

	// The way home: the Stair, then Hesper. With the lantern, the quest ends on her choice of what to do with the flame.
	Progress->ReachPlace(TEXT("StairOfSpines"), TEXT("The Stair of Spines"));
	TestEqual(TEXT("On the Stair, the last step is Hesper"), Progress->GetQuestProgress(Quest)->Step, 7);
	Progress->EndDialogue(TEXT("HesperVane"));
	TestTrue(TEXT("Cold Under the Ribs is done"), Progress->IsQuestComplete(Quest));

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingKeySettingsTest, "Underpainting.Game.KeySettings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingKeySettingsTest::RunTest(const FString& Parameters)
{
	UUnderpaintingSettingsSave* Settings = NewObject<UUnderpaintingSettingsSave>();

	// Every action has a key to begin with, and no two share one.
	TSet<FKey> Seen;
	for (const FUnderpaintingAction& Action : UUnderpaintingSettingsSave::Actions())
	{
		TestTrue(*FString::Printf(TEXT("%s has a default key"), *Action.Id.ToString()), Settings->GetKey(Action.Id).IsValid());
		TestFalse(*FString::Printf(TEXT("%s does not share its key"), *Action.Id.ToString()), Seen.Contains(Settings->GetKey(Action.Id)));
		Seen.Add(Settings->GetKey(Action.Id));
	}

	// A free key is simply taken.
	Settings->SetKey(TEXT("Dodge"), EKeys::C);
	TestEqual(TEXT("Dodge is on C"), Settings->GetKey(TEXT("Dodge")), EKeys::C);

	// A key already in use is swapped, so nothing is left without one.
	Settings->SetKey(TEXT("Flare"), EKeys::E);
	TestEqual(TEXT("Flare took E"), Settings->GetKey(TEXT("Flare")), EKeys::E);
	TestEqual(TEXT("Use got Flare's old key"), Settings->GetKey(TEXT("Use")), EKeys::F);

	Settings->ResetKeys();
	TestEqual(TEXT("Reset puts Use back on E"), Settings->GetKey(TEXT("Use")), EKeys::E);
	TestEqual(TEXT("Reset puts Dodge back"), Settings->GetKey(TEXT("Dodge")), EKeys::LeftControl);

	TestEqual(TEXT("Normal text size"), Settings->GetUIScale(), 1.0f);
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
