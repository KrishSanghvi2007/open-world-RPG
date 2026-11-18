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

	/** Name of the attack in progress, or None. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	FName GetCurrentAttackName() const;

	/** Stagger damage taken so far, as a fraction of Poise. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetPoiseFraction() const { return Row.Poise > 0 ? PoiseDamage / Row.Poise : 0.0f; }

	/** Row in data/enemies.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName EnemyId = TEXT("Blotter");

protected:

	/** How far (cm) an attack reaches. The data has timings and damage; reach is behaviour, so it lives in code. */
	virtual float GetAttackReach(const FEnemyAttack& Attack) const { return 200.0f; }

	/** Distance (cm) the enemy travels forward while the attack is live. 0 for a standing attack. */
	virtual float GetAttackLunge(const FEnemyAttack& Attack) const { return 0.0f; }

	/** Index into the row's Attacks to use at this distance (cm), or INDEX_NONE to keep closing in. */
	virtual int32 ChooseAttack(float DistanceToTarget) const;

	/** Called when an attack lands, after damage. */
	virtual void OnAttackLanded(const FEnemyAttack& Attack, AActor* Target) {}

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

	EEnemyState State = EEnemyState::Idle;
	FAttackClock AttackClock;
	int32 CurrentAttack = INDEX_NONE;
	bool bAttackLanded = false;
	float StateTime = 0.0f;
	float PoiseDamage = 0.0f;
	float TimeSinceHit = 0.0f;
	float CooldownRemaining = 0.0f;
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
};
