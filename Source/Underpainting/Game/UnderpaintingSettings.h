// The player's own settings: which key does what, how big the text is, whether key hints show.
// Kept in their own save file, apart from the game save, so a new game does not forget them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnderpaintingSettings.generated.h"

/** One thing a key can be set to do. */
struct FUnderpaintingAction
{
	FName Id;
	FString Label;
	FKey DefaultKey;
};

UCLASS()
class UUnderpaintingSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:

	/** Every action a key can be given, in the order the settings screen lists them. */
	static const TArray<FUnderpaintingAction>& Actions();

	/** The key for an action: the player's choice, or the default. */
	FKey GetKey(FName Action) const;

	/** Gives an action a key. If another action had that key, the two swap, so nothing is left without one. */
	void SetKey(FName Action, FKey Key);

	void ResetKeys() { Keys.Reset(); }

	/** Short name of the action's key for hints ("E", "Left Ctrl"). */
	FString DescribeKey(FName Action) const;

	/** Text size multiplier for the step chosen. */
	float GetUIScale() const;

	UPROPERTY(SaveGame) TMap<FName, FKey> Keys;

	/** 0 small, 1 normal, 2 large, 3 largest. */
	UPROPERTY(SaveGame) int32 TextSizeStep = 1;

	UPROPERTY(SaveGame) bool bKeyHints = true;
};

UCLASS()
class UUnderpaintingSettings : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UUnderpaintingSettings* Get(const UObject* WorldContext);

	const UUnderpaintingSettingsSave* Read() const { return Save; }

	void SetKey(FName Action, FKey Key);
	void ResetKeys();
	void CycleTextSize();
	void ToggleKeyHints();

	/** Fired when any key changes, so the player can bind again. */
	FSimpleMulticastDelegate OnKeysChanged;

private:

	void Write();

	UPROPERTY(Transient) TObjectPtr<UUnderpaintingSettingsSave> Save;
};
