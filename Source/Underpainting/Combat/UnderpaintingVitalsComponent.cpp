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

	if (HealRemaining > 0.0f)
	{
		const float Step = FMath::Min(HealRemaining, HealPerSecond * DeltaTime);
		HealRemaining -= Step;
		Health = FMath::Min(MaxHealth, Health + Step);
	}

	TimeSinceStaminaSpend += DeltaTime;
	// Only the part of this step that falls after the pause counts, so the rule holds at any frame rate.
	const float RegenTime = FMath::Min(DeltaTime, TimeSinceStaminaSpend - StaminaRegenDelay);
	if (RegenTime > 0.0f)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * StaminaRegenMultiplier * RegenTime);
	}
}

float UUnderpaintingVitalsComponent::ApplyDamage(float Damage, float StaggerDamage, AActor* DamageCauser)
{
	if (bDead || Damage <= 0.0f)
	{
		return 0.0f;
	}
	if (bInvulnerable)
	{
		OnDodged.Broadcast(DamageCauser);
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

void UUnderpaintingVitalsComponent::SetMaxGlim(float NewMax)
{
	MaxGlim = FMath::Max(0.0f, NewMax);
	Glim = FMath::Min(Glim, MaxGlim);
}

void UUnderpaintingVitalsComponent::SetHealthAndGlim(float NewHealth, float NewGlim)
{
	Health = FMath::Clamp(NewHealth, 1.0f, MaxHealth);
	Glim = FMath::Clamp(NewGlim, 0.0f, MaxGlim);
}

void UUnderpaintingVitalsComponent::Refill()
{
	if (!bDead)
	{
		Health = MaxHealth;
		Stamina = MaxStamina;
	}
}

bool UUnderpaintingVitalsComponent::TrySpendGlim(float Amount)
{
	if (bDead || Glim < Amount)
	{
		return false;
	}
	Glim -= Amount;
	return true;
}

void UUnderpaintingVitalsComponent::StartHealOverTime(float TotalAmount, float Duration)
{
	HealRemaining = TotalAmount;
	HealPerSecond = Duration > 0.0f ? TotalAmount / Duration : TotalAmount * 1000.0f;
}

void UUnderpaintingVitalsComponent::SetMaxHealth(float NewMax)
{
	MaxHealth = FMath::Max(1.0f, NewMax);
	Health = FMath::Min(Health, MaxHealth);
}
