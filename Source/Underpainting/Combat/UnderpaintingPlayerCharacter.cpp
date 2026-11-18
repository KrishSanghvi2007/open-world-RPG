#include "UnderpaintingPlayerCharacter.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingHUD.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingExploration.h"
#include "UnderpaintingWorldActors.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
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

	FlareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlareLight"));
	FlareLight->SetupAttachment(RootComponent);
	FlareLight->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	FlareLight->SetCastShadows(false);
	FlareLight->SetLightColor(FLinearColor(1.0f, 0.62f, 0.25f));
	FlareLight->SetAttenuationRadius(FlareRadius);
	FlareLight->bUseInverseSquaredFalloff = false;
	FlareLight->SetIntensity(0.0f);
}

void AUnderpaintingPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	Vitals->Initialise(100.0f, 100.0f, 100.0f);
	Vitals->OnDeath.AddDynamic(this, &AUnderpaintingPlayerCharacter::HandleDeath);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	EquipWeapon(WeaponId);
	EquipArmour(ArmourId);

	BaseDodgeDistance = DodgeDistance;
	BaseDodgeInvulnerableTime = DodgeInvulnerableTime;
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		ProgressChangedHandle = Progress->OnChanged.AddUObject(this, &AUnderpaintingPlayerCharacter::ApplyAttunements);
	}
	ApplyAttunements();
	ArriveFromZone();
	RestoreFromSave();
}

void AUnderpaintingPlayerCharacter::ArriveFromZone()
{
	UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const FName From = Subsystem ? Subsystem->ConsumeArrival() : NAME_None;
	if (From.IsNone())
	{
		return;
	}

	// Appear at the exit that leads back where you came from, a few steps inside it.
	for (TActorIterator<AUnderpaintingZoneExit> It(GetWorld()); It; ++It)
	{
		if (It->TargetZone == From)
		{
			FVector Inward = FVector::ZeroVector - It->GetActorLocation();
			if (const APlayerStart* Start = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass())))
			{
				Inward = Start->GetActorLocation() - It->GetActorLocation();
			}
			Inward.Z = 0.0f;
			Inward = Inward.GetSafeNormal();
			SetActorLocation(It->GetActorLocation() + Inward * 350.0f + FVector(0.0f, 0.0f, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* PlayerController = GetController())
			{
				PlayerController->SetControlRotation(Inward.Rotation());
			}
			break;
		}
	}

	// Arriving in a zone counts as reaching it.
	const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
	const FName Zone = Data ? Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)) : NAME_None;
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		const FRegionRow* Row = Data ? Data->FindRegion(Zone) : nullptr;
		Progress->ReachPlace(Zone, Row ? Row->DisplayName : FString());
	}
}

bool AUnderpaintingPlayerCharacter::UseFlare()
{
	if (Action == EPlayerAction::Dead || IsMenuOpen() || FlareRemaining > 0.0f)
	{
		return false;
	}
	if (!Vitals->TrySpendGlim(FlareGlimCost))
	{
		if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
		{
			Progress->OnNotice.Broadcast(TEXT("The lantern is too low to flare."));
		}
		return false;
	}
	FlareRemaining = FlareDuration;
	return true;
}

bool AUnderpaintingPlayerCharacter::IsFlareLighting(const FVector& Location) const
{
	return FlareRemaining > 0.0f && FVector::DistSquared(Location, GetActorLocation()) <= FMath::Square(FlareRadius);
}

bool AUnderpaintingPlayerCharacter::UseFixativeFlask()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress || Action == EPlayerAction::Dead || IsMenuOpen())
	{
		return false;
	}

	// The platform nearest the centre of the view, within throwing range.
	FVector ViewLocation = GetActorLocation();
	FRotator ViewRotation = GetActorRotation();
	if (const AController* PlayerController = GetController())
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	AUnderpaintingSketchPlatform* Best = nullptr;
	float BestScore = 0.6f;
	for (TActorIterator<AUnderpaintingSketchPlatform> It(GetWorld()); It; ++It)
	{
		if (Progress->IsPlatformFixed(It->GetPlatformKey()) || FVector::Dist(It->GetActorLocation(), GetActorLocation()) > 1200.0f)
		{
			continue;
		}
		const float Score = FVector::DotProduct(ViewRotation.Vector(), (It->GetActorLocation() - ViewLocation).GetSafeNormal());
		if (Score > BestScore)
		{
			Best = *It;
			BestScore = Score;
		}
	}
	if (!Best)
	{
		Progress->OnNotice.Broadcast(TEXT("Nothing sketched is in reach."));
		return false;
	}
	return Progress->FixPlatform(Best->GetPlatformKey());
}

void AUnderpaintingPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->OnChanged.Remove(ProgressChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AUnderpaintingPlayerCharacter::ApplyAttunements()
{
	// Attunement nodes change the Limner through a handful of named effects (data/attunements.json, EffectType).
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress)
	{
		return;
	}
	Vitals->SetMaxGlim(100.0f + Progress->GetAttunementValue(TEXT("MaxGlim")));
	Vitals->StaminaRegenMultiplier = 1.0f + Progress->GetAttunementValue(TEXT("StaminaRegenPercent")) / 100.0f;
	DodgeDistance = BaseDodgeDistance * (1.0f + Progress->GetAttunementValue(TEXT("DodgeDistancePercent")) / 100.0f);
	const float Invulnerable = Progress->GetAttunementValue(TEXT("DodgeInvulnerableSeconds"));
	DodgeInvulnerableTime = Invulnerable > 0.0f ? Invulnerable : BaseDodgeInvulnerableTime;
}

void AUnderpaintingPlayerCharacter::RestoreFromSave()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingProgressSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const UUnderpaintingSaveGame* Save = Subsystem ? Subsystem->GetLoadedSave() : nullptr;
	UUnderpaintingProgressSubsystem* Mutable = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	if (Save && Mutable && Mutable->ConsumeRestore() && Save->LevelName == FName(*UGameplayStatics::GetCurrentLevelName(this, true)))
	{
		SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PlayerController = GetController())
		{
			PlayerController->SetControlRotation(Save->PlayerTransform.Rotator());
		}
		Vitals->SetHealthAndGlim(Save->Health, Save->Glim);
	}
}

AUnderpaintingHUD* AUnderpaintingPlayerCharacter::GetUnderpaintingHUD() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	return PlayerController ? Cast<AUnderpaintingHUD>(PlayerController->GetHUD()) : nullptr;
}

bool AUnderpaintingPlayerCharacter::IsMenuOpen() const
{
	const AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	return HUD && HUD->IsMenuOpen();
}

void AUnderpaintingPlayerCharacter::DoMove(float Right, float Forward)
{
	// A conversation or a menu holds you where you stand.
	if (!IsMenuOpen())
	{
		Super::DoMove(Right, Forward);
	}
}

void AUnderpaintingPlayerCharacter::UpdateFocus()
{
	// "Use" goes to what you are looking at. In third person that means: of the usable things in reach of the
	// character, the one closest to the direction the camera is pointing. Height is ignored, so a node on the
	// ground and a tall lamp are equally easy; wide things forgive a little more.
	AUnderpaintingInteractable* Best = nullptr;
	float BestAngle = UseViewAngle;
	if (Action == EPlayerAction::None && !IsMenuOpen())
	{
		const float ViewYaw = GetController() ? GetController()->GetControlRotation().Yaw : GetActorRotation().Yaw;
		const FVector Here = GetActorLocation();

		for (TActorIterator<AUnderpaintingInteractable> It(GetWorld()); It; ++It)
		{
			FVector Origin, Extent;
			It->GetActorBounds(false, Origin, Extent);
			const FVector To = Origin - Here;
			const float Distance = To.Size2D();
			const float HalfWidth = FMath::Max(Extent.X, Extent.Y);
			if (Distance - HalfWidth > UseReach || FMath::Abs(To.Z) > 320.0f + Extent.Z || It->GetPrompt().IsEmpty())
			{
				continue;
			}

			// Standing on top of something counts as looking at it.
			float Angle = 0.0f;
			if (Distance > HalfWidth + 30.0f)
			{
				Angle = FMath::Abs(FRotator::NormalizeAxis(To.Rotation().Yaw - ViewYaw));
				Angle = FMath::Max(0.0f, Angle - FMath::RadiansToDegrees(FMath::Atan2(HalfWidth, Distance)));
			}
			if (Angle < BestAngle || (FMath::IsNearlyEqual(Angle, BestAngle, 0.5f) && Best && Distance < FVector::Dist2D(Best->GetActorLocation(), Here)))
			{
				Best = *It;
				BestAngle = Angle;
			}
		}
	}
	Focus = Best;
}

void AUnderpaintingPlayerCharacter::Interact()
{
	UpdateFocus();
	AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	if (HUD && HUD->IsMenuOpen())
	{
		HUD->Advance();
	}
	else if (Focus.IsValid() && Action == EPlayerAction::None)
	{
		Focus->Interact(this);
	}
}

void AUnderpaintingPlayerCharacter::MenuClose()
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		HUD->CloseMenu();
	}
}

void AUnderpaintingPlayerCharacter::InventoryPressed()
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		HUD->ToggleInventory();
	}
}

void AUnderpaintingPlayerCharacter::MenuNumber(int32 Number)
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		HUD->ChooseNumber(Number);
	}
}

void AUnderpaintingPlayerCharacter::OnRested()
{
	Vitals->Refill();
	SaveNow();
}

void AUnderpaintingPlayerCharacter::SaveNow()
{
	if (UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>())
	{
		Subsystem->SaveGame(this);
	}
}

void AUnderpaintingPlayerCharacter::LoadNow()
{
	UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>();
	if (Subsystem && Subsystem->LoadGame())
	{
		RestoreFromSave();
	}
}

void AUnderpaintingPlayerCharacter::StartNewGame()
{
	if (UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>())
	{
		Subsystem->NewGame();
		RestartLevel();
	}
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

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AUnderpaintingPlayerCharacter::InteractPressed);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUnderpaintingPlayerCharacter::MenuClosePressed);
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AUnderpaintingPlayerCharacter::InventoryPressed);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AUnderpaintingPlayerCharacter::SavePressed);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AUnderpaintingPlayerCharacter::LoadPressed);
	PlayerInputComponent->BindKey(EKeys::F8, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NewGamePressed);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AUnderpaintingPlayerCharacter::FlarePressed);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AUnderpaintingPlayerCharacter::FlaskPressed);
	PlayerInputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &AUnderpaintingPlayerCharacter::FlarePressed);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<1>);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<2>);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<3>);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<4>);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<5>);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<6>);
	PlayerInputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<7>);
	PlayerInputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<8>);
	PlayerInputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &AUnderpaintingPlayerCharacter::NumberPressed<9>);
	PlayerInputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AUnderpaintingPlayerCharacter::InteractPressed);

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
	return Action == EPlayerAction::None && !GetCharacterMovement()->IsFalling() && !IsMenuOpen();
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
	UpdateFocus();

	if (FlareRemaining > 0.0f)
	{
		FlareRemaining = FMath::Max(0.0f, FlareRemaining - DeltaTime);
	}
	// The flare burns bright and gutters in its last second, so you can feel it running out.
	FlareLight->SetIntensity(FlareRemaining > 0.0f ? 28.0f * FMath::Min(1.0f, FlareRemaining) : 0.0f);

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
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->ApplyDeathPenalty();
	}

	// Until Lampstands exist (build step 3), death restarts the level.
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AUnderpaintingPlayerCharacter::RestartLevel, 2.5f, false);
}

void AUnderpaintingPlayerCharacter::RestartLevel()
{
	// Wake where you last rested: reload the save's position (and its level, if it was another zone).
	UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>();
	const UUnderpaintingSaveGame* Save = Subsystem ? Subsystem->GetLoadedSave() : nullptr;
	if (Subsystem && Save && !Save->LevelName.IsNone())
	{
		Subsystem->RequestRestore();
		UGameplayStatics::OpenLevel(this, FName(*FString::Printf(TEXT("/Game/Maps/Matriarch/%s"), *Save->LevelName.ToString())));
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

FString AUnderpaintingPlayerCharacter::DescribeState()
{
	UpdateFocus();
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	FString Text = FString::Printf(TEXT("level=%s flare=%.1f prompt=[%s] menu=%d row=%s hp=%.0f glim=%.0f/%.0f"),
		*UGameplayStatics::GetCurrentLevelName(this, true), FlareRemaining,
		Focus.IsValid() ? *Focus->GetPrompt() : TEXT(""), HUD ? int32(HUD->GetMenu()) : -1, HUD ? *HUD->GetDialogueRow().ToString() : TEXT(""),
		Vitals->GetHealth(), Vitals->GetGlim(), Vitals->MaxGlim);
	if (Progress)
	{
		for (const FName& Quest : Progress->GetActiveQuests())
		{
			Text += FString::Printf(TEXT(" | %s step %d: %s"), *Quest.ToString(), Progress->GetQuestProgress(Quest)->Step + 1, *Progress->GetCurrentStepText(Quest));
		}
		const FUnderpaintingProgressData& Data = Progress->GetData();
		for (const TPair<FName, int32>& Pair : Data.WetPigment) { if (Pair.Value > 0) { Text += FString::Printf(TEXT(" | wet %s %d"), *Pair.Key.ToString(), Pair.Value); } }
		for (const TPair<FName, int32>& Pair : Data.DryPigment) { if (Pair.Value > 0) { Text += FString::Printf(TEXT(" | dry %s %d"), *Pair.Key.ToString(), Pair.Value); } }
		for (const TPair<FName, int32>& Pair : Data.Items) { if (Pair.Value > 0) { Text += FString::Printf(TEXT(" | %s x%d"), *Pair.Key.ToString(), Pair.Value); } }
		for (const FName& Node : Data.Attunements) { Text += FString::Printf(TEXT(" | attuned %s"), *Node.ToString()); }
		Text += FString::Printf(TEXT(" | lore %d places %d caches %d fixed %d"), Data.FoundLore.Num(), Data.DiscoveredPlaces.Num(), Data.OpenedCaches.Num(), Data.FixedPlatforms.Num());
		for (const TPair<FName, FQuestProgress>& Pair : Data.Quests) { if (Pair.Value.bComplete) { Text += FString::Printf(TEXT(" | DONE %s"), *Pair.Key.ToString()); } }
	}
	return Text;
}
