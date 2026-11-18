// Health, Stamina and Glim for anything that can fight. Numbers and rules come from docs/GDD.md section 5.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnderpaintingVitalsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVitalsDamaged, float, Damage, float, StaggerDamage, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalsDeath, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalsDodged, AActor*, Attacker);

UCLASS(ClassGroup = (Underpainting), meta = (BlueprintSpawnableComponent))
class UUnderpaintingVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UUnderpaintingVitalsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sets the maximums and fills everything up. */
	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void Initialise(float InMaxHealth, float InMaxStamina, float InMaxGlim);

	/**
	 * Applies a hit. Ignored while invulnerable or dead.
	 * @param Damage			Damage before armour.
	 * @param StaggerDamage		Amount that builds toward the target's Poise.
	 * @return Damage actually taken.
	 */
	UFUNCTION(BlueprintCallable, Category = "Vitals")
	float ApplyDamage(float Damage, float StaggerDamage, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void Heal(float Amount);

	/** Spends Stamina if there is enough, and restarts the regeneration pause. */
	UFUNCTION(BlueprintCallable, Category = "Vitals")
	bool TrySpendStamina(float Amount);

	/** Drains Stamina over time (sprinting). Returns false once it runs out. */
	bool DrainStamina(float Amount);

	/** Spends Glim if the lantern holds enough. */
	bool TrySpendGlim(float Amount);

	/** Adds or removes Glim, clamped to 0..Max. */
	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void AddGlim(float Amount);

	/** Changes the Glim capacity (attunement), keeping what is in the lantern. */
	void SetMaxGlim(float NewMax);

	/** Restores saved values after a load. */
	void SetHealthAndGlim(float NewHealth, float NewGlim);

	/** Heals this much in total, spread evenly over the time (a Moss Poultice: 40 over 4 s). */
	void StartHealOverTime(float TotalAmount, float Duration);

	/** Changes maximum Health, keeping current Health within it. */
	void SetMaxHealth(float NewMax);

	void AddStamina(float Amount) { if (!bDead) { Stamina = FMath::Clamp(Stamina + Amount, 0.0f, MaxStamina); } }

	/** Full Health and Stamina (resting at a Lampstand). */
	void Refill();

	UFUNCTION(BlueprintPure, Category = "Vitals")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetGlim() const { return Glim; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetHealthFraction() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetStaminaFraction() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetGlimFraction() const { return MaxGlim > 0.0f ? Glim / MaxGlim : 0.0f; }

	/** Advances Stamina regeneration. Split out of the tick so it can be tested without a world. */
	void AdvanceRegeneration(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitals")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitals")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitals")
	float MaxGlim = 100.0f;

	/** Stamina per second once regeneration has started. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitals")
	float StaminaRegenPerSecond = 30.0f;

	/** Multiplier on Stamina regeneration from attunement (1.2 = +20%). */
	UPROPERTY(BlueprintReadWrite, Category = "Vitals")
	float StaminaRegenMultiplier = 1.0f;

	/** Seconds after the last spend before Stamina starts to return. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitals")
	float StaminaRegenDelay = 0.8f;

	/** Fraction of incoming damage removed by armour, 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	float DamageReduction = 0.0f;

	/** While true, hits pass through (the first part of a dodge). */
	UPROPERTY(BlueprintReadWrite, Category = "Vitals")
	bool bInvulnerable = false;

	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FOnVitalsDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FOnVitalsDeath OnDeath;

	/** A hit arrived while invulnerable: the dodge worked. */
	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FOnVitalsDodged OnDodged;

private:

	float Health = 100.0f;
	float Stamina = 100.0f;
	float Glim = 100.0f;
	float TimeSinceStaminaSpend = 100.0f;
	float HealPerSecond = 0.0f;
	float HealRemaining = 0.0f;
	bool bDead = false;
};
