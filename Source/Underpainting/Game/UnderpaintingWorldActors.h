// The things in a level the player walks up to and uses: harvest nodes, pickups, crafting stations, Lampstands
// and people. Each is a grey-box placeholder shape for now; what it does is real.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingWorldActors.generated.h"

class AUnderpaintingPlayerCharacter;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UUnderpaintingProgress;

/** Base for anything the player can walk up to and press Interact on. */
UCLASS(abstract)
class AUnderpaintingInteractable : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingInteractable();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Text for the prompt ("Harvest Hide Ochre"). Empty means it cannot be used right now. */
	virtual FString GetPrompt() const { return FString(); }

	virtual void Interact(AUnderpaintingPlayerCharacter* Player) {}

protected:

	/** Called at start and whenever the player's progress changes (harvest, rest, load). */
	virtual void Refresh() {}

	UUnderpaintingProgress* GetProgress() const;
	void SetShape(const TCHAR* MeshPath, const FVector& Scale, const FVector& Offset);
	void SetColour(const FLinearColor& Colour);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

private:

	FDelegateHandle ChangedHandle;
};

/** A place where one pigment grows. Gives 2 and is empty until the player next rests. */
UCLASS()
class AUnderpaintingHarvestNode : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingHarvestNode();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Row in data/pigments.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FName PigmentId = TEXT("Ochre");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	int32 Yield = 2;

protected:

	virtual void Refresh() override;

private:

	/** Identifies this node in the save: level name plus actor name. */
	FName GetNodeKey() const;
};

/** Something that hands out an item, such as the flax press. Refills when the player rests. */
UCLASS()
class AUnderpaintingPickup : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingPickup();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Row in data/items.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FName ItemId = TEXT("LinseedOil");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	int32 Count = 2;

	/** What the player sees it as ("the flax press"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FString SourceName = TEXT("the flax press");

protected:

	virtual void Refresh() override;

private:

	FName GetNodeKey() const;
};

/** The Grinding Slab or the Armature Bench. */
UCLASS()
class AUnderpaintingCraftingStation : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingCraftingStation();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Station")
	ECraftingStation Station = ECraftingStation::GrindingSlab;
};

/** The rest point. Unlit, it needs Lamp Oil. Lit, it dries pigment, opens the palette and saves the game. */
UCLASS()
class AUnderpaintingLampstand : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingLampstand();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** Name of this Lampstand in quests and in the save. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lampstand")
	FName LampstandId = TEXT("Varnish");

	/** Lit from the start (wild pilot flames are; the village's is not). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lampstand")
	bool bStartsLit = false;

protected:

	virtual void Refresh() override;

	/** A small light, so it casts no shadow (lighting rule). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Flame;

private:

	bool IsLit() const;
};

/** Someone to talk to. Their lines come from data/dialogue.json. */
UCLASS()
class AUnderpaintingNPC : public AUnderpaintingInteractable
{
	GENERATED_BODY()

public:

	AUnderpaintingNPC();

	virtual FString GetPrompt() const override;
	virtual void Interact(AUnderpaintingPlayerCharacter* Player) override;

	/** The Npc value of this person's rows in data/dialogue.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NpcId = TEXT("HesperVane");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString DisplayName = TEXT("Hesper Vane");

protected:

	virtual void Refresh() override;
};
