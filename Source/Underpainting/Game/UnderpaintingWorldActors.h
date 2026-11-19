// The things in a level the player walks up to and uses: harvest nodes, pickups, crafting stations, Lampstands
// and people. Each is a grey-box placeholder shape for now; what it does is real.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingFigure.h"
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

	/** The prompt, or nothing if this thing is not in the world right now (see RequiresFlag and HiddenByFlag). */
	FString GetUsablePrompt() const { return IsAvailable() ? GetPrompt() : FString(); }

	/** In the world at all? Things can wait for a flag (a boss's death, a quest's ending) or be removed by one. */
	bool IsAvailable() const;

	/** If set, this thing does not exist until the flag is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World state")
	FName RequiresFlag;

	/** If set, this thing is gone once the flag is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World state")
	FName HiddenByFlag;

	virtual void Interact(AUnderpaintingPlayerCharacter* Player) {}

	virtual void OnConstruction(const FTransform& Transform) override;

	/** The finished look: a kit mesh shown in place of the placeholder shape. The placeholder stays as the thing
	 *  that is touched and used, so giving something a skin never changes how it plays. Set by tools/kit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> SkinMesh;

	/** Where the skin stands, measured from this actor in centimetres (usually straight down, to the ground). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	FVector SkinOffset = FVector::ZeroVector;

	/** Turn of the skin about the upright, in degrees, on top of the actor's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	float SkinYaw = 0.0f;

protected:

	bool HasSkin() const { return SkinMesh != nullptr; }

	/** Shows or hides whichever is the visible body: the skin if there is one, the placeholder if not. */
	void SetShown(bool bShown);

	/** Shrinks the skin (an emptied node, an opened cache) or restores it at 1. */
	void SetSkinSize(float Fraction);

	/** How strongly the part of the skin named "Glow" shines: a lit lantern or a dark one. */
	void SetSkinGlow(float Strength);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Skin;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SkinGlowMaterial;

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

	void PlaceSkin();
	void HandleProgressChanged();

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

public:

	bool IsLit() const;

	virtual void Tick(float DeltaTime) override;
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

	/** If this person is one of the Folded and has a made model, its ID (such as "d40"). None keeps the grey-box shape. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName FigureId;

	/** Once this flag is set the person has been set: they stand still (the "Still" movement) and gain a face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName SetFlag;

	/** Turns to face the player when they come close. Off for someone with a thing fixed in front of them (a choice at their face). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	bool bTurnsToPlayer = true;

	/** Movements played now and then while waiting ("Tap", "Face"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TArray<FName> Gestures;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void Refresh() override;

	UPROPERTY(Transient)
	FUnderpaintingFigure Figure;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Face;

private:

	bool IsSet() const;
	void PlaceFigure();

	float NextGestureAt = 6.0f;
	float GestureEndsAt = -1.0f;
	float FacingYaw = 0.0f;
	bool bFacing = false;
};
