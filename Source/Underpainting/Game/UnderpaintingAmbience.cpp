#include "UnderpaintingAmbience.h"

#include "UnderpaintingStyle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const TCHAR* AmbienceCube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* AmbienceSphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/** Keeps a value inside -Half..Half by wrapping it round. */
	float Wrap(float Value, float Half)
	{
		if (Half <= 0.0f)
		{
			return 0.0f;
		}
		const float Span = Half * 2.0f;
		float Result = FMath::Fmod(Value + Half, Span);
		if (Result < 0.0f)
		{
			Result += Span;
		}
		return Result - Half;
	}
}

AUnderpaintingAmbience::AUnderpaintingAmbience()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UStaticMeshComponent* AUnderpaintingAmbience::AddBit(const TCHAR* MeshPath)
{
	UStaticMeshComponent* Bit = NewObject<UStaticMeshComponent>(this);
	Bit->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath));
	Bit->SetMobility(EComponentMobility::Movable);
	Bit->SetCollisionProfileName(TEXT("NoCollision"));
	Bit->SetCastShadow(false);
	Bit->SetUsingAbsoluteLocation(true);
	Bit->SetUsingAbsoluteRotation(true);
	Bit->SetUsingAbsoluteScale(true);
	Bit->SetupAttachment(GetRootComponent());
	Bit->RegisterComponent();
	// One material shared by every piece, so the renderer can draw them together.
	if (Materials.Num() > 0)
	{
		Bit->SetMaterial(0, Materials[0]);
	}
	else if (UMaterialInstanceDynamic* Material = UnderpaintingStyle::MakeMaterial(Bit))
	{
		// Smoke is flat and soft; the rest keep a little brushwork.
		UnderpaintingStyle::SetColour(Material, Colour, Kind == EAmbienceKind::Smoke ? 0.9f : 0.6f);
		Materials.Add(Material);
	}
	Bits.Add(Bit);
	return Bit;
}

void AUnderpaintingAmbience::BeginPlay()
{
	Super::BeginPlay();

	FRandomStream Random(GetTypeHash(GetActorLocation()) + static_cast<int32>(Kind) * 7919);
	const int32 PerThing = (Kind == EAmbienceKind::Birds) ? 2 : 1;       // a bird is two wings
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Seeds.Add(FVector(Random.FRand(), Random.FRand(), Random.FRand()));
		for (int32 Part = 0; Part < PerThing; ++Part)
		{
			AddBit((Kind == EAmbienceKind::Smoke || Kind == EAmbienceKind::Motes || Kind == EAmbienceKind::Dust) ? AmbienceSphere : AmbienceCube);
		}
	}
	Clock = Random.FRandRange(0.0f, 100.0f);
}

void AUnderpaintingAmbience::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Clock += DeltaTime * Speed;

	const FVector Centre = GetActorLocation();
	for (int32 Index = 0; Index < Seeds.Num(); ++Index)
	{
		const FVector& Seed = Seeds[Index];
		const float Own = Seed.X * 6.2832f;        // each thing's own place in its cycle

		switch (Kind)
		{
		case EAmbienceKind::Birds:
		{
			// A loose circle, each bird at its own height and distance, beating its wings and gliding by turns.
			const float Radius = Extent.X * (0.45f + 0.55f * Seed.Y);
			const float Angle = Clock * (0.10f + 0.06f * Seed.Z) + Own;
			const FVector Place = Centre + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius * (Extent.Y / FMath::Max(Extent.X, 1.0f)),
				Extent.Z * (Seed.Z * 2.0f - 1.0f) + FMath::Sin(Clock * 0.4f + Own) * 80.0f);
			const float Heading = FMath::RadiansToDegrees(Angle) + 90.0f;
			const float Glide = FMath::Clamp(FMath::Sin(Clock * 0.5f + Own * 3.0f) * 2.0f + 0.6f, 0.0f, 1.0f);      // 0 while gliding
			const float Beat = FMath::Sin(Clock * 7.0f + Own * 5.0f) * 38.0f * Glide + 8.0f;
			const float Span = 55.0f * Size;
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const float Sign = Side == 0 ? -1.0f : 1.0f;
				const FRotator Turn(0.0f, Heading, Sign * Beat);
				UStaticMeshComponent* Wing = Bits[Index * 2 + Side];
				Wing->SetWorldLocationAndRotation(Place + Turn.RotateVector(FVector(0.0f, Sign * Span * 0.5f, 0.0f)), Turn);
				Wing->SetWorldScale3D(FVector(0.20f * Size, Span / 100.0f, 0.02f));
			}
			break;
		}
		case EAmbienceKind::Smoke:
		{
			// One puff after another: rising, widening, leaning with the wind, then thinning to nothing.
			const float Life = FMath::Frac(Clock * 0.07f + static_cast<float>(Index) / static_cast<float>(Seeds.Num()));
			const float Sway = FMath::Sin(Clock * 0.8f + Own) * 18.0f * Life;
			const FVector Place = Centre + FVector(Sway, 0.0f, Extent.Z * Life) + Wind * (Life * Life * 9.0f);
			const float Fade = Life > 0.75f ? (1.0f - Life) / 0.25f : FMath::Min(Life / 0.08f, 1.0f);
			const float Width = (0.28f + 0.9f * Life) * Size * Fade;
			Bits[Index]->SetWorldLocation(Place);
			Bits[Index]->SetWorldScale3D(FVector(Width, Width, Width * 0.8f));
			break;
		}
		case EAmbienceKind::Leaves:
		{
			const float Fall = 45.0f + 30.0f * Seed.Z;
			const FVector Place = Centre + FVector(
				Wrap((Seed.X * 2.0f - 1.0f) * Extent.X + Wind.X * Clock + FMath::Sin(Clock * 0.9f + Own) * 60.0f, Extent.X),
				Wrap((Seed.Y * 2.0f - 1.0f) * Extent.Y + Wind.Y * Clock + FMath::Cos(Clock * 0.7f + Own) * 60.0f, Extent.Y),
				Wrap((Seed.Z * 2.0f - 1.0f) * Extent.Z - Fall * Clock, Extent.Z));
			Bits[Index]->SetWorldLocationAndRotation(Place, FRotator(Clock * 70.0f + Own * 57.0f, Clock * 40.0f, Clock * 95.0f + Own * 30.0f));
			Bits[Index]->SetWorldScale3D(FVector(0.11f, 0.07f, 0.008f) * Size);
			break;
		}
		case EAmbienceKind::Dust:
		case EAmbienceKind::Motes:
		default:
		{
			const bool bDust = Kind == EAmbienceKind::Dust;
			const float Rise = bDust ? 9.0f : 3.0f;
			const FVector Drift = bDust ? FVector::ZeroVector : Wind;
			const FVector Place = Centre + FVector(
				Wrap((Seed.X * 2.0f - 1.0f) * Extent.X + Drift.X * Clock + FMath::Sin(Clock * 0.31f + Own) * 45.0f, Extent.X),
				Wrap((Seed.Y * 2.0f - 1.0f) * Extent.Y + Drift.Y * Clock + FMath::Cos(Clock * 0.27f + Own * 2.0f) * 45.0f, Extent.Y),
				Wrap((Seed.Z * 2.0f - 1.0f) * Extent.Z + Rise * Clock + FMath::Sin(Clock * 0.5f + Own * 3.0f) * 25.0f, Extent.Z));
			// Specks swell and shrink a little, so they seem to catch the light and lose it.
			const float Glint = (0.035f + 0.03f * Seed.Y) * Size * (0.7f + 0.3f * FMath::Sin(Clock * 1.3f + Own * 4.0f));
			Bits[Index]->SetWorldLocation(Place);
			Bits[Index]->SetWorldScale3D(FVector(Glint, Glint, Glint * 0.6f));
			break;
		}
		}
	}
}
