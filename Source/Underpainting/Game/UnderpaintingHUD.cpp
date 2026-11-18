#include "UnderpaintingHUD.h"

#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingEnemy.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingSettings.h"
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
		bConfirmNewGame = false;
	}
}

void AUnderpaintingHUD::MoveTab(int32 Direction)
{
	if (Menu == EUnderpaintingMenu::Inventory)
	{
		SatchelTab = (SatchelTab + Direction + 4) % 4;
		bConfirmNewGame = false;
	}
}

int32 AUnderpaintingHUD::SettingsRowCount() const
{
	return 2 + UUnderpaintingSettingsSave::Actions().Num() + 2;        // text size, hints, the keys, reset keys, new game
}

void AUnderpaintingHUD::MoveCursor(int32 Direction)
{
	if (Menu != EUnderpaintingMenu::Inventory)
	{
		return;
	}
	if (SatchelTab == 3)
	{
		SettingsCursor = (SettingsCursor + Direction + SettingsRowCount()) % SettingsRowCount();
		bConfirmNewGame = false;
	}
	else if (SatchelTab == 2)
	{
		NotesPage = FMath::Max(0, NotesPage + Direction);
	}
}

void AUnderpaintingHUD::ActivateSetting()
{
	UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	if (!Settings)
	{
		return;
	}
	const TArray<FUnderpaintingAction>& Actions = UUnderpaintingSettingsSave::Actions();
	const int32 KeyRow = SettingsCursor - 2;
	if (SettingsCursor == 0)
	{
		Settings->CycleTextSize();
	}
	else if (SettingsCursor == 1)
	{
		Settings->ToggleKeyHints();
	}
	else if (Actions.IsValidIndex(KeyRow))
	{
		CapturingAction = Actions[KeyRow].Id;        // the player controller hands us the next key pressed
	}
	else if (KeyRow == Actions.Num())
	{
		Settings->ResetKeys();
		AddNotice(TEXT("Keys put back as they were."));
	}
	else if (!bConfirmNewGame)
	{
		bConfirmNewGame = true;
	}
	else
	{
		bConfirmNewGame = false;
		CloseMenu();
		if (AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(GetOwningPawn()))
		{
			Player->StartNewGame();
		}
	}
}

void AUnderpaintingHUD::CaptureKey(FKey Key)
{
	const FName Action = CapturingAction;
	CapturingAction = NAME_None;
	// Keys the menus themselves need cannot be given away.
	static const TArray<FKey> Reserved = { EKeys::Escape, EKeys::Left, EKeys::Right, EKeys::Up, EKeys::Down,
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine,
		EKeys::F5, EKeys::F7, EKeys::F8, EKeys::F9, EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::SpaceBar };
	if (Reserved.Contains(Key))
	{
		AddNotice(Key == EKeys::Escape ? TEXT("Left as it was.") : *FString::Printf(TEXT("%s is kept for moving and menus."), *Key.GetDisplayName(false).ToString()));
		return;
	}
	if (UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this))
	{
		Settings->SetKey(Action, Key);
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
	if (Menu == EUnderpaintingMenu::Inventory && SatchelTab == 3)
	{
		ActivateSetting();        // on the settings tab, Use presses the highlighted row
		return;
	}
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
	else if (Menu == EUnderpaintingMenu::Inventory && SatchelTab == 0)
	{
		const TArray<FName> Gear = Progress->GetGear();
		if (Gear.IsValidIndex(Index))
		{
			Progress->Equip(Gear[Index]);
		}
	}
	else if (Menu == EUnderpaintingMenu::Inventory && SatchelTab == 2)
	{
		// Read a note again.
		const TArray<FName>& Found = Progress->GetData().FoundLore;
		if (Found.IsValidIndex(NotesPage * 9 + Index))
		{
			OpenReading(Found[NotesPage * 9 + Index]);
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
	const UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	const float UIScale = FMath::Max(1.0f, Canvas->ClipY / 720.0f) * 1.25f * (Settings ? Settings->Read()->GetUIScale() : 1.0f);
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
		if (Menu != EUnderpaintingMenu::Inventory)
		{
			DrawTracker(Progress);        // the satchel has the Journal; the tracker would sit on its corner
		}
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
		const FString Prompt = FString::Printf(TEXT("[%s]  %s"), Settings ? *Settings->Read()->DescribeKey(TEXT("Use")) : TEXT("E"), *Player->GetFocus()->GetUsablePrompt());
		float Width = 0.0f, Height = 0.0f;
		GetTextSize(Prompt, Width, Height, nullptr, 1.2f);
		const float X = (Canvas->ClipX - Width) * 0.5f;
		const float Y = Canvas->ClipY * 0.62f;
		DrawRect(BackColour, X - 12.0f, Y - 6.0f, Width + 24.0f, Height + 12.0f);
		DrawText(Prompt, TextColour, X, Y, nullptr, 1.2f);
	}

	// Last, so it covers everything: the Turning, its title card, and the end card.
	if (Player)
	{
		DrawTurning(Player, UUnderpaintingProgressSubsystem::Get(this));
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
		const UUnderpaintingSettings* ItemSettings = UUnderpaintingSettings::Get(this);
		const UUnderpaintingSettingsSave* ItemKeys = ItemSettings ? ItemSettings->Read() : GetDefault<UUnderpaintingSettingsSave>();
		DrawText(FString::Printf(TEXT("Poultice %d [%s]   Lamp Oil %d [%s]"), Progress->GetItemCount(TEXT("MossPoultice")), *ItemKeys->DescribeKey(TEXT("Poultice")),
			Progress->GetItemCount(TEXT("LampOil")), *ItemKeys->DescribeKey(TEXT("LampOil"))), DimColour, WellX, Y + 50.0f, nullptr, 0.85f);
		FString Extras;
		if (Progress->GetItemCount(TEXT("SanguineDraught")) > 0 || Player->GetDraughtRemaining() > 0.0f)
		{
			Extras += FString::Printf(TEXT("Draught %d [%s]%s   "), Progress->GetItemCount(TEXT("SanguineDraught")), *ItemKeys->DescribeKey(TEXT("Draught")),
				Player->GetDraughtRemaining() > 0.0f ? *FString::Printf(TEXT(" %.0fs"), Player->GetDraughtRemaining()) : TEXT(""));
		}
		if (Progress->GetItemCount(TEXT("BlueTincture")) > 0 || Player->GetTinctureRemaining() > 0.0f)
		{
			Extras += FString::Printf(TEXT("Tincture %d [%s]%s"), Progress->GetItemCount(TEXT("BlueTincture")), *ItemKeys->DescribeKey(TEXT("Tincture")),
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
		const UUnderpaintingSettings* KeySettings = UUnderpaintingSettings::Get(this);
		const UUnderpaintingSettingsSave* Keys = KeySettings ? KeySettings->Read() : nullptr;
		if (Keys && Keys->bKeyHints)
		{
			DrawText(FString::Printf(TEXT("%s use   %s light   %s heavy (hold: Stroke)   %s dodge   %s sprint   %s lock on   %s swap well   %s flare   %s satchel"),
				*Keys->DescribeKey(TEXT("Use")), *Keys->DescribeKey(TEXT("LightAttack")), *Keys->DescribeKey(TEXT("HeavyAttack")), *Keys->DescribeKey(TEXT("Dodge")),
				*Keys->DescribeKey(TEXT("Sprint")), *Keys->DescribeKey(TEXT("LockOn")), *Keys->DescribeKey(TEXT("SwapWell")), *Keys->DescribeKey(TEXT("Flare")),
				*Keys->DescribeKey(TEXT("Pack"))), DimColour, X, Canvas->ClipY - 30.0f, nullptr, 0.85f);
		}
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

	// In the middle of the screen while playing; tucked into the top left corner while a menu is open.
	const bool bAside = Menu != EUnderpaintingMenu::None;
	float Y = bAside ? 12.0f : Canvas->ClipY * 0.24f;
	for (const FNotice& Notice : Notices)
	{
		float Width = 0.0f, Height = 0.0f;
		GetTextSize(Notice.Text, Width, Height, nullptr, 1.0f);
		const float X = bAside ? 24.0f : (Canvas->ClipX - Width) * 0.5f;
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

namespace
{
	// The satchel is paper and ink (the UI style probe).
	const FLinearColor SheetPaper = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E9DFC8")));
	const FLinearColor PaperShade = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("CDBF9F")));
	const FLinearColor SheetInk = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("2E2A2A")));
	const FLinearColor InkSoft = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("6E6258")));
	const FLinearColor InkRed = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("9E2B25")));
	const FLinearColor InkGreen = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("3F6B4A")));
	const TCHAR* TabNames[] = { TEXT("Pack"), TEXT("Journal"), TEXT("Notes"), TEXT("Settings") };
	const TCHAR* TabStrokes[] = { TEXT("D9A441"), TEXT("9E2B25"), TEXT("2F4BA8"), TEXT("6B8F71") };
}

float AUnderpaintingHUD::DrawPaper(const FString& Title, float& OutX, float& OutWidth)
{
	const float Width = FMath::Min(1000.0f, Canvas->ClipX - 80.0f);
	const float Height = FMath::Min(620.0f, Canvas->ClipY - 96.0f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = 62.0f;

	// A sheet with a soft shadow, a darker deckle edge, and faint ruled lines.
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), X + 6.0f, Y + 8.0f, Width, Height);
	DrawRect(PaperShade, X - 2.0f, Y - 2.0f, Width + 4.0f, Height + 4.0f);
	DrawRect(SheetPaper, X, Y, Width, Height);
	for (float LineY = Y + 96.0f; LineY < Y + Height - 20.0f; LineY += 26.0f)
	{
		DrawRect(FLinearColor(PaperShade.R, PaperShade.G, PaperShade.B, 0.45f), X + 24.0f, LineY, Width - 48.0f, 1.0f);
	}

	// Tabs: each a word with a stroke of one pigment under it. The open one is inked; the others are pencil.
	float TabX = X + 28.0f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const bool bOpen = Index == SatchelTab;
		float TextW = 0.0f, TextH = 0.0f;
		GetTextSize(TabNames[Index], TextW, TextH, nullptr, 1.3f);
		DrawText(TabNames[Index], bOpen ? SheetInk : InkSoft, TabX, Y + 16.0f, nullptr, 1.3f);
		const FLinearColor Stroke = FLinearColor::FromSRGBColor(FColor::FromHex(TabStrokes[Index]));
		if (bOpen)
		{
			// A loaded brush dragged left to right: three passes, slightly off each other, thinning at the end.
			DrawRect(FLinearColor(Stroke.R, Stroke.G, Stroke.B, 0.85f), TabX - 6.0f, Y + 46.0f, TextW + 10.0f, 5.0f);
			DrawRect(FLinearColor(Stroke.R, Stroke.G, Stroke.B, 0.55f), TabX - 2.0f, Y + 50.0f, TextW + 16.0f, 3.0f);
			DrawRect(FLinearColor(Stroke.R, Stroke.G, Stroke.B, 0.30f), TabX + 8.0f, Y + 44.0f, TextW + 12.0f, 2.0f);
		}
		else
		{
			DrawRect(FLinearColor(Stroke.R, Stroke.G, Stroke.B, 0.35f), TabX, Y + 48.0f, TextW, 2.0f);
		}
		TabX += TextW + 44.0f;
	}
	DrawText(TEXT("[Left] [Right] turn the page"), InkSoft, X + Width - 250.0f, Y + 24.0f, nullptr, 0.85f);
	DrawText(Title, InkSoft, X + 28.0f, Y + 66.0f, nullptr, 0.9f);

	OutX = X + 28.0f;
	OutWidth = Width - 56.0f;
	return Y + 100.0f;
}

void AUnderpaintingHUD::DrawInventory(const UUnderpaintingProgress* Progress)
{
	static const TCHAR* Titles[] = { TEXT("What you carry, and what you wear."), TEXT("What you have been asked, and what you decided."),
		TEXT("What you have found to read."), TEXT("How the game answers your hands.") };
	float X = 0.0f, Width = 0.0f;
	const float Y = DrawPaper(Titles[SatchelTab], X, Width);
	switch (SatchelTab)
	{
	case 0: DrawPackTab(Progress, X, Y, Width); break;
	case 1: DrawJournalTab(Progress, X, Y, Width); break;
	case 2: DrawNotesTab(Progress, X, Y, Width); break;
	default: DrawSettingsTab(X, Y, Width); break;
	}
}

void AUnderpaintingHUD::DrawPackTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width)
{
	const FUnderpaintingProgressData& Data = Progress->GetData();
	const UUnderpaintingDataSubsystem* DataTables = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const float Column = Width / 3.0f;

	// Pigment: a dab of the colour, then wet and dry.
	float Cursor = Y;
	DrawText(TEXT("PIGMENT"), InkSoft, X, Cursor, nullptr, 0.9f);
	DrawText(TEXT("wet / dry"), InkSoft, X + 150.0f, Cursor, nullptr, 0.8f);
	Cursor += 26.0f;
	for (const TCHAR* Id : { TEXT("Ochre"), TEXT("TerreVerte"), TEXT("Sanguine"), TEXT("BoneBlack"), TEXT("Ceruse"), TEXT("SlumberBlue") })
	{
		const FPigmentRow* Row = DataTables ? DataTables->FindPigment(Id) : nullptr;
		const FLinearColor Dab = Row ? UUnderpaintingDataSubsystem::HexToColour(Row->ColourHex) : SheetInk;
		const bool bHave = Progress->GetPigment(Id) > 0;
		DrawRect(FLinearColor(Dab.R, Dab.G, Dab.B, bHave ? 1.0f : 0.25f), X, Cursor + 3.0f, 22.0f, 14.0f);
		DrawRect(FLinearColor(Dab.R, Dab.G, Dab.B, bHave ? 0.5f : 0.12f), X + 18.0f, Cursor + 7.0f, 12.0f, 8.0f);
		DrawText(FString::Printf(TEXT("%s"), *Progress->GetDisplayName(Id)), bHave ? SheetInk : InkSoft, X + 38.0f, Cursor, nullptr, 1.0f);
		DrawText(FString::Printf(TEXT("%d / %d"), Progress->GetWetPigment(Id), Progress->GetDryPigment(Id)), bHave ? SheetInk : InkSoft, X + 170.0f, Cursor, nullptr, 1.0f);
		Cursor += 26.0f;
	}
	Cursor += 14.0f;
	DrawWrapped(TEXT("Wet pigment is half lost if you are rubbed out. Resting at a Lampstand dries it."), X, Cursor, Column - 30.0f, InkSoft, 0.8f);

	// The pack.
	float PackY = Y;
	const float PackX = X + Column;
	DrawText(TEXT("PACK"), InkSoft, PackX, PackY, nullptr, 0.9f);
	PackY += 26.0f;
	for (const TPair<FName, int32>& Pair : Data.Items)
	{
		const FItemRow* Item = Progress->GetItemRow(Pair.Key);
		if (Pair.Value > 0 && Item && Item->Type != EItemType::Weapon && Item->Type != EItemType::Armour && PackY < Canvas->ClipY - 170.0f)
		{
			DrawText(FString::Printf(TEXT("%s"), *Item->DisplayName), SheetInk, PackX, PackY, nullptr, 1.0f);
			DrawText(FString::Printf(TEXT("x%d"), Pair.Value), InkSoft, PackX + Column - 70.0f, PackY, nullptr, 1.0f);
			PackY += 26.0f;
		}
	}

	// Gear.
	float GearY = Y;
	const float GearX = X + Column * 2.0f;
	DrawText(TEXT("GEAR"), InkSoft, GearX, GearY, nullptr, 0.9f);
	DrawText(TEXT("number to put on"), InkSoft, GearX + 70.0f, GearY, nullptr, 0.8f);
	GearY += 26.0f;
	const TArray<FName> Gear = Progress->GetGear();
	for (int32 Index = 0; Index < Gear.Num(); ++Index)
	{
		const FItemRow* Item = Progress->GetItemRow(Gear[Index]);
		if (!Item)
		{
			continue;
		}
		const bool bOn = Gear[Index] == Progress->GetEquippedWeapon() || Gear[Index] == Progress->GetEquippedArmour();
		if (bOn)
		{
			DrawRect(FLinearColor(0.85f, 0.42f, 0.10f, 0.30f), GearX - 6.0f, GearY, Column - 10.0f, 42.0f);
		}
		DrawText(FString::Printf(TEXT("%d.  %s"), Index + 1, *Item->DisplayName), SheetInk, GearX, GearY, nullptr, 1.0f);
		DrawText(Item->Type == EItemType::Weapon
			? FString::Printf(TEXT("%d / %d damage, reach %.1f m"), Item->LightDamage, Item->HeavyDamage, Item->Reach)
			: FString::Printf(TEXT("stops %.0f%% of damage"), Item->DamageReduction * 100.0f), InkSoft, GearX + 24.0f, GearY + 22.0f, nullptr, 0.8f);
		GearY += 46.0f;
	}
}

void AUnderpaintingHUD::DrawJournalTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width)
{
	const FUnderpaintingProgressData& Data = Progress->GetData();
	const float Column = Width * 0.5f;
	float Left = Y, Right = Y;

	DrawText(TEXT("IN HAND"), InkSoft, X, Left, nullptr, 0.9f);
	Left += 26.0f;
	DrawText(TEXT("DONE"), InkSoft, X + Column, Right, nullptr, 0.9f);
	Right += 26.0f;
	bool bAnyOpen = false;
	for (const TPair<FName, FQuestProgress>& Pair : Data.Quests)
	{
		const FQuestRow* Row = Progress->GetQuestRow(Pair.Key);
		if (!Row)
		{
			continue;
		}
		if (Pair.Value.bComplete)
		{
			DrawText(Row->DisplayName, SheetInk, X + Column, Right, nullptr, 1.05f);
			Right += 24.0f;
			const FName ChoiceId = Pair.Value.Choice;
			const FQuestChoice* Choice = Row->Choices.FindByPredicate([ChoiceId](const FQuestChoice& Entry) { return Entry.ChoiceId == ChoiceId; });
			if (Choice)
			{
				// What you decided, in red: it is permanent.
				Right = DrawWrapped(FString::Printf(TEXT("You chose: %s."), *Choice->Label), X + Column + 16.0f, Right, Column - 30.0f, InkRed, 0.9f) + 4.0f;
				Right = DrawWrapped(Choice->Consequence, X + Column + 16.0f, Right, Column - 30.0f, InkSoft, 0.8f);
			}
			Right += 14.0f;
			continue;
		}

		bAnyOpen = true;
		DrawText(Row->DisplayName, SheetInk, X, Left, nullptr, 1.05f);
		Left += 26.0f;
		for (int32 Step = 0; Step < Row->Steps.Num() && Step <= Pair.Value.Step; ++Step)
		{
			const bool bNow = Step == Pair.Value.Step;
			DrawText(bNow ? TEXT(">") : TEXT("x"), bNow ? InkRed : InkGreen, X + 4.0f, Left, nullptr, 0.9f);
			Left = DrawWrapped(Row->Steps[Step], X + 24.0f, Left, Column - 50.0f, bNow ? SheetInk : InkSoft, 0.9f) + 4.0f;
		}
		Left += 14.0f;
	}
	if (!bAnyOpen)
	{
		DrawText(TEXT("Nothing is being asked of you."), InkSoft, X, Left, nullptr, 0.95f);
	}
}

void AUnderpaintingHUD::DrawNotesTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width)
{
	const TArray<FName>& Found = Progress->GetData().FoundLore;
	const int32 Pages = FMath::Max(1, (Found.Num() + 8) / 9);
	NotesPage = FMath::Clamp(NotesPage, 0, Pages - 1);

	int32 Have = 0, Total = 0;
	FString Counts;
	Progress->CountLore(ELoreKind::FieldNote, Have, Total);
	Counts += FString::Printf(TEXT("Odile's notes %d of %d      "), Have, Total);
	Progress->CountLore(ELoreKind::Marginalia, Have, Total);
	Counts += FString::Printf(TEXT("Marginalia %d of %d      "), Have, Total);
	Progress->CountLore(ELoreKind::Ledger, Have, Total);
	Counts += FString::Printf(TEXT("Ledgers %d of %d"), Have, Total);
	DrawText(Counts, InkSoft, X, Y, nullptr, 0.9f);

	float Cursor = Y + 34.0f;
	if (Found.Num() == 0)
	{
		DrawText(TEXT("Nothing yet. Things worth reading do not announce themselves."), InkSoft, X, Cursor, nullptr, 0.95f);
		return;
	}
	for (int32 Slot = 0; Slot < 9; ++Slot)
	{
		const int32 Index = NotesPage * 9 + Slot;
		if (!Found.IsValidIndex(Index))
		{
			break;
		}
		const FLoreRow* Row = Progress->GetLore(Found[Index]);
		if (!Row)
		{
			continue;
		}
		const bool bHand = Row->Kind == ELoreKind::Marginalia;
		DrawText(FString::Printf(TEXT("%d.  %s"), Slot + 1, *Row->Title), bHand ? FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("2F4BA8"))) : SheetInk, X, Cursor, nullptr, 1.0f);
		DrawText(Row->Text.Left(84) + (Row->Text.Len() > 84 ? TEXT("...") : TEXT("")), InkSoft, X + 28.0f, Cursor + 22.0f, nullptr, 0.8f);
		Cursor += 50.0f;
	}
	DrawText(FString::Printf(TEXT("Number to read again      [Up] [Down] page %d of %d"), NotesPage + 1, Pages), InkSoft, X, Canvas->ClipY - 70.0f, nullptr, 0.85f);
}

void AUnderpaintingHUD::DrawSettingsTab(float X, float Y, float Width)
{
	const UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	if (!Settings)
	{
		return;
	}
	const UUnderpaintingSettingsSave* Save = Settings->Read();
	const TArray<FUnderpaintingAction>& Actions = UUnderpaintingSettingsSave::Actions();
	static const TCHAR* Sizes[] = { TEXT("small"), TEXT("normal"), TEXT("large"), TEXT("largest") };

	struct FRow { FString Label; FString Value; };
	TArray<FRow> Rows;
	Rows.Add({ TEXT("Text size"), Sizes[FMath::Clamp(Save->TextSizeStep, 0, 3)] });
	Rows.Add({ TEXT("Key hints along the bottom"), Save->bKeyHints ? TEXT("shown") : TEXT("hidden") });
	for (const FUnderpaintingAction& Action : Actions)
	{
		Rows.Add({ Action.Label, CapturingAction == Action.Id ? FString(TEXT("press a key...")) : Save->DescribeKey(Action.Id) });
	}
	Rows.Add({ TEXT("Put all keys back"), FString() });
	Rows.Add({ TEXT("New game"), bConfirmNewGame ? FString(TEXT("Press again to erase your save. This cannot be undone.")) : FString() });

	// Two columns so everything fits on the sheet: general on the left with the first keys, the rest on the right.
	const int32 PerColumn = (Rows.Num() + 1) / 2;
	const float Column = Width * 0.5f;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const float RowX = X + (Index >= PerColumn ? Column : 0.0f);
		const float RowY = Y + (Index % PerColumn) * 26.0f;
		const bool bHere = Index == SettingsCursor;
		if (bHere)
		{
			DrawRect(FLinearColor(0.85f, 0.42f, 0.10f, 0.30f), RowX - 8.0f, RowY - 2.0f, Column - 16.0f, 24.0f);
			DrawText(TEXT(">"), InkRed, RowX - 4.0f, RowY, nullptr, 1.0f);
		}
		DrawText(Rows[Index].Label, SheetInk, RowX + 14.0f, RowY, nullptr, 0.95f);
		const bool bWarn = Index == Rows.Num() - 1 || (Index >= 2 && Index < 2 + Actions.Num() && CapturingAction == Actions[Index - 2].Id);
		DrawText(Rows[Index].Value, bWarn ? InkRed : InkGreen, RowX + (Index == Rows.Num() - 1 ? 110.0f : 270.0f), RowY, nullptr, Index == Rows.Num() - 1 ? 0.8f : 0.95f);
	}
	const FString UseKey = Save->DescribeKey(TEXT("Use"));
	DrawText(FString::Printf(TEXT("[Up] [Down] choose      [%s] change      Moving is W A S D, looking is the mouse, jumping is Space."), *UseKey),
		InkSoft, X, Canvas->ClipY - 70.0f, nullptr, 0.85f);
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

void AUnderpaintingHUD::DrawTurning(const AUnderpaintingPlayerCharacter* Player, UUnderpaintingProgress* Progress)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float Delta = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	const auto Centred = [this, W](const FString& Text, float Y, const FLinearColor& Colour, float Scale)
	{
		float TextW = 0.0f, TextH = 0.0f;
		GetTextSize(Text, TextW, TextH, nullptr, Scale);
		DrawText(Text, Colour, (W - TextW) * 0.5f, Y, nullptr, Scale);
	};

	// The Turning: the ground moves, the view is taken away, three lines, and the world is different (GDD section 8).
	const float T = Player->GetTurningTime();
	if (T >= 0.0f)
	{
		const float Dark = FMath::Clamp((T - 2.0f) / 1.2f, 0.0f, 1.0f);
		DrawRect(FLinearColor(0.01f, 0.008f, 0.012f, Dark), 0.0f, 0.0f, W, H);
		if (T < 2.6f)
		{
			Centred(TEXT("The floor moves. Hold on."), H * 0.72f, GlimColour, 1.3f);
		}
		const TCHAR* Lines[] = { TEXT("Under your hand, the wall is warm."), TEXT("Far above, an eye the size of a lake half opens."), TEXT("She turns over in her sleep.") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Alpha = FMath::Clamp((T - 3.4f - Index * 1.6f) / 0.8f, 0.0f, 1.0f);
			if (Alpha > 0.0f)
			{
				Centred(Lines[Index], H * 0.40f + Index * 34.0f, FLinearColor(0.93f, 0.87f, 0.74f, Alpha), 1.25f);
			}
		}
		return;
	}

	// The changed level has just opened: come up out of the dark under the title.
	if (UUnderpaintingProgressSubsystem* Travel = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr)
	{
		if (Travel->ConsumeJustTurned())
		{
			TurnedCardRemaining = 5.0f;
		}
	}
	if (TurnedCardRemaining > 0.0f)
	{
		TurnedCardRemaining -= Delta;
		const float Alpha = FMath::Clamp(TurnedCardRemaining / 2.5f, 0.0f, 1.0f);
		DrawRect(FLinearColor(0.01f, 0.008f, 0.012f, Alpha), 0.0f, 0.0f, W, H);
		Centred(TEXT("THE TURNING"), H * 0.40f, FLinearColor(0.93f, 0.87f, 0.74f, FMath::Min(1.0f, TurnedCardRemaining)), 2.0f);
		Centred(TEXT("The light has moved. So has the way home."), H * 0.40f + 54.0f, FLinearColor(0.93f, 0.87f, 0.74f, FMath::Min(1.0f, TurnedCardRemaining)), 1.0f);
	}

	// The end of the slice: once, when Hesper has been answered.
	if (Progress && Menu == EUnderpaintingMenu::None && Progress->IsQuestComplete(TEXT("MQ_ColdUnderTheRibs")) && !Progress->HasFlag(TEXT("Slice.EndSeen")))
	{
		EndCardTime = FMath::Max(0.0f, EndCardTime) + Delta;
		const float Alpha = FMath::Clamp(EndCardTime / 2.0f, 0.0f, 0.86f);
		DrawRect(FLinearColor(0.01f, 0.008f, 0.012f, Alpha), 0.0f, 0.0f, W, H);
		const FLinearColor Ink(0.93f, 0.87f, 0.74f, FMath::Clamp(EndCardTime - 1.0f, 0.0f, 1.0f));
		Centred(TEXT("UNDERPAINTING"), H * 0.34f, Ink, 2.2f);
		Centred(TEXT("End of Act 1: Dusk"), H * 0.34f + 60.0f, Ink, 1.2f);
		Centred(TEXT("The lamp is still going out. One of them is warm."), H * 0.34f + 100.0f, Ink, 1.0f);
		Centred(TEXT("You can keep walking."), H * 0.34f + 150.0f, Ink * FLinearColor(1.0f, 1.0f, 1.0f, 0.7f), 0.9f);
		if (EndCardTime > 14.0f)
		{
			Progress->SetFlag(TEXT("Slice.EndSeen"));
			EndCardTime = -1.0f;
		}
	}
}
