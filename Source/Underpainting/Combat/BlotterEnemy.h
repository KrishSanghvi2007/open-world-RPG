// The Blotter: a tall, colourless Erasure that dabs the world away. It teaches dodging.
// Attacks (data/enemies.json): Dab, a poke; Wipe, a wide sweep; SmearLunge, a 5 m lunge that also drains Glim.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingEnemy.h"
#include "BlotterEnemy.generated.h"

UCLASS()
class ABlotterEnemy : public AUnderpaintingEnemy
{
	GENERATED_BODY()

public:

	ABlotterEnemy();

protected:

	virtual float GetAttackReach(const FEnemyAttack& Attack) const override;
	virtual float GetAttackLunge(const FEnemyAttack& Attack) const override;
	virtual int32 ChooseAttack(float DistanceToTarget) const override;
	virtual void OnAttackLanded(const FEnemyAttack& Attack, AActor* Target) override;

	/** Pure white belongs to Erasures (GDD art rule 4). */
	virtual FLinearColor GetBodyColour() const override { return FLinearColor::White; }
	virtual float GetBodyPurity() const override { return 1.0f; }

	/** Glim taken from the target when SmearLunge lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float LungeGlimDrain = 10.0f;
};
