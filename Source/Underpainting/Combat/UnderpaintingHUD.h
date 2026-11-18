// A plain drawn HUD for the first playable: the player's Health, Stamina and Glim, and a bar over each enemy.
// It is replaced by a designed HUD later; nothing else depends on it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UnderpaintingHUD.generated.h"

UCLASS()
class AUnderpaintingHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;

private:

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill, const FString& Label);
};
