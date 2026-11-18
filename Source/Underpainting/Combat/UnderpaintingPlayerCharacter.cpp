#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingSettings.h"
#include "UnderpaintingAutopilot.h"
#include "UnderpaintingFeel.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	Autopilot = CreateDefaultSubobject<UUnderpaintingAutopilot>(TEXT("Autopilot"));

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
	Vitals->OnDamaged.AddDynamic(this, &AUnderpaintingPlayerCharacter::HandleDamaged);
	LoadAnimations();

	// The camera trails a little and turns a little late, so movement reads as weight and not as a rail.
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->bEnableCameraLag = true;
		Boom->CameraLagSpeed = 11.0f;
		Boom->CameraLagMaxDistance = 90.0f;
		Boom->bEnableCameraRotationLag = true;
		Boom->CameraRotationLagSpeed = 22.0f;
	}
	LastStepLocation = GetActorLocation();

	// How much light this zone has decides what Strokes cost here.
	if (const UUnderpaintingDataSubsystem* ZoneData = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr)
	{
		const FRegionRow* Zone = ZoneData->FindRegion(ZoneData->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)));
		const UUnderpaintingProgress* World = UUnderpaintingProgressSubsystem::Get(this);
		const FName ZoneId = ZoneData->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true));
		const FName ZoneSteps(*FString::Printf(TEXT("footstep_%s"), *ZoneId.ToString()));
		FootstepSound = ZoneData->FindSound(ZoneSteps) ? ZoneSteps : FName(TEXT("footstep"));
		ZoneAmbientLight = !Zone ? 1.0f : (World ? World->GetZoneLight(ZoneData->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)), Zone->AmbientLight, Zone->AmbientLightAfterShift) : Zone->AmbientLight);
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

	// You wake where you last rested or arrived: walking into a zone, or starting out, is a place to wake.
	// And what you have and have done is written down as you go, so closing the game never costs more than a few seconds.
	if (UUnderpaintingProgressSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr)
	{
		const UUnderpaintingDataSubsystem* Zones = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
		const bool bInZone = Zones && !Zones->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)).IsNone();
		bInZoneLevel = bInZone;
		if (bInZone && (bArrivedThroughExit || !Saves->GetLoadedSave()))
		{
			FTimerHandle Unused;
			GetWorldTimerManager().SetTimer(Unused, this, &AUnderpaintingPlayerCharacter::Checkpoint, 0.5f, false);
		}
		if (UUnderpaintingProgress* Dirty = Saves->GetProgress())
		{
			ProgressDirtyHandle = Dirty->OnChanged.AddUObject(this, &AUnderpaintingPlayerCharacter::NoteProgressChanged);
		}
	}

	// After the Turning the level is swapped under you: you are still standing where you stood.
	FTransform Kept;
	UUnderpaintingProgressSubsystem* Travel = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	float CarriedHealth = 0.0f, CarriedGlim = 0.0f;
	if (Travel && Travel->ConsumeVitals(CarriedHealth, CarriedGlim))
	{
		Vitals->SetHealthAndGlim(CarriedHealth, CarriedGlim);        // walking between zones does not top you up
	}
	if (Travel && Travel->ConsumeKeptTransform(Kept))
	{
		SetActorTransform(Kept, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PlayerController = GetController())
		{
			PlayerController->SetControlRotation(Kept.Rotator());
		}
		Travel->SaveGame(this);        // the changed world is the world now
	}
}

void AUnderpaintingPlayerCharacter::DevStartTurning()
{
	if (TurningTime < 0.0f)
	{
		TurningTime = 0.0f;
		TurningBaseRotation = GetControlRotation();
		if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
		{
			HUD->CloseMenu();
		}
		GetCharacterMovement()->StopMovementImmediately();
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			DisableInput(PlayerController);
		}
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->PlayFlat(TEXT("rumble_turning"));
		}
		UE_LOG(LogUnderpainting, Log, TEXT("The Turning begins"));
	}
}

void AUnderpaintingPlayerCharacter::ArriveFromZone()
{
	UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const FName From = Subsystem ? Subsystem->ConsumeArrival() : NAME_None;
	if (From.IsNone())
	{
		return;
	}
	bArrivedThroughExit = true;

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
	// A burning Flare can be renewed once it starts to gutter (its last seconds), so light can be kept up across a long crossing.
	if (Action == EPlayerAction::Dead || IsMenuOpen() || FlareRemaining > FlareRenewWindow)
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
	HintDone(TEXT("Flare"));
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->Play(TEXT("flare"), GetActorLocation());
	}
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
		Progress->OnChanged.Remove(ProgressDirtyHandle);
	}
	if (bProgressDirty && bInZoneLevel && Action != EPlayerAction::Dead)
	{
		if (UUnderpaintingProgressSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr)
		{
			Saves->SaveProgressOnly();        // leaving, or closing the game: nothing done is lost
		}
	}
	if (UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this))
	{
		Settings->OnKeysChanged.Remove(KeysChangedHandle);
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
	// The Heart wall is decided: she turns.
	if (Progress->IsTurningDue() && TurningTime < 0.0f)
	{
		DevStartTurning();
	}

	// Gear follows the save: what was crafted and put on stays on.
	if (WeaponId != Progress->GetEquippedWeapon()) { EquipWeapon(Progress->GetEquippedWeapon()); }
	if (ArmourId != Progress->GetEquippedArmour()) { EquipArmour(Progress->GetEquippedArmour()); }
	Vitals->SetMaxGlim(100.0f + Progress->GetAttunementValue(TEXT("MaxGlim")) + Progress->GetUpgradeGlim());
	Vitals->StaminaRegenMultiplier = 1.0f + (Progress->GetAttunementValue(TEXT("StaminaRegenPercent")) + ArmourStaminaRegenPercent) / 100.0f;
	DodgeDistance = BaseDodgeDistance * (1.0f + Progress->GetAttunementValue(TEXT("DodgeDistancePercent")) / 100.0f);
	// Ceruse is lead: carrying it in a well costs 15 maximum Health.
	// Clean Ground (Ceruse tier 2) eases the toll to 10.
	Vitals->SetMaxHealth(Progress->IsInWell(TEXT("Ceruse")) ? (Progress->HasAttunement(TEXT("Ceruse_T2")) ? 90.0f : 85.0f) : 100.0f);
	const float Invulnerable = Progress->GetAttunementValue(TEXT("DodgeInvulnerableSeconds"));
	DodgeInvulnerableTime = Invulnerable > 0.0f ? Invulnerable : BaseDodgeInvulnerableTime;
}

void AUnderpaintingPlayerCharacter::RestoreFromSave()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingProgressSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const UUnderpaintingSaveGame* Save = Subsystem ? Subsystem->GetLoadedSave() : nullptr;
	UUnderpaintingProgressSubsystem* Mutable = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const FName Here(*UGameplayStatics::GetCurrentLevelName(this, true));
	const UUnderpaintingDataSubsystem* ZoneData = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	if (Save && Mutable && Mutable->IsRestorePending() && Mutable->GetSavedLevel() != Here && !Mutable->GetSavedLevel().IsNone()
		&& ZoneData && !ZoneData->FindZoneForLevel(Here.ToString()).IsNone())
	{
		// The game has started in one zone but the save was made in another: go there. (Test rooms are left alone.)
		UGameplayStatics::OpenLevel(this, FName(*FString::Printf(TEXT("/Game/Maps/Matriarch/%s"), *Mutable->GetSavedLevel().ToString())));
		return;
	}
	if (Save && Mutable && Mutable->ConsumeRestore() && Mutable->GetSavedLevel() == Here)
	{
		SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PlayerController = GetController())
		{
			PlayerController->SetControlRotation(Save->PlayerTransform.Rotator());
		}
		if (Mutable->ConsumeWakingFromDeath())
		{
			// After a death you wake whole, with the lantern at least half full: the walk back is the cost, not a second death.
			Vitals->SetHealthAndGlim(Vitals->MaxHealth, FMath::Max(Save->Glim, Vitals->MaxGlim * 0.5f));
			if (UUnderpaintingProgress* Woken = Mutable->GetProgress())
			{
				Woken->TopUpPoultices();        // or every retry after the first would be fought with none
			}
		}
		else
		{
			Vitals->SetHealthAndGlim(Save->Health, Save->Glim);
		}
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

void AUnderpaintingPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	// With a menu open the mouse is a pointer, not the camera.
	if (!IsMenuOpen())
	{
		Super::DoLook(Yaw, Pitch);
	}
}

void AUnderpaintingPlayerCharacter::LightPressed()
{
	// The light-attack key doubles as "press this" when a menu is open and it is a mouse button.
	if (IsMenuOpen())
	{
		MenuClickPressed();
	}
	else
	{
		StartLightAttack();
	}
}

void AUnderpaintingPlayerCharacter::MenuClickPressed()
{
	AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	if (HUD && HUD->IsMenuOpen())
	{
		HUD->Click();
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
			if (Distance - HalfWidth > UseReach || FMath::Abs(To.Z) > 320.0f + Extent.Z || It->GetUsablePrompt().IsEmpty())
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

namespace
{
	/** What using a thing sounds like, by what kind of thing it is (rows of data/sounds.json). */
	FName UseSoundFor(const AUnderpaintingInteractable* Thing)
	{
		if (Thing->IsA<AUnderpaintingHarvestNode>()) { return TEXT("harvest"); }
		if (Thing->IsA<AUnderpaintingPickup>() || Thing->IsA<AUnderpaintingCache>()) { return TEXT("pickup"); }
		if (Thing->IsA<AUnderpaintingLampstand>()) { return TEXT("rest"); }
		if (Thing->IsA<AUnderpaintingZoneExit>()) { return TEXT("door"); }
		if (Thing->IsA<AUnderpaintingLoreNote>() || Thing->IsA<AUnderpaintingInspectable>()) { return TEXT("read"); }
		return NAME_None;
	}
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
		const FName Sound = UseSoundFor(Focus.Get());
		Focus->Interact(this);
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->PlayFlat(Sound, 0.8f);
		}
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
		// A new game starts in Varnish, unless this is a test room.
		const UUnderpaintingDataSubsystem* ZoneData = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
		if (ZoneData && !ZoneData->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)).IsNone())
		{
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/Matriarch/L_Matriarch_Village_A")));
			return;
		}
		RestartLevel();
	}
}

void AUnderpaintingPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BindGameKeys();
	if (UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this))
	{
		KeysChangedHandle = Settings->OnKeysChanged.AddUObject(this, &AUnderpaintingPlayerCharacter::BindGameKeys);
	}
}

void AUnderpaintingPlayerCharacter::BindGameKeys()
{
	UInputComponent* Input = InputComponent;
	if (!Input)
	{
		return;
	}
	Input->KeyBindings.Reset();

	// The keys the player can change (the settings tab of the satchel). Moving, looking and jumping are Input Actions
	// in the engine's mapping asset and stay on WASD, the mouse and Space for now.
	const UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	const UUnderpaintingSettingsSave* Keys = Settings ? Settings->Read() : GetDefault<UUnderpaintingSettingsSave>();
	Input->BindKey(Keys->GetKey(TEXT("LightAttack")), IE_Pressed, this, &ThisClass::LightPressed);
	if (Keys->GetKey(TEXT("LightAttack")) != EKeys::LeftMouseButton)
	{
		Input->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ThisClass::MenuClickPressed);        // menus are always clickable
	}
	Input->BindKey(Keys->GetKey(TEXT("HeavyAttack")), IE_Pressed, this, &ThisClass::HeavyPressed);
	Input->BindKey(Keys->GetKey(TEXT("HeavyAttack")), IE_Released, this, &ThisClass::HeavyReleased);
	Input->BindKey(Keys->GetKey(TEXT("SwapWell")), IE_Pressed, this, &ThisClass::SwapWellPressed);
	Input->BindKey(Keys->GetKey(TEXT("Poultice")), IE_Pressed, this, &ThisClass::PoulticePressed);
	Input->BindKey(Keys->GetKey(TEXT("LampOil")), IE_Pressed, this, &ThisClass::LampOilPressed);
	Input->BindKey(Keys->GetKey(TEXT("Draught")), IE_Pressed, this, &ThisClass::DraughtPressed);
	Input->BindKey(Keys->GetKey(TEXT("Tincture")), IE_Pressed, this, &ThisClass::TincturePressed);
	Input->BindKey(Keys->GetKey(TEXT("Dodge")), IE_Pressed, this, &ThisClass::StartDodge);
	Input->BindKey(Keys->GetKey(TEXT("Sprint")), IE_Pressed, this, &ThisClass::SprintPressed);
	Input->BindKey(Keys->GetKey(TEXT("Sprint")), IE_Released, this, &ThisClass::SprintReleased);
	Input->BindKey(Keys->GetKey(TEXT("Use")), IE_Pressed, this, &ThisClass::InteractPressed);
	Input->BindKey(Keys->GetKey(TEXT("LockOn")), IE_Pressed, this, &ThisClass::LockOrClosePressed);
	Input->BindKey(Keys->GetKey(TEXT("Pack")), IE_Pressed, this, &ThisClass::InventoryPressed);
	Input->BindKey(Keys->GetKey(TEXT("Flare")), IE_Pressed, this, &ThisClass::FlarePressed);
	Input->BindKey(Keys->GetKey(TEXT("Flask")), IE_Pressed, this, &ThisClass::FlaskPressed);

	// Fixed: menus, the gamepad, and the debug keys (F5 save, F9 load, F8 new game, F7 fill the pack).
	Input->BindKey(EKeys::Left, IE_Pressed, this, &ThisClass::TabLeftPressed);
	Input->BindKey(EKeys::Right, IE_Pressed, this, &ThisClass::TabRightPressed);
	Input->BindKey(EKeys::Up, IE_Pressed, this, &ThisClass::CursorUpPressed);
	Input->BindKey(EKeys::Down, IE_Pressed, this, &ThisClass::CursorDownPressed);
	Input->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &ThisClass::TabLeftPressed);
	Input->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &ThisClass::TabRightPressed);
	Input->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ThisClass::InventoryPressed);
	Input->BindKey(EKeys::F5, IE_Pressed, this, &ThisClass::SavePressed);
	Input->BindKey(EKeys::F9, IE_Pressed, this, &ThisClass::LoadPressed);
	Input->BindKey(EKeys::F8, IE_Pressed, this, &ThisClass::NewGamePressed);
	Input->BindKey(EKeys::F7, IE_Pressed, this, &ThisClass::DevFillPressed);
	Input->BindKey(EKeys::One, IE_Pressed, this, &ThisClass::NumberPressed<1>);
	Input->BindKey(EKeys::Two, IE_Pressed, this, &ThisClass::NumberPressed<2>);
	Input->BindKey(EKeys::Three, IE_Pressed, this, &ThisClass::NumberPressed<3>);
	Input->BindKey(EKeys::Four, IE_Pressed, this, &ThisClass::NumberPressed<4>);
	Input->BindKey(EKeys::Five, IE_Pressed, this, &ThisClass::NumberPressed<5>);
	Input->BindKey(EKeys::Six, IE_Pressed, this, &ThisClass::NumberPressed<6>);
	Input->BindKey(EKeys::Seven, IE_Pressed, this, &ThisClass::NumberPressed<7>);
	Input->BindKey(EKeys::Eight, IE_Pressed, this, &ThisClass::NumberPressed<8>);
	Input->BindKey(EKeys::Nine, IE_Pressed, this, &ThisClass::NumberPressed<9>);
	Input->BindKey(EKeys::Gamepad_RightThumbstick, IE_Pressed, this, &ThisClass::ToggleLockOn);
	Input->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &ThisClass::FlarePressed);
	Input->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &ThisClass::InteractPressed);
	Input->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &ThisClass::StartLightAttack);
	Input->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ThisClass::HeavyPressed);
	Input->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Released, this, &ThisClass::HeavyReleased);
	Input->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ThisClass::StartDodge);
	Input->BindKey(EKeys::Gamepad_LeftThumbstick, IE_Pressed, this, &ThisClass::SprintPressed);
	Input->BindKey(EKeys::Gamepad_LeftThumbstick, IE_Released, this, &ThisClass::SprintReleased);
}

void AUnderpaintingPlayerCharacter::TabLeftPressed() { if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD()) { HUD->MoveTab(-1); } }
void AUnderpaintingPlayerCharacter::TabRightPressed() { if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD()) { HUD->MoveTab(1); } }
void AUnderpaintingPlayerCharacter::CursorUpPressed() { if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD()) { HUD->MoveCursor(-1); } }
void AUnderpaintingPlayerCharacter::CursorDownPressed() { if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD()) { HUD->MoveCursor(1); } }

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
	ArmourStaminaRegenPercent = Row->EffectType == EItemEffectType::StaminaRegenPercent ? Row->EffectValue : 0.0f;
}

bool AUnderpaintingPlayerCharacter::CanStartAction() const
{
	return Action == EPlayerAction::None && !GetCharacterMovement()->IsFalling() && !IsMenuOpen() && TurningTime < 0.0f;
}

void AUnderpaintingPlayerCharacter::StartLightAttack()
{
	if (!CanStartAction())
	{
		Buffer(EBuffered::Light);
	}
	else if (Vitals->TrySpendStamina(WeaponStaminaLight))
	{
		StartAttack(EPlayerAction::LightAttack);
	}
	else
	{
		Deny(false);
	}
}

void AUnderpaintingPlayerCharacter::StartHeavyAttack()
{
	if (!CanStartAction())
	{
		Buffer(EBuffered::Heavy);
	}
	else if (Vitals->TrySpendStamina(WeaponStaminaHeavy))
	{
		StartAttack(EPlayerAction::HeavyAttack);
	}
	else
	{
		Deny(false);
	}
}

// --- Feel: buffering, cancel windows, animation, aim ---

void AUnderpaintingPlayerCharacter::Buffer(EBuffered What)
{
	// Only a press made while busy with something is worth remembering; a press in a menu or in the air is not.
	if (Action != EPlayerAction::None && Action != EPlayerAction::Dead && !IsMenuOpen() && TurningTime < 0.0f)
	{
		Buffered = What;
		BufferedAt = GetWorld()->GetTimeSeconds();
	}
}

bool AUnderpaintingPlayerCharacter::CanCancelInto(EBuffered What) const
{
	if (GetCharacterMovement()->IsFalling() || IsMenuOpen())
	{
		return false;
	}
	const bool bSwing = Action == EPlayerAction::LightAttack || Action == EPlayerAction::HeavyAttack || Action == EPlayerAction::Stroke;
	if (bSwing && AttackClock.GetPhase() == EAttackPhase::Recovery)
	{
		// Never out of a wind-up or a live swing: you commit to the hit. Only the tail can be cut.
		const float Through = AttackClock.GetPhaseAlpha();
		const bool bLight = Action == EPlayerAction::LightAttack;
		if (What == EBuffered::Dodge)
		{
			return Through >= (bLight ? DodgeCancelLight : DodgeCancelHeavy);
		}
		return bLight && Through >= (What == EBuffered::Light ? LightChainAt : LightChainAt + 0.1f);
	}
	if (Action == EPlayerAction::Dodge)
	{
		return What != EBuffered::Dodge && DodgeElapsed >= DodgeDuration - 0.12f;
	}
	if (Action == EPlayerAction::Flinch)
	{
		return What == EBuffered::Dodge && FlinchRemaining <= FlinchTotal * 0.4f;
	}
	return false;
}

void AUnderpaintingPlayerCharacter::TryBuffered()
{
	if (Buffered == EBuffered::None)
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - BufferedAt > InputBufferTime || Action == EPlayerAction::Dead)
	{
		Buffered = EBuffered::None;
		return;
	}
	if (Action != EPlayerAction::None && !CanCancelInto(Buffered))
	{
		return;
	}
	if (Action != EPlayerAction::None)
	{
		EndAction();
	}
	if (!CanStartAction())
	{
		return;
	}
	const EBuffered What = Buffered;
	Buffered = EBuffered::None;
	switch (What)
	{
	case EBuffered::Light: StartLightAttack(); break;
	case EBuffered::Heavy: StartHeavyAttack(); break;
	case EBuffered::Stroke: if (!StartStroke()) { StartHeavyAttack(); } break;
	case EBuffered::Dodge: StartDodge(); break;
	default: break;
	}
}

void AUnderpaintingPlayerCharacter::LoadAnimations()
{
	// The mannequin pack that ships with the engine (docs/ASSETS.md). If it is not installed, the rules still work.
	const auto Load = [](const TCHAR* Path) { return LoadObject<UAnimSequenceBase>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet); };
	const auto Add = [&Load](TArray<TObjectPtr<UAnimSequenceBase>>& List, const TCHAR* Path)
	{
		if (UAnimSequenceBase* Animation = Load(Path))
		{
			List.Add(Animation);
		}
	};
	Add(LightAnims, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	Add(LightAnims, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
	Add(LightAnims, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
	HeavyAnim = Load(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack"));
	DashAnim = Load(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash"));
	for (const TCHAR* Path : { TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02.MM_HitReact_Front_Lgt_02"),
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_03.MM_HitReact_Front_Lgt_03"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_04.MM_HitReact_Front_Lgt_04") })
	{
		Add(HitAnimsLight, Path);
	}
	Add(HitAnimsMedium, TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01.MM_HitReact_Front_Med_01"));
	Add(HitAnimsMedium, TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_02.MM_HitReact_Front_Med_02"));
	HitAnimHeavy = Load(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
	HitAnimBack = Load(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01.MM_HitReact_Back_Med_01"));
	for (const TCHAR* Path : { TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_02.MM_Death_Front_02"),
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_03.MM_Death_Front_03") })
	{
		Add(DeathAnims, Path);
	}
	DeathBackAnim = Load(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
}

void AUnderpaintingPlayerCharacter::PlayAction(UAnimSequenceBase* Animation, float Duration)
{
	// The animation is stretched or squeezed to the action's real length, so the picture and the rule agree.
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (Animation && Anim)
	{
		Anim->PlaySlotAnimationAsDynamicMontage(Animation, TEXT("DefaultSlot"), 0.07f, 0.16f, Animation->GetPlayLength() / FMath::Max(Duration, 0.1f));
	}
}

void AUnderpaintingPlayerCharacter::StopActionAnimation()
{
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Anim->Montage_Stop(0.12f);
	}
}

void AUnderpaintingPlayerCharacter::Deny(bool bGlim)
{
	// Not enough breath, or not enough light: the bar says so, and so does a small sound.
	const float Now = GetWorld()->GetTimeSeconds();
	(bGlim ? Vitals->GlimDeniedAt : Vitals->StaminaDeniedAt) = Now;
	if (Now - LastDenyTime > 0.35f)
	{
		LastDenyTime = Now;
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->PlayFlat(TEXT("ui_deny"), 0.6f);
		}
	}
}

AUnderpaintingEnemy* AUnderpaintingPlayerCharacter::FindAimTarget() const
{
	// Where the player is pushing, or failing that where the Limner faces.
	FVector Wanted = GetLastMovementInputVector();
	Wanted.Z = 0.0f;
	Wanted = Wanted.IsNearlyZero() ? GetActorForwardVector() : Wanted.GetSafeNormal();

	AUnderpaintingEnemy* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		FVector To = It->GetActorLocation() - GetActorLocation();
		To.Z = 0.0f;
		const float Distance = To.Size();
		if (It->GetVitals()->IsDead() || It->IsHidden() || Distance > AimAssistRange)
		{
			continue;
		}
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Wanted, To.GetSafeNormal()), -1.0f, 1.0f)));
		if (Angle <= AimAssistAngle && Distance + Angle * 3.0f < BestScore)
		{
			Best = *It;
			BestScore = Distance + Angle * 3.0f;
		}
	}
	return Best;
}

void AUnderpaintingPlayerCharacter::HandleDamaged(float Damage, float StaggerDamage, AActor* DamageCauser)
{
	bHurtOnce = true;
	const FVector From = DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation() - GetActorForwardVector() * 100.0f;
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->NotePlayerHit(From, Damage);
		Feel->Shake(0.28f + Damage / 70.0f);
		Feel->HitStop(this, DamageCauser, 0.07f);
		Feel->Play(TEXT("player_hurt"), GetActorLocation(), FMath::Clamp(0.6f + Damage / 60.0f, 0.6f, 1.2f));
	}
	if (Vitals->GetHealth() <= 0.0f || Damage < 8.0f)
	{
		return;
	}

	// A hit knocks you out of what you were doing, unless you are mid-dodge or braced inside an Impasto (Lamp-Black Weight).
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const bool bBraced = Action == EPlayerAction::Stroke && StrokeName == TEXT("Impasto") && Progress && Progress->HasAttunement(TEXT("BoneBlack_T2"));
	if (Action == EPlayerAction::Dodge || bBraced)
	{
		return;
	}
	const bool bHard = Damage >= 22.0f;
	AttackClock.Stop();
	Action = EPlayerAction::Flinch;
	FlinchTotal = FlinchRemaining = bHard ? FlinchHeavy : FlinchLight;
	Buffered = EBuffered::None;
	Vitals->bInvulnerable = bDevInvulnerable;
	GetCharacterMovement()->MaxWalkSpeed = AttackMoveSpeed;

	FVector Away = GetActorLocation() - From;
	Away.Z = 0.0f;
	Away = Away.GetSafeNormal();
	const bool bFromBehind = FVector::DotProduct(GetActorForwardVector(), -Away) < -0.3f;
	UAnimSequenceBase* Animation = bFromBehind ? HitAnimBack.Get() : nullptr;
	if (!Animation)
	{
		const TArray<TObjectPtr<UAnimSequenceBase>>& List = bHard ? HitAnimsMedium : HitAnimsLight;
		Animation = bHard && Damage >= 28.0f && HitAnimHeavy ? HitAnimHeavy.Get() : (List.Num() > 0 ? List[FMath::RandRange(0, List.Num() - 1)].Get() : nullptr);
	}
	PlayAction(Animation, FlinchTotal + 0.2f);
	LaunchCharacter(Away * (bHard ? 420.0f : 240.0f) + FVector(0.0f, 0.0f, 40.0f), true, false);
}

void AUnderpaintingPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		const float Fall = -GetCharacterMovement()->Velocity.Z;
		Feel->Play(TEXT("land"), GetActorLocation(), FMath::Clamp(Fall / 900.0f, 0.4f, 1.2f));
		if (Fall > 900.0f)
		{
			Feel->Shake(0.22f);
		}
	}
}

void AUnderpaintingPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	bJumpedSinceGround = true;
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->Play(TEXT("jump"), GetActorLocation(), 0.7f);
	}
}

void AUnderpaintingPlayerCharacter::TickFeel(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this);
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	// Ledge grace: for a moment after walking off an edge, a jump still works.
	if (Movement->IsMovingOnGround())
	{
		LastGroundedTime = Now;
		bJumpedSinceGround = false;
	}
	JumpMaxCount = (!Movement->IsMovingOnGround() && !bJumpedSinceGround && Now - LastGroundedTime <= LedgeGraceTime) ? 2 : 1;

	// Footsteps, by distance covered on the ground.
	const float Moved = FVector::Dist2D(GetActorLocation(), LastStepLocation);
	LastStepLocation = GetActorLocation();
	if (Movement->IsMovingOnGround() && Action != EPlayerAction::Dodge && Moved < 200.0f)
	{
		StepDistance += Moved;
		const float Stride = Movement->MaxWalkSpeed > WalkSpeed + 10.0f ? 215.0f : 165.0f;
		if (StepDistance >= Stride)
		{
			StepDistance = 0.0f;
			if (Feel)
			{
				Feel->Play(FootstepSound, GetActorLocation() - FVector(0.0f, 0.0f, 85.0f), Movement->MaxWalkSpeed > WalkSpeed + 10.0f ? 0.85f : 0.6f);
			}
		}
	}

	// Locked on, the camera steps out over the shoulder and back a little, so the Limner does not stand in front of
	// what she is fighting. It eases there and eases home.
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		const bool bLocked = LockTarget.IsValid();
		Boom->SocketOffset = FMath::VInterpTo(Boom->SocketOffset, bLocked ? FVector(0.0f, 110.0f, 45.0f) : FVector::ZeroVector, DeltaTime, 4.0f);
		Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, bLocked ? 470.0f : 400.0f, DeltaTime, 4.0f);
	}

	// Camera shake rides on the camera itself, so the boom's lag does not smear it.
	if (UCameraComponent* Camera = GetFollowCamera())
	{
		Camera->SetRelativeLocationAndRotation(Feel ? Feel->GetShakeOffset() : FVector::ZeroVector, Feel ? Feel->GetShakeRotation() : FRotator::ZeroRotator);
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

	// Three lights in a row are three different swings.
	ComboIndex = (!bHeavy && GetWorld()->GetTimeSeconds() - LastSwingEndTime < 0.7f) ? ComboIndex + 1 : 0;
	UAnimSequenceBase* Animation = bHeavy ? HeavyAnim.Get() : (LightAnims.Num() > 0 ? LightAnims[ComboIndex % LightAnims.Num()].Get() : nullptr);
	PlayAction(Animation, AttackClock.Timing.Total());
	bSwingSounded = false;
	AimTarget = LockTarget.IsValid() ? LockTarget.Get() : FindAimTarget();
}

void AUnderpaintingPlayerCharacter::StartDodge()
{
	if (!CanStartAction())
	{
		Buffer(EBuffered::Dodge);
		return;
	}
	if (!Vitals->TrySpendStamina(DodgeStaminaCost))
	{
		Deny(false);
		return;
	}

	// Dodge the way the player is pushing; with no input, step back.
	FVector Direction = ForcedDodgeDirection.IsNearlyZero() ? GetLastMovementInputVector() : ForcedDodgeDirection;
	ForcedDodgeDirection = FVector::ZeroVector;
	Direction.Z = 0.0f;
	DodgeDirection = Direction.IsNearlyZero() ? -GetActorForwardVector() : Direction.GetSafeNormal();

	Action = EPlayerAction::Dodge;
	DodgeElapsed = 0.0f;
	bDodgeRewarded = false;
	Vitals->bInvulnerable = true;

	// Without lock-on the Limner turns into the dodge; locked on, she keeps facing what she is fighting.
	if (!LockTarget.IsValid())
	{
		SetActorRotation(FRotator(0.0f, DodgeDirection.Rotation().Yaw, 0.0f));
	}
	PlayAction(DashAnim, DodgeDuration);
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->Play(TEXT("dodge"), GetActorLocation(), 0.8f);
	}
}

void AUnderpaintingPlayerCharacter::StartDodgeInDirection(FVector Direction)
{
	ForcedDodgeDirection = Direction;
	StartDodge();
	ForcedDodgeDirection = FVector::ZeroVector;
}

void AUnderpaintingPlayerCharacter::DevClickAt(float U, float V)
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		HUD->DebugClick(U, V);
	}
}

void AUnderpaintingPlayerCharacter::DevAutopilot(const FString& Script)
{
	Autopilot->Run(Script);
}

FString AUnderpaintingPlayerCharacter::DevAutopilotStatus() const
{
	return Autopilot->Describe();
}

void AUnderpaintingPlayerCharacter::DevRecord(const FString& Name, float Seconds, float Fps)
{
	Autopilot->StartRecording(Name, Seconds, Fps);
}

void AUnderpaintingPlayerCharacter::SetSprinting(bool bInWantsSprint)
{
	bWantsSprint = bInWantsSprint;
}

// --- Teaching, and not getting stuck ---

void AUnderpaintingPlayerCharacter::HintDone(FName HintName)
{
	if (CurrentHint == HintName)
	{
		CurrentHint = NAME_None;
		if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
		{
			HUD->EndHintSoon();
		}
	}
}

void AUnderpaintingPlayerCharacter::TickHints(float DeltaTime)
{
	// What the player has just done ends the prompt that asked for it.
	if (!CurrentHint.IsNone())
	{
		const bool bSprinting = bWantsSprint && GetVelocity().SizeSquared2D() > FMath::Square(WalkSpeed + 20.0f);
		if ((CurrentHint == TEXT("Sprint") && bSprinting) || (CurrentHint == TEXT("LockOn") && LockTarget.IsValid())
			|| (CurrentHint == TEXT("Stroke") && Action == EPlayerAction::Stroke) || (CurrentHint == TEXT("Dodge") && Action == EPlayerAction::Dodge)
			|| ((CurrentHint == TEXT("Use") || CurrentHint == TEXT("Harvest") || CurrentHint == TEXT("Craft") || CurrentHint == TEXT("DarkLamp") || CurrentHint == TEXT("Rest")) && IsMenuOpen()))
		{
			HintDone(CurrentHint);
		}
	}

	// A lesson already learned by doing is never taught afterwards.
	if (bWantsSprint && GetVelocity().SizeSquared2D() > FMath::Square(WalkSpeed + 20.0f)) { LearnedByDoing.Add(TEXT("Sprint")); }
	if (LockTarget.IsValid()) { LearnedByDoing.Add(TEXT("LockOn")); }
	if (Action == EPlayerAction::Stroke) { LearnedByDoing.Add(TEXT("Stroke")); }
	if (FlareRemaining > 0.0f) { LearnedByDoing.Add(TEXT("Flare")); }

	WalkedTotal += GetVelocity().Size2D() * DeltaTime;
	bMenuSeen |= IsMenuOpen();
	HintClock += DeltaTime;
	if (HintClock < 0.25f)
	{
		return;
	}
	HintClock = 0.0f;

	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	const UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	if (!Data || !Progress || !HUD || !Settings || !Data->GetHints() || !Settings->Read()->bTeaching || Action == EPlayerAction::Dead || IsMenuOpen()
		|| Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)).IsNone())
	{
		return;        // test rooms teach nothing
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (HUD->IsHintShowing() ? Now - LastHintTime < 5.0f : Now - LastHintTime < 2.0f)
	{
		return;        // one thing at a time, and time to read it
	}

	// What is true right now.
	TSet<FName> True;
	TMap<FString, FString> Words;
	if (Now > 1.5f)
	{
		True.Add(TEXT("Start"));
	}
	if (WalkedTotal > 2500.0f)
	{
		True.Add(TEXT("Walked"));
	}
	if (bMenuSeen)
	{
		True.Add(TEXT("AfterFirstTalk"));
	}
	if (const AUnderpaintingInteractable* Usable = Focus.Get())
	{
		True.Add(TEXT("NearUsable"));
		if (Usable->IsA<AUnderpaintingHarvestNode>())
		{
			True.Add(TEXT("NearNode"));
		}
		else if (Usable->IsA<AUnderpaintingCraftingStation>())
		{
			True.Add(TEXT("NearStation"));
		}
		else if (const AUnderpaintingLampstand* Lampstand = Cast<AUnderpaintingLampstand>(Usable))
		{
			if (Lampstand->IsLit())
			{
				True.Add(TEXT("LitLamp"));
			}
			else if (Progress->GetItemCount(TEXT("LampOil")) > 0)
			{
				True.Add(TEXT("NearDarkLamp"));        // only once there is oil to pour: before that it is just a dark lamp
			}
		}
	}
	for (const TPair<FName, int32>& Wet : Progress->GetData().WetPigment)
	{
		if (Wet.Value > 0)
		{
			True.Add(TEXT("FirstHarvest"));
			break;
		}
	}
	const FName Active = Progress->GetActivePigment();
	const bool bAnyWell = !Progress->GetWell(0).IsNone() || !Progress->GetWell(1).IsNone();
	if (bAnyWell)
	{
		True.Add(TEXT("FirstWell"));
		const FName Shown = Active.IsNone() ? (Progress->GetWell(0).IsNone() ? Progress->GetWell(1) : Progress->GetWell(0)) : Active;
		if (const FPigmentRow* Pigment = Data->FindPigment(Shown))
		{
			Words.Add(TEXT("{Pigment}"), Pigment->DisplayName);
			Words.Add(TEXT("{Stroke}"), FName::NameToDisplayString(Pigment->StrokeName.ToString(), false));
		}
		if (GetStrokeGlimCost() > 0)
		{
			True.Add(TEXT("DarkZone"));
			Words.Add(TEXT("{Cost}"), FString::FromInt(GetStrokeGlimCost()));
			if (Vitals->GetGlim() < Vitals->MaxGlim * 0.3f)
			{
				True.Add(TEXT("LowGlim"));
			}
		}
	}
	if (!Progress->GetWell(0).IsNone() && !Progress->GetWell(1).IsNone())
	{
		True.Add(TEXT("TwoWells"));
	}
	if (bHurtOnce)
	{
		True.Add(TEXT("FirstHurt"));
	}
	if (Vitals->GetHealth() < Vitals->MaxHealth * 0.45f && Progress->GetItemCount(TEXT("MossPoultice")) > 0)
	{
		True.Add(TEXT("LowHealth"));
	}
	if (ZoneAmbientLight < 0.15f && Progress->GetItemCount(TEXT("PilotLantern")) > 0)
	{
		True.Add(TEXT("VaultDark"));
	}
	if (Progress->GetData().Deaths > 0)
	{
		True.Add(TEXT("AfterDeath"));
	}

	const AUnderpaintingEnemy* NearEnemy = nullptr;
	float NearEnemyDistance = FMath::Square(1300.0f);
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (It->GetState() != EEnemyState::Dead && !It->IsHidden() && Distance < NearEnemyDistance)
		{
			NearEnemy = *It;
			NearEnemyDistance = Distance;
		}
	}
	if (NearEnemy)
	{
		True.Add(TEXT("EnemyNear"));
		if (Progress->HasFlag(TEXT("Hint.Fight")))
		{
			True.Add(TEXT("EnemyStillNear"));
		}
		const TArray<FName> Weak = NearEnemy->GetActiveWeaknesses();
		const FPigmentRow* WeakTo = Weak.Num() > 0 ? Data->FindPigment(Weak[0]) : nullptr;
		if (WeakTo && bAnyWell)
		{
			True.Add(TEXT("Weakness"));
			Words.Add(TEXT("{Enemy}"), NearEnemy->GetDisplayName());
			Words.Add(TEXT("{Weakness}"), WeakTo->DisplayName);
		}
		if (NearEnemy->GetPoiseFraction() > 0.3f)
		{
			True.Add(TEXT("StaggerBar"));
		}
	}
	for (TActorIterator<AUnderpaintingPassableBack> It(GetWorld()); It; ++It)
	{
		if (FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) < FMath::Square(700.0f))
		{
			True.Add(TEXT("NearCanvas"));
			break;
		}
	}

	TSet<FName> Moot = LearnedByDoing;
	if (bMenuSeen) { Moot.Add(TEXT("Use")); }
	if (True.Contains(TEXT("FirstHarvest")) || Progress->IsLampstandLit(TEXT("Varnish"))) { Moot.Add(TEXT("Harvest")); }
	if (Progress->IsLampstandLit(TEXT("Varnish"))) { Moot.Add(TEXT("Craft")); Moot.Add(TEXT("DarkLamp")); }
	if (bAnyWell) { Moot.Add(TEXT("Rest")); }

	// The first prompt in the file whose moment this is, and which has not been shown.
	for (const TPair<FName, uint8*>& Pair : Data->GetHints()->GetRowMap())
	{
		const FHintRow* Row = reinterpret_cast<const FHintRow*>(Pair.Value);
		const FName Flag(*FString::Printf(TEXT("Hint.%s"), *Pair.Key.ToString()));
		if (!True.Contains(Row->Trigger) || Progress->HasFlag(Flag))
		{
			continue;
		}
		if (Moot.Contains(Pair.Key))
		{
			Progress->SetFlag(Flag);
			continue;
		}
		FString Text = Row->Text;
		for (const FUnderpaintingAction& Key : UUnderpaintingSettingsSave::Actions())
		{
			Text.ReplaceInline(*FString::Printf(TEXT("{%s}"), *Key.Id.ToString()), *FString::Printf(TEXT("[%s]"), *Settings->Read()->DescribeKey(Key.Id)));
		}
		for (const TPair<FString, FString>& Word : Words)
		{
			Text.ReplaceInline(*Word.Key, *Word.Value);
		}
		Progress->SetFlag(Flag);
		HUD->ShowHint(Text, Row->Seconds);
		CurrentHint = Pair.Key;
		LastHintTime = Now;
		UE_LOG(LogUnderpainting, Log, TEXT("HINT %s: %s"), *Pair.Key.ToString(), *Text);
		break;
	}
}

void AUnderpaintingPlayerCharacter::Checkpoint()
{
	if (Action == EPlayerAction::Dead)
	{
		return;
	}
	if (UUnderpaintingProgressSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr)
	{
		Saves->SaveGame(this, /*bQuiet*/ true);
	}
}

void AUnderpaintingPlayerCharacter::TickSafeGround(float DeltaTime)
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();

	// Remember where the ground was firm. Sketched platforms do not count: they are only there while a Flare burns.
	SafeSampleClock += DeltaTime;
	if (SafeSampleClock >= 0.5f && Movement->IsMovingOnGround() && Action != EPlayerAction::Dead)
	{
		SafeSampleClock = 0.0f;
		const AActor* Floor = Movement->CurrentFloor.HitResult.GetActor();
		if (Floor && !Floor->IsA<AUnderpaintingSketchPlatform>())
		{
			SafeTrail.Add(GetActorLocation());
			if (SafeTrail.Num() > 8)
			{
				SafeTrail.RemoveAt(0);
			}
		}
	}

	// A fall with no bottom ends back on the ledge, a little hurt, not in a restart.
	FallingTime = Movement->IsFalling() ? FallingTime + DeltaTime : 0.0f;
	if (FallingTime > 2.5f && SafeTrail.Num() > 0 && GetActorLocation().Z < SafeTrail[0].Z - 2000.0f)
	{
		ReturnToSafeGround(false);
	}

	// What you have and have done is written down every few seconds, wherever you will wake.
	AutosaveClock += DeltaTime;
	if (bProgressDirty && bInZoneLevel && AutosaveClock > 6.0f && Action != EPlayerAction::Dead)
	{
		AutosaveClock = 0.0f;
		bProgressDirty = false;
		if (UUnderpaintingProgressSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr)
		{
			Saves->SaveProgressOnly();
		}
	}
}

void AUnderpaintingPlayerCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	ReturnToSafeGround(false);        // never destroyed by the bottom of the world
}

void AUnderpaintingPlayerCharacter::ReturnToSafeGround(bool bByChoice)
{
	if (Action == EPlayerAction::Dead)
	{
		return;
	}
	FVector Where = SafeTrail.Num() > 0 ? SafeTrail[0] : GetActorLocation();
	const UUnderpaintingProgressSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	const UUnderpaintingSaveGame* Save = Saves ? Saves->GetLoadedSave() : nullptr;
	const APlayerStart* Start = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass()));
	if (bByChoice || SafeTrail.Num() == 0)
	{
		// By choice you go back to where you last woke in this zone: far enough to be out of whatever held you.
		if (Save && Saves->GetSavedLevel() == FName(*UGameplayStatics::GetCurrentLevelName(this, true)))
		{
			Where = Save->PlayerTransform.GetLocation();
		}
		else if (Start)
		{
			Where = Start->GetActorLocation();
		}
	}
	EndAction();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocation(Where + FVector(0.0f, 0.0f, 20.0f), false, nullptr, ETeleportType::TeleportPhysics);
	FallingTime = 0.0f;
	SafeTrail.Reset();
	SafeTrail.Add(Where);
	if (!bByChoice)
	{
		// Never the blow that kills.
		const float Cost = FMath::Min(15.0f, Vitals->GetHealth() - 1.0f);
		if (Cost > 0.0f)
		{
			Vitals->SetHealthAndGlim(Vitals->GetHealth() - Cost, Vitals->GetGlim());
		}
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->Shake(0.4f);
			Feel->PlayFlat(TEXT("player_hurt"), 0.7f);
		}
	}
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->OnNotice.Broadcast(bByChoice ? TEXT("Back where you last woke.") : TEXT("You fell. The last firm ground takes you back."));
	}
	UE_LOG(LogUnderpainting, Log, TEXT("Returned to safe ground (%s) at %s"), bByChoice ? TEXT("by choice") : TEXT("after a fall"), *Where.ToCompactString());
}

void AUnderpaintingPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TurningTime >= 0.0f)
	{
		// The ground moves under you for the first seconds; then the view is taken away (the HUD draws that).
		TurningTime += DeltaTime;
		if (AController* PlayerController = GetController())
		{
			const float Shake = FMath::Clamp(TurningTime / 1.5f, 0.0f, 1.0f) * 2.2f;
			if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
			{
				Feel->Shake(0.05f);        // topped up every frame: the ground does not stop
			}
			const float T = TurningTime * 23.0f;
			PlayerController->SetControlRotation(TurningBaseRotation + FRotator(FMath::Sin(T) * Shake, FMath::Sin(T * 0.7f + 1.3f) * Shake, 0.0f));
		}
		if (TurningTime >= TurningLength)
		{
			TurningTime = -1.0f;
			if (UUnderpaintingProgressSubsystem* Travel = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>())
			{
				if (!Travel->CompleteTurning(this))
				{
					EnableInput(Cast<APlayerController>(GetController()));        // nothing to reload here (a test room)
				}
			}
		}
		return;
	}

	DraughtRemaining = FMath::Max(0.0f, DraughtRemaining - DeltaTime);
	if (TinctureRemaining > 0.0f)
	{
		TinctureRemaining -= DeltaTime;
		if (TinctureRemaining <= 0.0f)
		{
			TinctureRemaining = 0.0f;
			GetCharacterMovement()->GravityScale = 1.0f;
		}
	}
	UpdateFocus();
	TickLockOn(DeltaTime);
	TickFeel(DeltaTime);
	TickSafeGround(DeltaTime);
	TickHints(DeltaTime);

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
	// It stutters through the seconds in which it can be renewed: that is the cue to press again.
	const float Gutter = FlareRemaining < FlareRenewWindow ? 0.72f + 0.28f * FMath::Sin(GetWorld()->GetTimeSeconds() * 21.0f) : 1.0f;
	FlareLight->SetIntensity(FlareRemaining > 0.0f ? 28.0f * FMath::Min(1.0f, FlareRemaining) * Gutter : 0.0f);

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

	case EPlayerAction::Flinch:
		FlinchRemaining -= DeltaTime;
		if (FlinchRemaining <= 0.0f)
		{
			EndAction();
		}
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

	TryBuffered();
}

void AUnderpaintingPlayerCharacter::TickAttack(float DeltaTime)
{
	// Through the wind-up the swing turns onto its target and, if it is just out of reach, steps in.
	if (AttackClock.GetPhase() == EAttackPhase::Windup)
	{
		if (const AUnderpaintingEnemy* Aim = AimTarget.Get())
		{
			FVector To = Aim->GetActorLocation() - GetActorLocation();
			To.Z = 0.0f;
			if (!LockTarget.IsValid())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, To.Rotation().Yaw, 0.0f), DeltaTime, 16.0f));
			}
			const bool bStepIn = To.Size() > WeaponReach * 0.9f && To.Size() < WeaponReach + 170.0f;
			GetCharacterMovement()->MaxWalkSpeed = bStepIn ? 340.0f : AttackMoveSpeed;
			if (bStepIn)
			{
				AddMovementInput(To.GetSafeNormal(), 1.0f);
			}
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = AttackMoveSpeed;
	}

	if (AttackClock.GetPhase() == EAttackPhase::Active && !bSwingSounded)
	{
		bSwingSounded = true;
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->Play(Action == EPlayerAction::Stroke ? TEXT("swing_stroke") : (Action == EPlayerAction::HeavyAttack ? TEXT("swing_heavy") : TEXT("swing_light")), GetActorLocation());
		}
	}

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
	if (UUnderpaintingFeel::bShowHitboxes)
	{
		FColor SwingColour = bHeavy ? FColor(232, 146, 58) : FColor(242, 227, 192);
		if (bStroke)
		{
			const UUnderpaintingDataSubsystem* ColourData = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
			const FPigmentRow* Pigment = ColourData ? ColourData->FindPigment(StrokePigment) : nullptr;
			SwingColour = Pigment ? FColor::FromHex(Pigment->ColourHex) : SwingColour;
		}
		DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + Radius, Radius,
			FRotationMatrix::MakeFromZ(End - Start).ToQuat(), SwingColour, false, 0.05f, 0, bStroke ? 4.0f : 1.0f);
	}
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
			// Loosened: 3 seconds, or 5 with Lucid (Slumber Blue tier 2).
			Enemy->ApplySlow(0.6f, Progress && Progress->HasAttunement(TEXT("SlumberBlue_T2")) ? 5.0f : 3.0f);
		}
		else if (StrokeName == TEXT("Scumble"))
		{
			Vitals->AddStamina(Progress && Progress->HasAttunement(TEXT("TerreVerte_T2")) ? 25.0f : 15.0f);
		}
		else if (StrokeName == TEXT("AllaPrima"))
		{
			Vitals->Heal(4.0f + (Progress && Progress->GetAttunementValue(TEXT("StrokeHealPerHit")) > 0.0f ? 2.0f : 0.0f));
		}
		else if (StrokeName == TEXT("Erase") && Enemy && Enemy->CancelAttack())
		{
			if (Progress) { Vitals->AddGlim(Progress->GetAttunementValue(TEXT("EraseGlimRefund"))); }
		}

		ApplyHit(Target, Damage, Stagger, *FString::Printf(TEXT("%s%s"), *StrokeName.ToString(), Contrast.Sign > 0 ? TEXT(" (contrast)") : (Contrast.Sign < 0 ? TEXT(" (muddy)") : TEXT(""))), Contrast.Sign);
		// Wet-on-Wet (Sanguine tier 2): a kill with Alla Prima gives its light back.
		const UUnderpaintingVitalsComponent* HitVitals = Target->FindComponentByClass<UUnderpaintingVitalsComponent>();
		if (StrokeName == TEXT("AllaPrima") && Progress && Progress->HasAttunement(TEXT("Sanguine_T2")) && HitVitals && HitVitals->IsDead())
		{
			Vitals->AddGlim(float(GetStrokeGlimCost()));
		}
		if (Progress && Contrast.Sign != 0)
		{
			Progress->OnNotice.Broadcast(Contrast.Sign > 0 ? TEXT("Contrast! +30%") : TEXT("Muddy. -30%"));
		}
	}
}

void AUnderpaintingPlayerCharacter::ApplyHit(AActor* Target, float Damage, float StaggerDamage, const TCHAR* What, int32 ContrastSign)
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
	if (DraughtRemaining > 0.0f)
	{
		Damage *= 1.0f + DraughtBonus;        // Sanguine Draught
	}

	const float Taken = TargetVitals->ApplyDamage(Damage, StaggerDamage, this);
	UE_LOG(LogUnderpainting, Log, TEXT("Player %s hit %s for %.1f (stagger %.1f)"), What, *Target->GetName(), Taken, StaggerDamage);

	// How the blow feels: a freeze, a shake, paint off the target, a number, a sound, and the target knocked back.
	if (Taken > 0.0f)
	{
		const bool bStrokeBlow = Action == EPlayerAction::Stroke;
		const bool bHeavyBlow = Action == EPlayerAction::HeavyAttack;
		AUnderpaintingEnemy* Struck = Cast<AUnderpaintingEnemy>(Target);
		FVector Away = Target->GetActorLocation() - GetActorLocation();
		Away.Z = 0.0f;
		Away = Away.GetSafeNormal();
		const FVector At = Target->GetActorLocation() - Away * 35.0f + FVector(0.0f, 0.0f, 25.0f);
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->HitStop(this, Target, (bStrokeBlow ? 0.11f : (bHeavyBlow ? 0.09f : 0.05f)) + (ContrastSign > 0 ? 0.04f : 0.0f));
			Feel->Shake(bStrokeBlow ? 0.38f : (bHeavyBlow ? 0.30f : 0.16f));

			// A plain blow knocks the target's own paint off it. A Stroke lays yours on.
			FLinearColor Paint = Struck ? Struck->GetPaintColour() : FLinearColor(0.9f, 0.85f, 0.7f);
			float Purity = Struck ? Struck->GetPaintPurity() : 0.0f;
			if (bStrokeBlow)
			{
				const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
				if (const FPigmentRow* Pigment = Data ? Data->FindPigment(StrokePigment) : nullptr)
				{
					Paint = UUnderpaintingDataSubsystem::HexToColour(Pigment->ColourHex);
					Purity = 0.0f;
				}
			}
			// It sprays back off the struck side and up, where it can be seen, not through the body.
			Feel->Splash(At - Away * 20.0f, (-Away * 0.7f + FVector(0.0f, 0.0f, 0.7f)).GetSafeNormal(), Paint, bStrokeBlow ? 20 : (bHeavyBlow ? 14 : 9), bStrokeBlow ? 460.0f : 380.0f, Purity);
			Feel->Number(At + FVector(0.0f, 0.0f, 70.0f), FString::FromInt(FMath::RoundToInt(Taken)),
				ContrastSign > 0 ? FLinearColor(1.0f, 0.72f, 0.2f) : (ContrastSign < 0 ? FLinearColor(0.5f, 0.48f, 0.46f) : FLinearColor(0.95f, 0.9f, 0.78f)),
				ContrastSign > 0 ? 1.5f : (bStrokeBlow || bHeavyBlow ? 1.2f : 1.0f));
			Feel->Play(bStrokeBlow ? TEXT("hit_stroke") : (bHeavyBlow ? TEXT("hit_heavy") : TEXT("hit_light")), At);
			if (ContrastSign > 0)
			{
				Feel->Play(TEXT("hit_contrast"), At);
			}
		}
		if (Struck)
		{
			Struck->ReceiveImpact(GetActorLocation(), bStrokeBlow ? 520.0f : (bHeavyBlow ? 420.0f : 240.0f));
		}
	}

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
	if (!Row)
	{
		return false;
	}
	if (!CanStartAction())
	{
		Buffer(EBuffered::Stroke);
		return Action != EPlayerAction::None;        // remembered: the caller should not fall back to a plain heavy
	}

	const int32 GlimCost = GetStrokeGlimCost();
	if (Vitals->GetGlim() < GlimCost)
	{
		Progress->OnNotice.Broadcast(TEXT("Not enough light for that Stroke."));
		Deny(true);
		return false;
	}
	if (!Vitals->TrySpendStamina(WeaponStaminaHeavy))
	{
		Deny(false);
		return true;        // out of breath: a plain heavy would fail the same way
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

	// Each Stroke borrows the swing that suits it: the slam, the flurry, the dash, the wipe.
	UAnimSequenceBase* Animation = HeavyAnim.Get();
	if (StrokeName == TEXT("Sfumato")) { Animation = DashAnim.Get(); }
	else if (StrokeName == TEXT("AllaPrima") && LightAnims.Num() > 0) { Animation = LightAnims[0].Get(); }
	else if (StrokeName == TEXT("Erase") && LightAnims.Num() > 1) { Animation = LightAnims[1].Get(); }
	else if ((StrokeName == TEXT("Glaze") || StrokeName == TEXT("Scumble")) && LightAnims.Num() > 2) { Animation = LightAnims[2].Get(); }
	PlayAction(Animation, AttackClock.Timing.Total());
	bSwingSounded = false;
	AimTarget = LockTarget.IsValid() ? LockTarget.Get() : FindAimTarget();
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

bool AUnderpaintingPlayerCharacter::UseDraught()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const FItemRow* Row = Progress ? Progress->GetItemRow(TEXT("SanguineDraught")) : nullptr;
	if (!Row || Action == EPlayerAction::Dead || IsMenuOpen())
	{
		return false;
	}
	if (!Progress->RemoveItem(TEXT("SanguineDraught"), 1))
	{
		Progress->OnNotice.Broadcast(TEXT("No Sanguine Draught."));
		return false;
	}
	DraughtBonus = Row->EffectValue / 100.0f;
	DraughtRemaining = Row->EffectDuration;
	Progress->OnNotice.Broadcast(FString::Printf(TEXT("Sanguine Draught. +%.0f%% damage for %.0f seconds"), Row->EffectValue, Row->EffectDuration));
	return true;
}

bool AUnderpaintingPlayerCharacter::UseTincture()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const FItemRow* Row = Progress ? Progress->GetItemRow(TEXT("BlueTincture")) : nullptr;
	if (!Row || Action == EPlayerAction::Dead || IsMenuOpen())
	{
		return false;
	}
	if (!Progress->RemoveItem(TEXT("BlueTincture"), 1))
	{
		Progress->OnNotice.Broadcast(TEXT("No Blue Tincture."));
		return false;
	}
	GetCharacterMovement()->GravityScale = Row->EffectValue;
	TinctureRemaining = Row->EffectDuration;
	Progress->OnNotice.Broadcast(FString::Printf(TEXT("Blue Tincture. You weigh less for %.0f seconds"), Row->EffectDuration));
	return true;
}

void AUnderpaintingPlayerCharacter::DevFill()
{
	UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	if (!Progress)
	{
		return;
	}
	for (const TCHAR* Pigment : { TEXT("Ochre"), TEXT("TerreVerte"), TEXT("Sanguine"), TEXT("BoneBlack"), TEXT("Ceruse"), TEXT("SlumberBlue") })
	{
		Progress->AddPigment(Pigment, 20, false);
	}
	for (const TCHAR* Material : { TEXT("LinseedOil"), TEXT("ShedHide"), TEXT("RibSplinter"), TEXT("TickChitin"), TEXT("GraphiteShard") })
	{
		Progress->AddItem(Material, 20);
	}
	Progress->SetFlag(TEXT("Dev.AllRecipes"));
	Progress->OnNotice.Broadcast(TEXT("DEBUG: pack filled, every recipe taught."));
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
		// The attacker hangs for a beat: the moment you slipped it should be felt.
		if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
		{
			Feel->HitStop(Attacker, nullptr, 0.14f);
			Feel->PlayFlat(TEXT("perfect_dodge"), 0.8f);
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
		// When the one you were fighting falls, the lock moves to the next one in view, if there is one.
		const bool bFell = Target->GetVitals()->IsDead();
		LockTarget = nullptr;
		if (bFell && Action != EPlayerAction::Dead)
		{
			ToggleLockOn();
		}
		return;
	}

	// The camera settles on the target, and (unless dodging) so does the Limner, so swings go where you are looking.
	if (AController* PlayerController = GetController())
	{
		// Aim from where the camera is (over the shoulder), not from the Limner, so the target sits in the open
		// middle of the view and she stands to one side of it.
		const UCameraComponent* Camera = GetFollowCamera();
		FRotator Wanted = Camera ? (Target->GetActorLocation() - Camera->GetComponentLocation()).Rotation() : To.Rotation();
		Wanted.Pitch = FMath::Clamp(To.Rotation().Pitch - 12.0f, -35.0f, 20.0f);
		// Quick when the target is well off-centre, gentle when it is nearly centred, so the view settles and does not twitch.
		const float Off = FMath::Abs(FRotator::NormalizeAxis(Wanted.Yaw - PlayerController->GetControlRotation().Yaw));
		PlayerController->SetControlRotation(FMath::RInterpTo(PlayerController->GetControlRotation(), Wanted, DeltaTime, FMath::GetMappedRangeValueClamped(FVector2D(2.0f, 40.0f), FVector2D(3.5f, 9.0f), Off)));
	}
	if (Action != EPlayerAction::Dodge && Action != EPlayerAction::Flinch && !IsMenuOpen())
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

	if (Action == EPlayerAction::LightAttack || Action == EPlayerAction::HeavyAttack || Action == EPlayerAction::Stroke)
	{
		LastSwingEndTime = GetWorld()->GetTimeSeconds();
		// Cut short (a cancel), the swing's animation is cut with it.
		if (AttackClock.bRunning)
		{
			StopActionAnimation();
		}
	}
	Action = EPlayerAction::None;
	AttackClock.Stop();
	AimTarget = nullptr;
	Vitals->bInvulnerable = bDevInvulnerable;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AUnderpaintingPlayerCharacter::HandleDeath(AActor* Killer)
{
	Action = EPlayerAction::Dead;
	AttackClock.Stop();
	Buffered = EBuffered::None;
	GetCharacterMovement()->DisableMovement();

	// Fall the way the blow sent you, and stay down.
	const bool bFromBehind = Killer && FVector::DotProduct(GetActorForwardVector(), (Killer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) < -0.3f;
	UAnimSequenceBase* Fall = bFromBehind && DeathBackAnim ? DeathBackAnim.Get() : (DeathAnims.Num() > 0 ? DeathAnims[FMath::RandRange(0, DeathAnims.Num() - 1)].Get() : nullptr);
	if (Fall && GetMesh())
	{
		GetMesh()->PlayAnimation(Fall, false);
	}
	if (UUnderpaintingFeel* Feel = UUnderpaintingFeel::Get(this))
	{
		Feel->Shake(0.6f);
		Feel->PlayFlat(TEXT("player_death"), 0.9f);
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}

	UE_LOG(LogUnderpainting, Log, TEXT("Player died to %s"), *GetNameSafe(Killer));
	if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
	{
		Progress->ApplyDeathPenalty();
	}

	// You wake where you last rested or arrived.
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AUnderpaintingPlayerCharacter::RestartLevel, 2.5f, false);
}

void AUnderpaintingPlayerCharacter::RestartLevel()
{
	// Wake where you last rested: reload the save's position (and its level, if it was another zone).
	UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>();
	const UUnderpaintingSaveGame* Save = Subsystem ? Subsystem->GetLoadedSave() : nullptr;
	if (Subsystem && Save && !Save->LevelName.IsNone())
	{
		Subsystem->SaveProgressOnly();        // what the death cost is written down too
		Subsystem->RequestRestore(Action == EPlayerAction::Dead);
		UGameplayStatics::OpenLevel(this, FName(*FString::Printf(TEXT("/Game/Maps/Matriarch/%s"), *Subsystem->GetSavedLevel().ToString())));
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

FString AUnderpaintingPlayerCharacter::DescribeState()
{
	UpdateFocus();
	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	FString Text = FString::Printf(TEXT("level=%s flare=%.1f prompt=[%s] menu=%d row=%s hp=%.0f/%.0f glim=%.0f/%.0f gear=%s+%s(-%.0f%%) regen x%.2f draught=%.0f tincture=%.0f gravity=%.1f"),
		*UGameplayStatics::GetCurrentLevelName(this, true), FlareRemaining,
		Focus.IsValid() ? *Focus->GetUsablePrompt() : TEXT(""), HUD ? int32(HUD->GetMenu()) : -1, HUD ? *HUD->GetDialogueRow().ToString() : TEXT(""),
		Vitals->GetHealth(), Vitals->MaxHealth, Vitals->GetGlim(), Vitals->MaxGlim, *WeaponId.ToString(), *ArmourId.ToString(), Vitals->DamageReduction * 100.0f,
		Vitals->StaminaRegenMultiplier, DraughtRemaining, TinctureRemaining, GetCharacterMovement()->GravityScale);
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

void AUnderpaintingPlayerCharacter::DevOpenSatchel(int32 Tab)
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		HUD->CloseMenu();
		HUD->ToggleInventory();
		for (int32 Guard = 0; Guard < 4 && HUD->GetSatchelTab() != Tab; ++Guard)
		{
			HUD->MoveTab(1);
		}
	}
}

void AUnderpaintingPlayerCharacter::DevMoveCursor(int32 Steps)
{
	if (AUnderpaintingHUD* HUD = GetUnderpaintingHUD())
	{
		for (int32 Step = 0; Step < FMath::Abs(Steps); ++Step)
		{
			HUD->MoveCursor(Steps > 0 ? 1 : -1);
		}
	}
}

bool AUnderpaintingPlayerCharacter::DevPressKey(FKey Key)
{
	AUnderpaintingHUD* HUD = GetUnderpaintingHUD();
	if (!HUD || !HUD->IsCapturingKey())
	{
		return false;
	}
	HUD->CaptureKey(Key);
	return true;
}

FString AUnderpaintingPlayerCharacter::DevDescribeKey(FName ActionId) const
{
	const UUnderpaintingSettings* Settings = UUnderpaintingSettings::Get(this);
	return Settings ? Settings->Read()->DescribeKey(ActionId) : FString();
}
