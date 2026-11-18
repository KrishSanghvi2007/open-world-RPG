#include "BlotterEnemy.h"

#include "UnderpaintingVitalsComponent.h"

namespace
{
	const FName NAME_Dab(TEXT("Dab"));
	const FName NAME_Wipe(TEXT("Wipe"));
	const FName NAME_SmearLunge(TEXT("SmearLunge"));
}

ABlotterEnemy::ABlotterEnemy()
{
	EnemyId = TEXT("Blotter");
}

float ABlotterEnemy::GetAttackReach(const FEnemyAttack& Attack) const
{
	if (Attack.AttackName == NAME_Wipe)
	{
		return 250.0f;
	}
	return 190.0f;
}

float ABlotterEnemy::GetAttackLunge(const FEnemyAttack& Attack) const
{
	// The lunge covers 5 m in total: most of it is travel.
	return Attack.AttackName == NAME_SmearLunge ? 330.0f : 0.0f;
}

int32 ABlotterEnemy::ChooseAttack(float DistanceToTarget) const
{
	const auto Find = [this](FName Name)
	{
		return Row.Attacks.IndexOfByPredicate([Name](const FEnemyAttack& Attack) { return Attack.AttackName == Name; });
	};

	// Far: lunge. Close: mostly the poke, sometimes the sweep. In between it keeps walking in.
	if (DistanceToTarget > 320.0f && DistanceToTarget <= 520.0f)
	{
		return Find(NAME_SmearLunge);
	}
	if (DistanceToTarget <= 180.0f)
	{
		return FMath::FRand() < 0.65f ? Find(NAME_Dab) : Find(NAME_Wipe);
	}
	if (DistanceToTarget <= 240.0f)
	{
		return Find(NAME_Wipe);
	}
	return INDEX_NONE;
}

void ABlotterEnemy::OnAttackLanded(const FEnemyAttack& Attack, AActor* Target)
{
	if (Attack.AttackName == NAME_SmearLunge && Target)
	{
		if (UUnderpaintingVitalsComponent* TargetVitals = Target->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			TargetVitals->AddGlim(-LungeGlimDrain);
		}
	}
}
