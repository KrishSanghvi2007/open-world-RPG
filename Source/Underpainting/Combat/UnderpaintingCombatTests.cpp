// Automated checks of the combat rules in docs/GDD.md section 5 and of the Blotter's numbers in data/enemies.json.
// Run in the editor's Session Frontend or headless with: Automation RunTests Underpainting.Combat

#include "Misc/AutomationTest.h"

#include "UnderpaintingCombatRules.h"
#include "UnderpaintingCombatTypes.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "Engine/DataTable.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingVitalsTest, "Underpainting.Combat.Vitals", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingVitalsTest::RunTest(const FString& Parameters)
{
	UUnderpaintingVitalsComponent* Vitals = NewObject<UUnderpaintingVitalsComponent>();
	Vitals->Initialise(100.0f, 100.0f, 100.0f);

	// A dodge costs 20 Stamina. Stamina waits 0.8 s, then returns at 30 per second.
	TestTrue(TEXT("Can pay for a dodge"), Vitals->TrySpendStamina(20.0f));
	TestEqual(TEXT("Stamina after a dodge"), Vitals->GetStamina(), 80.0f);
	Vitals->AdvanceRegeneration(0.5f);
	TestEqual(TEXT("No regeneration during the pause"), Vitals->GetStamina(), 80.0f);
	Vitals->AdvanceRegeneration(0.3f);
	Vitals->AdvanceRegeneration(0.5f);
	TestEqual(TEXT("Regeneration is 30 per second after the pause"), Vitals->GetStamina(), 95.0f, 0.01f);
	Vitals->AdvanceRegeneration(10.0f);
	TestEqual(TEXT("Stamina stops at its maximum"), Vitals->GetStamina(), 100.0f);

	TestTrue(TEXT("Spend most of the bar"), Vitals->TrySpendStamina(95.0f));
	TestFalse(TEXT("Cannot act without enough Stamina"), Vitals->TrySpendStamina(20.0f));
	TestEqual(TEXT("A refused action costs nothing"), Vitals->GetStamina(), 5.0f);

	// Armour removes a fraction of damage (Sized Smock: 5%).
	Vitals->DamageReduction = 0.05f;
	TestEqual(TEXT("Blotter Dab against the Sized Smock"), Vitals->ApplyDamage(14.0f, 14.0f, nullptr), 13.3f, 0.01f);
	TestEqual(TEXT("Health after the hit"), Vitals->GetHealth(), 86.7f, 0.01f);

	// The first part of a dodge cannot be hit.
	Vitals->bInvulnerable = true;
	TestEqual(TEXT("Hits pass through a dodge"), Vitals->ApplyDamage(26.0f, 26.0f, nullptr), 0.0f);
	Vitals->bInvulnerable = false;

	// Glim is clamped (the Blotter's lunge drains 10).
	Vitals->AddGlim(-10.0f);
	TestEqual(TEXT("Glim after a lunge"), Vitals->GetGlim(), 90.0f);
	Vitals->AddGlim(500.0f);
	TestEqual(TEXT("Glim stops at its maximum"), Vitals->GetGlim(), 100.0f);

	TestFalse(TEXT("Alive"), Vitals->IsDead());
	Vitals->ApplyDamage(1000.0f, 0.0f, nullptr);
	TestTrue(TEXT("Dead at zero Health"), Vitals->IsDead());
	TestEqual(TEXT("The dead take no further damage"), Vitals->ApplyDamage(10.0f, 0.0f, nullptr), 0.0f);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingAttackClockTest, "Underpainting.Combat.AttackClock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingAttackClockTest::RunTest(const FString& Parameters)
{
	FAttackClock Clock;
	TestEqual(TEXT("Idle clock"), Clock.GetPhase(), EAttackPhase::None);

	Clock.Start(FAttackTiming(0.6f, 0.2f, 0.8f));
	TestEqual(TEXT("Starts in wind-up"), Clock.GetPhase(), EAttackPhase::Windup);
	TestFalse(TEXT("Not finished"), Clock.Advance(0.59f));
	TestEqual(TEXT("Still winding up just before the hit"), Clock.GetPhase(), EAttackPhase::Windup);
	Clock.Advance(0.02f);
	TestEqual(TEXT("Active after the wind-up"), Clock.GetPhase(), EAttackPhase::Active);
	Clock.Advance(0.2f);
	TestEqual(TEXT("Recovery after the active window"), Clock.GetPhase(), EAttackPhase::Recovery);
	TestTrue(TEXT("Finishes at the total time"), Clock.Advance(0.8f));
	TestEqual(TEXT("Idle again"), Clock.GetPhase(), EAttackPhase::None);

	// A slower weapon (Bone Scraper, speed 0.75) takes longer for every part of the swing.
	const FAttackTiming Slow = FAttackTiming(0.3f, 0.15f, 0.45f).Scaled(0.75f);
	TestEqual(TEXT("Scaled wind-up"), Slow.Windup, 0.4f, 0.001f);
	TestEqual(TEXT("Scaled total"), Slow.Total(), 1.2f, 0.001f);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingEnemyDataTest, "Underpainting.Combat.EnemyRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingEnemyDataTest::RunTest(const FString& Parameters)
{
	TArray<FString> Problems;
	UDataTable* Enemies = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), TEXT("enemies"), FEnemyRow::StaticStruct(), Problems);
	UDataTable* Items = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), TEXT("items"), FItemRow::StaticStruct(), Problems);
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}
	if (!TestNotNull(TEXT("enemies table"), Enemies) || !TestNotNull(TEXT("items table"), Items))
	{
		return false;
	}

	// Enemy design principle 1: no wind-up under 0.6 s for standard enemies, and recovery at least as long as the wind-up.
	Enemies->ForeachRow<FEnemyRow>(TEXT("Test"), [this](const FName& Name, const FEnemyRow& Row)
	{
		TestTrue(*FString::Printf(TEXT("%s has at most 3 attacks per phase"), *Name.ToString()), Row.Tier == EEnemyTier::Boss || Row.Attacks.Num() <= 3);
		for (const FEnemyAttack& Attack : Row.Attacks)
		{
			const FString Label = FString::Printf(TEXT("%s %s"), *Name.ToString(), *Attack.AttackName.ToString());
			TestTrue(*(Label + TEXT(": wind-up is at least 0.6 s")), Attack.Windup >= 0.6f);
			TestTrue(*(Label + TEXT(": recovery is at least the wind-up")), Attack.Recovery >= Attack.Windup);
		}
	});

	// The boss changes hue at half Health, so it lists one weakness and one resistance per phase, in order.
	const FEnemyRow* Boss = Enemies->FindRow<FEnemyRow>(TEXT("ThePalimpsest"), TEXT("Test"), false);
	if (TestNotNull(TEXT("Old Palimpsest"), Boss))
	{
		TestEqual(TEXT("Boss changes at half Health"), Boss->Phase2HPPercent, 50);
		TestEqual(TEXT("Phase 1: weak to Bone Black"), Boss->Weaknesses[0], FName(TEXT("BoneBlack")));
		TestEqual(TEXT("Phase 2: weak to Terre Verte"), Boss->Weaknesses[1], FName(TEXT("TerreVerte")));
		TestEqual(TEXT("Phase 1: resists Ceruse"), Boss->Resistances[0], FName(TEXT("Ceruse")));
		TestEqual(TEXT("Phase 2: resists Sanguine"), Boss->Resistances[1], FName(TEXT("Sanguine")));
		int32 PhaseTwoAttacks = 0;
		for (const FEnemyAttack& Attack : Boss->Attacks) { PhaseTwoAttacks += Attack.Phase == 2 ? 1 : 0; }
		TestEqual(TEXT("Phase 2 adds two attacks"), PhaseTwoAttacks, 2);
		TestEqual(TEXT("Killing it refills the lantern"), Boss->GlimOnKill, 100);
	}

	// The fight the first playable is built around: Limner's Knife against one Blotter.
	const FEnemyRow* Blotter = Enemies->FindRow<FEnemyRow>(TEXT("Blotter"), TEXT("Test"), false);
	const FItemRow* Knife = Items->FindRow<FItemRow>(TEXT("LimnersKnife"), TEXT("Test"), false);
	if (TestNotNull(TEXT("Blotter"), Blotter) && TestNotNull(TEXT("Limner's Knife"), Knife))
	{
		TestEqual(TEXT("Blotter attacks"), Blotter->Attacks.Num(), 3);
		TestEqual(TEXT("Light hits to kill a Blotter"), FMath::CeilToInt(float(Blotter->MaxHP) / Knife->LightDamage), 5);
		TestEqual(TEXT("Heavy hits to kill a Blotter"), FMath::CeilToInt(float(Blotter->MaxHP) / Knife->HeavyDamage), 3);
		TestEqual(TEXT("Light hits to stagger a Blotter"), FMath::CeilToInt(float(Blotter->Poise) / (Knife->LightDamage * Knife->StaggerMultiplier)), 3);
	}

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderpaintingContrastTest, "Underpainting.Combat.ContrastAndLight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderpaintingContrastTest::RunTest(const FString& Parameters)
{
	using namespace UnderpaintingCombatRules;

	TArray<FString> Problems;
	UDataTable* Enemies = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), TEXT("enemies"), FEnemyRow::StaticStruct(), Problems);
	UDataTable* Pigments = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), TEXT("pigments"), FPigmentRow::StaticStruct(), Problems);
	UDataTable* Regions = UUnderpaintingDataSubsystem::LoadTableFromJson(GetTransientPackage(), TEXT("regions"), FRegionRow::StaticStruct(), Problems);
	if (!TestNotNull(TEXT("tables"), Enemies) || !Pigments || !Regions)
	{
		return false;
	}
	const FEnemyRow* Blotter = Enemies->FindRow<FEnemyRow>(TEXT("Blotter"), TEXT("Test"), false);

	// The Contrast rule against a white Blotter: Bone Black is its complement, Ceruse is its own hue.
	const FContrast Strong = Contrast(TEXT("BoneBlack"), Blotter->Weaknesses, Blotter->Resistances);
	TestEqual(TEXT("Complement: +30% damage"), Strong.DamageMultiplier, 1.3f);
	TestEqual(TEXT("Complement: +50% stagger"), Strong.StaggerMultiplier, 1.5f);
	const FContrast Muddy = Contrast(TEXT("Ceruse"), Blotter->Weaknesses, Blotter->Resistances);
	TestEqual(TEXT("Own hue: -30% damage"), Muddy.DamageMultiplier, 0.7f);
	TestEqual(TEXT("Own hue: stagger unchanged"), Muddy.StaggerMultiplier, 1.0f);
	const FContrast Plain = Contrast(TEXT("Ochre"), Blotter->Weaknesses, Blotter->Resistances);
	TestEqual(TEXT("Neither: no change"), Plain.DamageMultiplier, 1.0f);
	TestEqual(TEXT("A plain attack has no pigment"), Contrast(NAME_None, Blotter->Weaknesses, Blotter->Resistances).Sign, 0);

	// Every enemy's weakness is the complement of its hue, and its resistance the pigment of its hue.
	TMap<EPigmentHue, FName> ByHue;
	TMap<FName, FName> ComplementOf;
	Pigments->ForeachRow<FPigmentRow>(TEXT("Test"), [&](const FName& Name, const FPigmentRow& Row) { ByHue.Add(Row.Hue, Name); ComplementOf.Add(Name, Row.Complement); });
	Enemies->ForeachRow<FEnemyRow>(TEXT("Test"), [&](const FName& Name, const FEnemyRow& Row)
	{
		const FName Own = ByHue.FindRef(Row.Hue);
		TestTrue(*FString::Printf(TEXT("%s resists its own hue"), *Name.ToString()), Row.Resistances.Contains(Own));
		TestTrue(*FString::Printf(TEXT("%s is weak to its complement"), *Name.ToString()), Row.Weaknesses.Contains(ComplementOf.FindRef(Own)));
	});

	// The price of light: free in the painted village, half in the forest, full in the Vault, and a Flare helps.
	const int32 ImpastoCost = Pigments->FindRow<FPigmentRow>(TEXT("BoneBlack"), TEXT("Test"), false)->LightCost;
	const float Village = Regions->FindRow<FRegionRow>(TEXT("VarnishVillage"), TEXT("Test"), false)->AmbientLight;
	const float Forest = Regions->FindRow<FRegionRow>(TEXT("Scumblewood"), TEXT("Test"), false)->AmbientLight;
	const float Vault = Regions->FindRow<FRegionRow>(TEXT("GrisailleVault"), TEXT("Test"), false)->AmbientLight;
	TestEqual(TEXT("Impasto in the village is free"), StrokeGlimCost(ImpastoCost, Village), 0);
	TestEqual(TEXT("Impasto in the forest is half price"), StrokeGlimCost(ImpastoCost, Forest), 5);
	TestEqual(TEXT("Impasto in the Vault is full price"), StrokeGlimCost(ImpastoCost, Vault), 10);
	TestEqual(TEXT("A Flare in the Vault halves it"), StrokeGlimCost(ImpastoCost, EffectiveLight(Vault, true)), 5);
	TestEqual(TEXT("Odd costs round up"), StrokeGlimCost(15, 0.5f), 8);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
