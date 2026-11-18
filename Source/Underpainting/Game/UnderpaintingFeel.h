// Game feel in one place: the freeze when a blow lands (hit-stop), camera shake, paint splashes, the numbers
// that float off a hit, and sound. Gameplay code says what happened; this decides how it feels. Nothing here
// changes a rule or a number, so the game plays the same with all of it switched off.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnderpaintingFeel.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class UAudioComponent;

/** A burst of paint: a handful of dabs thrown out from a point, falling and shrinking. No particles, no new art. */
UCLASS()
class AUnderpaintingSplash : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingSplash();

	virtual void Tick(float DeltaTime) override;

	/** Direction is the way the paint is thrown (away from the blow). */
	void Burst(const FVector& Direction, const FLinearColor& Colour, int32 Count, float Speed, float Purity);

private:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Dabs;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DabMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	TArray<FVector> Velocities;
	TArray<FVector> Positions;
	TArray<float> Sizes;
	float Age = 0.0f;
	float Life = 0.7f;
};

/** A number rising off a hit. */
struct FUnderpaintingFloatingNumber
{
	FVector Location = FVector::ZeroVector;
	FString Text;
	FLinearColor Colour = FLinearColor::White;
	float Scale = 1.0f;
	float Age = 0.0f;
};

UCLASS()
class UUnderpaintingFeel : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	static UUnderpaintingFeel* Get(const UObject* WorldContext);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UUnderpaintingFeel, STATGROUP_Tickables); }
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE; }

	/** Freezes both for a moment of real time: the blow is felt before anything moves again. */
	void HitStop(AActor* First, AActor* Second, float Seconds);

	/** Adds camera shake. Trauma is 0 to 1; the shake is its square, so small knocks stay small. It fades by itself. */
	void Shake(float Trauma);

	/** What the camera should add this frame. */
	FVector GetShakeOffset() const { return ShakeOffset; }
	FRotator GetShakeRotation() const { return ShakeRotation; }

	/** Paint thrown from a point. */
	void Splash(const FVector& Location, const FVector& Direction, const FLinearColor& Colour, int32 Count = 10, float Speed = 420.0f, float Purity = 0.0f);

	/** A number rising from a point in the world (damage dealt). */
	void Number(const FVector& Location, const FString& Text, const FLinearColor& Colour, float Scale = 1.0f);
	const TArray<FUnderpaintingFloatingNumber>& GetNumbers() const { return Numbers; }

	/** Plays a row of data/sounds.json at a place in the world. Unknown names and missing files are silent. */
	void Play(FName Sound, const FVector& Location, float Volume = 1.0f, float Pitch = 1.0f);

	/** Plays a row of data/sounds.json flat, for menus and things that happen to the player. */
	void PlayFlat(FName Sound, float Volume = 1.0f, float Pitch = 1.0f);

	/** The player was hit from this direction: the HUD shows where from. */
	void NotePlayerHit(const FVector& FromLocation, float Damage);
	float GetPlayerHitAge() const;
	FVector GetPlayerHitFrom() const { return PlayerHitFrom; }
	float GetPlayerHitDamage() const { return PlayerHitDamage; }

	/** Development: draw the outline of every swing's reach. Off for play. */
	static bool bShowHitboxes;

	/** Master switches, for comparing with and without (and for anyone who dislikes shake). */
	bool bHitStop = true;
	bool bShake = true;
	float SoundVolume = 1.0f;

private:

	USoundBase* PickSound(FName Sound, float& OutVolume, float& OutPitch);

	struct FStopped
	{
		TWeakObjectPtr<AActor> Actor;
		float Remaining = 0.0f;
	};
	TArray<FStopped> Stopped;

	float Trauma = 0.0f;
	float ShakeTime = 0.0f;
	FVector ShakeOffset = FVector::ZeroVector;
	FRotator ShakeRotation = FRotator::ZeroRotator;

	TArray<FUnderpaintingFloatingNumber> Numbers;

	FVector PlayerHitFrom = FVector::ZeroVector;
	float PlayerHitDamage = 0.0f;
	float PlayerHitTime = -100.0f;

	/** Sounds already loaded, kept alive here. A null entry means "looked for and not found", so it is not looked for again. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<USoundBase>> Loaded;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Ambience;
};
