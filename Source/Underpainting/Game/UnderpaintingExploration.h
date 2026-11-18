// The pieces exploration is made of: ways between zones, named places, sketched platforms that are solid only
// while lit, the one-sided walls of the half-painted forest, things to read, and caches that reward curiosity.
// Rules: docs/GDD.md section 6. Words: data/lore.json.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingWorldActors.h"
#include "UnderpaintingExploration.generated.h"

class UBoxComponent;

/** A way out of this zone into another. */
UCLASS()
class AUnderpaintingZoneExit : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingZoneExit();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Zone row in data/regions.json that this exit leads to. A player arriving from that zone appears here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exit")
	FName TargetZone;

protected:

	virtual void Refresh() override;
};

/** A named place. Walking into it discovers it and counts as reaching it for quests. */
UCLASS()
class AUnderpaintingPlace : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingPlace();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Place")
	FName PlaceId;

	/** Shown the first time the player arrives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Place")
	FString DisplayName;

protected:

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Volume;
};

/** A sketched platform: only a drawing until a Flare lights it, or a Fixative Flask fixes it for good. */
UCLASS()
class AUnderpaintingSketchPlatform : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingSketchPlatform();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Sketch")
	bool IsSolid() const { return bSolid; }

	/** How long it stays solid after the light leaves it. */
	UPROPERTY(EditAnywhere, Category = "Sketch")
	float LingerTime = 1.2f;

	float UnlitFor = 100.0f;

	/** Identifies this platform in the save. */
	FName GetPlatformKey() const;

private:

	void SetSolid(bool bNewSolid, bool bFixed);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	bool bSolid = true;
	bool bApplied = false;
};

/**
 * A front with nothing behind it. From its painted side (the actor's forward direction) it is a wall.
 * From behind, where it is bare canvas marked with charcoal cross-hatching, you can walk through it.
 */
UCLASS()
class AUnderpaintingPassableBack : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingPassableBack();

	virtual void Tick(float DeltaTime) override;

	/** True while the player may pass (they are behind it, or already inside it). */
	UFUNCTION(BlueprintPure, Category = "Passable")
	bool IsOpen() const { return bOpen; }

private:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	bool bOpen = false;
};

/** Something to read: a margin note, one of Odile's notes, or a ledger. */
UCLASS()
class AUnderpaintingLoreNote : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingLoreNote();

	virtual void Tick(float DeltaTime) override;
	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Row in data/lore.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FName LoreId;

protected:

	virtual void Refresh() override;

private:

	/** Marginalia cannot be seen until a Flare has been lit near them once. */
	bool IsRevealed() const;
	FName RevealFlag() const;
};

/** A hidden store. Gives what it holds once. */
UCLASS()
class AUnderpaintingCache : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingCache();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Pigments or items inside. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cache")
	TArray<FIdCount> Contents;

protected:

	virtual void Refresh() override;

private:

	FName GetCacheKey() const;
};

/** Something to look at closely: a blood well with Ilka's chalk beside it, a mark on a wall. Counts as Inspect for quests. */
UCLASS()
class AUnderpaintingInspectable : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingInspectable();

	virtual FString GetPrompt() const override { return FString::Printf(TEXT("Look at %s"), *Title); }
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** What quests call it (the Target of an Inspect objective). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect")
	FName InspectId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect")
	FString Text;

protected:

	virtual void Refresh() override;

private:

	FName SeenFlag() const;
};

/** Where a quest is ended: the blank on the Heart wall, Corin's unpainted back, the brood nest. */
UCLASS()
class AUnderpaintingChoicePoint : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingChoicePoint();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Row in data/quests.json whose Choices are offered here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName QuestId;

	/** What the player sees it as ("the blank on the Heart wall"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FString DisplayName;

protected:

	virtual void Refresh() override;
};
