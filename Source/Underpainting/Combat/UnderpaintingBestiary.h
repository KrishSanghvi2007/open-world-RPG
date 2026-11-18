// The slice's other enemies. Numbers come from data/enemies.json; what each one does with them is here.
// Every enemy teaches one thing (docs/GDD.md section 5).

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingEnemy.h"
#include "UnderpaintingBestiary.generated.h"

/** The Madderback: a boar-sized blood-tick. Teaches positioning: its charge sticks in whatever it hits. */
UCLASS()
class AMadderbackEnemy : public AUnderpaintingEnemy
{
	GENERATED_BODY()

public:

	AMadderbackEnemy();

protected:

	virtual float GetAttackReach(const FEnemyAttack& Attack) const override;
	virtual float GetAttackLunge(const FEnemyAttack& Attack) const override;
	virtual bool IsAreaAttack(const FEnemyAttack& Attack) const override;
	virtual int32 ChooseAttack(float DistanceToTarget) const override;
	virtual void OnAttackLanded(const FEnemyAttack& Attack, AActor* Target) override;
	virtual void OnAttackFinished(const FEnemyAttack& Attack, float DistanceTravelled) override;
	virtual FLinearColor GetBodyColour() const override { return FLinearColor(0.34f, 0.03f, 0.02f); }
	virtual FVector GetBodyScale() const override { return FVector(1.5f, 1.5f, 1.1f); }

	/** Seconds stuck after charging into something solid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Tuning")
	float StuckTime = 2.0f;
};

/** The Pentiment: a discarded draft of a person. Teaches using light: a Flare undoes its Redraw. */
UCLASS()
class APentimentEnemy : public AUnderpaintingEnemy
{
	GENERATED_BODY()

public:

	APentimentEnemy();

protected:

	virtual float GetAttackReach(const FEnemyAttack& Attack) const override;
	virtual bool IsAreaAttack(const FEnemyAttack& Attack) const override;
	virtual int32 ChooseAttack(float DistanceToTarget) const override;
	virtual bool OnWindupTick(const FEnemyAttack& Attack, AActor* Target) override;
	virtual void OnAttackActive(const FEnemyAttack& Attack, AActor* Target) override;
	/** It keeps you at spear's length; step inside that and it sweeps. */
	virtual float GetApproachDistance() const override { return 250.0f; }
	virtual FLinearColor GetBodyColour() const override { return FLinearColor(0.03f, 0.07f, 0.38f); }

	/** After a Flare catches it mid-Redraw it does not try again for this long, so one Flare is one opening, not eight seconds of them. */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	float RedrawShyTime = 8.0f;

private:

	float RedrawShyUntil = -1.0f;

protected:

	virtual FVector GetBodyScale() const override { return FVector(0.45f, 0.45f, 2.0f); }
};

/**
 * Old Palimpsest, the boss. Phase 1 is white (weak to Bone Black). At half Health it scrapes itself down to
 * red (weak to Terre Verte), which reverses the lesson. The heart beside it beats every 20 seconds and lights
 * the arena for 4, making Strokes cheaper on the beat.
 */
UCLASS()
class AOldPalimpsest : public AUnderpaintingEnemy
{
	GENERATED_BODY()

public:

	AOldPalimpsest();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual float GetAttackReach(const FEnemyAttack& Attack) const override;
	virtual bool IsAreaAttack(const FEnemyAttack& Attack) const override;
	virtual int32 GetHitCount(const FEnemyAttack& Attack) const override;
	virtual int32 ChooseAttack(float DistanceToTarget) const override;
	virtual void OnAttackLanded(const FEnemyAttack& Attack, AActor* Target) override;
	virtual void OnPhaseTwo() override;
	virtual FLinearColor GetBodyColour() const override;
	virtual float GetBodyPurity() const override { return GetPhase() == 1 ? 1.0f : 0.0f; }
	virtual FVector GetBodyScale() const override { return FVector(2.2f, 2.2f, 4.2f); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Heart")
	float HeartbeatInterval = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Heart")
	float HeartbeatLightSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Heart")
	float HeartbeatLight = 0.5f;

private:

	float HeartbeatTimer = 0.0f;
	mutable int32 AttackCounter = 0;
};
