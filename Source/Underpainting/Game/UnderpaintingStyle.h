// Placeholder shapes made in C++ (enemies, nodes, Lampstands, people) take the style kit's Glaze material, so a
// playtest looks painted without any models. The material turns one colour into three painted values.
// If the style kit has not been built, they fall back to the engine's plain shape material.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace UnderpaintingStyle
{
	/** Gives the component a material whose colour can be set with SetColour. */
	inline UMaterialInstanceDynamic* MakeMaterial(UStaticMeshComponent* Component)
	{
		if (!Component || !Component->GetStaticMesh())
		{
			return nullptr;
		}
		if (UMaterialInterface* Glaze = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StyleKit/Materials/M_Glaze.M_Glaze")))
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Glaze, Component);
			Material->SetScalarParameterValue(TEXT("Tiling"), 120.0f);
			Component->SetMaterial(0, Material);
			return Material;
		}
		return Component->CreateAndSetMaterialInstanceDynamic(0);
	}

	/** Purity 1 keeps the colour exact and flat: used for Erasures, the only pure white in the game. */
	inline void SetColour(UMaterialInstanceDynamic* Material, const FLinearColor& Colour, float Purity = 0.0f)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Colour"), Colour);   // style kit
			Material->SetVectorParameterValue(TEXT("Color"), Colour);    // engine fallback
			Material->SetScalarParameterValue(TEXT("Purity"), Purity);
		}
	}
}
