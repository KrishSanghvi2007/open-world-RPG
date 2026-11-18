#include "UnderpaintingBestiary.h"
#include "UnderpaintingFeel.h"

#include "Underpainting.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	int32 FindAttack(const FEnemyRow& Row, const TCHAR* Name)
	{
		return Row.Attacks.IndexOfByPredicate([Name](const FEnemyAttack& Attack) { return Attack.AttackName == FName(Name); });
	}

	bool Is(const FEnemyAttack& Attack, const TCHAR* Name)
	{
		return Attack.AttackName == FName(Name);
	}
}

// --- Madderback ---

AMadderbackEnemy::AMadderbackEnemy()
{
	EnemyId = TEXT("Madderback");
	GetCapsuleComponent()->InitCapsuleSize(70.0f, 70.0f);
}

float AMadderbackEnemy::GetAttackReach(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("BloatBurst")) ? 300.0f : 200.0f;
}

float AMadderbackEnemy::GetAttackLunge(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("GoreCharge")) ? 800.0f : 0.0f;
}

bool AMadderbackEnemy::IsAreaAttack(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("BloatBurst"));
}

int32 AMadderbackEnemy::ChooseAttack(float DistanceToTarget) const
{
	// Cornered and low, it bursts. From a distance it charges. Up close it jabs.
	if (GetVitals()->GetHealthFraction() < 0.3f && DistanceToTarget <= 280.0f)
	{
		return FindAttack(Row, TEXT("BloatBurst"));
	}
	if (DistanceToTarget > 380.0f && DistanceToTarget <= 850.0f)
	{
		return FindAttack(Row, TEXT("GoreCharge"));
	}
	if (DistanceToTarget <= 190.0f)
	{
		return FindAttack(Row, TEXT("ProboscisJab"));
	}
	return INDEX_NONE;
}

void AMadderbackEnemy::OnAttackLanded(const FEnemyAttack& Attack, AActor* Target)
{
	if (Is(Attack, TEXT("ProboscisJab")))
	{
		Vitals->Heal(10.0f);     // it drinks
	}
}

void AMadderbackEnemy::OnAttackFinished(const FEnemyAttack& Attack, float DistanceTravelled)
{
	// A charge that was stopped short hit something solid: it is stuck, and wide open.
	if (Is(Attack, TEXT("GoreCharge")) && DistanceTravelled < GetAttackLunge(Attack) * 0.7f)
	{
		Reel(StuckTime);
		UE_LOG(LogUnderpainting, Log, TEXT("%s is stuck after its charge"), *GetName());
	}
}

// --- Pentiment ---

APentimentEnemy::APentimentEnemy()
{
	EnemyId = TEXT("Pentiment");
}

float APentimentEnemy::GetAttackReach(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("ContourSweep")) ? 250.0f : 300.0f;
}

bool APentimentEnemy::IsAreaAttack(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("ContourSweep"));
}

int32 APentimentEnemy::ChooseAttack(float DistanceToTarget) const
{
	// Patient. It redraws itself across a gap, thrusts at spear's length, and sweeps if you get round it.
	const bool bShy = GetWorld() && GetWorld()->GetTimeSeconds() < RedrawShyUntil;
	if (DistanceToTarget > 350.0f && DistanceToTarget <= 900.0f && !bShy)
	{
		return FindAttack(Row, TEXT("Redraw"));
	}
	if (DistanceToTarget <= 200.0f)
	{
		return FindAttack(Row, TEXT("ContourSweep"));
	}
	if (DistanceToTarget <= 300.0f)
	{
		return FindAttack(Row, TEXT("DrawnThrust"));
	}
	return INDEX_NONE;
}

bool APentimentEnemy::OnWindupTick(const FEnemyAttack& Attack, AActor* Target)
{
	// It is not fixed, so light unsettles it: a Flare during the fade cancels the Redraw and staggers it.
	if (Is(Attack, TEXT("Redraw")))
	{
		const AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(Target);
		if (Player && Player->IsFlareLighting(GetActorLocation()))
		{
			UE_LOG(LogUnderpainting, Log, TEXT("%s was caught in the light mid-redraw"), *GetName());
			RedrawShyUntil = GetWorld()->GetTimeSeconds() + RedrawShyTime;
			return false;
		}
	}
	return true;
}

void APentimentEnemy::OnAttackActive(const FEnemyAttack& Attack, AActor* Target)
{
	// Redraw: it reappears up to 6 m away, at spear's length from its target, and strikes.
	if (Is(Attack, TEXT("Redraw")) && Target)
	{
		FVector Away = GetActorLocation() - Target->GetActorLocation();
		Away.Z = 0.0f;
		Away = Away.GetSafeNormal();
		const FVector Wanted = Target->GetActorLocation() + Away.RotateAngleAxis(FMath::RandRange(-70.0f, 70.0f), FVector::UpVector) * 240.0f;
		const FVector Step = (Wanted - GetActorLocation()).GetClampedToMaxSize2D(600.0f);
		FVector Destination = GetActorLocation() + Step;
		Destination.Z = GetActorLocation().Z;
		SetActorLocation(Destination, true);
		FVector Facing = Target->GetActorLocation() - GetActorLocation();
		Facing.Z = 0.0f;
		SetActorRotation(Facing.Rotation());
	}
}

// --- Old Palimpsest ---

AOldPalimpsest::AOldPalimpsest()
{
	EnemyId = TEXT("ThePalimpsest");
	StaggerImmunityTime = 8.0f;       // a stagger is a reward, not a lock
	GetCapsuleComponent()->InitCapsuleSize(110.0f, 210.0f);
}

FLinearColor AOldPalimpsest::GetBodyColour() const
{
	// White, then madder red: the coat under the cold (docs/STORY.md, Act 1).
	return GetPhase() == 1 ? FLinearColor::White : FLinearColor(0.34f, 0.03f, 0.02f);
}

float AOldPalimpsest::GetAttackReach(const FEnemyAttack& Attack) const
{
	if (Is(Attack, TEXT("LongWipe"))) { return 500.0f; }
	if (Is(Attack, TEXT("BlottingSlam"))) { return 400.0f; }
	if (Is(Attack, TEXT("ScrapeLine"))) { return 1500.0f; }
	if (Is(Attack, TEXT("Unmake"))) { return 280.0f; }
	return 320.0f;
}

bool AOldPalimpsest::IsAreaAttack(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("BlottingSlam"));
}

int32 AOldPalimpsest::GetHitCount(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("HeartbloodFlail")) ? 3 : 1;
}

int32 AOldPalimpsest::ChooseAttack(float DistanceToTarget) const
{
	// Far away it sends the line. Close in it alternates, so the fight has a rhythm that can be learned.
	if (DistanceToTarget > 600.0f && DistanceToTarget <= 1500.0f)
	{
		return FindAttack(Row, TEXT("ScrapeLine"));
	}
	if (DistanceToTarget > 480.0f)
	{
		return INDEX_NONE;
	}

	const int32 Turn = AttackCounter++;
	if (GetPhase() == 2)
	{
		switch (Turn % 4)
		{
		case 0: return FindAttack(Row, TEXT("HeartbloodFlail"));
		case 1: return FindAttack(Row, TEXT("LongWipe"));
		case 2: return DistanceToTarget <= 260.0f ? FindAttack(Row, TEXT("Unmake")) : FindAttack(Row, TEXT("BlottingSlam"));
		default: return FindAttack(Row, TEXT("BlottingSlam"));
		}
	}
	return Turn % 2 == 0 ? FindAttack(Row, TEXT("LongWipe")) : (DistanceToTarget <= 380.0f ? FindAttack(Row, TEXT("BlottingSlam")) : FindAttack(Row, TEXT("LongWipe")));
}

void AOldPalimpsest::OnAttackLanded(const FEnemyAttack& Attack, AActor* Target)
{
	if (Is(Attack, TEXT("Unmake")) && Target)
	{
		if (UUnderpaintingVitalsComponent* TargetVitals = Target->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			TargetVitals->AddGlim(-30.0f);
		}
	}
}

void AOldPalimpsest::OnPhaseTwo()
{
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->OnNotice.Broadcast(TEXT("It scrapes itself raw. Under the white: red."));
	}
}

void AOldPalimpsest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// The heart beats, and for a few seconds the arena is half-lit: Strokes are half price on the beat.
	if (GetState() == EEnemyState::Idle || GetState() == EEnemyState::Dead)
	{
		return;
	}
	HeartbeatTimer += DeltaTime;
	if (HeartbeatTimer >= HeartbeatInterval)
	{
		HeartbeatTimer = 0.0f;
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->PlayFlat(TEXT("heartbeat"));
			Feel->Shake(0.12f);
		}
		if (AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Player->LendLight(HeartbeatLight, HeartbeatLightSeconds);
		}
		if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
		{
			Progress->OnNotice.Broadcast(TEXT("Her heart beats. The room warms."));
		}
	}
}
