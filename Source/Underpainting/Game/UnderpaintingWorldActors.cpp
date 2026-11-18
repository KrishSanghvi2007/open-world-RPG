#include "UnderpaintingWorldActors.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingHUD.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingStyle.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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
		ChangedHandle = Progress->OnChanged.AddUObject(this, &AUnderpaintingInteractable::Refresh);
	}
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
		Progress->HarvestNode(GetNodeKey(), PigmentId, Yield);
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
	const bool bLit = IsLit();
	Flame->SetIntensity(bLit ? 14.0f : 1.5f);
	SetColour(bLit ? HexColour(TEXT("E8923A")) : HexColour(TEXT("4B4A48")));
}

// --- NPC ---

AUnderpaintingNPC::AUnderpaintingNPC()
{
	SetShape(CylinderPath, FVector(0.6f, 0.6f, 1.8f), FVector::ZeroVector);
}

void AUnderpaintingNPC::Refresh()
{
	SetColour(HexColour(TEXT("2E5E6E")));
}

FString AUnderpaintingNPC::GetPrompt() const
{
	return FString::Printf(TEXT("Talk to %s"), *DisplayName);
}

void AUnderpaintingNPC::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (AUnderpaintingHUD* HUD = GetHUD(Player))
	{
		HUD->OpenDialogue(NpcId);
	}
}
