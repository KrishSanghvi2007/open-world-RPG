// Owns the player's progress for the running game, and saves and loads it. There is one save (docs/GDD.md
// section 7: "one save and no undo for the world").

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnderpaintingProgress.h"
#include "UnderpaintingProgressSubsystem.generated.h"

UCLASS()
class UUnderpaintingSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(SaveGame) FUnderpaintingProgressData Progress;

	/** Where the player was standing, and in which level. */
	UPROPERTY(SaveGame) FName LevelName;
	UPROPERTY(SaveGame) FTransform PlayerTransform;
	UPROPERTY(SaveGame) float Health = 100.0f;
	UPROPERTY(SaveGame) float Glim = 100.0f;
};

UCLASS()
class UUnderpaintingProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UUnderpaintingProgress* GetProgress() const { return Progress; }

	/** Convenience for gameplay code: the progress object for the game this object lives in. */
	static UUnderpaintingProgress* Get(const UObject* WorldContext);

	/** Writes the save. Call with the player so position and health are stored too. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Save")
	bool SaveGame(APawn* Player);

	/** Reads the save into the running game. Returns false if there is none. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Save")
	bool LoadGame();

	/** Deletes the save and starts again. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Save")
	void NewGame();

	UFUNCTION(BlueprintPure, Category = "Underpainting|Save")
	bool HasSave() const;

	/** The last save read or written, for the player to restore position and health from. */
	const UUnderpaintingSaveGame* GetLoadedSave() const { return LoadedSave; }

	/** Walks through a zone exit: loads the other zone's level and remembers where the player came from. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Travel")
	bool TravelToZone(const UObject* WorldContext, FName TargetZone);

	/** The zone the player just left through an exit, or None. Cleared once read. */
	FName ConsumeArrival();

	/** Asks for the player to be put back at the saved position when the next level starts (after a death). */
	void RequestRestore() { bRestorePending = true; ArrivingFromZone = NAME_None; }

	/** True once after a load: the player should be put back where the save says. */
	bool ConsumeRestore();

	static const TCHAR* SlotName() { return TEXT("Underpainting"); }

private:

	UPROPERTY(Transient) TObjectPtr<UUnderpaintingProgress> Progress;
	UPROPERTY(Transient) TObjectPtr<UUnderpaintingSaveGame> LoadedSave;
	FName ArrivingFromZone;
	bool bRestorePending = false;
};
