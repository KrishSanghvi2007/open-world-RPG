#include "UnderpaintingWorldActors.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingExploration.h"
#include "EngineUtils.h"
#include "UnderpaintingHUD.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingStyle.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	FLinearColor HexColour(const FString& Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	AUnderpaintingHUD* GetHUD(const AUnderpaintingPlayerCharacter* Player)
	{
		const APlayerController* Controller = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
		return Controller ? Cast<AUnderpaintingHUD>(Controller->GetHUD()) : nullptr;
	}
}

// --- Base ---

AUnderpaintingInteractable::AUnderpaintingInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
}

void AUnderpaintingInteractable::SetShape(const TCHAR* MeshPath, const FVector& Scale, const FVector& Offset)
{
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(MeshPath);
	if (Mesh.Succeeded())
	{
		Body->SetStaticMesh(Mesh.Object);
	}
	Body->SetRelativeScale3D(Scale);
	Body->SetRelativeLocation(Offset);
}

void AUnderpaintingInteractable::SetColour(const FLinearColor& Colour)
{
	if (!BodyMaterial)
	{
		BodyMaterial = UnderpaintingStyle::MakeMaterial(Body);
	}
	UnderpaintingStyle::SetColour(BodyMaterial, Colour);
}

UUnderpaintingProgress* AUnderpaintingInteractable::GetProgress() const
{
	return UUnderpaintingProgressSubsystem::Get(this);
}

void AUnderpaintingInteractable::BeginPlay()
{
	Super::BeginPlay();
	if (UUnderpaintingProgress* Progress = GetProgress())
	{
		ChangedHandle = Progress->OnChanged.AddUObject(this, &AUnderpaintingInteractable::HandleProgressChanged);
	}
	HandleProgressChanged();
}

bool AUnderpaintingInteractable::IsAvailable() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	if (!Progress)
	{
		return true;
	}
	return (RequiresFlag.IsNone() || Progress->HasFlag(RequiresFlag)) && (HiddenByFlag.IsNone() || !Progress->HasFlag(HiddenByFlag));
}

void AUnderpaintingInteractable::HandleProgressChanged()
{
	const bool bAvailable = IsAvailable();
	SetActorHiddenInGame(!bAvailable);
	SetActorEnableCollision(bAvailable);
	Refresh();
}

void AUnderpaintingInteractable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUnderpaintingProgress* Progress = GetProgress())
	{
		Progress->OnChanged.Remove(ChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

// --- Harvest node ---

AUnderpaintingHarvestNode::AUnderpaintingHarvestNode()
{
	SetShape(SpherePath, FVector(0.8f), FVector::ZeroVector);
	Body->SetCollisionProfileName(TEXT("NoCollision"));
}

FName AUnderpaintingHarvestNode::GetNodeKey() const
{
	return FName(*FString::Printf(TEXT("%s.%s"), *GetWorld()->GetMapName(), *GetName()));
}

FString AUnderpaintingHarvestNode::GetPrompt() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	if (!Progress || Progress->IsNodeDepleted(GetNodeKey()))
	{
		return FString();
	}
	return FString::Printf(TEXT("Harvest %s"), *Progress->GetDisplayName(PigmentId));
}

void AUnderpaintingHarvestNode::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (UUnderpaintingProgress* Progress = GetProgress())
	{
		const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
		const FName Zone = Data ? Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(this, true)) : NAME_None;
		Progress->HarvestNode(GetNodeKey(), PigmentId, Progress->GetNodeYield(Zone, PigmentId, Yield));
	}
}

void AUnderpaintingHarvestNode::Refresh()
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const bool bEmpty = Progress && Progress->IsNodeDepleted(GetNodeKey());

	FLinearColor Colour(0.5f, 0.5f, 0.5f);
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	if (const FPigmentRow* Pigment = Data ? Data->FindPigment(PigmentId) : nullptr)
	{
		Colour = UUnderpaintingDataSubsystem::HexToColour(Pigment->ColourHex);
	}

	// An empty node is a faded stub of itself.
	SetColour(bEmpty ? Colour * 0.25f + FLinearColor(0.2f, 0.2f, 0.2f) : Colour);
	Body->SetRelativeScale3D(FVector(bEmpty ? 0.35f : 0.8f));
}

// --- Pickup ---

AUnderpaintingPickup::AUnderpaintingPickup()
{
	SetShape(CylinderPath, FVector(1.2f, 1.2f, 1.0f), FVector::ZeroVector);
}

FName AUnderpaintingPickup::GetNodeKey() const
{
	return FName(*FString::Printf(TEXT("%s.%s"), *GetWorld()->GetMapName(), *GetName()));
}

FString AUnderpaintingPickup::GetPrompt() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	if (!Progress || Progress->IsNodeDepleted(GetNodeKey()))
	{
		return FString();
	}
	return FString::Printf(TEXT("Take %s from %s"), *Progress->GetDisplayName(ItemId), *SourceName);
}

void AUnderpaintingPickup::Interact(AUnderpaintingPlayerCharacter* Player)
{
	UUnderpaintingProgress* Progress = GetProgress();
	if (Progress && !Progress->IsNodeDepleted(GetNodeKey()) && Progress->AddItem(ItemId, Count) > 0)
	{
		// Reuses the node list so it refills on rest like everything else that regrows.
		FUnderpaintingProgressData Data = Progress->GetData();
		Data.DepletedNodes.Add(GetNodeKey());
		Progress->SetData(Data);
	}
}

void AUnderpaintingPickup::Refresh()
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const bool bEmpty = Progress && Progress->IsNodeDepleted(GetNodeKey());
	SetColour(bEmpty ? FLinearColor(0.12f, 0.10f, 0.08f) : HexColour(TEXT("6B4F3A")));
}

// --- Crafting station ---

AUnderpaintingCraftingStation::AUnderpaintingCraftingStation()
{
	SetShape(CubePath, FVector(1.6f, 1.0f, 0.9f), FVector::ZeroVector);
}

void AUnderpaintingCraftingStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetColour(Station == ECraftingStation::GrindingSlab ? HexColour(TEXT("8C8A85")) : HexColour(TEXT("6B4F3A")));
}

FString AUnderpaintingCraftingStation::GetPrompt() const
{
	return Station == ECraftingStation::GrindingSlab ? TEXT("Use the Grinding Slab") : TEXT("Use the Armature Bench");
}

void AUnderpaintingCraftingStation::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (AUnderpaintingHUD* HUD = GetHUD(Player))
	{
		HUD->OpenCrafting(Station);
	}
}

// --- Lampstand ---

AUnderpaintingLampstand::AUnderpaintingLampstand()
{
	SetShape(CylinderPath, FVector(0.4f, 0.4f, 3.0f), FVector::ZeroVector);

	Flame = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Body);
	Flame->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Flame->SetCastShadows(false);
	Flame->SetLightColor(FLinearColor(0.80f, 0.29f, 0.04f));
	Flame->SetAttenuationRadius(1200.0f);
	Flame->bUseInverseSquaredFalloff = false;
	Flame->SetIntensity(0.0f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
}

bool AUnderpaintingLampstand::IsLit() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	return bStartsLit || (Progress && Progress->IsLampstandLit(LampstandId));
}

FString AUnderpaintingLampstand::GetPrompt() const
{
	return IsLit() ? TEXT("Rest at the Lampstand") : TEXT("Pour Lamp Oil into the Lampstand");
}

void AUnderpaintingLampstand::Interact(AUnderpaintingPlayerCharacter* Player)
{
	UUnderpaintingProgress* Progress = GetProgress();
	if (!Progress)
	{
		return;
	}

	if (!IsLit())
	{
		Progress->RelightLampstand(LampstandId);
		return;
	}

	Progress->Rest();
	if (Player)
	{
		Player->OnRested();
	}
	if (AUnderpaintingHUD* HUD = GetHUD(Player))
	{
		HUD->OpenAttunement();
	}
}

void AUnderpaintingLampstand::Refresh()
{
	// A lit Lampstand is the brightest warm thing in sight: it is how you find your way. Its light reaches far and casts no shadow.
	const bool bLit = IsLit();
	Flame->SetAttenuationRadius(bLit ? 2600.0f : 1200.0f);
	Flame->SetIntensity(bLit ? 22.0f : 1.5f);
	SetColour(bLit ? HexColour(TEXT("E8923A")) : HexColour(TEXT("4B4A48")));
}

void AUnderpaintingLampstand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// A lit flame breathes; a guttering one stutters, which is what catches the eye from across a clearing.
	const float T = GetWorld()->GetTimeSeconds() + GetActorLocation().X * 0.01f;
	if (IsLit())
	{
		Flame->SetIntensity(22.0f * (0.93f + 0.07f * FMath::Sin(T * 2.3f)));
	}
	else
	{
		const float Stutter = FMath::PerlinNoise1D(T * 3.1f);
		Flame->SetIntensity(Stutter > 0.25f ? 5.0f : 0.8f);
	}
}

// --- NPC ---

AUnderpaintingNPC::AUnderpaintingNPC()
{
	SetShape(CylinderPath, FVector(0.6f, 0.6f, 1.8f), FVector::ZeroVector);
	PrimaryActorTick.bCanEverTick = true;
}

void AUnderpaintingNPC::BeginPlay()
{
	if (!FigureId.IsNone())
	{
		// One of the Folded: a real model stands where the grey shape was. The shape stays, unseen, to be bumped into.
		USkeletalMeshComponent* Model = NewObject<USkeletalMeshComponent>(this, TEXT("Figure"));
		Model->SetUsingAbsoluteLocation(true);         // the grey shape is stretched; the model must not be
		Model->SetUsingAbsoluteRotation(true);
		Model->SetUsingAbsoluteScale(true);
		Model->SetupAttachment(GetRootComponent());
		Model->RegisterComponent();
		if (Figure.Setup(Model, FigureId))
		{
			PlaceFigure();
			Body->SetVisibility(false);
			Figure.Play(TEXT("Idle"));

			// The face they are given if they are set: a plain warm disc, like a small moon, where the bare canvas was.
			Face = NewObject<UStaticMeshComponent>(this, TEXT("Face"));
			Face->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
			Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Face->SetCastShadow(false);
			Face->SetUsingAbsoluteLocation(true);          // the model's bones carry a scale of their own: keep the face its own size
			Face->SetUsingAbsoluteScale(true);
			Face->SetupAttachment(Model);
			Face->RegisterComponent();
			Face->SetWorldScale3D(FVector(0.26f, 0.26f, 0.3f));
			UnderpaintingStyle::SetColour(UnderpaintingStyle::MakeMaterial(Face), HexColour(TEXT("D9A441")), 0.0f);
			Face->SetVisibility(false);
		}
		else
		{
			Model->DestroyComponent();
		}
	}
	FacingYaw = GetActorRotation().Yaw;
	Super::BeginPlay();
}

void AUnderpaintingNPC::PlaceFigure()
{
	// Feet on the ground under the grey shape (which is 1.8 m tall and centred on the actor), facing the way the actor faces.
	Figure.Mesh->SetWorldLocation(GetActorLocation() - FVector(0.0f, 0.0f, 90.0f));
	Figure.Mesh->SetWorldRotation(FRotator(0.0f, GetActorRotation().Yaw - 90.0f, 0.0f));       // made facing -y; faces +x here
	Figure.Mesh->SetWorldScale3D(FVector::OneVector);
}

bool AUnderpaintingNPC::IsSet() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	return !SetFlag.IsNone() && Progress && Progress->HasFlag(SetFlag);
}

void AUnderpaintingNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Figure.IsValid())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (IsSet())
	{
		Figure.Play(TEXT("Still"));            // finished: one thing, for good
		if (Face)
		{
			Face->SetWorldLocation(Figure.Mesh->GetSocketLocation(TEXT("face")) + GetActorForwardVector() * 7.0f);
		}
		return;
	}

	// Turn to whoever is close, slowly, unless told to keep still. Otherwise go back to work.
	if (bTurnsToPlayer)
	{
		const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		const bool bNear = Player && FVector::DistSquared2D(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(450.0f);
		const float Wanted = bNear ? (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw : FacingYaw;
		FRotator Turned = GetActorRotation();
		Turned.Yaw = FMath::FixedTurn(Turned.Yaw, Wanted, 70.0f * DeltaTime);
		SetActorRotation(Turned);
		PlaceFigure();
	}

	// Now and then, one of the things they do with their hands.
	if (Now < GestureEndsAt)
	{
		return;
	}
	if (Gestures.Num() > 0 && Now >= NextGestureAt)
	{
		const FName Gesture = Gestures[FMath::RandRange(0, Gestures.Num() - 1)];
		if (Figure.Play(Gesture, false, 1.0f, true))
		{
			GestureEndsAt = Now + Figure.Length(Gesture);
		}
		NextGestureAt = Now + FMath::FRandRange(9.0f, 16.0f);
		return;
	}
	Figure.Play(TEXT("Idle"));
}

void AUnderpaintingNPC::Refresh()
{
	SetColour(HexColour(TEXT("2E5E6E")));
	if (Face)
	{
		Face->SetVisibility(IsSet());
	}
}

FString AUnderpaintingNPC::GetPrompt() const
{
	// While a decision is waiting on this very person (a choice set at their face), that is what you reach for, not talk.
	for (TActorIterator<AUnderpaintingChoicePoint> It(GetWorld()); It; ++It)
	{
		if (!It->GetUsablePrompt().IsEmpty() && FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) < FMath::Square(170.0f))
		{
			return FString();
		}
	}
	return FString::Printf(TEXT("Talk to %s"), *DisplayName);
}

void AUnderpaintingNPC::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (AUnderpaintingHUD* HUD = GetHUD(Player))
	{
		HUD->OpenDialogue(NpcId);
	}
}
