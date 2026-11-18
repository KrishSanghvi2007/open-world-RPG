// Shared pieces of the combat model: every attack, the player's and an enemy's, is wind-up, active, recovery.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EAttackPhase : uint8
{
	None,
	/** The tell. Nothing can be hit yet. */
	Windup,
	/** The hitbox is live. */
	Active,
	/** Committed and open to punishment. */
	Recovery
};

/** Durations of one attack, in seconds. */
USTRUCT(BlueprintType)
struct FAttackTiming
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float Windup = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float Active = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float Recovery = 0.4f;

	FAttackTiming() = default;
	FAttackTiming(float InWindup, float InActive, float InRecovery) : Windup(InWindup), Active(InActive), Recovery(InRecovery) {}

	/** A faster weapon (Speed above 1) shortens every part of the swing. */
	FAttackTiming Scaled(float Speed) const
	{
		const float Safe = FMath::Max(Speed, 0.1f);
		return FAttackTiming(Windup / Safe, Active / Safe, Recovery / Safe);
	}

	float Total() const { return Windup + Active + Recovery; }
};

/** Steps an attack through its phases. Plain data and arithmetic, so it can be tested without a world. */
struct FAttackClock
{
	FAttackTiming Timing;
	float Elapsed = 0.0f;
	bool bRunning = false;

	void Start(const FAttackTiming& InTiming)
	{
		Timing = InTiming;
		Elapsed = 0.0f;
		bRunning = true;
	}

	void Stop() { bRunning = false; }

	EAttackPhase GetPhase() const
	{
		if (!bRunning)
		{
			return EAttackPhase::None;
		}
		if (Elapsed < Timing.Windup)
		{
			return EAttackPhase::Windup;
		}
		if (Elapsed < Timing.Windup + Timing.Active)
		{
			return EAttackPhase::Active;
		}
		return EAttackPhase::Recovery;
	}

	/** Advances time. Returns true on the frame the attack finishes. */
	bool Advance(float DeltaTime)
	{
		if (!bRunning)
		{
			return false;
		}
		Elapsed += DeltaTime;
		if (Elapsed >= Timing.Total())
		{
			bRunning = false;
			return true;
		}
		return false;
	}

	/** 0 at the start of the current phase, 1 at its end. */
	float GetPhaseAlpha() const
	{
		switch (GetPhase())
		{
		case EAttackPhase::Windup: return Timing.Windup > 0.0f ? Elapsed / Timing.Windup : 1.0f;
		case EAttackPhase::Active: return Timing.Active > 0.0f ? (Elapsed - Timing.Windup) / Timing.Active : 1.0f;
		case EAttackPhase::Recovery: return Timing.Recovery > 0.0f ? (Elapsed - Timing.Windup - Timing.Active) / Timing.Recovery : 1.0f;
		default: return 0.0f;
		}
	}
};
