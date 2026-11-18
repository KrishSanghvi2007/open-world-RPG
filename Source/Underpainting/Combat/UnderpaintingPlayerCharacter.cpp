#include "UnderpaintingPlayerCharacter.h"

#include "Underpainting.h"
#include "UnderpaintingCombatRules.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingEnemy.h"
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
	Vitals->OnDodged.AddDynamic(this, &AUnderpaintingPlayerCharacter::HandleDodged);

	// How much light this zone has decides what Strokes cost here.
	if (const UUnderpaintingDataSubsystem* ZoneData = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr)
	{
		const FRegionRow* Zone = ZoneData->FindRegion(ZoneData->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)));
		ZoneAmbientLight = Zone ? Zone->AmbientLight : 1.0f;
	}
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
	// Ceruse is lead: carrying it in a well costs 15 maximum Health.
	Vitals->SetMaxHealth(Progress->IsInWell(TEXT("Ceruse")) ? 85.0f : 100.0f);
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
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AUnderpaintingPlayerCharacter::HeavyPressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AUnderpaintingPlayerCharacter::HeavyReleased);
	PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AUnderpaintingPlayerCharacter::SwapWellPressed);
	PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AUnderpaintingPlayerCharacter::PoulticePressed);
	PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &AUnderpaintingPlayerCharacter::LampOilPressed);
	PlayerInputComponent->BindKey(EKeys::Gamepad_RightThumbstick, IE_Pressed, this, &AUnderpaintingPlayerCharacter::ToggleLockOn);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AUnderpaintingPlayerCharacter::StartDodge);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AUnderpaintingPlayerCharacter::SprintPressed);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AUnderpaintingPlayerCharacter::SprintReleased);

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AUnderpaintingPlayerCharacter::InteractPressed);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUnderpaintingPlayerCharacter::LockOrClosePressed);
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
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AUnderpaintingPlayerCharacter::HeavyPressed);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Released, this, &AUnderpaintingPlayerCharacter::HeavyReleased);
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
	bDodgeRewarded = false;
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
	TickLockOn(DeltaTime);

	// Holding the heavy button turns the swing into the active well's Stroke.
	if (HeavyHeldSince >= 0.0f && GetWorld()->GetTimeSeconds() - HeavyHeldSince >= StrokeHoldTime)
	{
		HeavyHeldSince = -1.0f;
		if (!StartStroke())
		{
			StartHeavyAttack();
		}
	}

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
	case EPlayerAction::Stroke:
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
		// Sfumato is a blurred dash, 4 m, through whatever is in front.
		if (Action == EPlayerAction::Stroke && StrokeName == TEXT("Sfumato"))
		{
			FVector Velocity = GetActorForwardVector() * (400.0f / FMath::Max(AttackClock.Timing.Active, 0.05f));
			Velocity.Z = GetCharacterMovement()->Velocity.Z;
			GetCharacterMovement()->Velocity = Velocity;
			Vitals->bInvulnerable = true;
		}
		HitTest();
	}
	else if (Action == EPlayerAction::Stroke && StrokeName == TEXT("Sfumato"))
	{
		Vitals->bInvulnerable = bDevInvulnerable;
	}

	if (AttackClock.Advance(DeltaTime))
	{
		EndAction();
	}
}

void AUnderpaintingPlayerCharacter::HitTest()
{
	const bool bStroke = Action == EPlayerAction::Stroke;
	const bool bHeavy = Action == EPlayerAction::HeavyAttack;
	const float Radius = 55.0f;
	const float Reach = WeaponReach * (bStroke && StrokeName == TEXT("Impasto") ? 1.15f : 1.0f);
	const FVector Start = GetActorLocation() + GetActorForwardVector() * Radius;
	const FVector End = GetActorLocation() + GetActorForwardVector() * FMath::Max(Reach, Radius + 1.0f);

	// Alla Prima is three quick hits: the active window is split in three and each part can land once.
	if (bStroke && StrokeName == TEXT("AllaPrima"))
	{
		const int32 Part = FMath::Clamp(FMath::FloorToInt(AttackClock.GetPhaseAlpha() * 3.0f), 0, 2);
		if (Part > StrokeHitsDone)
		{
			StrokeHitsDone = Part;
			HitThisSwing.Reset();
		}
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerAttack), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

#if ENABLE_DRAW_DEBUG
	FColor SwingColour = bHeavy ? FColor(232, 146, 58) : FColor(242, 227, 192);
	if (bStroke)
	{
		const UUnderpaintingDataSubsystem* ColourData = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
		const FPigmentRow* Pigment = ColourData ? ColourData->FindPigment(StrokePigment) : nullptr;
		SwingColour = Pigment ? FColor::FromHex(Pigment->ColourHex) : SwingColour;
	}
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + Radius, Radius,
		FRotationMatrix::MakeFromZ(End - Start).ToQuat(), SwingColour, false, 0.05f, 0, bStroke ? 4.0f : 1.0f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || HitThisSwing.Contains(Target) || !Target->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			continue;
		}
		HitThisSwing.Add(Target);

		if (!bStroke)
		{
			const float Damage = bHeavy ? WeaponHeavyDamage : WeaponLightDamage;
			ApplyHit(Target, Damage, Damage * WeaponStaggerMultiplier, bHeavy ? TEXT("heavy") : TEXT("light"));
			continue;
		}

		// A Stroke: the Contrast rule decides how hard it lands.
		AUnderpaintingEnemy* Enemy = Cast<AUnderpaintingEnemy>(Target);
		const UnderpaintingCombatRules::FContrast Contrast = Enemy
			? UnderpaintingCombatRules::Contrast(StrokePigment, Enemy->GetActiveWeaknesses(), Enemy->GetActiveResistances())
			: UnderpaintingCombatRules::FContrast();
		float Damage = StrokeDamage * Contrast.DamageMultiplier;
		// Stagger builds from the Stroke's own damage: the Contrast rule gives +50% stagger, not +30% and then +50%.
		float Stagger = StrokeDamage * WeaponStaggerMultiplier * Contrast.StaggerMultiplier;

		UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
		if (StrokeName == TEXT("Impasto"))
		{
			Stagger *= 2.0f;
			if (Enemy) { Enemy->ApplySlow(0.6f, 3.0f + (Progress ? Progress->GetAttunementValue(TEXT("SmearDurationSeconds")) > 0.0f ? 2.0f : 0.0f : 0.0f)); }
		}
		else if (StrokeName == TEXT("Glaze") && Enemy)
		{
			Enemy->ApplyVulnerable(0.15f, 6.0f);
		}
		else if (StrokeName == TEXT("Sfumato") && Enemy)
		{
			Enemy->ApplySlow(0.6f, 3.0f);
		}
		else if (StrokeName == TEXT("Scumble"))
		{
			Vitals->AddStamina(15.0f);
		}
		else if (StrokeName == TEXT("AllaPrima"))
		{
			Vitals->Heal(4.0f + (Progress && Progress->GetAttunementValue(TEXT("StrokeHealPerHit")) > 0.0f ? 2.0f : 0.0f));
		}
		else if (StrokeName == TEXT("Erase") && Enemy && Enemy->CancelAttack())
		{
			if (Progress) { Vitals->AddGlim(Progress->GetAttunementValue(TEXT("EraseGlimRefund"))); }
		}

		ApplyHit(Target, Damage, Stagger, *FString::Printf(TEXT("%s%s"), *StrokeName.ToString(), Contrast.Sign > 0 ? TEXT(" (contrast)") : (Contrast.Sign < 0 ? TEXT(" (muddy)") : TEXT(""))));
		if (Progress && Contrast.Sign != 0)
		{
			Progress->OnNotice.Broadcast(Contrast.Sign > 0 ? TEXT("Contrast! +30%") : TEXT("Muddy. -30%"));
		}
	}
}

void AUnderpaintingPlayerCharacter::ApplyHit(AActor* Target, float Damage, float StaggerDamage, const TCHAR* What)
{
	UUnderpaintingVitalsComponent* TargetVitals = Target->FindComponentByClass<UUnderpaintingVitalsComponent>();
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (const AUnderpaintingEnemy* Enemy = Cast<AUnderpaintingEnemy>(Target))
	{
		Damage *= 1.0f + Enemy->GetVulnerability();        // a Glazed enemy takes more from everything
	}
	if (Progress)
	{
		StaggerDamage *= 1.0f + Progress->GetAttunementValue(TEXT("StaggerDamagePercent")) / 100.0f;
	}

	const float Taken = TargetVitals->ApplyDamage(Damage, StaggerDamage, this);
	UE_LOG(LogUnderpainting, Log, TEXT("Player %s hit %s for %.1f (stagger %.1f)"), What, *Target->GetName(), Taken, StaggerDamage);

	if (TargetVitals->IsDead() && Progress)
	{
		Vitals->Heal(Progress->GetAttunementValue(TEXT("HealOnKill")));
	}
}

bool AUnderpaintingPlayerCharacter::StartStroke()
{
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const UUnderpaintingDataSubsystem* Data = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FName Pigment = Progress ? Progress->GetActivePigment() : NAME_None;
	const FPigmentRow* Row = Data ? Data->FindPigment(Pigment) : nullptr;
	if (!Row || !CanStartAction())
	{
		return false;
	}

	const int32 GlimCost = GetStrokeGlimCost();
	if (Vitals->GetGlim() < GlimCost)
	{
		Progress->OnNotice.Broadcast(TEXT("Not enough light for that Stroke."));
		return false;
	}
	if (!Vitals->TrySpendStamina(WeaponStaminaHeavy))
	{
		return false;
	}
	Vitals->TrySpendGlim(GlimCost);

	StrokePigment = Pigment;
	StrokeName = Row->StrokeName;
	StrokeDamage = WeaponHeavyDamage * Row->StrokeDamageMultiplier;
	StrokeHitsDone = 0;
	Action = EPlayerAction::Stroke;
	HitThisSwing.Reset();

	// Each Stroke has its own wind-up (data/pigments.json). The flurry stays live longer; the slam recovers longer.
	const float Active = StrokeName == TEXT("AllaPrima") ? 0.6f : (StrokeName == TEXT("Sfumato") ? 0.3f : 0.2f);
	const float Recovery = StrokeName == TEXT("Impasto") ? 0.8f : (StrokeName == TEXT("Erase") ? 0.3f : 0.5f);
	AttackClock.Start(FAttackTiming(Row->StrokeWindup, Active, Recovery).Scaled(WeaponSpeed));
	GetCharacterMovement()->MaxWalkSpeed = AttackMoveSpeed;
	UE_LOG(LogUnderpainting, Log, TEXT("Player Stroke %s for %d Glim (light here %.2f)"), *StrokeName.ToString(), GlimCost, GetLightHere());
	return true;
}

float AUnderpaintingPlayerCharacter::GetLightHere() const
{
	const float Light = UnderpaintingCombatRules::EffectiveLight(ZoneAmbientLight, FlareRemaining > 0.0f);
	return GetWorld() && GetWorld()->GetTimeSeconds() < BorrowedLightUntil ? FMath::Max(Light, BorrowedLight) : Light;
}

void AUnderpaintingPlayerCharacter::LendLight(float Light, float Seconds)
{
	BorrowedLight = Light;
	BorrowedLightUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

int32 AUnderpaintingPlayerCharacter::GetStrokeGlimCost() const
{
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const UUnderpaintingDataSubsystem* Data = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FPigmentRow* Row = Data && Progress ? Data->FindPigment(Progress->GetActivePigment()) : nullptr;
	return Row ? UnderpaintingCombatRules::StrokeGlimCost(Row->LightCost, GetLightHere()) : 0;
}

void AUnderpaintingPlayerCharacter::HeavyPressed()
{
	HeavyHeldSince = GetWorld()->GetTimeSeconds();
}

void AUnderpaintingPlayerCharacter::HeavyReleased()
{
	// A tap is the plain heavy attack. (A hold has already become a Stroke in Tick.)
	if (HeavyHeldSince >= 0.0f)
	{
		HeavyHeldSince = -1.0f;
		StartHeavyAttack();
	}
}

void AUnderpaintingPlayerCharacter::SwapWell()
{
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->SwapActiveWell();
	}
}

bool AUnderpaintingPlayerCharacter::UsePoultice()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress || Action == EPlayerAction::Dead || IsMenuOpen() || Vitals->GetHealth() >= Vitals->MaxHealth)
	{
		return false;
	}
	if (!Progress->RemoveItem(TEXT("MossPoultice"), 1))
	{
		Progress->OnNotice.Broadcast(TEXT("No Moss Poultice."));
		return false;
	}
	// 40 Health over 4 seconds; Verdaccio (Terre Verte tier 2) makes it 60.
	Vitals->StartHealOverTime(40.0f * (1.0f + Progress->GetAttunementValue(TEXT("PoulticeHealPercent")) / 100.0f), 4.0f);
	Progress->OnNotice.Broadcast(TEXT("Moss Poultice."));
	return true;
}

bool AUnderpaintingPlayerCharacter::UseLampOil()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress || Action == EPlayerAction::Dead || IsMenuOpen() || Vitals->GetGlim() >= Vitals->MaxGlim)
	{
		return false;
	}
	if (!Progress->RemoveItem(TEXT("LampOil"), 1))
	{
		Progress->OnNotice.Broadcast(TEXT("No Lamp Oil."));
		return false;
	}
	Vitals->AddGlim(40.0f);
	Progress->OnNotice.Broadcast(TEXT("The lantern drinks. +40 Glim"));
	return true;
}

void AUnderpaintingPlayerCharacter::HandleDodged(AActor* Attacker)
{
	// A hit that passes through the safe part of a dodge gives light back: 5 Glim, or 10 with Golden Hour.
	if (Action == EPlayerAction::Dodge && !bDodgeRewarded)
	{
		bDodgeRewarded = true;
		const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
		const float Bonus = Progress ? Progress->GetAttunementValue(TEXT("PerfectDodgeGlim")) : 0.0f;
		Vitals->AddGlim(Bonus > 0.0f ? Bonus : 5.0f);
		if (Progress)
		{
			Progress->OnNotice.Broadcast(TEXT("Perfect dodge."));
		}
	}
}

void AUnderpaintingPlayerCharacter::LockOrClosePressed()
{
	if (IsMenuOpen())
	{
		MenuClose();
	}
	else
	{
		ToggleLockOn();
	}
}

void AUnderpaintingPlayerCharacter::ToggleLockOn()
{
	if (LockTarget.IsValid())
	{
		LockTarget = nullptr;
		return;
	}

	// The living enemy in range closest to where the camera points.
	const FVector ViewDirection = GetController() ? GetController()->GetControlRotation().Vector() : GetActorForwardVector();
	AUnderpaintingEnemy* Best = nullptr;
	float BestScore = 0.2f;
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetActorLocation() - GetActorLocation();
		if (It->GetVitals()->IsDead() || To.Size() > LockOnRange)
		{
			continue;
		}
		const float Score = FVector::DotProduct(ViewDirection.GetSafeNormal2D(), To.GetSafeNormal2D()) - To.Size() / LockOnRange * 0.3f;
		if (Score > BestScore)
		{
			Best = *It;
			BestScore = Score;
		}
	}
	LockTarget = Best;
}

void AUnderpaintingPlayerCharacter::TickLockOn(float DeltaTime)
{
	AUnderpaintingEnemy* Target = LockTarget.Get();
	if (!Target)
	{
		return;
	}
	const FVector To = Target->GetActorLocation() - GetActorLocation();
	if (Target->GetVitals()->IsDead() || To.Size() > LockOnRange * 1.4f || Action == EPlayerAction::Dead)
	{
		LockTarget = nullptr;
		return;
	}

	// The camera settles on the target, and (unless dodging) so does the Limner, so swings go where you are looking.
	if (AController* PlayerController = GetController())
	{
		FRotator Wanted = To.Rotation();
		Wanted.Pitch = FMath::Clamp(Wanted.Pitch - 12.0f, -35.0f, 20.0f);
		PlayerController->SetControlRotation(FMath::RInterpTo(PlayerController->GetControlRotation(), Wanted, DeltaTime, 8.0f));
	}
	if (Action != EPlayerAction::Dodge && !IsMenuOpen())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, To.Rotation().Yaw, 0.0f), DeltaTime, 12.0f));
	}
}

void AUnderpaintingPlayerCharacter::TickDodge(float DeltaTime)
{
	DodgeElapsed += DeltaTime;
	Vitals->bInvulnerable = bDevInvulnerable || DodgeElapsed < DodgeInvulnerableTime;

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
	Vitals->bInvulnerable = bDevInvulnerable;
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
		Text += FString::Printf(TEXT(" | wells [%s][%s] active %d stroke cost %d light %.2f lock %s"), *Progress->GetWell(0).ToString(), *Progress->GetWell(1).ToString(),
			Progress->GetActiveWellIndex(), GetStrokeGlimCost(), GetLightHere(), LockTarget.IsValid() ? *LockTarget->GetName() : TEXT("none"));
		Text += FString::Printf(TEXT(" | lore %d places %d caches %d fixed %d"), Data.FoundLore.Num(), Data.DiscoveredPlaces.Num(), Data.OpenedCaches.Num(), Data.FixedPlatforms.Num());
		for (const TPair<FName, FQuestProgress>& Pair : Data.Quests) { if (Pair.Value.bComplete) { Text += FString::Printf(TEXT(" | DONE %s"), *Pair.Key.ToString()); } }
	}
	return Text;
}

void AUnderpaintingPlayerCharacter::DevGrant(FName Id, int32 Count)
{
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		if (Progress->IsPigment(Id)) { Progress->AddPigment(Id, Count, false); }
		else { Progress->AddItem(Id, Count); }
	}
}

bool AUnderpaintingPlayerCharacter::DevAttune(FName NodeId)
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	return Progress && Progress->Attune(NodeId);
}

bool AUnderpaintingPlayerCharacter::DevSetWell(int32 Index, FName PigmentId)
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	return Progress && Progress->SetWell(Index, PigmentId);
}

void AUnderpaintingPlayerCharacter::DevSetInvulnerable(bool bOn)
{
	bDevInvulnerable = bOn;
	Vitals->bInvulnerable = bOn;
}

float AUnderpaintingPlayerCharacter::DevHurtEnemies(float Damage)
{
	float Total = 0.0f;
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		Total += It->GetVitals()->ApplyDamage(Damage, 0.0f, this);
	}
	return Total;
}

FString AUnderpaintingPlayerCharacter::DevDescribeEnemies()
{
	FString Text;
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		Text += FString::Printf(TEXT("%s hp=%.0f phase=%d state=%d attack=%s weak=%s | "), *It->GetName(), It->GetVitals()->GetHealth(), It->GetPhase(), int32(It->GetState()),
			*It->GetCurrentAttackName().ToString(), It->GetActiveWeaknesses().Num() > 0 ? *It->GetActiveWeaknesses()[0].ToString() : TEXT("-"));
	}
	return Text;
}
