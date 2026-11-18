#include "UnderpaintingEnemy.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingStyle.h"
#include "UnderpaintingVitalsComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AUnderpaintingEnemy::AUnderpaintingEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(45.0f, 110.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	Vitals = CreateDefaultSubobject<UUnderpaintingVitalsComponent>(TEXT("Vitals"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(RootComponent);
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetRelativeScale3D(FVector(0.7f, 0.7f, 2.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		PlaceholderBody->SetStaticMesh(Cylinder.Object);
	}

	TelegraphLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TelegraphLight"));
	TelegraphLight->SetupAttachment(RootComponent);
	TelegraphLight->SetRelativeLocation(FVector(40.0f, 0.0f, 60.0f));
	TelegraphLight->SetCastShadows(false);
	TelegraphLight->SetAttenuationRadius(450.0f);
	TelegraphLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.12f));
	TelegraphLight->SetIntensity(0.0f);
	TelegraphLight->bUseInverseSquaredFalloff = false;
}

void AUnderpaintingEnemy::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();

	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	if (const FEnemyRow* Found = Data ? Data->FindEnemy(EnemyId) : nullptr)
	{
		Row = *Found;
	}
	else
	{
		UE_LOG(LogUnderpainting, Error, TEXT("Enemy '%s' has no row in data/enemies.json"), *EnemyId.ToString());
	}

	Vitals->Initialise(FMath::Max(1, Row.MaxHP), 0.0f, 0.0f);
	Vitals->OnDamaged.AddDynamic(this, &AUnderpaintingEnemy::HandleDamaged);
	Vitals->OnDeath.AddDynamic(this, &AUnderpaintingEnemy::HandleDeath);
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed;

	BodyMaterial = UnderpaintingStyle::MakeMaterial(PlaceholderBody);
	UpdateTelegraph();
}

FName AUnderpaintingEnemy::GetCurrentAttackName() const
{
	return Row.Attacks.IsValidIndex(CurrentAttack) && State == EEnemyState::Attacking ? Row.Attacks[CurrentAttack].AttackName : NAME_None;
}

AActor* AUnderpaintingEnemy::FindTarget() const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UUnderpaintingVitalsComponent* PlayerVitals = Player ? Player->FindComponentByClass<UUnderpaintingVitalsComponent>() : nullptr;
	return PlayerVitals && !PlayerVitals->IsDead() ? Player : nullptr;
}

void AUnderpaintingEnemy::SetState(EEnemyState NewState)
{
	if (State != NewState)
	{
		State = NewState;
		StateTime = 0.0f;
	}
}

void AUnderpaintingEnemy::FaceTowards(const FVector& Location, float DeltaTime, float DegreesPerSecond)
{
	FVector Direction = Location - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.IsNearlyZero())
	{
		const FRotator Wanted = Direction.Rotation();
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.0f, Wanted.Yaw, 0.0f), DeltaTime, DegreesPerSecond));
	}
}

void AUnderpaintingEnemy::MoveTowards(const FVector& Location)
{
	FVector Direction = Location - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.0f);
	}
}

int32 AUnderpaintingEnemy::ChooseAttack(float DistanceToTarget) const
{
	// Use the shortest-reaching attack that can still connect.
	int32 Best = INDEX_NONE;
	float BestReach = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Row.Attacks.Num(); ++Index)
	{
		const float Reach = GetAttackReach(Row.Attacks[Index]) + GetAttackLunge(Row.Attacks[Index]);
		if (Row.Attacks[Index].Phase == 1 && DistanceToTarget <= Reach && Reach < BestReach)
		{
			Best = Index;
			BestReach = Reach;
		}
	}
	return Best;
}

void AUnderpaintingEnemy::BeginAttack(int32 AttackIndex)
{
	const FEnemyAttack& Attack = Row.Attacks[AttackIndex];
	CurrentAttack = AttackIndex;
	bAttackLanded = false;
	AttackClock.Start(FAttackTiming(Attack.Windup, Attack.Active, Attack.Recovery));
	SetState(EEnemyState::Attacking);
	UE_LOG(LogUnderpainting, Verbose, TEXT("%s winds up %s"), *GetName(), *Attack.AttackName.ToString());
}

void AUnderpaintingEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StateTime += DeltaTime;
	TimeSinceHit += DeltaTime;
	CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	SlowRemaining = FMath::Max(0.0f, SlowRemaining - DeltaTime);
	VulnerableRemaining = FMath::Max(0.0f, VulnerableRemaining - DeltaTime);
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed * (SlowRemaining > 0.0f ? SlowMultiplier : 1.0f);
	if (ReelRemaining > 0.0f)
	{
		// Reeling from an erased attack: no action until it passes.
		ReelRemaining -= DeltaTime;
		UpdateTelegraph();
		return;
	}
	if (TimeSinceHit >= PoiseResetTime)
	{
		PoiseDamage = 0.0f;
	}

	AActor* Target = FindTarget();
	const float Distance = Target ? FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();
	const float HomeDistance = FVector::Dist2D(HomeLocation, GetActorLocation());

	switch (State)
	{
	case EEnemyState::Idle:
		if (Target && Distance <= Row.AggroRadius * 100.0f)
		{
			SetState(EEnemyState::Chasing);
		}
		break;

	case EEnemyState::Chasing:
	{
		if (!Target || HomeDistance > Row.LeashRadius * 100.0f)
		{
			SetState(EEnemyState::Returning);
			break;
		}

		FaceTowards(Target->GetActorLocation(), DeltaTime);
		const int32 Attack = CooldownRemaining <= 0.0f ? ChooseAttack(Distance) : INDEX_NONE;
		if (Attack != INDEX_NONE)
		{
			BeginAttack(Attack);
		}
		else if (Distance > 140.0f)
		{
			MoveTowards(Target->GetActorLocation());
		}
		break;
	}

	case EEnemyState::Attacking:
		TickAttack(DeltaTime, Target);
		break;

	case EEnemyState::Staggered:
		if (StateTime >= StaggerDuration)
		{
			SetState(EEnemyState::Chasing);
		}
		break;

	case EEnemyState::Returning:
		if (Target && Distance <= Row.AggroRadius * 100.0f && HomeDistance < Row.LeashRadius * 100.0f * 0.5f)
		{
			SetState(EEnemyState::Chasing);
		}
		else if (HomeDistance < 80.0f)
		{
			SetState(EEnemyState::Idle);
		}
		else
		{
			FaceTowards(HomeLocation, DeltaTime);
			MoveTowards(HomeLocation);
		}
		break;

	default:
		break;
	}

	UpdateTelegraph();
}

void AUnderpaintingEnemy::TickAttack(float DeltaTime, AActor* Target)
{
	const FEnemyAttack& Attack = Row.Attacks[CurrentAttack];
	const EAttackPhase Phase = AttackClock.GetPhase();

	if (Phase == EAttackPhase::Windup)
	{
		// Track the target through the wind-up, then commit to a direction just before the hit.
		if (Target && AttackClock.Elapsed < Attack.Windup - AimLockTime)
		{
			FaceTowards(Target->GetActorLocation(), DeltaTime, 360.0f);
		}
	}
	else if (Phase == EAttackPhase::Active)
	{
		const float Lunge = GetAttackLunge(Attack);
		if (Lunge > 0.0f && Attack.Active > 0.0f)
		{
			FVector Velocity = GetActorForwardVector() * (Lunge / Attack.Active);
			Velocity.Z = GetCharacterMovement()->Velocity.Z;
			GetCharacterMovement()->Velocity = Velocity;
		}
		if (!bAttackLanded)
		{
			HitTest(Attack);
		}
	}

	if (AttackClock.Advance(DeltaTime))
	{
		CurrentAttack = INDEX_NONE;
		CooldownRemaining = AttackCooldown;
		SetState(EEnemyState::Chasing);
	}
}

void AUnderpaintingEnemy::HitTest(const FEnemyAttack& Attack)
{
	const float Radius = 60.0f;
	const float Reach = GetAttackReach(Attack);
	const FVector Start = GetActorLocation() + GetActorForwardVector() * Radius;
	const FVector End = GetActorLocation() + GetActorForwardVector() * FMath::Max(Reach, Radius + 1.0f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyAttack), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

#if ENABLE_DRAW_DEBUG
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + Radius, Radius,
		FRotationMatrix::MakeFromZ(End - Start).ToQuat(), FColor(196, 87, 75), false, 0.05f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other || Other->IsA<AUnderpaintingEnemy>())
		{
			continue;
		}

		if (UUnderpaintingVitalsComponent* OtherVitals = Other->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			const float Taken = OtherVitals->ApplyDamage(Attack.Damage, Attack.Damage, this);
			if (Taken > 0.0f)
			{
				bAttackLanded = true;
				UE_LOG(LogUnderpainting, Log, TEXT("%s %s hit %s for %.1f"), *GetName(), *Attack.AttackName.ToString(), *Other->GetName(), Taken);
				OnAttackLanded(Attack, Other);
				break;
			}
		}
	}
}

void AUnderpaintingEnemy::HandleDamaged(float Damage, float StaggerDamage, AActor* DamageCauser)
{
	TimeSinceHit = 0.0f;
	PoiseDamage += StaggerDamage;

	if (State == EEnemyState::Idle || State == EEnemyState::Returning)
	{
		SetState(EEnemyState::Chasing);
	}

	if (Row.Poise > 0 && PoiseDamage >= Row.Poise && State != EEnemyState::Dead)
	{
		PoiseDamage = 0.0f;
		AttackClock.Stop();
		CurrentAttack = INDEX_NONE;
		SetState(EEnemyState::Staggered);
		UE_LOG(LogUnderpainting, Log, TEXT("%s is staggered"), *GetName());
	}
}

void AUnderpaintingEnemy::HandleDeath(AActor* Killer)
{
	SetState(EEnemyState::Dead);
	AttackClock.Stop();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Glim returns to the lantern of whoever finished it.
	if (UUnderpaintingVitalsComponent* KillerVitals = Killer ? Killer->FindComponentByClass<UUnderpaintingVitalsComponent>() : nullptr)
	{
		KillerVitals->AddGlim(Row.GlimOnKill);
	}

	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		// Drops go to the pack as wet pigment or items.
		for (const FDropEntry& Drop : Row.Drops)
		{
			if (FMath::FRand() <= Drop.Chance)
			{
				if (Progress->IsPigment(Drop.Id)) { Progress->AddPigment(Drop.Id, Drop.Count, true); }
				else { Progress->AddItem(Drop.Id, Drop.Count); }
			}
		}
		Progress->NotifyEvent(TEXT("Kill"), EnemyId, 1);
	}

	UE_LOG(LogUnderpainting, Log, TEXT("%s died"), *GetName());
	SetLifeSpan(1.5f);
}

void AUnderpaintingEnemy::UpdateTelegraph()
{
	const bool bWindup = State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Windup;
	const bool bActive = State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Active;

	// The warm rim grows through the wind-up so the moment of the hit can be read.
	TelegraphLight->SetIntensity(bWindup ? FMath::Lerp(4.0f, 30.0f, AttackClock.GetPhaseAlpha()) : (bActive ? 30.0f : 0.0f));

	if (BodyMaterial)
	{
		FLinearColor Colour = GetBodyColour();
		if (bWindup || bActive)
		{
			Colour = FMath::Lerp(Colour, FLinearColor(1.0f, 0.45f, 0.12f), bActive ? 1.0f : 0.25f + 0.6f * AttackClock.GetPhaseAlpha());
		}
		else if (State == EEnemyState::Staggered)
		{
			Colour = Colour * 0.45f;
		}
		UnderpaintingStyle::SetColour(BodyMaterial, Colour, GetBodyPurity());
	}

	// A staggered or dead enemy slumps.
	const float Lean = State == EEnemyState::Staggered ? 25.0f : (State == EEnemyState::Dead ? 85.0f : 0.0f);
	PlaceholderBody->SetRelativeRotation(FRotator(Lean, 0.0f, 0.0f));
}

void AUnderpaintingEnemy::ApplySlow(float SpeedMultiplier, float Duration)
{
	SlowMultiplier = FMath::Clamp(SpeedMultiplier, 0.1f, 1.0f);
	SlowRemaining = FMath::Max(SlowRemaining, Duration);
}

void AUnderpaintingEnemy::ApplyVulnerable(float ExtraDamageFraction, float Duration)
{
	VulnerableFraction = ExtraDamageFraction;
	VulnerableRemaining = FMath::Max(VulnerableRemaining, Duration);
}

bool AUnderpaintingEnemy::CancelAttack(float ReelSeconds)
{
	const EAttackPhase Phase = AttackClock.GetPhase();
	if (State != EEnemyState::Attacking || (Phase != EAttackPhase::Windup && Phase != EAttackPhase::Active))
	{
		return false;
	}
	AttackClock.Stop();
	CurrentAttack = INDEX_NONE;
	CooldownRemaining = AttackCooldown;
	ReelRemaining = ReelSeconds;
	SetState(EEnemyState::Chasing);
	UE_LOG(LogUnderpainting, Log, TEXT("%s had its attack erased"), *GetName());
	return true;
}
