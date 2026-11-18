#include "UnderpaintingExploration.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingHUD.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");

	FLinearColor Hex(const TCHAR* Text)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Text));
	}

	AUnderpaintingPlayerCharacter* GetPlayer(const UObject* WorldContext)
	{
		return Cast<AUnderpaintingPlayerCharacter>(UGameplayStatics::GetPlayerPawn(WorldContext, 0));
	}

	FName KeyFor(const AActor* Actor)
	{
		return FName(*FString::Printf(TEXT("%s.%s"), *Actor->GetWorld()->GetMapName(), *Actor->GetName()));
	}
}

// --- Zone exit ---

AUnderpaintingZoneExit::AUnderpaintingZoneExit()
{
	SetShape(CubeMesh, FVector(0.3f, 4.0f, 4.0f), FVector::ZeroVector);
	Body->SetCollisionProfileName(TEXT("NoCollision"));
}

FString AUnderpaintingZoneExit::GetPrompt() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UUnderpaintingDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FRegionRow* Target = Data ? Data->FindRegion(TargetZone) : nullptr;
	return Target ? FString::Printf(TEXT("Go to %s"), *Target->DisplayName) : FString();
}

void AUnderpaintingZoneExit::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (UUnderpaintingProgressSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UUnderpaintingProgressSubsystem>())
	{
		Subsystem->TravelToZone(this, TargetZone);
	}
}

void AUnderpaintingZoneExit::Refresh()
{
	SetColour(Hex(TEXT("2F4BA8")));
}

// --- Place ---

AUnderpaintingPlace::AUnderpaintingPlace()
{
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	SetRootComponent(Volume);
	Volume->SetBoxExtent(FVector(500.0f, 500.0f, 300.0f));
	Volume->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Volume->OnComponentBeginOverlap.AddDynamic(this, &AUnderpaintingPlace::HandleOverlap);
}

void AUnderpaintingPlace::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AUnderpaintingPlayerCharacter>(OtherActor))
	{
		if (UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this))
		{
			Progress->ReachPlace(PlaceId, DisplayName);
		}
	}
}

// --- Sketch platform ---

AUnderpaintingSketchPlatform::AUnderpaintingSketchPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CubeMesh);
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
	}
}

FName AUnderpaintingSketchPlatform::GetPlatformKey() const
{
	return KeyFor(this);
}

void AUnderpaintingSketchPlatform::BeginPlay()
{
	Super::BeginPlay();
	BodyMaterial = Body->CreateAndSetMaterialInstanceDynamic(0);
	Tick(0.0f);
}

void AUnderpaintingSketchPlatform::SetSolid(bool bNewSolid, bool bFixed)
{
	if (bApplied && bNewSolid == bSolid)
	{
		return;
	}
	bApplied = true;
	bSolid = bNewSolid;
	Body->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (BodyMaterial)
	{
		// Solid: the one blue of the Vault. Only drawn: a pale ghost of itself. Fixed: touched with amber.
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), bFixed ? Hex(TEXT("5A6FB8")) : (bSolid ? Hex(TEXT("2F4BA8")) : Hex(TEXT("C9C6BE"))));
	}
	Body->SetRenderCustomDepth(!bSolid);
}

void AUnderpaintingSketchPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
	const bool bFixed = Progress && Progress->IsPlatformFixed(GetPlatformKey());
	const AUnderpaintingPlayerCharacter* Player = GetPlayer(this);
	const bool bLit = Player && Player->IsFlareLighting(GetActorLocation());
	SetSolid(bFixed || bLit, bFixed);
}

// --- Passable back ---

AUnderpaintingPassableBack::AUnderpaintingPassableBack()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CubeMesh);
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
	}
	Body->SetCollisionProfileName(TEXT("BlockAll"));
}

void AUnderpaintingPassableBack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	// Work in the wall's own space: +X is its painted front.
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Player->GetActorLocation());
	const FVector HalfSize = GetActorScale3D() * 50.0f + FVector(60.0f, 60.0f, 200.0f);
	const bool bInside = FMath::Abs(Local.X) < HalfSize.X && FMath::Abs(Local.Y) < HalfSize.Y && FMath::Abs(Local.Z) < HalfSize.Z;

	// Once you are in it, it stays open until you are out, so you can never be caught in a wall.
	if (!bInside)
	{
		bOpen = Local.X < 0.0f;
	}
	Body->SetCollisionResponseToChannel(ECC_Pawn, bOpen ? ECR_Ignore : ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

// --- Lore note ---

AUnderpaintingLoreNote::AUnderpaintingLoreNote()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	SetShape(CubeMesh, FVector(0.05f, 0.5f, 0.65f), FVector::ZeroVector);
	Body->SetCollisionProfileName(TEXT("NoCollision"));
}

FName AUnderpaintingLoreNote::RevealFlag() const
{
	return FName(*FString::Printf(TEXT("Revealed.%s"), *LoreId.ToString()));
}

bool AUnderpaintingLoreNote::IsRevealed() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const FLoreRow* Row = Progress ? Progress->GetLore(LoreId) : nullptr;
	return Row && (Row->Kind != ELoreKind::Marginalia || Progress->HasFlag(RevealFlag()));
}

void AUnderpaintingLoreNote::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsRevealed())
	{
		const AUnderpaintingPlayerCharacter* Player = GetPlayer(this);
		UUnderpaintingProgress* Progress = GetProgress();
		if (Player && Progress && Player->IsFlareLighting(GetActorLocation()))
		{
			Progress->SetFlag(RevealFlag());
			Progress->OnNotice.Broadcast(TEXT("Writing shows in the light."));
		}
	}
}

FString AUnderpaintingLoreNote::GetPrompt() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const FLoreRow* Row = Progress ? Progress->GetLore(LoreId) : nullptr;
	return Row && IsRevealed() ? FString::Printf(TEXT("Read: %s"), *Row->Title) : FString();
}

void AUnderpaintingLoreNote::Interact(AUnderpaintingPlayerCharacter* Player)
{
	UUnderpaintingProgress* Progress = GetProgress();
	if (!Progress || !IsRevealed())
	{
		return;
	}
	Progress->FindLore(LoreId);
	const APlayerController* Controller = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (AUnderpaintingHUD* HUD = Controller ? Cast<AUnderpaintingHUD>(Controller->GetHUD()) : nullptr)
	{
		HUD->OpenReading(LoreId);
	}
}

void AUnderpaintingLoreNote::Refresh()
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const FLoreRow* Row = Progress ? Progress->GetLore(LoreId) : nullptr;
	const bool bRevealed = IsRevealed();
	Body->SetVisibility(bRevealed);

	// Read notes fade; unread ones are warm paper (Odile), grey ledger, or the Hand's blue.
	FLinearColor Colour = Hex(TEXT("F2E3C0"));
	if (Row && Row->Kind == ELoreKind::Marginalia) { Colour = Hex(TEXT("2F4BA8")); }
	if (Row && Row->Kind == ELoreKind::Ledger) { Colour = Hex(TEXT("8C8A85")); }
	SetColour(Progress && Progress->HasFoundLore(LoreId) ? Colour * 0.4f : Colour);
}

// --- Cache ---

AUnderpaintingCache::AUnderpaintingCache()
{
	SetShape(CubeMesh, FVector(0.8f, 0.8f, 0.6f), FVector::ZeroVector);
}

FName AUnderpaintingCache::GetCacheKey() const
{
	return KeyFor(this);
}

FString AUnderpaintingCache::GetPrompt() const
{
	const UUnderpaintingProgress* Progress = GetProgress();
	return Progress && !Progress->IsCacheOpened(GetCacheKey()) ? TEXT("Open the cache") : FString();
}

void AUnderpaintingCache::Interact(AUnderpaintingPlayerCharacter* Player)
{
	if (UUnderpaintingProgress* Progress = GetProgress())
	{
		Progress->OpenCache(GetCacheKey(), Contents);
	}
}

void AUnderpaintingCache::Refresh()
{
	const UUnderpaintingProgress* Progress = GetProgress();
	const bool bOpened = Progress && Progress->IsCacheOpened(GetCacheKey());
	SetColour(bOpened ? Hex(TEXT("3A3A3A")) : Hex(TEXT("C4574B")));
	Body->SetRelativeScale3D(bOpened ? FVector(0.8f, 0.8f, 0.2f) : FVector(0.8f, 0.8f, 0.6f));
}
