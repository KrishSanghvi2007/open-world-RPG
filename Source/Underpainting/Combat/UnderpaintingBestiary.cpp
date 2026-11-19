#include "UnderpaintingBestiary.h"
#include "UnderpaintingFeel.h"

#include "Underpainting.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "UnderpaintingStyle.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
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

// --- The Hatcher ---

AHatcherEnemy::AHatcherEnemy()
{
	EnemyId = TEXT("Hatcher");
	GetCapsuleComponent()->InitCapsuleSize(60.0f, 60.0f);

	Thread = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Thread"));
	Thread->SetupAttachment(RootComponent);
	Thread->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Thread->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Thread->SetStaticMesh(Cylinder.Object);
	}
}

void AHatcherEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (IsActorBeingDestroyed())
	{
		return;
	}
	HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	InkMaterial = UnderpaintingStyle::MakeMaterial(Thread);
	UnderpaintingStyle::SetColour(InkMaterial, FLinearColor(0.012f, 0.01f, 0.01f), 1.0f);
	SilkMaterial = UMaterialInstanceDynamic::Create(InkMaterial->Parent, this);
	SilkMaterial->CopyParameterOverrides(InkMaterial);
	UnderpaintingStyle::SetColour(SilkMaterial, FLinearColor(0.95f, 0.89f, 0.75f), 1.0f);        // silk is bare paper: it catches the light
	BuildWeb();
}

void AHatcherEnemy::BuildWeb()
{
	// Spokes from where she hangs out to the web's edge, and a ring part-way out with one gap: the way in. The gap is
	// on the side she faces, so whoever places her decides where the door is.
	const FVector Centre = GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight - 75.0f);       // strands run at about knee height
	const float Facing = GetActorRotation().Yaw;
	const float Step = 360.0f / FMath::Max(3, Spokes);
	TArray<FVector> RingPoints;
	for (int32 Index = 0; Index < Spokes; ++Index)
	{
		const FVector Out = FRotator(0.0f, Facing + Step * (Index + 0.5f), 0.0f).Vector();          // no spoke through the door
		StrandStart.Add(Centre + Out * 45.0f);
		StrandEnd.Add(Centre + Out * WebRadius);
		RingPoints.Add(Centre + Out * WebRadius * RingFraction);
	}
	for (int32 Index = 0; Index < Spokes - 1; ++Index)                                              // the last chord is the door
	{
		StrandStart.Add(RingPoints[Index]);
		StrandEnd.Add(RingPoints[Index + 1]);
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 Index = 0; Index < StrandStart.Num(); ++Index)
	{
		UStaticMeshComponent* Strand = NewObject<UStaticMeshComponent>(this);
		Strand->SetStaticMesh(Cube);
		Strand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Strand->SetCastShadow(false);
		Strand->SetMobility(EComponentMobility::Movable);
		Strand->SetUsingAbsoluteLocation(true);
		Strand->SetUsingAbsoluteRotation(true);
		Strand->SetUsingAbsoluteScale(true);
		Strand->SetupAttachment(RootComponent);
		Strand->RegisterComponent();
		const FVector Along = StrandEnd[Index] - StrandStart[Index];
		Strand->SetWorldLocation((StrandStart[Index] + StrandEnd[Index]) * 0.5f);
		Strand->SetWorldRotation(Along.Rotation());
		Strand->SetWorldScale3D(FVector(Along.Size() / 100.0f, 0.04f, 0.04f));
		Strand->SetMaterial(0, SilkMaterial);
		Strand->SetVisibility(false);
		StrandMeshes.Add(Strand);
	}
}

void AHatcherEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (State == EEnemyState::Dead)
	{
		Thread->SetVisibility(false);
		for (UStaticMeshComponent* Strand : StrandMeshes)
		{
			Strand->SetVisibility(true);           // with her gone the web is only string: it shows, and holds nobody
		}
		return;
	}

	// Up on the thread while nothing has disturbed her; down it when something has.
	const bool bUp = State == EEnemyState::Idle;
	Hang = FMath::Clamp(Hang + (bUp ? DeltaTime / 2.0f : -DeltaTime / FMath::Max(0.1f, DescendTime)), 0.0f, 1.0f);
	const float Eased = FMath::SmoothStep(0.0f, 1.0f, Hang);
	const float Turn = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.4f) * 25.0f * Eased;
	const float Height = Eased * HangHeight;
	if (Figure.IsValid())
	{
		// Head down on the thread; level on the ground.
		const FQuat Level = FRotator(0.0f, -90.0f, 0.0f).Quaternion();
		const FQuat HeadDown = FRotator(-88.0f * Eased, Turn, 0.0f).Quaternion();
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight + Height));
		GetMesh()->SetRelativeRotation(HeadDown * Level);
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, IsHanging() ? ECR_Ignore : ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, IsHanging() ? ECR_Ignore : ECR_Block);

	// The thread she hangs by: from her up into the dark.
	const float Top = HangHeight + 700.0f;
	Thread->SetVisibility(Hang > 0.02f);
	Thread->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight + (Height + Top) * 0.5f + 40.0f));
	Thread->SetRelativeScale3D(FVector(0.02f, 0.02f, FMath::Max(1.0f, Top - Height) / 100.0f));

	TickWeb(DeltaTime);
}

void AHatcherEnemy::TickWeb(float DeltaTime)
{
	AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Feet = Player->GetActorLocation();
	for (int32 Index = 0; Index < StrandMeshes.Num(); ++Index)
	{
		const FVector Middle = (StrandStart[Index] + StrandEnd[Index]) * 0.5f;
		const bool bLit = Player->IsFlareLighting(Middle) || Now < ShowAllUntil;
		StrandMeshes[Index]->SetVisibility(bLit);

		// Touching one: held for a moment, every strand shows, and she knows.
		const float Distance = FMath::PointDistToSegment(FVector(Feet.X, Feet.Y, Middle.Z), StrandStart[Index], StrandEnd[Index]);
		if (Distance < TouchDistance && FMath::Abs(Feet.Z - Middle.Z) < 220.0f && Now >= NextHoldAllowed)
		{
			NextHoldAllowed = Now + 6.0f;
			ShowAllUntil = Now + 5.0f;
			Player->Snare(HoldSeconds, 0.2f);
			if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
			{
				Progress->OnNotice.Broadcast(TEXT("Held. A strand you did not see. Light shows them."));
			}
			if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
			{
				Feel->Shake(0.25f);
				Feel->Play(TEXT("enemy_windup"), Middle, 1.0f, 1.6f);
			}
			if (State == EEnemyState::Idle || State == EEnemyState::Returning)
			{
				SetState(EEnemyState::Chasing);
				CooldownRemaining = DescendTime + 0.3f;          // she has to come down before she can strike
			}
			UE_LOG(LogUnderpainting, Log, TEXT("%s: the player touched strand %d"), *GetName(), Index);
		}
	}
}

void AHatcherEnemy::UpdateFigure(float DeltaTime)
{
	if (Hang > 0.5f && State != EEnemyState::Dead)
	{
		Figure.Play(TEXT("Hang"));
		return;
	}
	Super::UpdateFigure(DeltaTime);
}

float AHatcherEnemy::GetAttackReach(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("Rush")) ? 160.0f : 210.0f;
}

float AHatcherEnemy::GetAttackLunge(const FEnemyAttack& Attack) const
{
	return Is(Attack, TEXT("Rush")) ? 420.0f : 0.0f;
}

int32 AHatcherEnemy::ChooseAttack(float DistanceToTarget) const
{
	if (Hang > 0.05f)
	{
		return INDEX_NONE;                     // still on the thread
	}
	// From a little way off, the rush. Up close, the fangs.
	if (DistanceToTarget > 300.0f && DistanceToTarget <= 560.0f)
	{
		return FindAttack(Row, TEXT("Rush"));
	}
	return DistanceToTarget <= 200.0f ? FindAttack(Row, TEXT("FangStrike")) : INDEX_NONE;
}

