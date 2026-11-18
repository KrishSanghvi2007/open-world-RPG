#include "UnderpaintingEnemy.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingFeel.h"
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
#include "EngineUtils.h"
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
	PlaceholderBody->SetRelativeScale3D(FVector(0.7f, 0.7f, 2.2f));   // re-applied from GetBodyScale() in BeginPlay
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

	TelegraphGround = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelegraphGround"));
	TelegraphGround->SetupAttachment(RootComponent);
	TelegraphGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphGround->SetCastShadow(false);
	TelegraphGround->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	DiscMesh = Cylinder.Succeeded() ? Cylinder.Object : nullptr;
	BarMesh = Cube.Succeeded() ? Cube.Object : nullptr;
	if (DiscMesh)
	{
		TelegraphGround->SetStaticMesh(DiscMesh);
	}
}

void AUnderpaintingEnemy::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();

	const UUnderpaintingProgress* WorldState = UUnderpaintingProgressSubsystem::Get(this);
	if (WorldState && ((!RemovedByFlag.IsNone() && WorldState->HasFlag(RemovedByFlag)) || (!RequiresFlag.IsNone() && !WorldState->HasFlag(RequiresFlag))))
	{
		Destroy();
		return;
	}

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

	PlaceholderBody->SetRelativeScale3D(GetBodyScale());
	BodyMaterial = UnderpaintingStyle::MakeMaterial(PlaceholderBody);
	GroundMaterial = UnderpaintingStyle::MakeMaterial(TelegraphGround);
	BreathOffset = FMath::FRandRange(0.0f, 6.0f);
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
	if (Direction.IsNearlyZero())
	{
		return;
	}
	Direction.Normalize();

	// No path-finding: it feels ahead, and if something solid is in the way it goes round on one side and
	// keeps to that side for a moment so it does not dither. Enough for pillars, trunks and corners.
	const auto IsBlocked = [this](const FVector& Way)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyFeeler), false, this);
		const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
		const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + Way * 110.0f, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(45.0f), Params);
		return bHit && !Cast<APawn>(Hit.GetActor());        // other creatures and the player are not walls
	};

	const float Now = GetWorld()->GetTimeSeconds();
	if (IsBlocked(Direction))
	{
		for (const float Angle : { 50.0f, 85.0f, 120.0f })
		{
			for (const float Sign : { AvoidSign, -AvoidSign })
			{
				// Having picked a side, stay on it until the moment has passed.
				if (Sign != AvoidSign && Now < AvoidUntil)
				{
					continue;
				}
				const FVector Way = Direction.RotateAngleAxis(Sign * Angle, FVector::UpVector);
				if (!IsBlocked(Way))
				{
					AvoidSign = Sign;
					AvoidUntil = Now + 0.8f;
					AddMovementInput(Way, 1.0f);
					return;
				}
			}
		}
	}
	AddMovementInput(Direction, 1.0f);
}

int32 AUnderpaintingEnemy::ChooseAttack(float DistanceToTarget) const
{
	// Use the shortest-reaching attack that can still connect.
	int32 Best = INDEX_NONE;
	float BestReach = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Row.Attacks.Num(); ++Index)
	{
		const float Reach = GetAttackReach(Row.Attacks[Index]) + GetAttackLunge(Row.Attacks[Index]);
		if (Row.Attacks[Index].Phase <= Phase && DistanceToTarget <= Reach && Reach < BestReach)
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
	AttackBeganAt = GetWorld()->GetTimeSeconds();
	bAttackLanded = false;
	HitsLanded = 0;
	// A multi-hit attack stays live long enough for all its hits.
	const float Active = Attack.Active + (GetHitCount(Attack) - 1) * GetHitInterval(Attack);
	AttackClock.Start(FAttackTiming(Attack.Windup, Active, Attack.Recovery));
	SetState(EEnemyState::Attacking);
	bSwingSounded = false;
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		// Bigger things sound lower.
		Feel->Play(TEXT("enemy_windup"), GetActorLocation(), 0.8f, FMath::Clamp(1.6f / FMath::Max(GetBodyScale().Z, 0.5f), 0.5f, 1.3f));
	}
	UE_LOG(LogUnderpainting, Verbose, TEXT("%s winds up %s"), *GetName(), *Attack.AttackName.ToString());
}

void AUnderpaintingEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StateTime += DeltaTime;
	TimeSinceHit += DeltaTime;
	CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	SlowRemaining = FMath::Max(0.0f, SlowRemaining - DeltaTime);
	StaggerImmunityRemaining = FMath::Max(0.0f, StaggerImmunityRemaining - DeltaTime);
	VulnerableRemaining = FMath::Max(0.0f, VulnerableRemaining - DeltaTime);
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed * (SlowRemaining > 0.0f ? SlowMultiplier : 1.0f);
	// A two-phase enemy changes when its Health crosses the threshold in the data.
	if (Phase == 1 && Row.Phase2HPPercent > 0 && State != EEnemyState::Dead && Vitals->GetHealthFraction() * 100.0f <= Row.Phase2HPPercent)
	{
		Phase = 2;
		PhaseChangeRemaining = PhaseChangeTime;
		AttackClock.Stop();
		CurrentAttack = INDEX_NONE;
		ApplyVulnerable(0.5f, PhaseChangeTime);
		SetState(EEnemyState::Chasing);
		OnPhaseTwo();
		UE_LOG(LogUnderpainting, Log, TEXT("%s enters phase 2"), *GetName());
	}
	if (PhaseChangeRemaining > 0.0f)
	{
		PhaseChangeRemaining -= DeltaTime;
		UpdateTelegraph();
		return;
	}

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
		// Never more than two attacking at once, and never two beginning together (enemy design principle 4 allows up to three;
		// Act 1 teaches, so its groups take turns).
		const bool bMyTurn = CountAttackers(this) < MaxAttackers && SecondsSinceAnyAttackBegan(this) >= AttackStagger;
		const int32 Attack = CooldownRemaining <= 0.0f && bMyTurn ? ChooseAttack(Distance) : INDEX_NONE;
		if (Attack != INDEX_NONE)
		{
			BeginAttack(Attack);
		}
		else if (Distance > GetApproachDistance())
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
			StaggerImmunityRemaining = StaggerImmunityTime;
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
	const EAttackPhase PhaseNow = AttackClock.GetPhase();
	const float ElapsedBefore = AttackClock.Elapsed;

	if (PhaseNow == EAttackPhase::Windup)
	{
		if (!OnWindupTick(Attack, Target))
		{
			AttackClock.Stop();
			CurrentAttack = INDEX_NONE;
			ForceStagger();
			return;
		}
		// Track the target through the wind-up, then commit to a direction just before the hit.
		if (Target && AttackClock.Elapsed < Attack.Windup - AimLockTime)
		{
			FaceTowards(Target->GetActorLocation(), DeltaTime, 360.0f);
		}
	}
	else if (PhaseNow == EAttackPhase::Active)
	{
		if (!bSwingSounded)
		{
			bSwingSounded = true;
			if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
			{
				Feel->Play(TEXT("enemy_swing"), GetActorLocation(), 0.9f, FMath::Clamp(1.6f / FMath::Max(GetBodyScale().Z, 0.5f), 0.5f, 1.3f));
			}
		}
		const float ActiveTime = AttackClock.Timing.Active;
		const float Lunge = GetAttackLunge(Attack);
		if (Lunge > 0.0f && ActiveTime > 0.0f)
		{
			FVector Velocity = GetActorForwardVector() * (Lunge / ActiveTime);
			Velocity.Z = GetCharacterMovement()->Velocity.Z;
			GetCharacterMovement()->Velocity = Velocity;
		}

		// Each hit of a multi-hit attack gets its own slice of the active window.
		const int32 Slice = FMath::Clamp(FMath::FloorToInt((AttackClock.Elapsed - Attack.Windup) / GetHitInterval(Attack)), 0, GetHitCount(Attack) - 1);
		if (GetHitCount(Attack) > 1 && Slice > HitsLanded)
		{
			HitsLanded = Slice;
			bAttackLanded = false;
		}
		if (!bAttackLanded)
		{
			HitTest(Attack);
		}
	}

	const bool bFinished = AttackClock.Advance(DeltaTime);

	// The moment the wind-up ends.
	if (ElapsedBefore < Attack.Windup && AttackClock.Elapsed >= Attack.Windup)
	{
		AttackStartLocation = GetActorLocation();
		OnAttackActive(Attack, Target);
	}

	if (bFinished)
	{
		const float Travelled = FVector::Dist2D(AttackStartLocation, GetActorLocation());
		CurrentAttack = INDEX_NONE;
		CooldownRemaining = AttackCooldown;
		SetState(EEnemyState::Chasing);
		OnAttackFinished(Attack, Travelled);
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
	if (IsAreaAttack(Attack))
	{
		// Everything within reach, all the way round.
		GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation() + FVector(0.0f, 0.0f, 1.0f), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Reach), Params);
#if ENABLE_DRAW_DEBUG
	if (UUnderpaintingFeel::bShowHitboxes)
		DrawDebugSphere(GetWorld(), GetActorLocation(), Reach, 16, FColor(196, 87, 75), false, 0.05f);
#endif
	}
	else
	{
		GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);
	}

#if ENABLE_DRAW_DEBUG
	if (UUnderpaintingFeel::bShowHitboxes)
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
			// A blow that is dodged is spent: it passed through. Without this, an attack that stays live longer than a dodge
			// protects would land anyway the moment the protection ended, and dodging well would be worth nothing.
			const bool bDodged = OtherVitals->bInvulnerable;
			const float Taken = OtherVitals->ApplyDamage(Attack.Damage, Attack.Damage, this);
			if (bDodged)
			{
				bAttackLanded = true;
				break;
			}
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
	if (StaggerImmunityRemaining <= 0.0f)
	{
		PoiseDamage += StaggerDamage;
	}

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
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->Play(TEXT("stagger"), GetActorLocation());
			Feel->Shake(0.2f);
		}
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

	// It comes apart: a burst of its own paint, and what is left sinks.
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->Splash(GetActorLocation() + FVector(0.0f, 0.0f, 30.0f), FVector::UpVector, GetBodyColour(), Row.Tier == EEnemyTier::Boss ? 36 : 22, 560.0f, GetBodyPurity());
		Feel->Play(TEXT("enemy_death"), GetActorLocation(), 1.0f, FMath::Clamp(1.6f / FMath::Max(GetBodyScale().Z, 0.5f), 0.5f, 1.3f));
		Feel->HitStop(this, Killer, 0.12f);
		Feel->Shake(Row.Tier == EEnemyTier::Boss ? 0.8f : 0.3f);
	}
	TelegraphGround->SetVisibility(false);
	TelegraphLight->SetIntensity(0.0f);
	UE_LOG(LogUnderpainting, Log, TEXT("%s died"), *GetName());
	SetLifeSpan(1.5f);
}

void AUnderpaintingEnemy::UpdateTelegraph()
{
	const bool bWindup = State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Windup;
	const bool bActive = State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Active;

	// The warm rim grows through the wind-up so the moment of the hit can be read.
	const bool bWhiteFlash = (bWindup || bActive) && Row.Attacks.IsValidIndex(CurrentAttack) && Row.Attacks[CurrentAttack].Telegraph == EAttackTelegraph::WhiteFlash;
	TelegraphLight->SetLightColor(bWhiteFlash ? FLinearColor::White : FLinearColor(1.0f, 0.45f, 0.12f));
	TelegraphLight->SetIntensity(bWindup ? FMath::Lerp(4.0f, bWhiteFlash ? 60.0f : 30.0f, AttackClock.GetPhaseAlpha()) : (bActive ? 30.0f : 0.0f));

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
		// Struck: it blinks pale for an instant.
		if (FlashRemaining > 0.0f)
		{
			Colour = FMath::Lerp(Colour, FLinearColor(1.0f, 0.95f, 0.85f), FMath::Clamp(FlashRemaining / 0.12f, 0.0f, 1.0f) * 0.8f);
		}
		UnderpaintingStyle::SetColour(BodyMaterial, Colour, GetBodyPurity());
	}

	// The mark on the ground: where the hit will land, growing to full size at the moment it does.
	const bool bShowGround = (bWindup || bActive) && Row.Attacks.IsValidIndex(CurrentAttack);
	TelegraphGround->SetVisibility(bShowGround);
	if (bShowGround)
	{
		const FEnemyAttack& Attack = Row.Attacks[CurrentAttack];
		const float Fill = bActive ? 1.0f : FMath::Lerp(0.2f, 1.0f, FMath::Square(AttackClock.GetPhaseAlpha()));
		const float Floor = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.0f;
		if (IsAreaAttack(Attack))
		{
			const float Diameter = 2.0f * GetAttackReach(Attack) * Fill;
			TelegraphGround->SetStaticMesh(DiscMesh);
			TelegraphGround->SetRelativeLocation(FVector(0.0f, 0.0f, Floor));
			TelegraphGround->SetRelativeScale3D(FVector(Diameter / 100.0f, Diameter / 100.0f, 0.02f));
		}
		else
		{
			const float Length = (GetAttackReach(Attack) + GetAttackLunge(Attack)) * Fill;
			TelegraphGround->SetStaticMesh(BarMesh);
			TelegraphGround->SetRelativeLocation(FVector(Length * 0.5f, 0.0f, Floor));
			TelegraphGround->SetRelativeScale3D(FVector(Length / 100.0f, 1.3f, 0.02f));
		}
		if (GroundMaterial)
		{
			TelegraphGround->SetMaterial(0, GroundMaterial);
			const FLinearColor Mark = bWhiteFlash ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.42f, 0.08f);
			UnderpaintingStyle::SetColour(GroundMaterial, bActive ? Mark : Mark * FMath::Lerp(0.45f, 1.0f, AttackClock.GetPhaseAlpha()), 1.0f);
		}
	}

	UpdateBody();
}

void AUnderpaintingEnemy::UpdateBody()
{
	// A stand-in for animation: the placeholder gathers itself before a blow, snaps forward into it, flinches when
	// struck, sags when staggered and comes apart when it dies. Timing comes from the same clock as the rules.
	const FVector Base = GetBodyScale();
	FVector Scale = Base;
	FRotator Turn = FRotator::ZeroRotator;
	FVector Offset = FVector::ZeroVector;
	const float Time = GetWorld()->GetTimeSeconds();
	const float Delta = GetWorld()->GetDeltaSeconds();

	if (State == EEnemyState::Dead)
	{
		const float Through = FMath::Clamp(StateTime / 0.9f, 0.0f, 1.0f);
		Turn.Pitch = 88.0f * Through * Through;
		Scale = Base * (1.0f - 0.8f * Through * Through);
		Offset.Z = -50.0f * Through;
	}
	else if (State == EEnemyState::Attacking)
	{
		const float Through = AttackClock.GetPhaseAlpha();
		switch (AttackClock.GetPhase())
		{
		case EAttackPhase::Windup:
			// Gathering: it sinks, swells and leans back, and trembles just before it lets go.
			Scale = Base * FVector(1.0f + 0.2f * Through, 1.0f + 0.2f * Through, 1.0f - 0.22f * Through);
			Turn.Pitch = -18.0f * Through;
			Offset.Z = -Base.Z * 11.0f * Through;
			if (Through > 0.72f)
			{
				Offset.Y = FMath::Sin(Time * 75.0f) * 3.5f;
			}
			break;
		case EAttackPhase::Active:
			Scale = Base * FVector(0.88f, 0.88f, 1.16f);
			Turn.Pitch = 19.0f;
			Offset.X = 30.0f;
			break;
		default:
			// Spent: it eases back upright through the recovery.
			Scale = Base * FMath::Lerp(FVector(0.88f, 0.88f, 1.16f), FVector::OneVector, FMath::SmoothStep(0.0f, 0.6f, Through));
			Turn.Pitch = FMath::Lerp(19.0f, 0.0f, FMath::SmoothStep(0.0f, 0.7f, Through));
			Offset.X = FMath::Lerp(30.0f, 0.0f, FMath::SmoothStep(0.0f, 0.7f, Through));
			break;
		}
	}
	else if (State == EEnemyState::Staggered || ReelRemaining > 0.0f)
	{
		Turn.Pitch = 22.0f + FMath::Sin(Time * 5.0f) * 4.0f;
		Turn.Roll = FMath::Sin(Time * 3.1f) * 7.0f;
		Scale = Base * FVector(1.06f, 1.06f, 0.9f);
	}
	else
	{
		// Alive and unoccupied: a slow breath, and a lean and a bob when it walks.
		const float Pace = FMath::Clamp(GetVelocity().Size2D() / FMath::Max(1.0f, float(Row.MoveSpeed)), 0.0f, 1.0f);
		Scale.Z = Base.Z * (1.0f + 0.025f * FMath::Sin(Time * 2.2f + BreathOffset));
		Turn.Pitch = 9.0f * Pace;
		Offset.Z = Pace * FMath::Abs(FMath::Sin(Time * 9.0f + BreathOffset)) * 5.0f;
	}

	// A blow rocks it away from where the blow came from.
	FlinchRemaining = FMath::Max(0.0f, FlinchRemaining - Delta);
	FlashRemaining = FMath::Max(0.0f, FlashRemaining - Delta);
	if (FlinchRemaining > 0.0f && State != EEnemyState::Dead)
	{
		const float Kick = FMath::Square(FlinchRemaining / 0.28f);
		Turn.Pitch -= FlinchLocal.X * 26.0f * Kick;
		Turn.Roll += FlinchLocal.Y * 26.0f * Kick;
		Scale *= FVector(1.0f + 0.12f * Kick, 1.0f + 0.12f * Kick, 1.0f - 0.1f * Kick);
	}

	PlaceholderBody->SetRelativeScale3D(Scale);
	PlaceholderBody->SetRelativeRotation(Turn);
	PlaceholderBody->SetRelativeLocation(Offset);
}

void AUnderpaintingEnemy::ReceiveImpact(const FVector& From, float Knockback)
{
	if (State == EEnemyState::Dead)
	{
		return;
	}
	FVector Away = GetActorLocation() - From;
	Away.Z = 0.0f;
	Away = Away.GetSafeNormal();
	FlinchLocal = GetActorTransform().InverseTransformVectorNoScale(-Away);
	FlinchRemaining = 0.28f;
	FlashRemaining = 0.12f;

	// A boss is not moved, and nothing is moved out of a blow it has already let go.
	const bool bRooted = Row.Tier == EEnemyTier::Boss || (State == EEnemyState::Attacking && AttackClock.GetPhase() == EAttackPhase::Active);
	if (!bRooted)
	{
		LaunchCharacter(Away * Knockback * GetKnockbackScale() + FVector(0.0f, 0.0f, 30.0f), true, false);
	}
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
	const EAttackPhase ClockPhase = AttackClock.GetPhase();
	if (State != EEnemyState::Attacking || (ClockPhase != EAttackPhase::Windup && ClockPhase != EAttackPhase::Active))
	{
		return false;
	}
	// A white flash means "this cannot be cancelled: move" (enemy design principle 2).
	if (Row.Attacks.IsValidIndex(CurrentAttack) && Row.Attacks[CurrentAttack].Telegraph == EAttackTelegraph::WhiteFlash)
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

TArray<FName> AUnderpaintingEnemy::GetActiveWeaknesses() const
{
	if (!Row.Phase2Hue.IsNone() && Row.Weaknesses.IsValidIndex(Phase - 1))
	{
		return { Row.Weaknesses[Phase - 1] };
	}
	return Row.Weaknesses;
}

TArray<FName> AUnderpaintingEnemy::GetActiveResistances() const
{
	if (!Row.Phase2Hue.IsNone() && Row.Resistances.IsValidIndex(Phase - 1))
	{
		return { Row.Resistances[Phase - 1] };
	}
	return Row.Resistances;
}

void AUnderpaintingEnemy::ForceStagger()
{
	if (State != EEnemyState::Dead)
	{
		PoiseDamage = 0.0f;
		AttackClock.Stop();
		CurrentAttack = INDEX_NONE;
		SetState(EEnemyState::Staggered);
	}
}

float AUnderpaintingEnemy::SecondsSinceAnyAttackBegan(const UObject* WorldContext)
{
	float Latest = -100.0f;
	for (TActorIterator<AUnderpaintingEnemy> It(WorldContext->GetWorld()); It; ++It)
	{
		Latest = FMath::Max(Latest, It->AttackBeganAt);
	}
	return WorldContext->GetWorld()->GetTimeSeconds() - Latest;
}

int32 AUnderpaintingEnemy::CountAttackers(const UObject* WorldContext)
{
	int32 Count = 0;
	for (TActorIterator<AUnderpaintingEnemy> It(WorldContext->GetWorld()); It; ++It)
	{
		Count += It->State == EEnemyState::Attacking ? 1 : 0;
	}
	return Count;
}
