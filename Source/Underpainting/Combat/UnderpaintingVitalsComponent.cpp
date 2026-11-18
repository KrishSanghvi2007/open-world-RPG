#include "UnderpaintingVitalsComponent.h"

UUnderpaintingVitalsComponent::UUnderpaintingVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUnderpaintingVitalsComponent::Initialise(float InMaxHealth, float InMaxStamina, float InMaxGlim)
{
	MaxHealth = InMaxHealth;
	MaxStamina = InMaxStamina;
	MaxGlim = InMaxGlim;
	Health = MaxHealth;
	Stamina = MaxStamina;
	Glim = MaxGlim;
	bDead = false;
}

void UUnderpaintingVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceRegeneration(DeltaTime);
}

void UUnderpaintingVitalsComponent::AdvanceRegeneration(float DeltaTime)
{
	if (bDead)
	{
		return;
	}

	TimeSinceStaminaSpend += DeltaTime;
	// Only the part of this step that falls after the pause counts, so the rule holds at any frame rate.
	const float RegenTime = FMath::Min(DeltaTime, TimeSinceStaminaSpend - StaminaRegenDelay);
	if (RegenTime > 0.0f)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * RegenTime);
	}
}

float UUnderpaintingVitalsComponent::ApplyDamage(float Damage, float StaggerDamage, AActor* DamageCauser)
{
	if (bDead || bInvulnerable || Damage <= 0.0f)
	{
		return 0.0f;
	}

	const float Taken = Damage * (1.0f - FMath::Clamp(DamageReduction, 0.0f, 1.0f));
	Health = FMath::Max(0.0f, Health - Taken);
	OnDamaged.Broadcast(Taken, StaggerDamage, DamageCauser);

	if (Health <= 0.0f)
	{
		bDead = true;
		OnDeath.Broadcast(DamageCauser);
	}

	return Taken;
}

void UUnderpaintingVitalsComponent::Heal(float Amount)
{
	if (!bDead)
	{
		Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	}
}

bool UUnderpaintingVitalsComponent::TrySpendStamina(float Amount)
{
	if (bDead || Stamina < Amount)
	{
		return false;
	}

	Stamina -= Amount;
	TimeSinceStaminaSpend = 0.0f;
	return true;
}

bool UUnderpaintingVitalsComponent::DrainStamina(float Amount)
{
	if (bDead || Stamina <= 0.0f)
	{
		return false;
	}

	Stamina = FMath::Max(0.0f, Stamina - Amount);
	TimeSinceStaminaSpend = 0.0f;
	return Stamina > 0.0f;
}

void UUnderpaintingVitalsComponent::AddGlim(float Amount)
{
	Glim = FMath::Clamp(Glim + Amount, 0.0f, MaxGlim);
}
