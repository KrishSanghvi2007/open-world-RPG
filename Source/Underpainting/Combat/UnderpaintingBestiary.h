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
	virtual float GetKnockbackScale() const override { return 0.5f; }        // a tick the size of a cart
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

/**
 * The Hatcher: the great striped spider of the Folded (drawing d28, docs/story/CAST_IN_GAME.md). Teaches looking
 * before you walk. Her web is strung round where she hangs and cannot be seen in half-light; a Flare shows every
 * strand. Touch one and you are held, and she comes down her thread. Leave the web alone and she never attacks.
 */
UCLASS()
class AHatcherEnemy : public AUnderpaintingEnemy
{
	GENERATED_BODY()

public:

	AHatcherEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** True while she hangs on her thread, out of reach. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsHanging() const { return Hang > 0.3f; }

	virtual bool IsThreat() const override { return !IsHanging(); }

	/** Metres from her to the nearest point of her web, for the teaching prompt. */
	float GetWebRadius() const { return WebRadius; }

protected:

	virtual FName GetFigureId() const override { return TEXT("d28"); }
	virtual float GetAttackReach(const FEnemyAttack& Attack) const override;
	virtual float GetAttackLunge(const FEnemyAttack& Attack) const override;
	virtual int32 ChooseAttack(float DistanceToTarget) const override;
	virtual float GetApproachDistance() const override { return 170.0f; }
	virtual float GetKnockbackScale() const override { return 0.7f; }
	virtual FLinearColor GetBodyColour() const override { return FLinearColor(0.02f, 0.016f, 0.014f); }
	virtual float GetBodyPurity() const override { return 1.0f; }
	virtual FVector GetBodyScale() const override { return FVector(1.6f, 1.6f, 0.9f); }
	virtual void UpdateFigure(float DeltaTime) override;

	/** How high she hangs (cm), and how long she takes to come down (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float HangHeight = 480.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float DescendTime = 1.1f;

	/** The web: spokes out to WebRadius, and a ring at RingFraction of it with one gap, the way in, on the side she faces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	int32 Spokes = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float WebRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float RingFraction = 0.62f;

	/** How close (cm) to a strand counts as touching it, and how long (s) a touch holds you. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float TouchDistance = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Web")
	float HoldSeconds = 2.2f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Thread;

private:

	void BuildWeb();
	void TickWeb(float DeltaTime);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> StrandMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SilkMaterial;

	TArray<FVector> StrandStart;
	TArray<FVector> StrandEnd;
	float ShowAllUntil = -1.0f;
	float NextHoldAllowed = 0.0f;
	float Hang = 1.0f;
	float HalfHeight = 60.0f;
};

