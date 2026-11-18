// The drawn HUD: bars, the quest tracker, the compass, the interaction prompt, short notices, and the menus
// (dialogue, crafting, the palette, quest choices, reading, and the satchel with its Pack, Journal, Notes and
// Settings tabs). Everything is drawn on the Canvas and driven by keys; the game rules do not depend on it.
// The satchel is the UI style probe: a sheet of paper, ink, and strokes of the six pigments.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingHUD.generated.h"

class UUnderpaintingProgress;

UENUM()
enum class EUnderpaintingMenu : uint8
{
	None,
	Dialogue,
	Crafting,
	Attunement,
	Inventory,
	Reading,
	Choice
};

UCLASS()
class AUnderpaintingHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

	bool IsMenuOpen() const { return Menu != EUnderpaintingMenu::None; }
	EUnderpaintingMenu GetMenu() const { return Menu; }

	/** Starts a conversation with whoever owns these dialogue rows. Does nothing if they have nothing to say. */
	void OpenDialogue(FName NpcId);
	void OpenCrafting(ECraftingStation Station);
	void OpenAttunement();
	void ToggleInventory();

	/** Shows a piece of lore from data/lore.json. */
	void OpenReading(FName LoreId);

	/** Shows a title and a paragraph (a chalk note, a mark on a wall). */
	void OpenText(const FString& Title, const FString& Text);

	/** Offers a quest's endings, then asks again before making one. */
	void OpenChoice(FName QuestId, const FString& Subject);
	void CloseMenu();

	/** The Interact key while a menu is open: next line of dialogue, or close. */
	void Advance();

	/** A number key, 1 to 9: pick a reply, craft a recipe, or buy a node. */
	void ChooseNumber(int32 Number);

	/** The satchel's tabs: left and right. */
	void MoveTab(int32 Direction);

	/** Up and down a list (the settings rows, the pages of notes). */
	void MoveCursor(int32 Direction);

	/** True while the settings screen is waiting for the player to press the key they want. */
	bool IsCapturingKey() const { return !CapturingAction.IsNone(); }

	/** The key pressed while waiting: it becomes the binding. */
	void CaptureKey(FKey Key);

	int32 GetSatchelTab() const { return SatchelTab; }

	/** The mouse button, while a menu is open: presses whatever row or button the pointer is on. */
	void Click();

	/** Development: a click at a fraction of the screen, as if the pointer were there. */
	void DebugClick(float U, float V);
	FVector2D DebugPointer = FVector2D(-1.0f, -1.0f);
	FVector2D LastClip = FVector2D(1280.0f, 720.0f);

	/** The dialogue row on screen, for tests and tools. */
	FName GetDialogueRow() const { return DialogueRow; }

private:

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill, const FString& Label);
	void DrawPanel(float X, float Y, float Width, float Height);
	float DrawWrapped(const FString& Text, float X, float Y, float Width, const FLinearColor& Colour, float Scale = 1.0f);
	void DrawPlayerBars(const class AUnderpaintingPlayerCharacter* Player);
	void DrawEnemyBars();
	void DrawTracker(const UUnderpaintingProgress* Progress);
	void DrawNotices();
	void DrawDialogue(const UUnderpaintingProgress* Progress);
	void DrawCrafting(const UUnderpaintingProgress* Progress);
	void DrawAttunement(const UUnderpaintingProgress* Progress);
	void DrawInventory(const UUnderpaintingProgress* Progress);
	float DrawPaper(const FString& Title, float& OutX, float& OutWidth);
	void DrawPackTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width);
	void DrawJournalTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width);
	void DrawNotesTab(const UUnderpaintingProgress* Progress, float X, float Y, float Width);
	void DrawSettingsTab(float X, float Y, float Width);
	void ActivateSetting();
	int32 SettingsRowCount() const;
	void DrawReading(const UUnderpaintingProgress* Progress);
	void DrawChoice(const UUnderpaintingProgress* Progress);
	void DrawTurning(const AUnderpaintingPlayerCharacter* Player, UUnderpaintingProgress* Progress);
	void DrawFeel(const AUnderpaintingPlayerCharacter* Player);
	void DrawVital(float X, float Y, float Width, float Height, float Fraction, float& Ghost, const FLinearColor& Fill, const FString& Label, float DeniedAt, bool bLow);
	void Sound(FName Row, float Volume = 0.7f);

	/** Marks a rectangle as something the mouse can press, and tints it while the pointer is over it. Call before drawing its text. */
	bool Hot(float X, float Y, float Width, float Height, int32 Number, FName Special = NAME_None);

	struct FHot
	{
		float X = 0.0f, Y = 0.0f, Width = 0.0f, Height = 0.0f;
		int32 Number = 0;
		FName Special;
	};
	TArray<FHot> Hots;
	FVector2D Pointer = FVector2D(-1.0f, -1.0f);
	bool bPointerShown = false;

	// Bars: where each was a moment ago, so a loss shows as a pale strip that drains away.
	float GhostHealth = 1.0f;
	float GhostStamina = 1.0f;
	float GhostGlim = 1.0f;
	float LastGlim = -1.0f;
	float GlimFlash = 0.0f;

	/** Seconds left of the "she has turned" title card after the changed level opens. */
	float TurnedCardRemaining = 0.0f;
	/** Seconds the end card has been up, or -1. */
	float EndCardTime = -1.0f;
	void DrawCompass(const class AUnderpaintingPlayerCharacter* Player, const UUnderpaintingProgress* Progress);
	void ShowDialogueRow(FName RowId);
	void AddNotice(const FString& Text);
	TArray<FName> GetAttunementList(const UUnderpaintingProgress* Progress) const;

	float ProjectionScale = 1.0f;
	EUnderpaintingMenu Menu = EUnderpaintingMenu::None;
	FName DialogueNpc;
	FName DialogueRow;
	ECraftingStation CraftingStation = ECraftingStation::GrindingSlab;
	FName ReadingLore;
	FString FreeTitle;
	FString FreeText;
	FName ChoiceQuest;
	FString ChoiceSubject;
	int32 ChoicePending = INDEX_NONE;

	// The satchel.
	int32 SatchelTab = 0;
	int32 SettingsCursor = 0;
	int32 NotesPage = 0;
	FName CapturingAction;
	bool bConfirmNewGame = false;

	struct FNotice
	{
		FString Text;
		float Time = 0.0f;
	};
	TArray<FNotice> Notices;
	FDelegateHandle NoticeHandle;
};
