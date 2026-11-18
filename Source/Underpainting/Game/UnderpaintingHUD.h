// A plain drawn HUD for the walking skeleton: bars, the quest tracker, the interaction prompt, short notices,
// and four simple menus (dialogue, crafting, the palette, the pack). Everything is text on panels, driven by
// number keys. It is replaced by a designed UI in milestone M9; the game rules do not depend on it.

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
	void DrawReading(const UUnderpaintingProgress* Progress);
	void DrawChoice(const UUnderpaintingProgress* Progress);
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

	struct FNotice
	{
		FString Text;
		float Time = 0.0f;
	};
	TArray<FNotice> Notices;
	FDelegateHandle NoticeHandle;
};
