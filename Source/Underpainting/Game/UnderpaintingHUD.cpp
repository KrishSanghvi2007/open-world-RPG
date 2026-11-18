#include "UnderpaintingHUD.h"

#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingEnemy.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "UnderpaintingWorldActors.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Palette from docs/GDD.md: madder rose, terre verte, lantern amber, cream text on violet shadow.
	const FLinearColor HealthColour(0.55f, 0.10f, 0.07f);
	const FLinearColor StaminaColour(0.15f, 0.27f, 0.17f);
	const FLinearColor GlimColour(0.80f, 0.29f, 0.04f);
	const FLinearColor BackColour(0.07f, 0.04f, 0.10f, 0.82f);
	const FLinearColor TextColour(0.90f, 0.78f, 0.53f);
	const FLinearColor DimColour(0.55f, 0.48f, 0.42f);
	const FLinearColor GoodColour(0.55f, 0.75f, 0.50f);
	const float LineHeight = 22.0f;
	const float NoticeSeconds = 5.0f;
}

void AUnderpaintingHUD::BeginPlay()
{
	Super::BeginPlay();
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		NoticeHandle = Progress->OnNotice.AddUObject(this, &AUnderpaintingHUD::AddNotice);
	}
}

void AUnderpaintingHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->OnNotice.Remove(NoticeHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AUnderpaintingHUD::AddNotice(const FString& Text)
{
	Notices.Add({ Text, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f });
	if (Notices.Num() > 6)
	{
		Notices.RemoveAt(0);
	}
}

// --- Menus: state ---

void AUnderpaintingHUD::ShowDialogueRow(FName RowId)
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress || RowId.IsNone() || !Progress->GetDialogueRow(RowId))
	{
		const FName Npc = DialogueNpc;
		CloseMenu();
		if (Progress && !Npc.IsNone())
		{
			Progress->EndDialogue(Npc);
		}
		return;
	}
	DialogueRow = RowId;
	Progress->EnterDialogueRow(RowId);
}

void AUnderpaintingHUD::OpenDialogue(FName NpcId)
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const FName Entry = Progress ? Progress->FindDialogueEntry(NpcId) : NAME_None;
	if (Entry.IsNone())
	{
		return;
	}
	Menu = EUnderpaintingMenu::Dialogue;
	DialogueNpc = NpcId;
	ShowDialogueRow(Entry);
}

void AUnderpaintingHUD::OpenCrafting(ECraftingStation Station)
{
	Menu = EUnderpaintingMenu::Crafting;
	CraftingStation = Station;
}

void AUnderpaintingHUD::OpenAttunement()
{
	Menu = EUnderpaintingMenu::Attunement;
}

void AUnderpaintingHUD::OpenReading(FName LoreId)
{
	Menu = EUnderpaintingMenu::Reading;
	ReadingLore = LoreId;
}

void AUnderpaintingHUD::OpenText(const FString& Title, const FString& Text)
{
	Menu = EUnderpaintingMenu::Reading;
	ReadingLore = NAME_None;
	FreeTitle = Title;
	FreeText = Text;
}

void AUnderpaintingHUD::OpenChoice(FName QuestId, const FString& Subject)
{
	Menu = EUnderpaintingMenu::Choice;
	ChoiceQuest = QuestId;
	ChoiceSubject = Subject;
	ChoicePending = INDEX_NONE;
}

void AUnderpaintingHUD::ToggleInventory()
{
	if (Menu == EUnderpaintingMenu::Inventory)
	{
		CloseMenu();
	}
	else if (Menu == EUnderpaintingMenu::None)
	{
		Menu = EUnderpaintingMenu::Inventory;
	}
}

void AUnderpaintingHUD::CloseMenu()
{
	// Walking away from a conversation still counts as having had it.
	if (Menu == EUnderpaintingMenu::Dialogue && !DialogueNpc.IsNone())
	{
		const FName Npc = DialogueNpc;
		Menu = EUnderpaintingMenu::None;
		DialogueNpc = NAME_None;
		DialogueRow = NAME_None;
		if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
		{
			Progress->EndDialogue(Npc);
		}
		return;
	}
	Menu = EUnderpaintingMenu::None;
	DialogueNpc = NAME_None;
	DialogueRow = NAME_None;
}

void AUnderpaintingHUD::Advance()
{
	if (Menu != EUnderpaintingMenu::Dialogue)
	{
		CloseMenu();
		return;
	}

	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const FDialogueRow* Row = Progress ? Progress->GetDialogueRow(DialogueRow) : nullptr;
	if (Row && Progress->GetOfferedChoices(*Row).Num() > 0)
	{
		return;   // a reply has to be chosen with a number key
	}
	ShowDialogueRow(Row ? Row->Next : NAME_None);
}

TArray<FName> AUnderpaintingHUD::GetAttunementList(const UUnderpaintingProgress* Progress) const
{
	// Tier 1 of every pigment, then any tier 2 whose tier 1 is owned. At most 9 fit the number keys.
	TArray<FName> Result;
	TArray<FName> Later;
	for (const FName& Node : Progress->GetAllAttunementNodes())
	{
		const FString Name = Node.ToString();
		if (Name.EndsWith(TEXT("_T1")))
		{
			Result.Add(Node);
		}
		else if (Progress->HasAttunement(FName(*Name.LeftChop(1).Append(TEXT("1")))))
		{
			Later.Add(Node);
		}
	}
	Result.Append(Later);
	Result.SetNum(FMath::Min(Result.Num(), 9));
	return Result;
}

void AUnderpaintingHUD::ChooseNumber(int32 Number)
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress || Number < 1)
	{
		return;
	}
	const int32 Index = Number - 1;

	if (Menu == EUnderpaintingMenu::Dialogue)
	{
		const FDialogueRow* Row = Progress->GetDialogueRow(DialogueRow);
		const TArray<int32> Offered = Row ? Progress->GetOfferedChoices(*Row) : TArray<int32>();
		if (Offered.IsValidIndex(Index))
		{
			ShowDialogueRow(Progress->ChooseDialogue(DialogueRow, Offered[Index]));
		}
	}
	else if (Menu == EUnderpaintingMenu::Crafting)
	{
		const TArray<FName> Recipes = Progress->GetRecipesForStation(CraftingStation);
		if (Recipes.IsValidIndex(Index))
		{
			FString Why;
			if (!Progress->CanCraft(Recipes[Index], &Why))
			{
				AddNotice(Why);
			}
			else
			{
				Progress->Craft(Recipes[Index]);
			}
		}
	}
	else if (Menu == EUnderpaintingMenu::Inventory)
	{
		const TArray<FName> Gear = Progress->GetGear();
		if (Gear.IsValidIndex(Index))
		{
			Progress->Equip(Gear[Index]);
		}
	}
	else if (Menu == EUnderpaintingMenu::Choice)
	{
		const FQuestRow* Row = Progress->GetQuestRow(ChoiceQuest);
		if (!Row)
		{
			return;
		}
		if (ChoicePending == INDEX_NONE)
		{
			// First press: pick an ending. Nothing happens yet.
			if (Row->Choices.IsValidIndex(Index))
			{
				FString Why;
				if (Progress->CanChoose(ChoiceQuest, Row->Choices[Index].ChoiceId, &Why))
				{
					ChoicePending = Index;
				}
				else
				{
					AddNotice(Why);
				}
			}
		}
		else if (Number == 1)
		{
			// Second press, on "yes": it is done, and cannot be undone.
			Progress->MakeChoice(ChoiceQuest, Row->Choices[ChoicePending].ChoiceId);
			CloseMenu();
		}
		else if (Number == 2)
		{
			ChoicePending = INDEX_NONE;
		}
	}
	else if (Menu == EUnderpaintingMenu::Attunement)
	{
		const TArray<FName> Nodes = GetAttunementList(Progress);
		if (Nodes.IsValidIndex(Index))
		{
			FString Why;
			if (!Progress->CanAttune(Nodes[Index], &Why))
			{
				AddNotice(Why);
			}
			else
			{
				Progress->Attune(Nodes[Index]);
			}
		}
	}
}

// --- Drawing helpers ---

void AUnderpaintingHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill, const FString& Label)
{
	DrawRect(BackColour, X - 2.0f, Y - 2.0f, Width + 4.0f, Height + 4.0f);
	DrawRect(Fill, X, Y, Width * FMath::Clamp(Fraction, 0.0f, 1.0f), Height);
	if (!Label.IsEmpty())
	{
		DrawText(Label, TextColour, X + Width + 10.0f, Y - 2.0f, nullptr, 1.0f);
	}
}

void AUnderpaintingHUD::DrawPanel(float X, float Y, float Width, float Height)
{
	DrawRect(BackColour, X, Y, Width, Height);
	DrawRect(GlimColour, X, Y, Width, 2.0f);
}

float AUnderpaintingHUD::DrawWrapped(const FString& Text, float X, float Y, float Width, const FLinearColor& Colour, float Scale)
{
	// Greedy word wrap, measured with the HUD font.
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "));
	FString Line;
	for (const FString& Word : Words)
	{
		const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		float TextWidth = 0.0f, TextHeight = 0.0f;
		GetTextSize(Candidate, TextWidth, TextHeight, nullptr, Scale);
		if (TextWidth > Width && !Line.IsEmpty())
		{
			DrawText(Line, Colour, X, Y, nullptr, Scale);
			Y += LineHeight * Scale;
			Line = Word;
		}
		else
		{
			Line = Candidate;
		}
	}
	if (!Line.IsEmpty())
	{
		DrawText(Line, Colour, X, Y, nullptr, Scale);
		Y += LineHeight * Scale;
	}
	return Y;
}

// --- Drawing ---

void AUnderpaintingHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(GetOwningPawn());
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);

	// Everything is laid out for a 720-line screen and scaled up from there, so text stays readable.
	const float UIScale = FMath::Max(1.0f, Canvas->ClipY / 720.0f) * 1.25f;
	const float RealX = Canvas->ClipX, RealY = Canvas->ClipY;
	FCanvas* RawCanvas = Canvas->Canvas;
	RawCanvas->PushAbsoluteTransform(FScaleMatrix(FVector(UIScale, UIScale, 1.0f)));
	Canvas->ClipX = RealX / UIScale;
	Canvas->ClipY = RealY / UIScale;
	ProjectionScale = UIScale;

	DrawEnemyBars();
	if (Player)
	{
		DrawPlayerBars(Player);
	}
	if (Player && Progress && Menu == EUnderpaintingMenu::None)
	{
		DrawCompass(Player, Progress);
	}
	if (Progress)
	{
		DrawTracker(Progress);
		switch (Menu)
		{
		case EUnderpaintingMenu::Dialogue: DrawDialogue(Progress); break;
		case EUnderpaintingMenu::Crafting: DrawCrafting(Progress); break;
		case EUnderpaintingMenu::Attunement: DrawAttunement(Progress); break;
		case EUnderpaintingMenu::Inventory: DrawInventory(Progress); break;
		case EUnderpaintingMenu::Reading: DrawReading(Progress); break;
		case EUnderpaintingMenu::Choice: DrawChoice(Progress); break;
		default: break;
		}
	}
	DrawNotices();

	// The interaction prompt.
	if (Player && Menu == EUnderpaintingMenu::None && Player->GetFocus())
	{
		const FString Prompt = FString::Printf(TEXT("[E]  %s"), *Player->GetFocus()->GetUsablePrompt());
		float Width = 0.0f, Height = 0.0f;
		GetTextSize(Prompt, Width, Height, nullptr, 1.2f);
		const float X = (Canvas->ClipX - Width) * 0.5f;
		const float Y = Canvas->ClipY * 0.62f;
		DrawRect(BackColour, X - 12.0f, Y - 6.0f, Width + 24.0f, Height + 12.0f);
		DrawText(Prompt, TextColour, X, Y, nullptr, 1.2f);
	}

	RawCanvas->PopTransform();
	Canvas->ClipX = RealX;
	Canvas->ClipY = RealY;
}

void AUnderpaintingHUD::DrawPlayerBars(const AUnderpaintingPlayerCharacter* Player)
{
	const UUnderpaintingVitalsComponent* Vitals = Player->GetVitals();
	if (!Vitals)
	{
		return;
	}

	const float X = 40.0f;
	const float Y = Canvas->ClipY - 110.0f;
	DrawBar(X, Y, 300.0f, 16.0f, Vitals->GetHealthFraction(), HealthColour, FString::Printf(TEXT("Health %d"), FMath::CeilToInt(Vitals->GetHealth())));
	DrawBar(X, Y + 26.0f, 300.0f, 12.0f, Vitals->GetStaminaFraction(), StaminaColour, FString::Printf(TEXT("Stamina %d"), FMath::CeilToInt(Vitals->GetStamina())));
	DrawBar(X, Y + 48.0f, 300.0f * Vitals->MaxGlim / 100.0f, 12.0f, Vitals->GetGlimFraction(), GlimColour, FString::Printf(TEXT("Glim %d / %d"), FMath::CeilToInt(Vitals->GetGlim()), FMath::CeilToInt(Vitals->MaxGlim)));

	// The palette: two wells, the active one marked, with its Stroke and what that Stroke costs here.
	if (const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
		const float WellX = X + 470.0f;
		for (int32 Index = 0; Index < UUnderpaintingProgress::NumWells; ++Index)
		{
			const FPigmentRow* Pigment = Data ? Data->FindPigment(Progress->GetWell(Index)) : nullptr;
			const bool bActive = Index == Progress->GetActiveWellIndex();
			const float BoxX = WellX + Index * 54.0f;
			DrawRect(bActive ? TextColour : BackColour, BoxX - 3.0f, Y - 3.0f, 46.0f, 46.0f);
			DrawRect(Pigment ? UUnderpaintingDataSubsystem::HexToColour(Pigment->ColourHex) : FLinearColor(0.12f, 0.11f, 0.10f), BoxX, Y, 40.0f, 40.0f);
		}
		if (const FPigmentRow* Active = Data ? Data->FindPigment(Progress->GetActivePigment()) : nullptr)
		{
			const int32 Cost = Player->GetStrokeGlimCost();
			DrawText(FString::Printf(TEXT("%s  (hold heavy)"), *Active->StrokeName.ToString()), TextColour, WellX + 120.0f, Y, nullptr, 1.0f);
			DrawText(Cost > 0 ? FString::Printf(TEXT("%d Glim here   light %.0f%%"), Cost, Player->GetLightHere() * 100.0f) : FString(TEXT("free in this light")),
				DimColour, WellX + 120.0f, Y + 22.0f, nullptr, 0.9f);
		}
		DrawText(FString::Printf(TEXT("Poultice %d [H]   Lamp Oil %d [J]"), Progress->GetItemCount(TEXT("MossPoultice")), Progress->GetItemCount(TEXT("LampOil"))),
			DimColour, WellX, Y + 50.0f, nullptr, 0.85f);
		FString Extras;
		if (Progress->GetItemCount(TEXT("SanguineDraught")) > 0 || Player->GetDraughtRemaining() > 0.0f)
		{
			Extras += FString::Printf(TEXT("Draught %d [K]%s   "), Progress->GetItemCount(TEXT("SanguineDraught")),
				Player->GetDraughtRemaining() > 0.0f ? *FString::Printf(TEXT(" %.0fs"), Player->GetDraughtRemaining()) : TEXT(""));
		}
		if (Progress->GetItemCount(TEXT("BlueTincture")) > 0 || Player->GetTinctureRemaining() > 0.0f)
		{
			Extras += FString::Printf(TEXT("Tincture %d [L]%s"), Progress->GetItemCount(TEXT("BlueTincture")),
				Player->GetTinctureRemaining() > 0.0f ? *FString::Printf(TEXT(" %.0fs"), Player->GetTinctureRemaining()) : TEXT(""));
		}
		if (!Extras.IsEmpty())
		{
			DrawText(Extras, DimColour, WellX, Y + 70.0f, nullptr, 0.85f);
		}
	}

	if (Vitals->IsDead())
	{
		DrawText(TEXT("You were rubbed out."), TextColour, Canvas->ClipX * 0.5f - 110.0f, Canvas->ClipY * 0.4f, nullptr, 1.6f);
	}
	else if (Menu == EUnderpaintingMenu::None)
	{
		DrawText(TEXT("E use   F flare   G fix   Tab pack   Mouse L light, R heavy (hold: Stroke)   Q lock on   Z swap well   L-Ctrl dodge   L-Shift sprint   F5/F9 save/load"), DimColour, X, Canvas->ClipY - 30.0f, nullptr, 0.85f);
	}
}

void AUnderpaintingHUD::DrawEnemyBars()
{
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		const AUnderpaintingEnemy* Enemy = *It;
		const UUnderpaintingVitalsComponent* Vitals = Enemy->GetVitals();
		if (!Vitals || Vitals->IsDead())
		{
			continue;
		}

		const AUnderpaintingPlayerCharacter* Viewer = Cast<AUnderpaintingPlayerCharacter>(GetOwningPawn());
		if (Viewer && FVector::Dist(Viewer->GetActorLocation(), Enemy->GetActorLocation()) > 2500.0f)
		{
			continue;   // bars are for what you are fighting, not for what is three rooms away
		}
		const FVector Screen = Project(Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f));
		const FVector2D Point(Screen.X / ProjectionScale, Screen.Y / ProjectionScale);
		if (Screen.Z <= 0.0f)
		{
			continue;
		}

		const float Width = 90.0f;
		DrawBar(Point.X - Width * 0.5f, Point.Y, Width, 8.0f, Vitals->GetHealthFraction(), HealthColour, FString());
		DrawBar(Point.X - Width * 0.5f, Point.Y + 12.0f, Width, 4.0f, Enemy->GetPoiseFraction(), TextColour, FString());

		FString Status = Enemy->GetDisplayName();
		if (Enemy->GetState() == EEnemyState::Staggered)
		{
			Status += TEXT("  (staggered)");
		}
		else if (!Enemy->GetCurrentAttackName().IsNone())
		{
			Status += TEXT("  ") + Enemy->GetCurrentAttackName().ToString();
		}
		DrawText(Status, TextColour, Point.X - Width * 0.5f, Point.Y - 20.0f, nullptr, 0.9f);
		if (Enemy->IsGlazed())
		{
			DrawText(TEXT("glazed"), GlimColour, Point.X + Width * 0.5f + 8.0f, Point.Y - 4.0f, nullptr, 0.85f);
		}
		if (Viewer && Viewer->GetLockTarget() == Enemy)
		{
			DrawRect(TextColour, Point.X - 5.0f, Point.Y + 22.0f, 10.0f, 10.0f);
		}
	}
}

void AUnderpaintingHUD::DrawTracker(const UUnderpaintingProgress* Progress)
{
	const float Width = 420.0f;
	const float X = Canvas->ClipX - Width - 30.0f;
	float Y = 30.0f;
	for (const FName& QuestId : Progress->GetActiveQuests())
	{
		const FString Step = Progress->GetCurrentStepText(QuestId);
		DrawRect(BackColour, X - 10.0f, Y - 6.0f, Width + 20.0f, 64.0f);
		DrawText(Progress->GetDisplayName(QuestId), GlimColour, X, Y, nullptr, 0.9f);
		DrawWrapped(Step, X, Y + 18.0f, Width, TextColour, 0.9f);
		Y += 74.0f;
	}
}

void AUnderpaintingHUD::DrawNotices()
{
	const float Now = GetWorld()->GetTimeSeconds();
	Notices.RemoveAll([Now](const FNotice& Notice) { return Now - Notice.Time > NoticeSeconds; });

	float Y = Canvas->ClipY * 0.24f;
	for (const FNotice& Notice : Notices)
	{
		float Width = 0.0f, Height = 0.0f;
		GetTextSize(Notice.Text, Width, Height, nullptr, 1.0f);
		const float X = (Canvas->ClipX - Width) * 0.5f;
		DrawRect(BackColour, X - 10.0f, Y - 3.0f, Width + 20.0f, Height + 6.0f);
		DrawText(Notice.Text, TextColour, X, Y, nullptr, 1.0f);
		Y += Height + 10.0f;
	}
}

void AUnderpaintingHUD::DrawDialogue(const UUnderpaintingProgress* Progress)
{
	const FDialogueRow* Row = Progress->GetDialogueRow(DialogueRow);
	if (!Row)
	{
		return;
	}

	const float Width = FMath::Min(900.0f, Canvas->ClipX - 80.0f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 330.0f;
	DrawPanel(X, Y, Width, 200.0f);
	DrawText(Row->Speaker, GlimColour, X + 20.0f, Y + 14.0f, nullptr, 1.1f);
	float Cursor = DrawWrapped(Row->Text, X + 20.0f, Y + 44.0f, Width - 40.0f, TextColour, 1.1f) + 10.0f;

	const TArray<int32> Offered = Progress->GetOfferedChoices(*Row);
	if (Offered.Num() == 0)
	{
		DrawText(TEXT("[E] continue"), DimColour, X + 20.0f, Y + 170.0f, nullptr, 0.9f);
	}
	for (int32 Index = 0; Index < Offered.Num(); ++Index)
	{
		DrawText(FString::Printf(TEXT("%d.  %s"), Index + 1, *Row->Choices[Offered[Index]].Label), GoodColour, X + 30.0f, Cursor, nullptr, 1.0f);
		Cursor += LineHeight;
	}
}

void AUnderpaintingHUD::DrawCrafting(const UUnderpaintingProgress* Progress)
{
	const TArray<FName> Recipes = Progress->GetRecipesForStation(CraftingStation);
	const float Width = 760.0f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = 140.0f;
	DrawPanel(X, Y, Width, 90.0f + Recipes.Num() * 50.0f);
	DrawText(CraftingStation == ECraftingStation::GrindingSlab ? TEXT("The Grinding Slab") : TEXT("The Armature Bench"), GlimColour, X + 20.0f, Y + 14.0f, nullptr, 1.2f);

	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	float Cursor = Y + 50.0f;
	for (int32 Index = 0; Index < Recipes.Num(); ++Index)
	{
		const FRecipeRow* Recipe = Data ? Data->FindRecipe(Recipes[Index]) : nullptr;
		if (!Recipe)
		{
			continue;
		}
		FString Needs;
		for (const FIdCount& Input : Recipe->Inputs)
		{
			Needs += FString::Printf(TEXT("%s%d %s"), Needs.IsEmpty() ? TEXT("") : TEXT(", "), Input.Count, *Progress->GetDisplayName(Input.Id));
		}
		FString Why;
		const bool bCan = Progress->CanCraft(Recipes[Index], &Why);
		DrawText(FString::Printf(TEXT("%d.  %s x%d"), Index + 1, *Progress->GetDisplayName(Recipe->OutputItem), Recipe->OutputCount), bCan ? GoodColour : DimColour, X + 20.0f, Cursor, nullptr, 1.05f);
		DrawText(FString::Printf(TEXT("needs %s%s%s"), *Needs, bCan ? TEXT("") : TEXT("   -   "), bCan ? TEXT("") : *Why), DimColour, X + 50.0f, Cursor + 21.0f, nullptr, 0.9f);
		Cursor += 50.0f;
	}
	DrawText(TEXT("Number to craft   [E] or [Q] leave"), DimColour, X + 20.0f, Cursor + 6.0f, nullptr, 0.9f);
}

void AUnderpaintingHUD::DrawAttunement(const UUnderpaintingProgress* Progress)
{
	const TArray<FName> Nodes = GetAttunementList(Progress);
	const float Width = FMath::Min(1000.0f, Canvas->ClipX - 80.0f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = 110.0f;
	DrawPanel(X, Y, Width, 100.0f + Nodes.Num() * 50.0f);
	DrawText(TEXT("The Lampstand: your palette"), GlimColour, X + 20.0f, Y + 14.0f, nullptr, 1.2f);
	DrawText(TEXT("Your pigment has dried. Take dry pigment into yourself to grow."), DimColour, X + 20.0f, Y + 40.0f, nullptr, 0.9f);

	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	float Cursor = Y + 70.0f;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FAttunementRow* Node = Data ? Data->FindAttunement(Nodes[Index]) : nullptr;
		if (!Node)
		{
			continue;
		}
		const bool bOwned = Progress->HasAttunement(Nodes[Index]);
		FString Why;
		const bool bCan = Progress->CanAttune(Nodes[Index], &Why);
		FString Cost = FString::Printf(TEXT("%d %s"), Node->CostAmount, *Progress->GetDisplayName(Node->CostPigment));
		if (Node->TemperAmount > 0)
		{
			Cost += FString::Printf(TEXT(" + %d %s"), Node->TemperAmount, *Progress->GetDisplayName(Node->TemperPigment));
		}
		DrawText(FString::Printf(TEXT("%d.  %s   (%s)   %s"), Index + 1, *Node->DisplayName, *Cost, bOwned ? TEXT("YOURS") : (bCan ? TEXT("") : *Why)),
			bOwned ? GlimColour : (bCan ? GoodColour : DimColour), X + 20.0f, Cursor, nullptr, 1.0f);
		DrawText(Node->EffectText.Left(130), DimColour, X + 50.0f, Cursor + 21.0f, nullptr, 0.85f);
		Cursor += 50.0f;
	}
	DrawText(TEXT("Number to attune   [E] or [Q] stand up"), DimColour, X + 20.0f, Cursor + 6.0f, nullptr, 0.9f);
}

void AUnderpaintingHUD::DrawInventory(const UUnderpaintingProgress* Progress)
{
	const FUnderpaintingProgressData& Data = Progress->GetData();
	const float Width = 520.0f;
	const float X = 60.0f;
	const float Y = 110.0f;

	TArray<FString> Lines;
	Lines.Add(TEXT("PIGMENT (wet / dry)"));
	TSet<FName> PigmentIds;
	for (const TPair<FName, int32>& Pair : Data.WetPigment) { PigmentIds.Add(Pair.Key); }
	for (const TPair<FName, int32>& Pair : Data.DryPigment) { PigmentIds.Add(Pair.Key); }
	for (const FName& Id : PigmentIds)
	{
		if (Progress->GetPigment(Id) > 0)
		{
			Lines.Add(FString::Printf(TEXT("   %s   %d / %d"), *Progress->GetDisplayName(Id), Progress->GetWetPigment(Id), Progress->GetDryPigment(Id)));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("PACK"));
	for (const TPair<FName, int32>& Pair : Data.Items)
	{
		if (Pair.Value > 0)
		{
			Lines.Add(FString::Printf(TEXT("   %s   x%d"), *Progress->GetDisplayName(Pair.Key), Pair.Value));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("GEAR (press its number to put it on)"));
	const TArray<FName> Gear = Progress->GetGear();
	for (int32 GearIndex = 0; GearIndex < Gear.Num(); ++GearIndex)
	{
		const FItemRow* Item = Progress->GetItemRow(Gear[GearIndex]);
		if (!Item)
		{
			continue;
		}
		const bool bOn = Gear[GearIndex] == Progress->GetEquippedWeapon() || Gear[GearIndex] == Progress->GetEquippedArmour();
		const FString Numbers = Item->Type == EItemType::Weapon
			? FString::Printf(TEXT("%d / %d damage, reach %.1f m"), Item->LightDamage, Item->HeavyDamage, Item->Reach)
			: FString::Printf(TEXT("stops %.0f%% of damage"), Item->DamageReduction * 100.0f);
		Lines.Add(FString::Printf(TEXT("   %d.  %s   %s%s"), GearIndex + 1, *Item->DisplayName, *Numbers, bOn ? TEXT("   [ON]") : TEXT("")));
	}
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("ATTUNED: %d      DEATHS: %d"), Data.Attunements.Num(), Data.Deaths));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("FOUND"));
	int32 Found = 0, Total = 0;
	Progress->CountLore(ELoreKind::FieldNote, Found, Total);
	Lines.Add(FString::Printf(TEXT("   Odile's notes   %d of %d"), Found, Total));
	Progress->CountLore(ELoreKind::Marginalia, Found, Total);
	Lines.Add(FString::Printf(TEXT("   Marginalia   %d of %d"), Found, Total));
	Progress->CountLore(ELoreKind::Ledger, Found, Total);
	Lines.Add(FString::Printf(TEXT("   Ledgers   %d of %d"), Found, Total));
	Lines.Add(FString::Printf(TEXT("   Places   %d      Caches   %d"), Data.DiscoveredPlaces.Num(), Data.OpenedCaches.Num()));

	DrawPanel(X, Y, Width, 60.0f + Lines.Num() * LineHeight);
	DrawText(TEXT("Your pack"), GlimColour, X + 20.0f, Y + 14.0f, nullptr, 1.2f);
	float Cursor = Y + 48.0f;
	for (const FString& Line : Lines)
	{
		DrawText(Line, Line.StartsWith(TEXT("   ")) ? TextColour : DimColour, X + 20.0f, Cursor, nullptr, 1.0f);
		Cursor += LineHeight;
	}
}

void AUnderpaintingHUD::DrawReading(const UUnderpaintingProgress* Progress)
{
	const FLoreRow* Row = Progress->GetLore(ReadingLore);
	if (!Row && FreeText.IsEmpty())
	{
		return;
	}
	const FString Title = Row ? Row->Title : FreeTitle;
	const FString Body = Row ? Row->Text : FreeText;

	// A sheet of paper: cream for Odile and the ledgers, the Vault's blue-grey for the Hand.
	const bool bHand = Row && Row->Kind == ELoreKind::Marginalia;
	const float Width = FMath::Min(760.0f, Canvas->ClipX - 80.0f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY * 0.22f;
	const FLinearColor Paper = bHand ? FLinearColor(0.62f, 0.64f, 0.70f, 0.96f) : FLinearColor(0.87f, 0.76f, 0.52f, 0.96f);
	const FLinearColor Ink = bHand ? FLinearColor(0.03f, 0.07f, 0.38f) : FLinearColor(0.08f, 0.05f, 0.04f);
	DrawRect(Paper, X, Y, Width, 300.0f);
	DrawText(Title, Ink * 0.8f + FLinearColor(0.1f, 0.1f, 0.1f), X + 30.0f, Y + 22.0f, nullptr, 0.95f);
	DrawWrapped(Body, X + 30.0f, Y + 62.0f, Width - 60.0f, Ink, 1.15f);
	DrawText(TEXT("[E] put it away"), Ink * 0.7f + FLinearColor(0.15f, 0.15f, 0.15f), X + 30.0f, Y + 268.0f, nullptr, 0.85f);
}

void AUnderpaintingHUD::DrawCompass(const AUnderpaintingPlayerCharacter* Player, const UUnderpaintingProgress* Progress)
{
	// A strip at the top of the screen. The dying Lamp hangs in the west and never moves, so it is the one fixed
	// bearing everyone in this world steers by. In level space, north is +X and west is -Y.
	const APlayerController* Controller = Cast<APlayerController>(Player->GetController());
	if (!Controller)
	{
		return;
	}
	const float ViewYaw = Controller->GetControlRotation().Yaw;
	const float Width = 460.0f;
	const float Centre = Canvas->ClipX * 0.5f;
	const float Y = 26.0f;
	const float DegreesShown = 150.0f;
	DrawRect(BackColour, Centre - Width * 0.5f, Y - 6.0f, Width, 34.0f);
	DrawRect(TextColour, Centre - 1.0f, Y + 24.0f, 2.0f, 6.0f);

	const auto Mark = [&](float WorldYaw, const FString& Text, const FLinearColor& Colour, float Scale)
	{
		const float Delta = FRotator::NormalizeAxis(WorldYaw - ViewYaw);
		if (FMath::Abs(Delta) <= DegreesShown * 0.5f)
		{
			float TextWidth = 0.0f, TextHeight = 0.0f;
			GetTextSize(Text, TextWidth, TextHeight, nullptr, Scale);
			DrawText(Text, Colour, Centre + Delta / DegreesShown * Width - TextWidth * 0.5f, Y, nullptr, Scale);
		}
	};
	Mark(0.0f, TEXT("N"), DimColour, 1.0f);
	Mark(90.0f, TEXT("E"), DimColour, 1.0f);
	Mark(180.0f, TEXT("S"), DimColour, 1.0f);
	Mark(-90.0f, TEXT("LAMP"), GlimColour, 0.9f);

	// The current quest step, if what it is about is somewhere in this level.
	const TArray<FName> ActiveQuests = Progress->GetActiveQuests();
	if (ActiveQuests.Num() > 0)
	{
		const FName Target = Progress->GetCurrentObjectiveTarget(ActiveQuests[0]);
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(this, Target, Tagged);
		const AActor* Nearest = nullptr;
		float NearestDistance = TNumericLimits<float>::Max();
		for (const AActor* Actor : Tagged)
		{
			const AUnderpaintingInteractable* Usable = Cast<AUnderpaintingInteractable>(Actor);
			if (Usable && Usable->GetUsablePrompt().IsEmpty())
			{
				continue;   // an emptied node is not where the objective is
			}
			const float Distance = FVector::Dist(Actor->GetActorLocation(), Player->GetActorLocation());
			if (Distance < NearestDistance)
			{
				Nearest = Actor;
				NearestDistance = Distance;
			}
		}
		if (Nearest)
		{
			const FVector To = Nearest->GetActorLocation() - Player->GetActorLocation();
			Mark(To.Rotation().Yaw, FString::Printf(TEXT("v %dm"), FMath::RoundToInt(NearestDistance / 100.0f)), GoodColour, 0.9f);
		}
	}
}

void AUnderpaintingHUD::DrawChoice(const UUnderpaintingProgress* Progress)
{
	const FQuestRow* Row = Progress->GetQuestRow(ChoiceQuest);
	if (!Row)
	{
		return;
	}

	const float Width = FMath::Min(980.0f, Canvas->ClipX - 80.0f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = 120.0f;

	if (ChoicePending != INDEX_NONE && Row->Choices.IsValidIndex(ChoicePending))
	{
		// The confirmation: cost and immediate consequence in plain words, then the question (GDD section 7).
		const FQuestChoice& Choice = Row->Choices[ChoicePending];
		DrawPanel(X, Y, Width, 300.0f);
		DrawText(Choice.Label, GlimColour, X + 24.0f, Y + 16.0f, nullptr, 1.25f);
		const FString Cost = Choice.CostPigment.IsNone() ? FString(TEXT("It costs nothing.")) :
			FString::Printf(TEXT("It costs %d %s. You have %d."), Choice.CostAmount, *Progress->GetDisplayName(Choice.CostPigment), Progress->GetPigment(Choice.CostPigment));
		DrawText(Cost, TextColour, X + 24.0f, Y + 54.0f, nullptr, 1.0f);
		float Cursor = DrawWrapped(Choice.Consequence, X + 24.0f, Y + 84.0f, Width - 48.0f, TextColour, 1.0f) + 16.0f;
		Cursor = DrawWrapped(TEXT("This cannot be painted over. It will be carried for as long as the world lasts."), X + 24.0f, Cursor, Width - 48.0f, HealthColour * 2.2f, 1.0f) + 12.0f;
		DrawText(TEXT("1.  Do it."), GoodColour, X + 40.0f, Cursor, nullptr, 1.1f);
		DrawText(TEXT("2.  Not yet."), TextColour, X + 40.0f, Cursor + 28.0f, nullptr, 1.1f);
		return;
	}

	DrawPanel(X, Y, Width, 110.0f + Row->Choices.Num() * 116.0f);
	DrawText(ChoiceSubject, GlimColour, X + 24.0f, Y + 16.0f, nullptr, 1.25f);
	DrawText(TEXT("What you decide here is permanent."), DimColour, X + 24.0f, Y + 46.0f, nullptr, 0.9f);
	float Cursor = Y + 80.0f;
	for (int32 Index = 0; Index < Row->Choices.Num(); ++Index)
	{
		const FQuestChoice& Choice = Row->Choices[Index];
		FString Why;
		const bool bCan = Progress->CanChoose(ChoiceQuest, Choice.ChoiceId, &Why);
		const FString Cost = Choice.CostPigment.IsNone() ? FString(TEXT("free")) : FString::Printf(TEXT("%d %s"), Choice.CostAmount, *Progress->GetDisplayName(Choice.CostPigment));
		DrawText(FString::Printf(TEXT("%d.  %s   (%s)%s%s"), Index + 1, *Choice.Label, *Cost, bCan ? TEXT("") : TEXT("   -   "), bCan ? TEXT("") : *Why),
			bCan ? GoodColour : DimColour, X + 24.0f, Cursor, nullptr, 1.05f);
		DrawWrapped(Choice.Consequence, X + 54.0f, Cursor + 24.0f, Width - 80.0f, DimColour, 0.85f);
		Cursor += 116.0f;
	}
	DrawText(TEXT("Number to choose (you will be asked again)   [E] or [Q] walk away"), DimColour, X + 24.0f, Cursor, nullptr, 0.9f);
}
