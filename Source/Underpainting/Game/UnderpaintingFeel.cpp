#include "UnderpaintingFeel.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingStyle.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// --- Splash ---

AUnderpaintingSplash::AUnderpaintingSplash()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	DabMesh = Sphere.Succeeded() ? Sphere.Object : nullptr;
}

void AUnderpaintingSplash::Burst(const FVector& Direction, const FLinearColor& Colour, int32 Count, float Speed, float Purity)
{
	const FVector Along = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Each dab is its own small shape: a dozen of them for half a second costs nothing, and they take the paint material.
		UStaticMeshComponent* Dab = NewObject<UStaticMeshComponent>(this);
		Dab->SetStaticMesh(DabMesh);
		Dab->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dab->SetCastShadow(false);
		Dab->SetMobility(EComponentMobility::Movable);
		Dab->SetupAttachment(GetRootComponent());
		Dab->RegisterComponent();
		if (!Material)
		{
			Material = UnderpaintingStyle::MakeMaterial(Dab);
			UnderpaintingStyle::SetColour(Material, Colour, Purity);
		}
		else
		{
			Dab->SetMaterial(0, Material);
		}
		Dabs.Add(Dab);

		// A cone along the direction of the blow, lifted a little so the paint arcs.
		const FVector Spread = FMath::VRandCone(Along, FMath::DegreesToRadians(65.0f));
		Velocities.Add(Spread * Speed * FMath::FRandRange(0.35f, 1.0f) + FVector(0.0f, 0.0f, Speed * FMath::FRandRange(0.15f, 0.6f)));
		Positions.Add(FVector::ZeroVector);
		Sizes.Add(FMath::FRandRange(0.09f, 0.24f));
	}
}

void AUnderpaintingSplash::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;
	if (Age >= Life)
	{
		Destroy();
		return;
	}

	const float Left = 1.0f - Age / Life;
	for (int32 Index = 0; Index < Dabs.Num(); ++Index)
	{
		Velocities[Index].Z -= 1100.0f * DeltaTime;
		Velocities[Index] *= FMath::Max(0.0f, 1.0f - 1.8f * DeltaTime);
		Positions[Index] += Velocities[Index] * DeltaTime;
		// Dabs stretch along their flight and shrink as they go.
		const FVector Scale = FVector(Sizes[Index] * (1.0f + Velocities[Index].Size() / 900.0f), Sizes[Index], Sizes[Index]) * Left;
		Dabs[Index]->SetRelativeTransform(FTransform(Velocities[Index].Rotation(), Positions[Index], Scale));
	}
}

// --- Feel ---

bool UUnderpaintingFeel::bShowHitboxes = false;

UUnderpaintingFeel* UUnderpaintingFeel::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UUnderpaintingFeel>() : nullptr;
}

void UUnderpaintingFeel::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Each zone has its own air: a row named "amb_<Zone>" in data/sounds.json, looping under everything.
	const UUnderpaintingDataSubsystem* Data = InWorld.GetGameInstance() ? InWorld.GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FName Zone = Data ? Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(&InWorld, true)) : NAME_None;
	const FName Row(*FString::Printf(TEXT("amb_%s"), Zone.IsNone() ? TEXT("Room") : *Zone.ToString()));
	float Volume = 1.0f, Pitch = 1.0f;
	if (USoundBase* Sound = PickSound(Row, Volume, Pitch))
	{
		Ambience = UGameplayStatics::SpawnSound2D(&InWorld, Sound, Volume * SoundVolume, 1.0f, 0.0f, nullptr, false, false);
	}
}

void UUnderpaintingFeel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float Real = FApp::GetDeltaTime();

	// Hit-stop ends on real time: the frozen actors' own clocks are nearly stopped.
	for (int32 Index = Stopped.Num() - 1; Index >= 0; --Index)
	{
		Stopped[Index].Remaining -= Real;
		if (Stopped[Index].Remaining <= 0.0f)
		{
			if (AActor* Actor = Stopped[Index].Actor.Get())
			{
				Actor->CustomTimeDilation = 1.0f;
			}
			Stopped.RemoveAtSwap(Index);
		}
	}

	// Shake: smooth noise scaled by trauma squared, fading in about half a second.
	Trauma = FMath::Max(0.0f, Trauma - 1.9f * Real);
	ShakeTime += Real;
	const float Amount = bShake ? Trauma * Trauma : 0.0f;
	const float T = ShakeTime * 31.0f;
	ShakeOffset = FVector(0.0f, FMath::Sin(T * 1.3f + 0.7f), FMath::Sin(T * 1.7f)) * 9.0f * Amount;
	ShakeRotation = FRotator(FMath::Sin(T * 1.1f + 2.1f) * 1.6f, FMath::Sin(T * 0.9f) * 1.6f, FMath::Sin(T * 1.5f + 4.0f) * 1.1f) * Amount;

	for (int32 Index = Numbers.Num() - 1; Index >= 0; --Index)
	{
		Numbers[Index].Age += Real;
		Numbers[Index].Location.Z += 70.0f * Real;
		if (Numbers[Index].Age > 0.9f)
		{
			Numbers.RemoveAt(Index);
		}
	}
}

void UUnderpaintingFeel::HitStop(AActor* First, AActor* Second, float Seconds)
{
	if (!bHitStop || Seconds <= 0.0f)
	{
		return;
	}
	for (AActor* Actor : { First, Second })
	{
		if (!Actor)
		{
			continue;
		}
		FStopped* Existing = Stopped.FindByPredicate([Actor](const FStopped& Entry) { return Entry.Actor.Get() == Actor; });
		if (Existing)
		{
			Existing->Remaining = FMath::Max(Existing->Remaining, Seconds);
		}
		else
		{
			Stopped.Add({ Actor, Seconds });
		}
		Actor->CustomTimeDilation = 0.03f;
	}
}

void UUnderpaintingFeel::Shake(float InTrauma)
{
	Trauma = FMath::Clamp(Trauma + InTrauma, 0.0f, 1.0f);
}

void UUnderpaintingFeel::Splash(const FVector& Location, const FVector& Direction, const FLinearColor& Colour, int32 Count, float Speed, float Purity)
{
	if (UWorld* World = GetWorld())
	{
		if (AUnderpaintingSplash* Burst = World->SpawnActor<AUnderpaintingSplash>(Location, FRotator::ZeroRotator))
		{
			Burst->Burst(Direction, Colour, FMath::Clamp(Count, 1, 40), Speed, Purity);
		}
	}
}

void UUnderpaintingFeel::Number(const FVector& Location, const FString& Text, const FLinearColor& Colour, float Scale)
{
	FUnderpaintingFloatingNumber Entry;
	Entry.Location = Location + FVector(FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), 0.0f);
	Entry.Text = Text;
	Entry.Colour = Colour;
	Entry.Scale = Scale;
	Numbers.Add(Entry);
	if (Numbers.Num() > 24)
	{
		Numbers.RemoveAt(0);
	}
}

USoundBase* UUnderpaintingFeel::PickSound(FName Sound, float& OutVolume, float& OutPitch)
{
	const UWorld* World = GetWorld();
	const UUnderpaintingDataSubsystem* Data = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>() : nullptr;
	const FSoundRow* Row = Data ? Data->FindSound(Sound) : nullptr;
	if (!Row || Row->Assets.Num() == 0)
	{
		return nullptr;
	}
	const FString& Path = Row->Assets[FMath::RandRange(0, Row->Assets.Num() - 1)];
	TObjectPtr<USoundBase>* Known = Loaded.Find(Path);
	if (!Known)
	{
		// Packs under Content/ThirdParty are not in git: if one is missing the game is simply quieter.
		Known = &Loaded.Add(Path, LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
		if (!*Known)
		{
			UE_LOG(LogUnderpainting, Warning, TEXT("Sound '%s' wants %s, which is not installed (see docs/ASSETS.md)"), *Sound.ToString(), *Path);
		}
	}
	OutVolume = Row->Volume;
	OutPitch = FMath::FRandRange(Row->PitchMin, FMath::Max(Row->PitchMin, Row->PitchMax));
	return Known->Get();
}

void UUnderpaintingFeel::Play(FName Sound, const FVector& Location, float Volume, float Pitch)
{
	float RowVolume = 1.0f, RowPitch = 1.0f;
	if (USoundBase* Asset = PickSound(Sound, RowVolume, RowPitch))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Asset, Location, Volume * RowVolume * SoundVolume, Pitch * RowPitch);
	}
}

void UUnderpaintingFeel::PlayFlat(FName Sound, float Volume, float Pitch)
{
	float RowVolume = 1.0f, RowPitch = 1.0f;
	if (USoundBase* Asset = PickSound(Sound, RowVolume, RowPitch))
	{
		UGameplayStatics::PlaySound2D(this, Asset, Volume * RowVolume * SoundVolume, Pitch * RowPitch);
	}
}

void UUnderpaintingFeel::NotePlayerHit(const FVector& FromLocation, float Damage)
{
	PlayerHitFrom = FromLocation;
	PlayerHitDamage = Damage;
	PlayerHitTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
}

float UUnderpaintingFeel::GetPlayerHitAge() const
{
	return GetWorld() ? GetWorld()->GetRealTimeSeconds() - PlayerHitTime : 100.0f;
}
