// The arithmetic of combat, kept apart from actors so it can be tested: the Contrast rule and the price of light.
// Rules: docs/GDD.md sections 4.5 and 5.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingDataTypes.h"

namespace UnderpaintingCombatRules
{
	/** Result of the Contrast rule for one Stroke against one enemy. */
	struct FContrast
	{
		float DamageMultiplier = 1.0f;
		float StaggerMultiplier = 1.0f;
		/** +1 complementary (strong), -1 the enemy's own hue ("muddy"), 0 neither. */
		int32 Sign = 0;
	};

	/**
	 * A Stroke of the complementary pigment does +30% damage and +50% stagger. A Stroke of the enemy's own hue
	 * does -30%. The enemy's weaknesses and resistances lists carry this (boss phases narrow them elsewhere).
	 */
	inline FContrast Contrast(FName StrokePigment, const TArray<FName>& Weaknesses, const TArray<FName>& Resistances)
	{
		FContrast Result;
		if (StrokePigment.IsNone())
		{
			return Result;
		}
		if (Weaknesses.Contains(StrokePigment))
		{
			Result.DamageMultiplier = 1.3f;
			Result.StaggerMultiplier = 1.5f;
			Result.Sign = 1;
		}
		else if (Resistances.Contains(StrokePigment))
		{
			Result.DamageMultiplier = 0.7f;
			Result.Sign = -1;
		}
		return Result;
	}

	/** Glim a Stroke costs where you stand: the pigment's light cost x (1 - ambient light), rounded up. Free in full light. */
	inline int32 StrokeGlimCost(int32 LightCost, float AmbientLight)
	{
		return FMath::CeilToInt(LightCost * (1.0f - FMath::Clamp(AmbientLight, 0.0f, 1.0f)) - KINDA_SMALL_NUMBER);
	}

	/** Light where the player stands: the zone's ambient light, or at least 0.5 inside a Flare. */
	inline float EffectiveLight(float ZoneAmbient, bool bInsideFlare)
	{
		return bInsideFlare ? FMath::Max(ZoneAmbient, 0.5f) : ZoneAmbient;
	}
}
