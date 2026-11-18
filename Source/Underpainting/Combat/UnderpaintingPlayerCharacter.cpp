#include "UnderpaintingPlayerCharacter.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AUnderpaintingPlayerCharacter::AUnderpaintingPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	Vitals = CreateDefaultSubobject<UUnderpaintingVitalsComponent>(TEXT("Vitals"));
}

void AUnderpaintingPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	Vitals->Initialise(100.0f, 100.0f, 100.0f);
	Vitals->OnDeath.AddDynamic(this, &AUnderpaintingPlayerCharacter::HandleDeath);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	EquipWeapon(WeaponId);
	EquipArmour(ArmourId);
}

void AUnderpaintingPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Fixed keys for the first playable. These move to Input Actions (rebindable) when the options menu exists.
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartLightAttack);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartHeavyAttack);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartDodge);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AUnderpaintingPlayerCharacter::SprintPressed);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AUnderpaintingPlayerCharacter::SprintReleased);

	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartLightAttack);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartHeavyAttack);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartDodge);
	PlayerInputComponent->BindKey(EKeys::Gamepad_LeftThumbstick, IE_Pressed, this, &AUnderpaintingPlayerCharacter::SprintPressed);
	PlayerInputComponent->BindKey(EKeys::Gamepad_LeftThumbstick, IE_Released, this, &AUnderpaintingPlayerCharacter::SprintReleased);
}

void AUnderpaintingPlayerCharacter::EquipWeapon(FName ItemId)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FItemRow* Row = Data ? Data->FindItem(ItemId) : nullptr;
	if (!Row || Row->Type != EItemType::Weapon)
	{
		UE_LOG(LogUnderpainting, Warning, TEXT("EquipWeapon: '%s' is not a weapon in data/items.json"), *ItemId.ToString());
		return;
	}

	WeaponId = ItemId;
	WeaponLightDamage = Row->LightDamage;
	WeaponHeavyDamage = Row->HeavyDamage;
	WeaponSpeed = Row->AttackSpeed;
	WeaponStaminaLight = Row->StaminaCostLight;
	WeaponStaminaHeavy = Row->StaminaCostHeavy;
	WeaponReach = Row->Reach * 100.0f;
	WeaponStaggerMultiplier = Row->StaggerMultiplier;
}

void AUnderpaintingPlayerCharacter::EquipArmour(FName ItemId)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FItemRow* Row = Data ? Data->FindItem(ItemId) : nullptr;
	if (!Row || Row->Type != EItemType::Armour)
	{
		UE_LOG(LogUnderpainting, Warning, TEXT("EquipArmour: '%s' is not armour in data/items.json"), *ItemId.ToString());
		return;
	}

	ArmourId = ItemId;
	Vitals->DamageReduction = Row->DamageReduction;
}

bool AUnderpaintingPlayerCharacter::CanStartAction() const
{
	return Action == EPlayerAction::None && !GetCharacterMovement()->IsFalling();
}

void AUnderpaintingPlayerCharacter::StartLightAttack()
{
	if (CanStartAction() && Vitals->TrySpendStamina(WeaponStaminaLight))
	{
		StartAttack(EPlayerAction::LightAttack);
	}
}

void AUnderpaintingPlayerCharacter::StartHeavyAttack()
{
	if (CanStartAction() && Vitals->TrySpendStamina(WeaponStaminaHeavy))
	{
		StartAttack(EPlayerAction::HeavyAttack);
	}
}

void AUnderpaintingPlayerCharacter::StartAttack(EPlayerAction NewAction)
{
	const bool bHeavy = NewAction == EPlayerAction::HeavyAttack;
	Action = NewAction;
	HitThisSwing.Reset();
	AttackClock.Start((bHeavy ? HeavyTiming : LightTiming).Scaled(WeaponSpeed));

	// Every swing commits you: you can shuffle, not run.
	GetCharacterMovement()->MaxWalkSpeed = AttackMoveSpeed;

	if (UAnimMontage* Montage = bHeavy ? HeavyAttackMontage : LightAttackMontage)
	{
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Play(Montage, Montage->GetPlayLength() / FMath::Max(AttackClock.Timing.Total(), 0.1f));
		}
	}
}

void AUnderpaintingPlayerCharacter::StartDodge()
{
	if (!CanStartAction() || !Vitals->TrySpendStamina(DodgeStaminaCost))
	{
		return;
	}

	// Dodge the way the player is pushing; with no input, step back.
	FVector Direction = GetLastMovementInputVector();
	Direction.Z = 0.0f;
	DodgeDirection = Direction.IsNearlyZero() ? -GetActorForwardVector() : Direction.GetSafeNormal();

	Action = EPlayerAction::Dodge;
	DodgeElapsed = 0.0f;
	Vitals->bInvulnerable = true;

	if (DodgeMontage)
	{
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Play(DodgeMontage, DodgeMontage->GetPlayLength() / FMath::Max(DodgeDuration, 0.1f));
		}
	}
}

void AUnderpaintingPlayerCharacter::SetSprinting(bool bInWantsSprint)
{
	bWantsSprint = bInWantsSprint;
}

void AUnderpaintingPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (Action)
	{
	case EPlayerAction::LightAttack:
	case EPlayerAction::HeavyAttack:
		TickAttack(DeltaTime);
		break;

	case EPlayerAction::Dodge:
		TickDodge(DeltaTime);
		break;

	case EPlayerAction::None:
	{
		const bool bMoving = !GetVelocity().IsNearlyZero(10.0f) && !GetLastMovementInputVector().IsNearlyZero();
		const bool bSprinting = bWantsSprint && bMoving && Vitals->DrainStamina(SprintStaminaPerSecond * DeltaTime);
		GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
		break;
	}

	default:
		break;
	}
}

void AUnderpaintingPlayerCharacter::TickAttack(float DeltaTime)
{
	if (AttackClock.GetPhase() == EAttackPhase::Active)
	{
		HitTest();
	}

	if (AttackClock.Advance(DeltaTime))
	{
		EndAction();
	}
}

void AUnderpaintingPlayerCharacter::HitTest()
{
	const bool bHeavy = Action == EPlayerAction::HeavyAttack;
	const float Radius = 55.0f;
	const FVector Start = GetActorLocation() + GetActorForwardVector() * Radius;
	const FVector End = GetActorLocation() + GetActorForwardVector() * FMath::Max(WeaponReach, Radius + 1.0f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerAttack), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

#if ENABLE_DRAW_DEBUG
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + Radius, Radius,
		FRotationMatrix::MakeFromZ(End - Start).ToQuat(), bHeavy ? FColor(232, 146, 58) : FColor(242, 227, 192), false, 0.05f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || HitThisSwing.Contains(Target))
		{
			continue;
		}

		if (UUnderpaintingVitalsComponent* TargetVitals = Target->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			HitThisSwing.Add(Target);
			const float Damage = bHeavy ? WeaponHeavyDamage : WeaponLightDamage;
			const float Taken = TargetVitals->ApplyDamage(Damage, Damage * WeaponStaggerMultiplier, this);
			UE_LOG(LogUnderpainting, Log, TEXT("Player %s attack hit %s for %.1f"), bHeavy ? TEXT("heavy") : TEXT("light"), *Target->GetName(), Taken);
		}
	}
}

void AUnderpaintingPlayerCharacter::TickDodge(float DeltaTime)
{
	DodgeElapsed += DeltaTime;
	Vitals->bInvulnerable = DodgeElapsed < DodgeInvulnerableTime;

	// Fast at the start, easing out: most of the distance is covered while the dodge still protects you.
	const float Alpha = FMath::Clamp(DodgeElapsed / DodgeDuration, 0.0f, 1.0f);
	const float Speed = (DodgeDistance / DodgeDuration) * 2.0f * (1.0f - Alpha);
	FVector Velocity = DodgeDirection * Speed;
	Velocity.Z = GetCharacterMovement()->Velocity.Z;
	GetCharacterMovement()->Velocity = Velocity;

	if (DodgeElapsed >= DodgeDuration)
	{
		EndAction();
	}
}

void AUnderpaintingPlayerCharacter::EndAction()
{
	if (Action == EPlayerAction::Dead)
	{
		return;
	}

	Action = EPlayerAction::None;
	AttackClock.Stop();
	Vitals->bInvulnerable = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AUnderpaintingPlayerCharacter::HandleDeath(AActor* Killer)
{
	Action = EPlayerAction::Dead;
	AttackClock.Stop();
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}

	UE_LOG(LogUnderpainting, Log, TEXT("Player died to %s"), *GetNameSafe(Killer));

	// Until Lampstands exist (build step 3), death restarts the level.
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AUnderpaintingPlayerCharacter::RestartLevel, 2.5f, false);
}

void AUnderpaintingPlayerCharacter::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
