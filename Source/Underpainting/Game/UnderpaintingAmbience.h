// Ambient life: small moving things that make a place feel inhabited and cost almost nothing.
// Birds wheeling far off, pigment drifting on the air, smoke from a chimney, leaves coming down, dust rising.
// Each is a handful of simple painted shapes moved by arithmetic. They touch nothing and nothing touches them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderpaintingAmbience.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EAmbienceKind : uint8
{
	Birds,      // dark shapes wheeling at a distance, wings beating
	Motes,      // specks of pigment drifting on the wind
	Smoke,      // puffs rising from one point and leaning with the wind
	Leaves,     // dabs of leaf coming down, turning
	Dust,       // crumbs rising slowly: the Vault's still air
};

UCLASS()
class AUnderpaintingAmbience : public AActor
{
	GENERATED_BODY()

public:

	AUnderpaintingAmbience();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	EAmbienceKind Kind = EAmbienceKind::Motes;

	/** How many (birds, specks, puffs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience", meta = (ClampMin = "1", ClampMax = "64"))
	int32 Count = 20;

	/** Half the size of the space they move in, in centimetres, centred on this actor. Smoke rises to Extent.Z. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FVector Extent = FVector(1500.0f, 1500.0f, 400.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FLinearColor Colour = FLinearColor(0.70f, 0.36f, 0.05f);

	/** 1 is the usual pace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	float Speed = 1.0f;

	/** 1 is the usual size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	float Size = 1.0f;

	/** Which way the air moves, in centimetres a second. The Lamp is in the west, and the wind comes from it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FVector Wind = FVector(8.0f, 30.0f, 0.0f);

private:

	UStaticMeshComponent* AddBit(const TCHAR* MeshPath);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bits;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	TArray<FVector> Seeds;      // three numbers from 0 to 1 for each thing, fixed at the start
	float Clock = 0.0f;
};
