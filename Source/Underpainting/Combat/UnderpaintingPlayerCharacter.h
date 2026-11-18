// The Limner: movement from the template character, plus Stamina, sprint, dodge and light and heavy attacks.
// Rules and numbers: docs/GDD.md section 5. Weapon and armour numbers: data/items.json.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingCharacter.h"
#include "UnderpaintingCombatTypes.h"
#include "UnderpaintingPlayerCharacter.generated.h"

class UUnderpaintingVitalsComponent;
class UAnimMontage;

UENUM(BlueprintType)
enum class EPlayerAction : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Dodge,
	Dead
};

UCLASS(abstract)
class AUnderpaintingPlayerCharacter : public AUnderpaintingCharacter
{
	GENERATED_BODY()

public:

	AUnderpaintingPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartLightAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartHeavyAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartDodge();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetSprinting(bool bWantsSprint);

	/** Swaps the weapon for another row in data/items.json. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipWeapon(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipArmour(FName ItemId);

	UFUNCTION(BlueprintPure, Category = "Combat")
	UUnderpaintingVitalsComponent* GetVitals() const { return Vitals; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EPlayerAction GetCurrentAction() const { return Action; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EAttackPhase GetAttackPhase() const { return AttackClock.GetPhase(); }

	UFUNCTION(BlueprintPure, Category = "Combat")
	FName GetWeaponId() const { return WeaponId; }

protected:

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnderpaintingVitalsComponent> Vitals;

	/** Rows in data/items.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Equipment")
	FName WeaponId = TEXT("LimnersKnife");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Equipment")
	FName ArmourId = TEXT("SizedSmock");

	/** Swing timings for a weapon of speed 1.0; a weapon's AttackSpeed scales them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	FAttackTiming LightTiming = FAttackTiming(0.25f, 0.15f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	FAttackTiming HeavyTiming = FAttackTiming(0.50f, 0.20f, 0.60f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeStaminaCost = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeDuration = 0.6f;

	/** Seconds at the start of a dodge during which hits pass through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeInvulnerableTime = 0.30f;

	/** Centimetres travelled by one dodge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float SprintStaminaPerSecond = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float SprintSpeed = 750.0f;

	/** Movement speed while committed to a swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float AttackMoveSpeed = 120.0f;

	/** Optional animations. The rules work without them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> LightAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> HeavyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

private:

	bool CanStartAction() const;
	void StartAttack(EPlayerAction NewAction);
	void TickAttack(float DeltaTime);
	void TickDodge(float DeltaTime);
	void EndAction();
	void HitTest();

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	void RestartLevel();
	void SprintPressed() { SetSprinting(true); }
	void SprintReleased() { SetSprinting(false); }

	EPlayerAction Action = EPlayerAction::None;
	FAttackClock AttackClock;
	float DodgeElapsed = 0.0f;
	FVector DodgeDirection = FVector::ForwardVector;
	bool bWantsSprint = false;

	// Weapon numbers, read from data/items.json when equipped.
	float WeaponLightDamage = 14.0f;
	float WeaponHeavyDamage = 26.0f;
	float WeaponSpeed = 1.0f;
	float WeaponStaminaLight = 12.0f;
	float WeaponStaminaHeavy = 25.0f;
	float WeaponReach = 160.0f;
	float WeaponStaggerMultiplier = 1.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitThisSwing;

	FTimerHandle RestartTimer;
};
