// An enemy driven by a row of data/enemies.json: health, poise, speed, senses and attack timings.
// Behaviour follows the enemy design principles in docs/GDD.md section 5: a visible wind-up before every hit
// and a recovery at least as long, so every dodge earns an opening.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UnderpaintingCombatTypes.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingEnemy.generated.h"

class UUnderpaintingVitalsComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle,
	Chasing,
	Attacking,
	Staggered,
	Returning,
	Dead
};

UCLASS()
class AUnderpaintingEnemy : public ACharacter
{
	GENERATED_BODY()

public:

	AUnderpaintingEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EAttackPhase GetAttackPhase() const { return AttackClock.GetPhase(); }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UUnderpaintingVitalsComponent* GetVitals() const { return Vitals; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FString GetDisplayName() const { return Row.DisplayName; }

	/** Seconds until the attack being wound up goes live, or -1 if none is. */
	float GetTimeUntilHit() const { return State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Windup ? AttackClock.Timing.Windup - AttackClock.Elapsed : -1.0f; }

	/** Name of the attack in progress, or None. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	FName GetCurrentAttackName() const;

	/** Stagger damage taken so far, as a fraction of Poise. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetPoiseFraction() const { return Row.Poise > 0 ? PoiseDamage / Row.Poise : 0.0f; }

	const FEnemyRow& GetRow() const { return Row; }

	/** 1, or 2 once a two-phase enemy has passed its threshold. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetPhase() const { return Phase; }

	/**
	 * The pigments that count as this enemy's weakness and resistance right now. A two-phase enemy lists one
	 * of each per phase, in order, because its hue changes (data/README.md, the boss rule).
	 */
	TArray<FName> GetActiveWeaknesses() const;
	TArray<FName> GetActiveResistances() const;

	/** Holds the enemy still and helpless for a time (stuck in a tree, reeling). */
	void Reel(float Seconds) { ReelRemaining = FMath::Max(ReelRemaining, Seconds); }

	/** Forces a stagger now (a Flare on a Pentiment). */
	void ForceStagger();

	/** Enemies attacking at this moment, across the level. Never more than MaxAttackers start a new attack. */
	static int32 CountAttackers(const UObject* WorldContext);
	static constexpr int32 MaxAttackers = 2;

	/**
	 * Enemies in a group take turns to begin: no attack starts within this long of another's start, so two blows
	 * never land in the same instant and each one can be read and dodged. (A boss fights alone; this never holds it.)
	 */
	static constexpr float AttackStagger = 0.7f;

	/** Seconds since any enemy in the level last began an attack. */
	static float SecondsSinceAnyAttackBegan(const UObject* WorldContext);

	float AttackBeganAt = -100.0f;

	/** Slows the enemy (0.6 = 40% slower) for a time. */
	void ApplySlow(float SpeedMultiplier, float Duration);

	/** The Glaze mark: the enemy takes this much more damage (0.15 = +15%) for a time. */
	void ApplyVulnerable(float ExtraDamageFraction, float Duration);

	/** Erase: an attack that is winding up or landing is wiped out and the enemy reels briefly. Returns true if there was one. */
	bool CancelAttack(float ReelSeconds = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsGlazed() const { return VulnerableRemaining > 0.0f; }

	/** Extra damage fraction from being glazed, if any. */
	float GetVulnerability() const { return VulnerableRemaining > 0.0f ? VulnerableFraction : 0.0f; }

	/** A blow landed from there: flinch away from it, flash, and (unless too heavy or mid-strike) be knocked back. */
	void ReceiveImpact(const FVector& From, float Knockback);

	/** The colour of the paint a blow knocks off it. */
	FLinearColor GetPaintColour() const { return GetBodyColour(); }
	float GetPaintPurity() const { return GetBodyPurity(); }

	/** Row in data/enemies.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName EnemyId = TEXT("Blotter");

	/** If set, this enemy is not here once the flag is set (a quest thinned them out). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName RemovedByFlag;

	/** If set, this enemy is only here once the flag is set (a quest brought more of them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName RequiresFlag;

protected:

	/** How far (cm) an attack reaches. The data has timings and damage; reach is behaviour, so it lives in code. */
	virtual float GetAttackReach(const FEnemyAttack& Attack) const { return 200.0f; }

	/** Distance (cm) the enemy travels forward while the attack is live. 0 for a standing attack. */
	virtual float GetAttackLunge(const FEnemyAttack& Attack) const { return 0.0f; }

	/** True for attacks that hit everything around the enemy (a burst, a full sweep) instead of in front of it. */
	virtual bool IsAreaAttack(const FEnemyAttack& Attack) const { return false; }

	/** How many separate hits the attack's active window holds (a flail of three is 3). */
	virtual int32 GetHitCount(const FEnemyAttack& Attack) const { return 1; }

	/** Seconds between the hits of a multi-hit attack. */
	virtual float GetHitInterval(const FEnemyAttack& Attack) const { return 0.4f; }

	/** Called when the wind-up ends and the attack goes live (teleports happen here). */
	virtual void OnAttackActive(const FEnemyAttack& Attack, AActor* Target) {}

	/** Called every frame of a wind-up. Return false to abandon the attack (the enemy is then staggered). */
	virtual bool OnWindupTick(const FEnemyAttack& Attack, AActor* Target) { return true; }

	/** Called when the attack is over, with how far the enemy travelled while it was live. */
	virtual void OnAttackFinished(const FEnemyAttack& Attack, float DistanceTravelled) {}

	/** Called when a two-phase enemy enters its second phase. */
	virtual void OnPhaseTwo() {}

	/** How far a blow moves it: 1 for something person-sized, less for something heavy. Bosses are never moved. */
	virtual float GetKnockbackScale() const { return 1.0f; }

	/** Size of the placeholder body relative to a person. */
	virtual FVector GetBodyScale() const { return FVector(0.7f, 0.7f, 2.2f); }

	/** Index into the row's Attacks to use at this distance (cm), or INDEX_NONE to keep closing in. */
	virtual int32 ChooseAttack(float DistanceToTarget) const;

	/** How close it walks before it stops and waits for its next attack (cm). */
	virtual float GetApproachDistance() const { return 140.0f; }

	/** Called when an attack lands, after damage. */
	virtual void OnAttackLanded(const FEnemyAttack& Attack, AActor* Target) {}

	/** 1 keeps the body's colour flat and exact (Erasures); 0 paints it in three values like everything else. */
	virtual float GetBodyPurity() const { return 0.0f; }

	/** Colour of the placeholder body. */
	virtual FLinearColor GetBodyColour() const { return FLinearColor(0.9f, 0.9f, 0.9f); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnderpaintingVitalsComponent> Vitals;

	/** Grey-box stand-in for the creature until it has a model. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	/** The warm rim telegraph: lit during a wind-up. A small light, so it casts no shadow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> TelegraphLight;

	/** The mark on the ground that shows where an attack will land. It grows through the wind-up and is full when the hit comes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TelegraphGround;

	/** Seconds a staggered enemy is helpless. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float StaggerDuration = 2.0f;

	/** Seconds without being hit before built-up stagger is forgotten. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float PoiseResetTime = 5.0f;

	/** Pause between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float AttackCooldown = 0.6f;

	/** The enemy stops turning to follow its target this long before the hit, so a late dodge works. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float AimLockTime = 0.2f;

	/** Seconds after a stagger ends during which no new stagger can build. Bosses use this so they cannot be kept down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float StaggerImmunityTime = 0.0f;

	/** Seconds a two-phase enemy spends changing, wide open (it takes +50% damage). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float PhaseChangeTime = 2.5f;

	FEnemyRow Row;

private:

	UFUNCTION()
	void HandleDamaged(float Damage, float StaggerDamage, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	AActor* FindTarget() const;
	void FaceTowards(const FVector& Location, float DeltaTime, float DegreesPerSecond = 540.0f);
	void MoveTowards(const FVector& Location);
	void BeginAttack(int32 AttackIndex);
	void TickAttack(float DeltaTime, AActor* Target);
	void HitTest(const FEnemyAttack& Attack);
	void SetState(EEnemyState NewState);
	void UpdateTelegraph();
	void UpdateBody();

	EEnemyState State = EEnemyState::Idle;
	FAttackClock AttackClock;
	int32 CurrentAttack = INDEX_NONE;
	bool bAttackLanded = false;
	float SlowRemaining = 0.0f;
	float SlowMultiplier = 1.0f;
	float VulnerableRemaining = 0.0f;
	float VulnerableFraction = 0.0f;
	float ReelRemaining = 0.0f;
	int32 Phase = 1;
	float StaggerImmunityRemaining = 0.0f;
	float PhaseChangeRemaining = 0.0f;
	int32 HitsLanded = 0;
	FVector AttackStartLocation = FVector::ZeroVector;
	float StateTime = 0.0f;

	// Walking round things: which way it last turned to get past an obstacle, and until when it keeps to that side.
	float AvoidSign = 1.0f;
	float AvoidUntil = -1.0f;
	float PoiseDamage = 0.0f;
	float TimeSinceHit = 0.0f;
	float CooldownRemaining = 0.0f;
	FVector HomeLocation = FVector::ZeroVector;

	// How it moves and reacts (a stand-in for animation until it has a model).
	float FlinchRemaining = 0.0f;
	FVector FlinchLocal = FVector::ZeroVector;
	float FlashRemaining = 0.0f;
	float BreathOffset = 0.0f;
	bool bSwingSounded = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DiscMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BarMesh;
};
